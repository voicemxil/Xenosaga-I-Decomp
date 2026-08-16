/* Script-VM class loader / constant-pool primitives.
 *
 * loadConstString and lookupClassField are the two highest fan-in
 * unmatched functions in the game (197 and 181 callers across the
 * Java_*, CHR_*, JTHREAD, LAYOUT, SCENE, SCRIPT and STAGE families).
 * See include/game/JavaClass.h for the recovered struct layouts.
 */
#include "game/JavaClass.h"

extern void *xmalloc(unsigned int nSize, int nTag);
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
JFIELD *lookupClassField(JCLASS *pClass, void *pName, int nFlags)
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

/* --- Object-heap allocator (xheap_*) ------------------------------------
 *
 * The script VM's object heap is a bump allocator: one contiguous region
 * carved into 12-byte-header blocks chained head-to-tail through pNext,
 * with `freeBlock` pointing at the most recently allocated block (so
 * freeBlock->pNext is the bump pointer). Nothing is ever individually
 * reclaimed -- xfree only zeroes a block's tag so the heap walker stops
 * counting it. Reclamation is by frame: xheap_push saves (freeBlock,
 * jthreadCurrent) onto a downward-growing stack at the top of the same
 * region, and xheap_pop restores them, discarding everything allocated
 * since in one step. `frame_stack` is that stack pointer, so the heap and
 * the frame stack grow toward each other and xheap_current_clear's
 * "clear to frame_stack - 8" is the gap between them.
 *
 * The symbol names (heap_top, heap_size, frame_stack, freeBlock,
 * jthreadCurrent) are the originals, recovered from the ELF symbol table.
 */
typedef struct XBLOCK
{
    struct XBLOCK *pNext;   /* 0x00 - one past this block's payload, i.e. the
                                      next block's header */
    int            unk04;   /* 0x04 - never read or written by this family */
    unsigned short nTag;    /* 0x08 - owner tag, 0 == freed */
    unsigned short nWords;  /* 0x0A - payload size in words */
} XBLOCK;                   /* header size 12, payload follows */

extern XBLOCK *heap_top;
extern int     heap_size;
extern int    *frame_stack;
extern XBLOCK *freeBlock;
extern void   *jthreadCurrent;

extern void *memset(void *, int, unsigned int);
extern void reloadConstString(void *pLimit);
extern void reloadClassEntry(void *pLimit);
extern void xmemchk(XBLOCK *p);

/* Reset the object heap. nHeap != 0 reuses the region already registered
 * (used to re-init without re-describing the memory).
 *
 * This really does return void: JNI.c's JNI_initSystem does
 * `return xheap_init(...)`, so the original TU must have had no
 * prototype in scope and relied on the C89 implicit `int f()` rule --
 * it passes on whatever happens to be left in $v0.  Declaring an int
 * return here costs exactly one `move $v0,$v1` and was the whole
 * difference against the original. */
void xheap_init(int nHeap, void *pStart, int nSize)
{
    if (nHeap == 0) {
        heap_top = (XBLOCK *) pStart;
        heap_size = nSize;
    }
    memset(heap_top, 0, heap_size);
    freeBlock = heap_top;
    freeBlock->nWords = 0;
    frame_stack = (int *) ((char *) heap_top + heap_size);
    freeBlock->pNext = freeBlock;
}

/* Zero the unallocated gap between the bump pointer and the frame stack. */
int xheap_current_clear(void)
{
    char *p;
    char *pEnd;

    p = (char *) freeBlock->pNext;
    pEnd = (char *) (frame_stack - 2);
    while (p < pEnd) {
        p[0] = 0;
        p[1] = 0;
        p[2] = 0;
        p[3] = 0;
        p += 4;
    }
}

/* Save the allocation mark and the current thread; returns the old mark. */
int xheap_push(void)
{
    int *p;
    int nMark;

    p = frame_stack - 2;
    nMark = (int) freeBlock;
    frame_stack = p;
    p[1] = (int) jthreadCurrent;
    p[0] = nMark;
    return nMark;
}

/* Restore the mark saved by xheap_push, dropping every object allocated
 * since, and roll the interned-string and class tables back with it. */
int xheap_pop(void)
{
    int *p;
    XBLOCK *pMark;

    p = frame_stack;
    pMark = (XBLOCK *) p[0];
    jthreadCurrent = (void *) p[1];
    freeBlock = pMark;
    reloadConstString(pMark->pNext);
    reloadClassEntry(freeBlock->pNext);
    frame_stack = frame_stack + 2;
    return (int) freeBlock;
}

/* Total the live bytes per allocation tag into a 32-entry table; returns
 * the grand total. */
int xheap_info(int *pInfo)
{
    XBLOCK *p;
    int nTotal;
    int i;

    for (i = 31; i >= 0; i--) {
        pInfo[i] = 0;
    }
    nTotal = 0;
    for (p = heap_top; p != 0; p = p->pNext) {
        if (p->nTag != 0) {
            nTotal += p->nWords * 4 + 12;
            pInfo[p->nTag & 31] += p->nWords * 4 + 12;
        }
    }
    return nTotal;
}

/* Same walk as xheap_info over a scratch table -- the reporting that
 * consumed it was compiled out of the retail build. */
void infoMemory(void)
{
    int aInfo[32];
    XBLOCK *p;
    int i;

    for (i = 31; i >= 0; i--) {
        aInfo[i] = 0;
    }
    for (p = heap_top; p != 0; p = p->pNext) {
        if (p->nTag != 0) {
            aInfo[p->nTag & 31] += p->nWords * 4 + 12;
        }
    }
}

/* Bump-allocate nSize bytes, rounded up to a word, tagged nTag. */
/* NEAR MISS, 24 built / 25 original, one word.  The instruction
 * multiset is identical apart from a single `move $a2,$a0` the original
 * emits in the prologue: there the word count lands in $a0, clobbering
 * the incoming nSize, so nSize needs its own copy to survive to the
 * `beqz`.  gcc here allocates $v0/$v1 for the shifts instead and keeps
 * nSize in $a0, so no copy is needed -- a pure allocator tie-break.
 * Swept without success: nSize reassigned vs a separate nBytes local;
 * the rounding hoisted above the test as a declaration initialiser;
 * LAUNDER and LAUNDER_V on the rounded byte count (both recover the
 * original's redundant `srl` -- gcc otherwise CSEs `(x << 2) >> 2` back
 * to x through its equivalence table -- but neither forces the copy);
 * `if (nSize == 0) return 0;` early-exit vs a single-exit pRet local.
 * Forcing it needs the word count pinned to $a0, which is a third
 * steering construct on a 100-byte function -- not worth it. */
void *xmalloc(unsigned int nSize, int nTag)
{
    XBLOCK *p;
    unsigned int nBytes;

    if (nSize == 0) {
        return 0;
    }
    nBytes = ((nSize + 3) >> 2) << 2;
    p = freeBlock->pNext;
    p->nWords = nBytes >> 2;
    p->nTag = nTag;
    p->pNext = (XBLOCK *) ((char *) p + nBytes + 12);
    freeBlock = p;
    xmemchk(p);
    return (char *) p + 12;
}

/* Mark a block dead; the heap walker skips tag 0 and the space is only
 * really reclaimed by the enclosing xheap_pop. */
void xfree(void *pMem)
{
    if (pMem != 0) {
        ((XBLOCK *) ((char *) pMem - 12))->nTag = 0;
    }
}

/* --- Class records, objects and descriptors ----------------------------- */

typedef struct NATIVEENTRY
{
    char *pName;  /* 0x00 - "Class.method" key, NULL terminates the table */
    void *pFunc;  /* 0x04 */
} NATIVEENTRY;

/* One entry of a class-name hash bucket chain. Only the chain link is
 * known here; reloadClassEntry is a pure address-order truncation. */
typedef struct CLASSENTRY
{
    char               pad00[8];
    JCLASS            *pClass; /* 0x08 - resolved class, NULL until loaded */
    struct CLASSENTRY *pNext;  /* 0x0C */
} CLASSENTRY;

extern JCLASS      *classClass;
extern JCLASS      *classString;
extern JCLASS      *classObject;
extern JCLASS      *classStringBuffer;
extern CLASSENTRY **classEntryPool;
extern NATIVEENTRY  default_native[];

extern char D_004CD628[]; /* "java/lang/Object" */
extern char D_004CD640[]; /* "java/lang/StringBuffer" */
extern char D_004CD658[]; /* "java/lang/String" */

extern JCLASS *classFromSig(char **ppSig);
extern void *lookupArray(JCLASS *pElement);
extern int sizeofDescripterType(char **ppSig);
extern void initPrimitiveTypes(void);
extern void loadStaticClass(JCLASS **ppOut, char *pName);
extern int strcmp(const char *, const char *);

/* Resolve one JVM type signature to its class, leaving the caller's
 * string untouched (classFromSig advances the cursor it is handed). */
JCLASS *getClassFromSignature(char *pSig)
{
    char *p;

    p = pSig;
    return classFromSig(&p);
}

/* Allocate a bare instance of pClass and stamp its type word. */
void *newObject(JCLASS *pClass)
{
    int *pObj;

    pObj = (int *) xmalloc(pClass->u.nInstanceSize, 14);
    pObj[0] = pClass->pType;
    return pObj;
}

/* Allocate an empty class record. The type word comes from classClass,
 * so a class record is itself a java.lang.Class instance. */
JCLASS *newClass(void)
{
    JCLASS *p;

    p = (JCLASS *) xmalloc(0x40, 14);
    p->u.nInstanceSize = 0;
    p->nStaticFieldCount = 0;
    p->pType = classClass->pType;
    return p;
}

/* Wrap an interned constant-pool string as a java.lang.String: a 12-byte
 * char-array object (length at 0x04, bytes at 0x08) inside a String. */
void *Const2JavaString(STRENTRY *pStr)
{
    int *pArray;
    int *pObj;

    pArray = (int *) xmalloc(12, 14);
    pArray[1] = pStr->length;
    pArray[2] = (int) pStr->data;
    pObj = (int *) newObject(classString);
    pObj[1] = (int) pArray;
    return pObj;
}

/* Sum the sizes of a run of type descriptors, stopping on the terminator
 * (sizeofDescripterType returns negative for ')' / end of list). The
 * running total is returned from BEFORE the terminator was added in. */
int sizeofDescripter(char **ppSig)
{
    int nRet;
    int n;
    int nSize;

    nSize = 0;
    do {
        n = sizeofDescripterType(ppSig);
        nRet = nSize;
        nSize += n;
    } while (n >= 0);
    return nRet;
}

/* Split a method descriptor "(args)R" into the argument stack size, the
 * return type letter and the return size. */
void methodDescripter(char *pSig, short *pnArgSize, short *pnRetSize,
                      char *pcRetType)
{
    char *p;

    p = pSig;
    *pnArgSize = sizeofDescripter(&p);
    *pcRetType = *p;
    *pnRetSize = sizeofDescripter(&p);
}

/* Look up a "Class.method" key in the static native-method table. */
void *findNativeMethod(char *pName)
{
    NATIVEENTRY *p;

    for (p = default_native; p->pName != 0; p++) {
        if (strcmp(p->pName, pName) == 0) {
            return p->pFunc;
        }
    }
    return 0;
}

/* Discard every cached class entry allocated at or above pLimit, in all
 * 512 buckets. The chains are in descending allocation order, so the
 * first entry below the limit ends the walk. */
void reloadClassEntry(void *pLimit)
{
    CLASSENTRY **ppBucket;
    CLASSENTRY *pHead;
    CLASSENTRY *p;
    int i;

    ppBucket = classEntryPool;
    for (i = 511; i >= 0; i--) {
        pHead = *ppBucket;
        if (pHead != 0) {
            p = pHead;
            while (p != 0 && (unsigned int) p >= (unsigned int) pLimit) {
                p = p->pNext;
            }
            *ppBucket = p;
        }
        ppBucket++;
    }
}

/* Resolve the element signature of an array class name ("[Lfoo;") and
 * hand it to the array-class cache. */
void *loadArray(STRENTRY *pName)
{
    JCLASS *pElement;

    pElement = getClassFromSignature(pName->data + 1);
    if (pElement != 0) {
        return lookupArray(pElement);
    }
    return 0;
}

/* Load the three classes the VM itself needs by name. */
void initBaseClasses(void)
{
    initPrimitiveTypes();
    loadStaticClass(&classObject, D_004CD628);
    loadStaticClass(&classStringBuffer, D_004CD640);
    loadStaticClass(&classString, D_004CD658);
}

extern CLASSENTRY *lookupClassEntry(STRENTRY *pName, int nFlags);
extern JCLASS *findClass(CLASSENTRY *pEntry);
extern int processClass(JCLASS *pClass, int nStage);

/* Walk the superclass chain for a method with the same interned
 * name/signature pair as pWant.
 *
 * NOTE -- the inner scan does not advance through the method table: the
 * original reads pSuper->pMethods once per class and then compares that
 * one entry nMethodCount times.  That is faithfully what the retail code
 * does (gcc hoisted the table load and the first pair of operands out of
 * the loop, which it could only do because they are loop invariant), so
 * the missing `[i]` is a bug in the original source, not in this
 * transcription. */
JMETHOD *findSuperMethod(JMETHOD *pWant, JCLASS *pClass)
{
    JCLASS *pSuper;
    JMETHOD *pM;
    int i;

    for (pSuper = pClass->pSuper; pSuper != 0; pSuper = pSuper->pSuper) {
        for (i = pSuper->nMethodCount - 1; i >= 0; i--) {
            pM = pSuper->pMethods;
            if (pM->pName == pWant->pName && pM->pSig == pWant->pSig) {
                return pM;
            }
        }
    }
    return 0;
}

/* Build one of the primitive/wrapper class records (int, float, byte...). */
void initWrapperClass(JCLASS **ppOut, char *pName, char cSigChar,
                      int nElemSize)
{
    *ppOut = newClass();
    (*ppOut)->nFlags = 0x100;
    (*ppOut)->pName = loadConstString(pName, -1);
    (*ppOut)->u.prim.cSigChar = cSigChar;
    (*ppOut)->u.prim.nElemSize = nElemSize;
}

/* Resolve a class by interned name, loading it on demand unless the
 * caller asked for a cache-only lookup. */
JCLASS *loadClass(STRENTRY *pName, int bNoLoad)
{
    CLASSENTRY *pEntry;
    JCLASS *pClass;

    pEntry = lookupClassEntry(pName, bNoLoad);
    pClass = pEntry->pClass;
    if (pClass == 0) {
        if (bNoLoad == 0) {
            pClass = findClass(pEntry);
        }
        if (pClass == 0) {
            return 0;
        }
        pEntry->pClass = pClass;
    }
    if (processClass(pClass, 11) == 0) {
        pClass = 0;
    }
    return pClass;
}

/* Lay out the instance fields, each aligned to its own rounded-up word
 * size, and record the resulting instance size. */
void resolveInstanceField(JCLASS *pClass)
{
    JFIELD *p;
    int nOfs;
    int nAlign;
    int i;

    nOfs = pClass->u.nInstanceSize;
    if (nOfs == 0) {
        nOfs = 4;
    }
    i = pClass->nFieldCount - pClass->nStaticFieldCount - 1;
    p = &pClass->fields[pClass->nStaticFieldCount];
    for (; i >= 0; i--) {
        nAlign = ((p->nSize + 3) >> 2) << 2;
        nOfs = (nOfs + nAlign - 1) / nAlign * nAlign;
        p->nOffset = nOfs;
        nOfs += nAlign;
        p++;
    }
    pClass->u.nInstanceSize = nOfs;
}

/* Load a named class into a global slot and run it to the linked stage. */
void loadStaticClass(JCLASS **ppOut, char *pName)
{
    CLASSENTRY *pEntry;
    JCLASS *pClass;

    pEntry = lookupClassEntry(loadConstString(pName, -1), 0);
    if (pEntry->pClass == 0) {
        pClass = findClass(pEntry);
        if (pClass == 0) {
            *ppOut = 0;
            return;
        }
        pEntry->pClass = pClass;
        *ppOut = pClass;
    }
    processClass(pEntry->pClass, 11);
}

/* Allocate an array object: 12-byte header (type word, length, data
 * pointer) followed by the elements. */
void *newArray(JCLASS *pElement, int nCount)
{
    int *pArray;
    int *pData;
    JCLASS *pArrayClass;
    int nType;

    if ((pElement->nFlags & 0x100) != 0) {
        pArray = (int *) xmalloc(pElement->u.prim.nElemSize * nCount + 12, 14);
    } else {
        pArray = (int *) xmalloc(nCount * 4 + 12, 14);
    }
    pArrayClass = (JCLASS *) lookupArray(pElement);
    pData = pArray + 3;
    nType = pArrayClass->pType;
    pArray[1] = nCount;
    pArray[0] = nType;
    pArray[2] = (int) pData;
    return pArray;
}

/* Turn every still-unresolved string constant (tag 8) into a live
 * java.lang.String and retag it (0x18) so the pass is idempotent. */
int resolveConstants(JCLASS *pClass)
{
    int *pConst;
    int *p;
    unsigned char *pTag;
    int nCount;
    int i;

    i = 0;
    nCount = pClass->nConstCount;
    if (nCount != 0) {
        pConst = pClass->pConst;
        p = pConst;
        do {
            pTag = (unsigned char *) pConst[0] + i;
            if (*pTag == 8) {
                *pTag = 0x18;
                p[0] = (int) Const2JavaString((STRENTRY *) p[0]);
                nCount = pClass->nConstCount;
            }
            i++;
            p++;
        } while (i < nCount);
    }
    return 1;
}

/* Pack the static fields into one xmalloc'd block. The first pass turns
 * each field's declared size into its offset within the block (parked in
 * the nSize slot); the second swaps nSize back and rewrites nOffset as
 * the absolute address of the field's storage. */
void allocStaticField(JCLASS *pClass)
{
    JFIELD *p;
    char *pBlock;
    int nTotal;
    int nAlign;
    int i;

    if (pClass->nStaticFieldCount != 0) {
        nTotal = 0;
        p = pClass->fields;
        for (i = pClass->nStaticFieldCount - 1; i >= 0; i--) {
            nAlign = ((p->nSize + 3) >> 2) << 2;
            nTotal += nAlign;
            p->nSize = (nTotal - 1) / nAlign * nAlign;
            p++;
        }
        pBlock = (char *) xmalloc(nTotal, 10);
        p = pClass->fields;
        for (i = pClass->nStaticFieldCount - 1; i >= 0; i--) {
            unsigned int nOfs = p->nSize;
            int nSize = p->nOffset;

            p->nSize = nSize;
            p->nOffset = (int) (pBlock + nOfs);
            p++;
        }
    }
}
