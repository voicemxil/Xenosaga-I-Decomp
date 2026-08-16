/* Cross-check predicates: line-of-sight tests between actors and map units */

typedef struct {
    float x;
    float y;
    float z;
    float w;
} VECTOR;

typedef struct {
    int nFlags;             /* 0x000 */
    char pad04[0x10];       /* to 0x014 */
    float fY;                /* 0x014 */
    char pad18[0x68];       /* to 0x080 */
    unsigned char nIdx80;    /* 0x080 */
    char pad81[5];           /* to 0x086 */
    short nUnk86;             /* 0x086 */
    char pad88[0x848];       /* to 0x8D0 */
    int nUnk8D0;              /* 0x8D0 */
    char pad8D4[0x114];      /* to 0x9E8 */
    float f9E8;               /* 0x9E8 */
    char pad9EC[0x84];       /* to 0xA70 */
} ACTOR_CC;

extern ACTOR_CC actor[];

typedef struct {
    int nFlags;              /* 0x000 */
    char pad04[0x10];        /* to 0x014 */
    float fY;                 /* 0x014 */
    char pad18[0x8C];        /* to 0x0A4 */
    short nUnkA4;              /* 0x0A4 */
    char padA6[0x127];       /* to 0x1CD */
    signed char nByte1CD;   /* 0x1CD */
    char pad1CE[0x132];      /* to 0x300 */
} MAPUNIT_CC;

extern MAPUNIT_CC MapUnit[];

extern float fabsf(float f);
extern int CheckCrossCircle(void *a0, void *a1, void *a2, float radius);
extern int CrossCheckMapUnitCircle(VECTOR *pPos, void *pOther, MAPUNIT_CC *pUnit);
extern int CrossCheckMapUnitBox(VECTOR *pPos, void *pOther, MAPUNIT_CC *pUnit);

/* TODO: near-miss - the fabsf/compare block and switch dispatch are logically
   right and register-correct (v1==m copy confirmed via the `MAPUNIT_CC *m`
   local), but the "int nRet = 0;" default only gets materialized right after
   the branch here, while the original hoists "move v0,zero" to right after
   the prologue (before any of the float loads). Declaration order of
   nRet/m did not change this. Two attempts spent; leaving as TODO. */
/* True when a target point sits close enough to a map unit's Y level and the
   unit's shape check (circle or box) reports a crossing hit */
int CrossCheckMapUnitAt(VECTOR *pPos, void *pOther, MAPUNIT_CC *pUnit)
{
    MAPUNIT_CC *m = pUnit;
    int nRet = 0;

    if (fabsf(pPos->y - m->fY) > 0.6f) {
        return nRet;
    }
    switch (m->nByte1CD) {
    case 1:
        return CrossCheckMapUnitCircle(pPos, pOther, m);
    case 2:
        return CrossCheckMapUnitBox(pPos, pOther, m);
    }
    return nRet;
}

/* TODO: near-miss - only 1 real logic diff (nByte1CD now signed char, fixed)
   plus a prologue-scheduling diff: the original computes s0 as a big
   constant (lui+addiu) minus 0x1A0 (416) in a 3rd addiu, implying the base
   value it starts from is some OTHER symbol/field 0x1A0 bytes past MapUnit's
   element 0 (maybe the uwamono sub-struct typed as the loop pointer), not
   MapUnit itself; our direct `&MapUnit[i]` gives the same runtime address
   but a different (masked-irrelevant since immediate, not relocation)
   0-vs-416 addiu operand and different sd-interleaving order. Two attempts
   spent; leaving as TODO. */
/* Scan every map unit for one whose Y band, flags and shape make it cross
   the pPos/pOther segment; returns the matching index or -1 */
/* TODO: near-miss (16 of 53 words) but the delta is ONE instruction and the
   loop body is byte-identical (words 18-39 all line up). The original builds
   the loop base in two steps:
       lui   v0, %hi(X)
       addiu v0, v0, %lo(X)      <- X resolves to 0x0048AEC0
       addiu s0, v0, -416        <- s0 = 0x0048AD20 = &MapUnit[0]
   i.e. it materialises a symbol 0x1A0 bytes PAST MapUnit and then backs off,
   where we fold straight to `addiu s0,v0,%lo(MapUnit)`. That extra addiu is
   the whole difference: it pushes the eight prologue register saves one slot
   each, which is what the other 15 diffs are.
   So the question is only "what symbol lives at MapUnit+0x1A0, and why does
   the original address MapUnit through it". Since checkfile masks %hi/%lo,
   the immediates above are the LINKED values, so X is a real distinct symbol
   in the original, not a folding artefact. Whoever picks this up: find the
   0x0048AEC0 symbol in the ELF symbol table; the source almost certainly
   names that one and reaches MapUnit as a negative offset from it (or
   declares the two adjacently in one TU so gcc CSEs the page address).
   Do NOT go looking for a scheduling lever -- there is nothing wrong with
   the schedule. */
int CrossCheckMapUnit(VECTOR *pPos, void *pOther)
{
    int i;
    MAPUNIT_CC *m;

    for (i = 0; i < 64; i++) {
        m = &MapUnit[i];
        if (m->nUnkA4 == -1) {
            continue;
        }
        if ((m->nFlags & 0x100000) != 0) {
            continue;
        }
        if (m->nByte1CD == 0) {
            continue;
        }
        if ((m->nFlags & 0x10000) == 0) {
            continue;
        }
        if (CrossCheckMapUnitAt(pPos, pOther, m) != 0) {
            return i;
        }
    }
    return -1;
}

/* TODO: near-miss (LENGTH, 64 orig vs 58 built) - the original tracks the
   CheckCrossCircle third argument via a SEPARATE zero-initialized
   accumulator (s3, stride 0xA70) added to a fixed base (s5 = actor+0x10)
   computed once outside the loop, rather than `(char *)pA + 0x10` computed
   fresh each iteration from the loop pointer -- same runtime address, but
   6 fewer instructions get emitted our way. This is the
   "pointer-into-pool, parallel accumulator" shape noted for
   CrossCheckMapUnitAim; have not found the source form that reproduces two
   independently-incremented pointers into the same array. One attempt
   spent; leaving as TODO. */
/* Scan every actor for one (other than pRef itself) within 0.3 Y units of
   pRef whose circle crosses the pRef/arg segment; returns the matching
   index or -1 */
int CrossCheckActor(ACTOR_CC *pRef, void *arg)
{
    int i;
    ACTOR_CC *pA;

    for (i = 0; i < 64; i++) {
        pA = &actor[i];
        if (fabsf(pRef->fY - pA->fY) > 0.3f) {
            continue;
        }
        if (pA->nUnk86 < 0) {
            continue;
        }
        if ((pA->nFlags & 8) != 0) {
            continue;
        }
        if (pA->nUnk8D0 == 0) {
            continue;
        }
        if (i == pRef->nIdx80) {
            continue;
        }
        if (CheckCrossCircle((char *)pRef + 0x10, arg, (char *)pA + 0x10, pA->f9E8)) {
            return i;
        }
    }
    return -1;
}
