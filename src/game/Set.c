/* Set - setter helpers spanning several gameplay translation units; grouped
 * here only because their names share the Set/Set_ prefix, not because they
 * share a translation unit in the original (see per-function notes).
 */

/* enepc[] element is a large per-enemy-pc struct; only a handful of fields
 * are needed here, so pad the rest out to the real 0x38B0 stride. */
typedef struct {
    char pad000[0x40];
    short field40;                /* 0x40 */
    short field42;                /* 0x42 */
    char pad044[0x74 - 0x44];
    short nMarkTimer;             /* 0x74 */
    short nMarkFlag;              /* 0x76 */
    char pad078[0x1080 - 0x78];
    short field1080;              /* 0x1080 */
    char pad1082[0x3094 - 0x1082];
    short field3094;              /* 0x3094 */
    char pad3096[0x38B0 - 0x3096];
} ENEPC;

/* The unit-ish struct passed in only needs its enemy-mark index and, for
 * the spline setters, the movement-speed field at 0x9E4. */
typedef struct {
    char pad000[0x80];
    unsigned char nEnemyMarkIdx;  /* 0x80 */
    char pad081[0x9E4 - 0x81];
    float speed;                  /* 0x9E4 */
} SPLINE_UNIT;

extern ENEPC enepc[];
extern void BSpline_Init(void *pSpline, void *pPath, float speed);

/* Raw EE kernel syscall stub (see src/sdk/kernel.c); declared here with an
   int return so the syscall's v0 result flows straight back out. */
extern int _SetTLBEntry(int nIndex);

/* TODO: near-match (LOGIC, 8/12) - byte-identical length (0x30) to the
 * original, and the compare/branch-vs-call skeleton is right, but two
 * details differ: (1) original's branch polarity takes the *success* path
 * (bnez -> jal) and falls through to a shared "b -> li v0,-1" tail that
 * lands on the common epilogue, where ours computes the same test but
 * schedules it before the sp/ra setup and doesn't share the epilogue with
 * the -1 path; (2) tried both `if(in_range) return call()` and the
 * inverted `if(!in_range) return -1` forms plus an explicit nRet local for
 * a shared-tail goto shape -- the local grew the function to 0x38 bytes
 * (duplicate epilogue got worse, not better) and the inverted form also
 * regressed. Kept the smallest (8-diff, exact-length) variant.
 * ROOT CAUSE (later session): the blocker is gcc's sibling-call pass. We
 * emit `ld ra; j _SetTLBEntry; addiu sp,16`; the original emits a real
 * `jal ...; nop` plus a shared epilogue. -fno-optimize-sibling-calls DOES
 * restore the jal, but lands at 11 words vs the original 12 (still missing
 * the original's `b` to the shared epilogue, i.e. the -1 block laid out
 * physically first), so the flag is not a fix and is NOT worth requesting:
 * with it Set.c is still 2 match / 1 not. No source shape reproduces the
 * jal without it -- also tried unprototyped `extern int _SetTLBEntry();`,
 * LAUNDER/SCHED_NOP on the call result, and dropping the `return` on the
 * call path. MMathCalcDirVector in MMath.c has the identical blocker. */
/* Install a TLB entry for indices 13..47 (the mappable range), else -1 */
int SetTLBEntry(int nIndex)
{
    if ((unsigned int)(nIndex - 13) < 35) {
        return _SetTLBEntry(nIndex);
    }
    return -1;
}

/* Arm (or clear) the on-screen enemy mark timer for a unit's enepc slot */
void Set_EnemyMark(SPLINE_UNIT *pUnit, short nTimer)
{
    ENEPC *pEnepc = &enepc[pUnit->nEnemyMarkIdx];

    pEnepc->nMarkTimer = nTimer;
    pEnepc->nMarkFlag = 0;
}

/* Start a unit's movement along a freshly-seeded random b-spline path */
void Set_Spline_By_Random(SPLINE_UNIT *pUnit)
{
    void *pPath = (char *)pUnit + 0x10;
    ENEPC *pEnepc = &enepc[pUnit->nEnemyMarkIdx];

    BSpline_Init((char *)pEnepc + 0x80, pPath, pUnit->speed);
    pEnepc->field1080 = 4;
    pEnepc->field42 = 1;
    pEnepc->field40 = 0;
}
