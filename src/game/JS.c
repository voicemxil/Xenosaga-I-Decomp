/* Script VM class registration and constant-pool helpers */

typedef struct PRIM
{
    int nType; /* 0x00 - entry kind (1 = integer const, 8 = method) */
    int f4;    /* 0x04 */
    int f8;    /* 0x08 */
} PRIM;

typedef struct JCLASS
{
    int  f0;
    int  f4;
    int  nMethods;    /* 0x08 */
    PRIM aMethod[8];  /* 0x0C - sizeof(JCLASS)=108, per JS_init's alloc stride */
} JCLASS;

int numPrimitive;
PRIM *primitive;
int numClass;
JCLASS *classes;
extern int tokenType;

int STR_tokenGetType(void)
{
    return tokenType;
}

extern void *RSRC_alloc(int hRsrc, int nSize, int nFlags);

extern int D_004DA5A8[4];

/* Reset the pools and allocate them from the resource arena */
void JS_init(int hRsrc, int nPrimitives, int nClasses)
{
    numClass = 0;
    numPrimitive = 0;
    primitive = RSRC_alloc(hRsrc, nPrimitives * sizeof(PRIM), 0);
    classes = RSRC_alloc(hRsrc, nClasses * sizeof(JCLASS), 0);
}

/* FLAG/TOOL REQUEST (verified). Source is exhaustively tuned: 24 shapes
 * of the two bump-allocations and the four stores were swept, and the
 * one below is the unique minimum at 9 diffs. Those 9 are a pure sched2
 * permutation with an identical instruction multiset -- the original
 * issues `lw primitive` three slots later and `numPrimitive++` six slots
 * later than gcc 2.96 does here.
 *
 * The permutation is exactly two "move one instruction down" edits, and
 * --rotate with a negative LEN already IS that edit. What blocks it is
 * that the two windows OVERLAP (10..13 and 12..18), and rotate_insns
 * resolves every site against one left-to-right scan, so only the first
 * of two overlapping sites can fire. Applying the same two sites as two
 * SEQUENTIAL fix_cc_asm invocations (the .s is rewritten in place, so
 * chaining is exact) gives a byte-perfect match:
 *
 *   pass 1:  --rotate JS_loadClass:12:-7      (numPrimitive++ to the end)
 *   pass 2:  --rotate JS_loadClass:10:-4      (lw primitive down three)
 *
 * Verified: JS.c goes 6 match/1 not -> 7 match/0 not, and the function is
 * byte-identical over all 27 words. Individually the sites give 7 diffs
 * (site 10 alone) and 4 diffs (site 12 alone); both in one pass gives 7,
 * i.e. the second site is silently dropped.
 *
 * The ask is therefore a tools/fix_cc_asm.py change, not a per-file flag:
 * let --rotate apply its sites one at a time, re-resolving indices after
 * each (or let a pass list be split into ordered groups). Once that
 * exists, wire the two sites above for JS.c and register:
 *   JS_loadClass = 0x0026B1A0, 0x6C; // JS.c
 */
/* Register a class: one class slot plus a type-7 primitive referencing it */
JCLASS *JS_loadClass(int nName)
{
    JCLASS *pClass;
    PRIM *pPrim;

    pClass = &classes[numClass++];
    pPrim = &primitive[numPrimitive++];

    pClass->f0 = nName;
    pPrim->nType = 7;
    pPrim->f4 = nName;
    pPrim->f8 = (int) pClass;
    return pClass;
}

/* Append an integer constant to the primitive pool */
int JS_loadConstInteger(int nValue, int nExtra)
{
    PRIM *pPrim = &primitive[numPrimitive++];

    pPrim->nType = 1;
    pPrim->f4 = nValue;
    pPrim->f8 = nExtra;
    return 0;
}

/* Bind a class to its peer table and reset its method count */
void JS_classSetup(JCLASS *pClass, int nPeer)
{
    pClass->f4 = nPeer;
    pClass->nMethods = 0;
}

/* Append a method entry to a class */
void JS_classAddMethod(JCLASS *pClass, int nName, int pFunc)
{
    PRIM *pMethod = &pClass->aMethod[pClass->nMethods++];

    pMethod->nType = 8;
    pMethod->f4 = nName;
    pMethod->f8 = pFunc;
}

/* Return the native peer for the light class, or null for other handles */
int *JS_classLight_getPeer(int nHandle)
{
    if (nHandle == -1) {
        return D_004DA5A8;
    }
    return 0;
}
