/* Predicate helpers - actor liveness, broken map units, segment crossing and
   enemy status queries */

typedef struct {
    float x;
    float y;
    float z;
    float w;
} VECTOR;

typedef struct {
    int nFlags;                 /* 0x00 */
    char pad04[0x7E];
    unsigned char nUnk82;       /* 0x82 */
    char pad83[3];
    short nUnk86;               /* 0x86 */
    char pad88[0x874];
    int nUnk8FC;                /* 0x8FC */
    char pad900[0x170];
} ACTOR;

typedef struct {
    char pad0[0x48];
    signed char nAttr;          /* 0x48 */
    char pad49[0x21];
    unsigned char nFlags6A;     /* 0x6A */
    char pad6B[0x3845];
} ENEPC;

typedef struct {
    char pad0[4];
    signed char nSignal;        /* 0x04 */
    char pad5[0x3F];
    int nUnk44;                 /* 0x44 */
} UWAMONO;

typedef struct {
    int nFlags;                 /* 0x00 */
    char pad04[0xA0];
    short nUnkA4;               /* 0xA4 */
    char padA6[0xFA];
    UWAMONO uwamono;            /* 0x1A0 */
} MAPUNIT;

extern ACTOR actor[];
extern ENEPC enepc[];

extern float sqrtf(float f);

/* True when the actor slot holds a live, undestroyed actor */
int CheckActorExist(ACTOR *pActor)
{
    if (pActor->nUnk86 <= 0) {
        return 0;
    }
    if ((pActor->nFlags & 8) != 0) {
        return 0;
    }
    return pActor->nUnk8FC == 0;
}

/* Return the break state of a map unit, 0 when it cannot break */
int CheckBrokenMapUnit(MAPUNIT *pUnit)
{
    UWAMONO *pUwamono = &pUnit->uwamono;
    int nState;

    if (pUnit->nUnkA4 == -1) {
        return 0;
    }
    if ((pUnit->nFlags & 0x10000) == 0) {
        return 0;
    }
    if ((pUnit->nFlags & 4) != 0) {
        return 0;
    }
    if (pUwamono->nSignal == 1) {
        return 0;
    }
    nState = pUwamono->nUnk44;
    if (nState == -1) {
        nState = 0;
    }
    return nState;
}

/* Classify which side of segment p0-p1 the point p2 falls on */
int CheckPointLine(VECTOR *p0, VECTOR *p1, VECTOR *p2)
{
    VECTOR v1;
    VECTOR v2;
    float f;

    v1.x = p1->x - p0->x;
    v1.z = p1->z - p0->z;
    v2.x = p2->x - p0->x;
    v2.z = p2->z - p0->z;
    f = v1.x * v2.z - v1.z * v2.x;
    if (f > 0.0f) {
        return 1;
    }
    if (f < 0.0f) {
        return 2;
    }
    return 0;
}

/* True when segment p0-p1 crosses segment p2-p3 */
int CheckCrossLine(VECTOR *p0, VECTOR *p1, VECTOR *p2, VECTOR *p3)
{
    int a, b, c, d;

    a = CheckPointLine(p0, p1, p2);
    b = CheckPointLine(p0, p1, p3);
    if (a != b) {
        c = CheckPointLine(p2, p3, p0);
        d = CheckPointLine(p2, p3, p1);
        if (c != d) {
            return 1;
        }
    }
    return 0;
}

/* Full 3D distance between two points */
float CheckDist3D(VECTOR *p1, VECTOR *p2)
{
    float dx = p1->x - p2->x;
    float dy = p1->y - p2->y;
    float dz = p1->z - p2->z;

    return sqrtf(dx * dx + dy * dy + dz * dz);
}

/* Planar (XZ) distance between two points */
float CheckDist2D(VECTOR *p1, VECTOR *p2)
{
    float dx = p1->x - p2->x;
    float dz = p1->z - p2->z;

    return sqrtf(dx * dx + dz * dz);
}

/* True when any live enemy is burning */
int Check_EnemyBurn(void)
{
    short i;

    for (i = 0; i < 16; i++) {
        if (actor[i].nUnk86 > 0 && actor[i].nUnk82 != 1) {
            if (enepc[i].nAttr == 11 || enepc[i].nAttr == 7) {
                return 1;
            }
        }
    }
    return 0;
}

/* Sign of the cross product of segment p0-p1 against segment p2-p3 */
int CheckSlope(VECTOR *p0, VECTOR *p1, VECTOR *p2, VECTOR *p3)
{
    float f = (p1->x - p0->x) * (p3->z - p2->z) - (p1->z - p0->z) * (p3->x - p2->x);

    if (f == 0.0f) {
        return 0;
    }
    if (f > 0.0f) {
        return 1;
    }
    return -1;
}

/* True when a point lies strictly inside the XZ box */
int CheckInBox(VECTOR *pPos, VECTOR *pMin, VECTOR *pMax)
{
    if (pPos->x < pMax->x && pMin->x < pPos->x &&
        pPos->z < pMax->z && pMin->z < pPos->z) {
        return 1;
    }
    return 0;
}
