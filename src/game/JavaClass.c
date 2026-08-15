/* Script-VM class loader / constant-pool primitives.
 *
 * loadConstString and lookupClassField are the two highest fan-in
 * unmatched functions in the game (197 and 181 callers across the
 * Java_*, CHR_*, JTHREAD, LAYOUT, SCENE, SCRIPT and STAGE families).
 * See include/game/JavaClass.h for the recovered struct layouts.
 */
#include "game/JavaClass.h"

extern void *xmalloc(int nSize, int nTag);
extern unsigned int StringUtf_getHash(const char *p, unsigned int nLen);
extern int strlen(const char *s);
extern int memcmp(const void *, const void *, unsigned int);

STRENTRY **g_StrTable;
int g_StrCount;

/* MATCHED, 80/80 words, no fix_cc_asm flags. Three source-shape facts did
 * it, and all three are reusable on the rest of this family:
 *
 * 1. The search loop must NOT be a `for`/`while` -- gcc 2.96 rotates those
 *    (peels the first test, duplicates the `lhu` into a bottom-test delay
 *    slot) and the original is un-rotated: the null test IS the loop-top
 *    branch target, and both back-edges jump to it. Writing it as an
 *    explicit `loop:` label with `goto loop` keeps the test at the top,
 *    which is also what makes the four scheduling nops after `lhu` appear
 *    on their own (they are gcc's own padding for that block, not a
 *    delay-slot artefact -- nothing needed to be forced).
 *
 * 2. Both `continue` arms must share ONE `next:` block (`goto next`), not
 *    a duplicated `p = p->next; goto loop;`. gcc's reorg then annul-copies
 *    that block's first insn into each branch's delay slot and retargets
 *    the branch past it, which is exactly how the original's
 *    `bnel/lw` + `bnezl/lw` pair is built. Duplicating the arms instead
 *    makes gcc invert the second test's polarity to fill the slot
 *    (`beqz ->epilogue` + `b ->loop`), a 4-word miss.
 *
 * 3. `g_StrCount++` as a read-modify-write in place pins its store one
 *    slot too early. Splitting it -- `cnt = g_StrCount + 1;` before the
 *    bucket-head store and `g_StrCount = cnt;` after -- is the only shape
 *    of the 120 tail-statement permutations that gives the original's
 *    store order. (`--rotate loadConstString:68:2` also matched, but a
 *    plain-C fix is preferred over a flag; see CONTRIBUTING.)
 *
 * The $s3/$s4 bucket-index/byte-offset roles noted in the old TODO were
 * never a real allocator tie-break: they fell into place by themselves
 * once the tail store order was right. */
/* Intern (pStr, nLength) into the global string table, allocating and
 * chaining a new STRENTRY on first sight. nLength == -1 means "compute
 * via strlen"; nLength == 0 is rejected outright. */
void *loadConstString(char *pStr, int nLength)
{
    STRENTRY *p;
    int idx, hash, cnt;

    if (g_StrTable == 0) {
        STRENTRY **pRow;
        int i;

        g_StrTable = (STRENTRY **) xmalloc(STRTAB_SIZE * 4, 2);
        pRow = g_StrTable + STRTAB_SIZE - 1;
        i = STRTAB_SIZE - 1;
        do {
            *pRow = 0;
            pRow--;
        } while (--i >= 0);
    }

    if (nLength == 0)
        return 0;
    if (nLength < 0)
        nLength = strlen(pStr);

    hash = StringUtf_getHash(pStr, nLength);
    idx = hash & (STRTAB_SIZE - 1);

    p = g_StrTable[idx];
loop:
    if (p == 0)
        goto notfound;
    if (p->length != nLength)
        goto next;
    if (memcmp(p->data, pStr, nLength) == 0)
        return p;
next:
    p = p->next;
    goto loop;

notfound:
    p = (STRENTRY *) xmalloc(sizeof(STRENTRY), 20);
    p->hash = (unsigned short) idx;
    p->data = pStr;
    p->length = (unsigned short) nLength;
    p->next = g_StrTable[idx];
    cnt = g_StrCount + 1;
    g_StrTable[idx] = p;
    g_StrCount = cnt;
    return p;
}

/* MATCHED, 31/31 words, no fix_cc_asm flags. Two independent levers, and
 * the second one is the exact opposite of what loadConstString needed --
 * worth remembering when picking a loop idiom in this family:
 *
 * 1. Assign `n` BEFORE `p` in both arms of the if/else. That single
 *    statement swap is what puts the counter in $a0 (the incoming
 *    pClass register, reused once the last field load retires) and the
 *    cursor in $a1 (the incoming pName register, which is why pName has
 *    to be copied to $a3 in the entry branch's delay slot). With `p`
 *    first the roles invert and $t0 gets dragged in as a fifth register.
 *    The old TODO called this a "register-role swap needing permute.py";
 *    it is just source statement order.
 *
 * 2. The search must be a real `for`, NOT the hand-written
 *    `if (n < 0) goto notfound; loop: ...; goto loop;`. gcc 2.96 turns a
 *    `for` into guard-test + do-while, which is the original's shape:
 *    a `bltz` guard at the entry and a separate `bgez` back-edge at the
 *    bottom, each with its own `n--` (one in the found-branch's delay
 *    slot, one before the guard). Written with explicit gotos, gcc
 *    cross-jumps the two `n--; test` blocks into one shared
 *    `n--; bgezl` and tail-merges both returns onto a single `j $31`,
 *    losing a word and rewriting the whole loop. (loadConstString wanted
 *    the goto form because its loop must stay un-rotated; this one wants
 *    the `for` because it must BE rotated. The test is whether the
 *    original's back-edge targets the null/bounds test or the body.) */
/* Search a class's field table for pName, restricted to the static or
 * instance sub-range by nFlags. Fields are stored static-first: entries
 * [0, nStaticFieldCount) are static, [nStaticFieldCount, nFieldCount)
 * are instance. Every known caller passes nFlags == 0 (instance). */
JFIELD *lookupClassField(JCLASS_FIELDS *pClass, void *pName, int nFlags)
{
    JFIELD *p;
    int n;

    nFlags &= 0xff;
    if (nFlags != 0) {
        n = pClass->nStaticFieldCount;
        p = pClass->fields;
    } else {
        n = pClass->nFieldCount - pClass->nStaticFieldCount;
        p = pClass->fields + pClass->nStaticFieldCount;
    }

    for (n--; n >= 0; n--, p++) {
        if (p->name == pName)
            return p;
    }
    return 0;
}
