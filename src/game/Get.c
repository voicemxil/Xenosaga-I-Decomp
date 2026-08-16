/* Assorted accessor helpers - map unit signals, ladder/arrival tables and
   enemy actor lookups */

#include "matching.h"

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
extern float LocaterAngle[8][2];

extern float atan2f(float y, float x);
extern float sqrtf(float f);
extern float sinf(float f);
extern float cosf(float f);
extern int xglSRand(void);

float Get_Distance(VECTOR *pFrom, VECTOR *pTo);

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

/* The separate max/min locals plus the LAUNDERs are what reproduce the
 * original's redundant move-then-subtract (it keeps max and min live in
 * their own registers across the xglSRand call instead of folding the
 * subtraction into the argument setup). The last tie-break -- the
 * original saves ra AFTER the subtract, gcc before it -- is not
 * reachable from C (PIN(range,"$16") and SCHED_NOP at the boundary both
 * regressed it), so it is corrected by --swap-adjacent Get_Rnd:5, a
 * transposition of two independent instructions verified by
 * tools/audit_swaps.py. */
/* Return a random integer in the inclusive range */
int Get_Rnd(int minimum, int maximum)
{
    int max = maximum;
    int min = minimum;
    int range;
    int random;

    LAUNDER(max);
    LAUNDER(min);
    range = max - min;
    LAUNDER_V(range);
    random = xglSRand();
    range++;
    return min + random % range;
}

/* Return the sector containing an angle, or -1 outside every sector. */
signed char Get_LocaterType_Angle(float angle)
{
    short index = 0;

    while (index < 8) {
        if ((LocaterAngle[index][0] < angle) &&
            (angle < LocaterAngle[index][1])) {
            break;
        }
        index++;
    }
    if (index != 8) {
        return index;
    }
    return -1;
}

/* TODO: near-match (LOGIC) - the interpolation arithmetic is recovered, but
 * the original uses a different temporary floating-register allocation and
 * store schedule. Find the original local-variable/source-expression shape. */
/* Interpolate a position between two points; the output always has w = 1. */
/* Store order matters: the `result->w = 1.0f` store must be written LAST in
   both branches, not third. gcc 2.9x emits it earlier than written, so the
   source order that reproduces the original is not the original's emission
   order. (Same lever fixed GetCenter's store block; found by exhausting all
   24 orderings per branch.) */
void Get_MiddlePoint(VECTOR *from, VECTOR *to, short current, short total,
    VECTOR *result)
{
    if (current == total) {
        result->x = to->x;
        result->y = to->y;
        result->z = to->z;
        result->w = 1.0f;
    } else {
        float current_float = (float)current;
        float total_float = (float)total;

        result->x = from->x + (to->x - from->x) * current_float / total_float;
        result->y = from->y + (to->y - from->y) * current_float / total_float;
        result->z = from->z + (to->z - from->z) * current_float / total_float;
        result->w = 1.0f;
    }
}

/* Produce a planar step from one point toward another. */
void Get_One_Step(VECTOR *from, VECTOR *to, VECTOR *result, float amount)
{
    float dx = to->x - from->x;
    float dz = to->z - from->z;
    float scale = 1.0f / Get_Distance(from, to);
    float x_step = dx * scale * amount;
    float z_step = dz * scale * amount;

    result->y = 0.0f;
    result->w = 1.0f;
    result->x = x_step;
    result->z = z_step;
}

/* Project a point by an angle and planar distance */
void Get_Point_By_AngleLength(VECTOR *source, VECTOR *result,
    float angle, float length)
{
    result->x = source->x + sinf(angle) * length;
    result->y = source->y;
    result->z = source->z + cosf(angle) * length;
    result->w = source->w;
}

/* Return the signed planar angle relative to a facing direction */
float Get_Angle_Relative(VECTOR *source, VECTOR *target, float facing)
{
    float angle = Get_Angle(source, target) - facing;

    if (angle < -3.141592741f) {
        angle += 6.283185482f;
    }
    if (angle > 3.141592741f) {
        angle -= 6.283185482f;
    }
    return angle;
}

/* Fold a value into the highest interval below a maximum */
float Get_Multi_Max_Under(float value, float step, float maximum)
{
    float result = value;

    if (result < maximum) {
        maximum -= step;
        if (result <= maximum) {
            do {
                result += step;
            } while (result <= maximum);
        }
    } else {
        maximum -= step;
        while (!(result <= maximum)) {
            result -= step;
        }
    }
    return result;
}

/* Wrap an angle into the signed radius interval */
float Get_Decimal_Surplus_for_Radius(float angle)
{
    if (angle <= 0.0f) {
        if (angle < -3.141592741f) {
            do {
                angle += 6.283185482f;
            } while (angle < -3.141592741f);
        }
    } else {
        while (3.141592741f <= angle) {
            angle -= 6.283185482f;
        }
    }
    return angle;
}

typedef struct {
    char pad0[0x10];
    float fX;      /* 0x10 */
    float fY;      /* 0x14 */
    float fZ;      /* 0x18 */
    char pad1C[0x86 - 0x1C];
    short hType;   /* 0x86 */
} CENTEROBJ;

extern VECTOR D_004DC9D0;
extern float D_00347D08;

/* TODO: near-miss (LOGIC, 12 diffs) - gcc schedules the branch-taken constant
 * load with a bnezl and hoists the gp-relative float load unconditionally;
 * both if/else orderings tried give the same 12-word diff (store-group
 * reorder included). Retried this session: writing it as
 * `float extra = D_00347D08; if (hType >= 1618) extra = 3.0f;` (matching the
 * original's "unconditional default, conditional override" shape exactly)
 * turns the bnezl into a plain bnez with the default load correctly filling
 * its delay slot -- but gcc then reuses that same delay slot for the load
 * instead of the &D_004DC9D0 address computation the original puts there,
 * pushing everything downstream by one slot (still 12 diffs, 9 opcode).
 * LAUNDER_V(extra) right after the initializer to pin it out of the delay
 * slot regressed to LENGTH (28 vs 25 words) -- it also blocked the
 * address-computation code motion the original relies on.
 * Later session: 12 -> 6 by writing the w store LAST (exhaustive over all
 * 24 orderings of the four output stores; the original emits w's swc1 near
 * the end, not third). The remaining 6 are still exactly the delay-slot
 * preference described above. Also tried hoisting &D_004DC9D0 into a local
 * VECTOR * before the if, in four forms (plain, after the extra load, with
 * LAUNDER, and pinned to $v0): all regress to 22-23 diffs, because gcc then
 * keeps the base in a register across the branch instead of rematerialising
 * the lui in the delay slot, which is the whole point. */
/* Compute an object's on-screen center point, biasing Y by a type-dependent offset */
void GetCenter(VECTOR *out, CENTEROBJ *p)
{
    float extra = D_00347D08;

    if (p->hType >= 1618) {
        extra = 3.0f;
    }
    out->x = D_004DC9D0.x + p->fX;
    out->y = D_004DC9D0.y + p->fY + extra;
    out->z = D_004DC9D0.z + p->fZ;
    out->w = 1.0f;
}

/* TODO: near-miss (LOGIC) - register/control-flow shape differs substantially
 * from the natural indexed-loop C below (26/27 words differ); parked after 2
 * attempts, needs a from-asm-first rewrite using a single advancing pointer
 * rather than a separate index local. */
/* Scan a (possibly SJIS double-byte) string for the byte preceding the terminator or the first
   run of high-bit bytes, returning 0 for an empty or NULL string */
int GetLastWord(char *s)
{
    int i;
    signed char c;

    if (s == 0) {
        return 0;
    }
    i = 0;
    c = s[0];
    if (c != 0 && !(c & 0x80)) {
        i = 1;
        for (;;) {
            c = s[i];
            if (c == 0) {
                break;
            }
            if (c & 0x80) {
                break;
            }
            i++;
        }
    }
    if (i == 0) {
        return 0;
    }
    return s[i - 1];
}

typedef struct {
    int f00;             /* 0x00 */
    char pad04[4];
    short f08;            /* 0x08 */
    char pad0A[0xE];
    int f18;               /* 0x18 */
    char pad1C[4];
    long long f20;          /* 0x20 */
} UNDUPARAM;

extern UNDUPARAM D_003B2610;
extern void UnduParamInit(UNDUPARAM *param);
extern int UnduDataGetHeader(int, int);
extern void UnduCheck(void *, int, UNDUPARAM *);
extern void *D_00338684[];

/* Three details are load-bearing here and in Get_Attr_NU below.
 *
 * The setup calls go through the `param` local while the query and the
 * result read name D_003B2610 directly: the original keeps the struct
 * base in TWO registers (s4 from the address computation, s2 a copy of
 * it), and that split only appears when the address reaches the two
 * halves of the function by two different routes.
 *
 * `nFlags | 0x800` sits AFTER UnduParamInit(): computed before the call
 * gcc fuses the argument copy and the or into one `ori s0,a2,0x800`,
 * where the original has the plain `move s0,a2` arg copy and a separate
 * `ori s0,s0,0x800`.
 *
 * `param->f00 = 0` is unconditional (UnduParamInit has already zeroed
 * it, so this is a redundant re-store either way). Inside the `if` it
 * cannot go anywhere; hoisted out, it is what the original puts in the
 * nType branch's delay slot. */
int Get_Attr(void *actor, int nType, int nFlags)
{
    UNDUPARAM *param;

    param = &D_003B2610;
    UnduParamInit(param);
    param->f08 = nFlags | 0x800;
    param->f00 = 0;
    if (nType == 0) {
        param->f18 = ((int *)D_00338684[0])[312];
    } else {
        param->f18 = UnduDataGetHeader(nType, 0x8000);
    }
    UnduCheck(actor, 0, &D_003B2610);
    return (int)D_003B2610.f20;
}

/* Same two-register/delay-slot levers as Get_Attr above. Note this
 * variant does NOT set the 0x800 flag bit -- the original stores nFlags
 * straight into f08, with no `ori`. */
int Get_Attr_NU(void *actor, int nType, int nFlags)
{
    UNDUPARAM *param;

    param = &D_003B2610;
    UnduParamInit(param);
    param->f08 = nFlags;
    param->f00 = 0;
    if (nType == 0) {
        param->f18 = ((int *)D_00338684[0])[312];
    } else {
        param->f18 = UnduDataGetHeader(nType, 0x8000);
    }
    UnduCheck(actor, 0, &D_003B2610);
    return (int)D_003B2610.f20;
}
