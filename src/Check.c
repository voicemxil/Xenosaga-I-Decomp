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

/* Same fields as VECTOR, but 8-byte aligned so a whole-struct copy uses
   ld/sd instead of the unaligned ldl/ldr/sdl/sdr sequence */
typedef struct {
    float x;
    float y;
    float z;
    float w;
} __attribute__((aligned(8))) VECTOR8;

typedef struct {
    unsigned char pad0C0[0xC0];
    VECTOR8 pos;                /* 0xC0 */
    unsigned char pad1D0[0xD5]; /* 0xD0 .. 0x1A5 */
    signed char nMapIdx;         /* 0x1A5 */
} DOORACTOR;

typedef struct {
    unsigned char pad00[0x10];
    VECTOR8 pos;                 /* 0x10 */
    unsigned char pad20[0x34];   /* 0x20 .. 0x54 */
    float fDir;                   /* 0x54 */
} DOORTARGET;

typedef struct {
    unsigned char pad00[0xC0];
    VECTOR8 pos;                  /* 0xC0 */
    unsigned char pad0D0[0x230]; /* pad to the 0x300 array stride */
} MAPUNIT2;

/* Non-gp_rel: an incomplete array keeps a <=8-byte extern out of sdata */
extern DOORTARGET *D_00338684[];
extern MAPUNIT2 MapUnit[];
extern short D_00490DBA[];
extern float D_004D7F4C;
extern float D_004D7F50;

extern float fabsf(float f);
extern float atan2f(float y, float x);
extern float nearDir(float a, float b);
void CheckDoorPos(DOORACTOR *a, VECTOR8 *pOut);

/* Compute a door's approach distance, blending toward its linked map unit */
/* TODO: near-miss - the a.y/pDoor.y load registers ($f1/$f2 vs original's
   $f2/$f1, with the 2.0f constant landing in $f2 not $f3) are an allocator
   tie-break; every statement ordering and operand direction tried gives the
   same pair of registers, so it looks unreachable from C alone. */
float CheckDoorDist(DOORACTOR *a)
{
    DOORTARGET *pDoor = D_00338684[0];
    VECTOR8 buf;

    if (fabsf(pDoor->pos.y - a->pos.y) > 2.0f) {
        return D_004D7F4C;
    }
    CheckDoorPos(a, &buf);
    return CheckDist2D((VECTOR *)&pDoor->pos, (VECTOR *)&buf);
}

/* True when the actor is aligned with its linked door's direction, within
   the switch-activation gate */
int CheckDoorSwitch(DOORACTOR *a)
{
    DOORTARGET *pDoor = D_00338684[0];
    VECTOR8 buf;
    int nRet;

    CheckDoorPos(a, &buf);
    if ((D_00490DBA[0] & 0x20) != 0) {
        if (fabsf(nearDir(atan2f(buf.x - pDoor->pos.x, buf.z - pDoor->pos.z),
                           pDoor->fDir)) < D_004D7F50) {
            return 1;
        }
    }
    nRet = 0;
    return nRet;
}

/* Fill pOut with a door actor's position, blended toward its linked map unit */
void CheckDoorPos(DOORACTOR *a, VECTOR8 *pOut)
{
    int nMapIdx = a->nMapIdx;
    MAPUNIT2 *m;
    float x, dx, z, dz;

    if (nMapIdx == -1) {
        *pOut = a->pos;
    } else {
        m = &MapUnit[nMapIdx];
        x = a->pos.x;
        dx = x - m->pos.x;
        pOut->x = x - dx * 0.5f;
        pOut->y = a->pos.y;
        z = a->pos.z;
        dz = z - m->pos.z;
        pOut->z = z - dz * 0.5f;
    }
}

/* True when any live, non-frozen enemy is electrified */
/* TODO: near-miss - LOGIC, 27/34 instructions differ. The original walks
   actor/enepc with two hand-incremented pointers (actor stride 0xA70,
   enepc stride 0x38B0) rather than index expressions, and its inner
   attribute check reuses a "move v0,a0" copy of the enepc pointer before
   the lb -- Check_EnemyBurn's simpler two-condition body matches with
   plain indexing, so the extra nesting here likely needs the pointer-pair
   loop shape instead. */
int Check_EnemyElec(void)
{
    short i;

    for (i = 0; i < 16; i++) {
        if (actor[i].nUnk86 > 0 && actor[i].nUnk82 != 1) {
            if (enepc[i].nAttr == 0xC || (enepc[i].nFlags6A & 2) != 0) {
                return 1;
            }
        }
    }
    return 0;
}

/* True when segment p1-p2 crosses segment p3-p4 */
int Check_CrossingOver(VECTOR *p1, VECTOR *p2, VECTOR *p3, VECTOR *p4)
{
    int a, b, c, d;

    a = CheckPointLine(p1, p2, p3);
    b = CheckPointLine(p1, p2, p4);
    if (a != b) {
        c = CheckPointLine(p3, p4, p1);
        d = CheckPointLine(p3, p4, p2);
        if (c != d) {
            return 1;
        }
    }
    return 0;
}

extern float Get_Decimal_Surplus_for_Radius(float angle);

/* True when angle (wrapped) falls within the [lo, hi] arc (wrapped, handling wraparound) */
/* TODO: near-miss (LOGIC) - the callee-saved register that ends up holding
 * "hi" vs "angle" across the first call is swapped from the original, and
 * the &&/|| short-circuit form uses an extra v0/v1 temp the original
 * doesn't; a nested-if rewrite fixed the boolean shape but made the length
 * worse (41 vs 45), so the register save order looks like the harder half
 * of this one. Parked after 2 attempts. */
int Check_Angle(float angle, float lo, float hi)
{
    float wLo, wHi, wAngle;

    wLo = Get_Decimal_Surplus_for_Radius(lo);
    wHi = Get_Decimal_Surplus_for_Radius(hi);
    wAngle = Get_Decimal_Surplus_for_Radius(angle);

    if (wLo <= wHi) {
        return (wLo <= wAngle) && (wAngle <= wHi);
    } else {
        return (wLo <= wAngle) || (wAngle <= wHi);
    }
}
