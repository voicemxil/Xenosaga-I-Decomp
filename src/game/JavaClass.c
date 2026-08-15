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

/* TODO: near-miss (LOGIC, 41 diffs of 80) -- the zero-init loop (do{*p--=0;}
 * while(--i>=0), verified byte-exact against the original's decrement-then-
 * store/test-after shape) and the not-found/found control flow both match.
 * What's left: (1) the hash-bucket index and its <<2 byte offset land in
 * $s3/$s4 the opposite way round from the original in every attempted
 * source shape (separate idx/ofs locals, pointer-vs-array-subscript access,
 * declaration-order swaps -- none moved the allocator); (2) the search
 * loop's `lhu length; nop*4; bnel` back-edge only gets one nop where the
 * original has four, a scheduling gap no loop idiom tried here closes.
 * Both look like the same class of gcc2.96 allocator/scheduler tie-break
 * as JNT_getRootTrans's $v0/$v1 swap (see JNT.c) -- may need an analogous
 * register-swap fixer plus a delay-slot-count fixer, neither built yet. */
/* Intern (pStr, nLength) into the global string table, allocating and
 * chaining a new STRENTRY on first sight. nLength == -1 means "compute
 * via strlen"; nLength == 0 is rejected outright. */
void *loadConstString(char *pStr, int nLength)
{
    STRENTRY *p;
    int idx, hash;

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

    for (p = g_StrTable[idx]; p != 0; p = p->next) {
        if (p->length == nLength && memcmp(p->data, pStr, nLength) == 0)
            return p;
    }

    p = (STRENTRY *) xmalloc(sizeof(STRENTRY), 20);
    p->hash = (unsigned short) idx;
    p->next = g_StrTable[idx];
    p->data = pStr;
    p->length = (unsigned short) nLength;
    g_StrTable[idx] = p;
    g_StrCount++;
    return p;
}

/* TODO: near-miss (LENGTH, 26 diffs, 30 orig vs 31 built words) -- the
 * two-region setup (static-first / instance-after) and the goto-shared
 * found/notfound exits both match in shape. The remaining diff is a
 * register-role swap in the search loop itself (the original keeps the
 * class-pointer's nStaticFieldCount reload in one register across both
 * branches; ours re-derives it), which then perturbs which register the
 * loop counter lands in. Tried: hoisting nStaticFieldCount into its own
 * local before the if/else, computing both branches' n via a shared
 * expression -- no change. Needs a permute.py pass over the two setup
 * blocks. */
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
        p = pClass->fields;
        n = pClass->nStaticFieldCount;
    } else {
        p = pClass->fields + pClass->nStaticFieldCount;
        n = pClass->nFieldCount - pClass->nStaticFieldCount;
    }

    n--;
    if (n < 0)
        goto notfound;
loop:
    if (p->name == pName)
        return p;
    n--;
    if (n >= 0) {
        p++;
        goto loop;
    }
notfound:
    return 0;
}
