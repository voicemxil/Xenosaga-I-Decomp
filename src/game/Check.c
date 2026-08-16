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
    char pad6B[0x3775];
    int nUnk37E0;               /* 0x37E0 */
    char pad37E4[0xCC];
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
extern float Get_Decimal_Surplus_for_Radius(float f);
void CheckDoorPos(DOORACTOR *a, VECTOR8 *pOut);

/* Compute a door's approach distance, blending toward its linked map unit.

   65535.0f is the "unreachable" sentinel. Spelling it as a literal rather
   than as the extern at 0x004D7F4C is what keeps it out of the branch delay
   slot: gcc will not put an `li.s` (potentially multi-instruction) into a
   slot, so the load stays above the branch and the slot is filled eagerly
   from the epilogue instead -- which is the original's bc1tl. */
float CheckDoorDist(DOORACTOR *a)
{
    DOORTARGET *pDoor = D_00338684[0];
    VECTOR8 buf;

    if (fabsf(pDoor->pos.y - a->pos.y) > 2.0f) {
        return 65535.0f;
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

/* True when any live, non-frozen enemy is electrified.

   This was a long-standing near-miss at 27 diffs, blamed on induction
   variables. The real cause was the ENEPC layout: the struct was all char
   padding, so it had alignment 1, and gcc then refuses to fold the array
   index into a scaled address. Giving the struct a genuine int member (the
   0x37E0 flag word Check_EnemyFound reads) raises its alignment to 4 and
   the plain enepc[i] indexing falls out matching. */
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

/* True when `angle` lies in the [lo,hi] arc, wrapping when lo > hi.

   The parameters are transformed in place rather than into fresh locals:
   that is what makes each result share its argument's callee-saved
   register (angle and its wrapped value both in $f20, hi and its in $f21),
   which is the whole register assignment. With separate locals gcc leaves
   the last result in $f0 and swaps the $f20/$f21 roles. The && / || also
   have to be spelled as `if (...) return 1; return 0;` -- returning the
   comparison directly makes gcc build the boolean in a $v1 temp. */
int Check_Angle(float angle, float lo, float hi)
{
    lo = Get_Decimal_Surplus_for_Radius(lo);
    hi = Get_Decimal_Surplus_for_Radius(hi);
    angle = Get_Decimal_Surplus_for_Radius(angle);

    if (lo <= hi) {
        if (lo <= angle && angle <= hi) {
            return 1;
        }
        return 0;
    }
    if (lo <= angle || angle <= hi) {
        return 1;
    }
    return 0;
}

/* True when any live, non-frozen enemy carries a "found" attribute */
int Check_EnemyFound(void)
{
    short i;

    for (i = 0; i < 16; i++) {
        if (actor[i].nUnk86 > 0 && actor[i].nUnk82 != 1) {
            if ((enepc[i].nUnk37E0 & 1) != 0) {
                if (enepc[i].nAttr == 6 || enepc[i].nAttr == 10 ||
                    enepc[i].nAttr == 5) {
                    return 1;
                }
            }
        }
    }
    return 0;
}

extern float Get_Distance3D(float *, float *);
extern float Get_Angle(float *, float *);
extern int Check_Angle(float, float, float);
extern float D_004D81C8;

/* Is b inside the cone of half-width fWidth (in degrees) centred on
   fAngle at a, and no further than fDist? */
int Check_InsideFan_Wooo(float *a, float *b, float fAngle, float fDist,
                         float fWidth)
{
    float angle;
    float half;

    if (fDist < Get_Distance3D(a, b)) {
        return 0;
    }
    angle = Get_Angle(a, b);
    half = fWidth / 180.0f * D_004D81C8 * 0.5f;
    return Check_Angle(angle, fAngle - half, fAngle + half) != 0;
}
