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

/* A script parse-tree node as the built-in Light class methods see it.
   The three value slots are ints for setColor (0..255 channels, scaled to
   0..1 on the way in) and floats for setDirection2. */
/* Reading the node's value slots through a union gives those reads the
   union's alias set, which aliases everything.  That is what keeps the
   float stores below from being hoisted past them: with plain `int`
   slots, gcc 2.96's -fstrict-aliasing proves a float store cannot touch
   an int load and reorders the whole block. */
typedef union JSVALUE
{
    int   i;
    float f;
} JSVALUE;

typedef struct JSNODE
{
    char    pad000[0x0C];
    int     nMode;      /* 0x0C */
    char    pad010[0x18];
    int     nIndex;     /* 0x28 - 1..3 select the directional lights */
    JSVALUE aValue[3];  /* 0x2C */
} JSNODE;

extern void xglStudioGetLight(void **ppLight);
extern void initLight2(void);

/* TOOL REQUEST (verified, with numbers).  Source is at its minimum here:
   sixteen shapes were swept (index local vs inlined, separate range
   temp, pre-computed offset, `<<5` vs `*32`, a float* source cursor, an
   array for the studio pointer, an early `nRet = 1`, a trailing
   `bad: return 1` block, one shared float temp, two alternating float
   temps in both declaration orders, reversed store order, and an
   int-typed copy).  Reusing nIndex for its own scaled offset -- the form
   below -- is the best at 8 diffs, down from 10.  One shared float temp
   reaches 4 but pins all three copies to $f0, which the whole-function
   FP rename below cannot then split, so it is the wrong base to build
   on; the trailing-`bad` block that setColor needs makes this one worse
   (10 diffs), because here the guard is the function's first branch.

   (Numbers below re-measured after the JSVALUE union landed; the union
   is what fixes setColor's whole FP body, but it does not change this
   function's register parity.)

   Six of those eight are a floating-point register-numbering tie-break:
   the original alternates $f0/$f1/$f0 across the three copies and gcc
   2.96 alternates $f1/$f0/$f1.  --swap-regs cannot express it, because
   its `\$%d\b` pattern never matches an `$fN` token.  Teaching it FP
   tokens (or adding --swap-fpregs FUNC:A-B) is the ask; I prototyped that
   rename as a post-pass and measured it:
     FP swap alone                                8 diffs -> 2 diffs
     FP swap + --swap-adjacent
         JS_classLight_setDirection2:6            MATCH, all 26 words
         (JS.c 6 match/1 not -> 7 match/0 not)
   The last two diffs are the `li v0,1` / `lw a0,nIndex` pair issuing in
   the other order, which is what that swap-adjacent site fixes.
   Once both exist, register:
     JS_classLight_setDirection2 = 0x0026BF18, 0x68; // JS.c

   Light.setDirection2(x, y, z) -- write one directional light's vector
   straight into the current studio's light block. */
int JS_classLight_setDirection2(JSNODE *pNode)
{
    void *pLight;
    int nIndex;
    float *pDir;

    xglStudioGetLight(&pLight);
    nIndex = pNode->nIndex;
    if ((unsigned int)(nIndex - 1) >= 3) {
        return 1;
    }
    nIndex = nIndex * 32;
    pDir = (float *)((char *)pLight + nIndex);
    pDir[0] = pNode->aValue[0].f;
    pDir[1] = pNode->aValue[1].f;
    pDir[2] = pNode->aValue[2].f;
    initLight2();
    return 0;
}


/* Light.setColor(r, g, b) -- slot 0 is the ambient colour at the head of
   the light block, slots 1..3 are the directional colours 16 bytes into
   each 32-byte light record.  Node mode 3 carries 0..255 integers that are
   scaled to 0..1; any other mode carries floats already. */
int JS_classLight_setColor(JSNODE *pNode)
{
    void *pLight;
    int nIndex;
    float *pDst;
    float fScale;

    xglStudioGetLight(&pLight);
    nIndex = pNode->nIndex;
    if (nIndex == 0) {
        pDst = (float *)pLight;
    } else {
        if ((unsigned int)(nIndex - 1) >= 3) {
            goto bad;
        }
        pDst = (float *)((char *)pLight + nIndex * 32 - 16);
    }
    if (pNode->nMode == 3) {
        fScale = 255.0f;
        pDst[0] = (float)pNode->aValue[0].i / fScale;
        pDst[1] = (float)pNode->aValue[1].i / fScale;
        pDst[2] = (float)pNode->aValue[2].i / fScale;
    } else {
        pDst[0] = pNode->aValue[0].f;
        pDst[1] = pNode->aValue[1].f;
        pDst[2] = pNode->aValue[2].f;
    }
    initLight2();
    return 0;

    /* The bad-index exit has to be its own trailing block: written as a
       plain `return 1` inside the guard, gcc hoists the constant above the
       branch and fills the delay slot with an epilogue load instead, which
       costs that word plus an alignment nop. */
bad:
    return 1;
}
