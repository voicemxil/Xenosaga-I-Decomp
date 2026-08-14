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
