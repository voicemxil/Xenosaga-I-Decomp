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

/* TODO: near-match (LENGTH) - gcc schedules the range subtraction relative to
 * the xglSRand call differently than the original; every variant tried
 * (combined vs split subtraction/increment) is one instruction short. */
/* Return a random integer in the inclusive range */
int Get_Rnd(int minimum, int maximum)
{
    int range = maximum - minimum;
    int random;

    random = xglSRand();
    range++;
    return minimum + random % range;
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
void Get_MiddlePoint(VECTOR *from, VECTOR *to, short current, short total,
    VECTOR *result)
{
    if (current == total) {
        result->x = to->x;
        result->y = to->y;
        result->w = 1.0f;
        result->z = to->z;
    } else {
        float current_float = (float)current;
        float total_float = (float)total;

        result->x = from->x + (to->x - from->x) * current_float / total_float;
        result->y = from->y + (to->y - from->y) * current_float / total_float;
        result->w = 1.0f;
        result->z = from->z + (to->z - from->z) * current_float / total_float;
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

/* TODO: near-miss (LOGIC) - gcc schedules the branch-taken constant load with a
 * bnezl and hoists the gp-relative float load unconditionally; both if/else
 * orderings tried give the same 12-word diff (store-group reorder included). */
/* Compute an object's on-screen center point, biasing Y by a type-dependent offset */
void GetCenter(VECTOR *out, CENTEROBJ *p)
{
    float extra;

    if (p->hType < 1618) {
        extra = D_00347D08;
    } else {
        extra = 3.0f;
    }
    out->x = D_004DC9D0.x + p->fX;
    out->y = D_004DC9D0.y + p->fY + extra;
    out->w = 1.0f;
    out->z = D_004DC9D0.z + p->fZ;
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
