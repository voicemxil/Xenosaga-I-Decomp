/* Assorted accessor helpers - map unit signals, ladder/arrival tables and
   enemy actor lookups */

typedef struct {
    char pad0[0x1A4];
    signed char nSignal;        /* 0x1A4 */
} UWAMONO;

typedef struct {
    float x;
    float y;
    float z;
    float w;
} VECTOR;

typedef struct {
    unsigned char pad0[0x80];
    unsigned char nReaction;    /* 0x80 */
} JCHR;

typedef struct {
    int nUnk00;                 /* 0x00 */
    char pad04[0x38AC];
} JAVAREACTION;

typedef struct {
    char pad0[0x130];
    short aMotion[16];          /* 0x130 */
} MOTIONTBL;

typedef struct {
    char pad0[0x37B8];
    int nId;                    /* 0x37B8 */
    char pad37BC[0xF4];
} ENEPC;

extern JAVAREACTION D_0037C8F0[];
extern ENEPC enepc[];

extern float atan2f(float y, float x);
extern float sqrtf(float f);

/* Read the signal byte cached on a map unit */
signed char GetUwamonoSignal(UWAMONO *pUnit)
{
    return pUnit->nSignal;
}

/* Look an arrival point id up in the fixed table, -1 when unknown */
short Get_Arrival(int nId)
{
    int aTbl[4] = { 0x200000, 0x210000, 0x220000, 0x230000 };
    short i;

    for (i = 0; i < 4; i++) {
        if (nId == aTbl[i]) {
            return i;
        }
    }
    return -1;
}

/* Fetch the java reaction code for a character */
int Get_JAVAReaction(JCHR *pChr)
{
    return D_0037C8F0[pChr->nReaction].nUnk00;
}

/* Fetch a default motion id from the table */
short Get_DefaultMotion(MOTIONTBL *pTbl, short nIdx)
{
    short *pMotion = pTbl->aMotion;

    return pMotion[nIdx];
}

/* Find the actor slot holding the given enemy id, -1 when absent */
short Get_ActorNumber(int nId)
{
    short i;

    for (i = 0; i < 64; i++) {
        if (enepc[i].nId == nId) {
            return i;
        }
    }
    return -1;
}

/* Planar (XZ) distance between two points */
float Get_Distance(VECTOR *pFrom, VECTOR *pTo)
{
    float dx, dz, x0, z0;

    x0 = pFrom->x;
    z0 = pFrom->z;
    dx = pTo->x - x0;
    dz = pTo->z - z0;
    return sqrtf(dx * dx + dz * dz);
}

/* Full 3D distance between two points */
float Get_Distance3D(VECTOR *pFrom, VECTOR *pTo)
{
    float dx, dy, dz, x0, y0, z0;

    x0 = pFrom->x;
    y0 = pFrom->y;
    z0 = pFrom->z;
    dx = pTo->x - x0;
    dy = pTo->y - y0;
    dz = pTo->z - z0;
    return sqrtf(dx * dx + dy * dy + dz * dz);
}

/* Horizontal angle from one point to another */
float Get_Angle(VECTOR *pFrom, VECTOR *pTo)
{
    return atan2f(pTo->x - pFrom->x, pTo->z - pFrom->z);
}

/* Solve for the X-axis crossing ratio between two points, 0 when parallel */
int GetIntersectionPointLineX(float *pRate, VECTOR *p1, VECTOR *p2, float x)
{
    if (p1->x == p2->x) {
        *pRate = 0.0f;
        return 0;
    }
    *pRate = (x - p1->x) / (p2->x - p1->x);
    return 1;
}

/* Solve for the Z-axis crossing ratio between two points, 0 when parallel */
int GetIntersectionPointLineZ(float *pRate, VECTOR *p1, VECTOR *p2, float z)
{
    if (p1->z == p2->z) {
        *pRate = 0.0f;
        return 0;
    }
    *pRate = (z - p1->z) / (p2->z - p1->z);
    return 1;
}

/* Model file name accessor (stubbed out in the shipped build) */
void GetMdlFileName(void)
{
}
