/* Calc - geometry helpers shared by the field/mech code: the plane
 * distance and segment/plane intersection used for AGWS hangar and drill
 * placement. The vector subtracts and adds are VU0 macro-mode inline
 * asm, exactly as in xglPlaneParameter. */

#include "common.h"

typedef struct {
    float x;
    float y;
    float z;
    float w;
} MC_VECTOR;

extern void xglVectorInner(float *pDest, const float *pLeft, const float *pRight);
extern void xglVectorOuter(float *pDest, const float *pLeft, const float *pRight);
extern void xglVectorNormal(void *pDest, void *pSource);
extern void xglVectorLength(float *pDest, const float *pVector);

/* dst = *pTo - *pFrom, xyz only (VU0) */
#define VEC_SUB(dst, from, to)                          \
    __asm__ __volatile__(                               \
        "lqc2 $vf3, 0x0(%1)\n"                          \
        "lqc2 $vf2, 0x0(%2)\n"                          \
        "vsub.xyz $vf2xyz, $vf2xyz, $vf3xyz\n"          \
        "sqc2 $vf2, 0x0(%0)\n"                          \
        : : "r"(dst), "r"(from), "r"(to) : "memory")

/* Normal of the triangle (p0,p1,p2) rotated back into the p0->p1 edge's
 * plane: e0 x (e0 x e1), i.e. the in-plane vector perpendicular to e0. */
void CalcVerticalVector(MC_VECTOR *p0, MC_VECTOR *p1, MC_VECTOR *p2,
                        MC_VECTOR *pDst)
{
    MC_VECTOR e0;
    MC_VECTOR e1;
    MC_VECTOR n;

    VEC_SUB(&e0, p0, p1);
    VEC_SUB(&e1, p0, p2);
    xglVectorOuter((float *)&n, (float *)&e0, (float *)&e1);
    xglVectorOuter((float *)pDst, (float *)&e0, (float *)&n);
}

/* Signed distance from p0 to the p2 side, measured along the unit
 * in-plane perpendicular built by CalcVerticalVector. */
float CalcLength(MC_VECTOR *p0, MC_VECTOR *p1, MC_VECTOR *p2)
{
    MC_VECTOR v;
    MC_VECTOR d;
    float len;

    CalcVerticalVector(p0, p1, p2, &v);
    xglVectorNormal(&v, &v);
    VEC_SUB(&d, p2, p0);
    xglVectorInner(&len, (float *)&v, (float *)&d);
    return len;
}

/* Intersection of the segment p2->p3 with the plane through p0/p1/p2's
 * perpendicular; the result is written to pDst. */
void CalcCrossPoint(MC_VECTOR *p0, MC_VECTOR *p1, MC_VECTOR *p2,
                    MC_VECTOR *p3, MC_VECTOR *pDst)
{
    MC_VECTOR v;
    float dot;
    float len;

    len = CalcLength(p0, p1, p2);
    CalcVerticalVector(p0, p1, p2, &v);
    xglVectorNormal(&v, &v);
    VEC_SUB(pDst, p2, p3);
    xglVectorInner(&dot, (float *)pDst, (float *)&v);
    dot = len / dot;
    xglVectorLength(&len, (float *)pDst);
    xglVectorNormal(pDst, pDst);
    pDst->x = pDst->x * (dot * len);
    pDst->y = pDst->y * (dot * len);
    pDst->z = pDst->z * (dot * len);
    __asm__ __volatile__(
        "lqc2 $vf20, 0x0(%0)\n"
        "lqc2 $vf21, 0x0(%1)\n"
        "vadd.xyzw $vf20xyzw, $vf20xyzw, $vf21xyzw\n"
        "sqc2 $vf20, 0x0(%0)\n"
        : : "r"(pDst), "r"(p2) : "memory");
}

/* --- map-unit proximity ---------------------------------------------- */

/* 8-aligned so a whole-vector copy becomes the original's ld/sd pair
 * (same idiom as Hit.c's HitAlignedVector). */
typedef union MC_ALIGNED_VECTOR {
    MC_VECTOR value;
    long long alignment[2];
} MC_ALIGNED_VECTOR;

typedef struct MC_MATRIX {
    float m[4][4];
} MC_MATRIX;

typedef struct CALC_MAP_UNIT {
    char      pad_000[0x10];
    MC_ALIGNED_VECTOR position;     /* 0x010 */
    char      pad_020[0x60];
    MC_MATRIX *pDir;                /* 0x080 */
    char      pad_084[0x2C];
    MC_ALIGNED_VECTOR bounds;       /* 0x0B0 */
    char      pad_0C0[0x240];
} CALC_MAP_UNIT;

extern int CrossCheckMapUnitCircle(MC_VECTOR *pPos, void *pArg,
                                   CALC_MAP_UNIT *pUnit, MC_VECTOR *pOut);

/* Push pPos out to the rim of the unit's bounding circle. If the point is
 * already inside the radius the position is copied through unchanged; if
 * nothing was crossed the caller is told with a y of -1000. */
void CalcNearCrossPointCircle(MC_VECTOR *pPos, void *pArg,
                              CALC_MAP_UNIT *pUnit, MC_VECTOR *pOut)
{
    MC_VECTOR d;
    MC_VECTOR n;
    float fLen;

    if (CrossCheckMapUnitCircle(pPos, pArg, pUnit, pOut) != 0) {
        d = *pPos;
        d.y = pUnit->position.value.y;
        VEC_SUB(&d, pPos, &pUnit->position);
        xglVectorLength(&fLen, (float *)&d);
        xglVectorNormal(&n, &d);
        fLen = fLen - pUnit->bounds.value.x;
        if (fLen < 0.0f) {
            *pOut = *pPos;
            return;
        }
        pOut->x = pPos->x + fLen * n.x;
        pOut->y = pPos->y;
        pOut->z = pPos->z + fLen * n.z;
        return;
    }
    pOut->y = -1000.0f;
}

extern int CheckCrossLine(MC_VECTOR *pStart, MC_VECTOR *pEnd,
                          MC_VECTOR *pA, MC_VECTOR *pB);
extern void CheckNearPoint(MC_VECTOR *pFrom, MC_VECTOR *pA,
                           MC_VECTOR *pB, MC_VECTOR *pOut);

/* Nearest point at which the segment pStart->pEnd crosses the unit's
 * oriented bounding rectangle. pOut->y is left at -1000 when nothing was
 * crossed.
 *
 * TODO near-miss (9/210 words, all in the prologue).  The original
 * interleaves the two 16-byte ld/sd copies as pos.lo, bounds.hi,
 * pos.hi, bounds.lo and slots the pDir load and the vHit.y store between
 * them; gcc here emits the two copies back to back, which permutes which
 * temporary register carries which half.  Swept every ordering of the
 * five prologue statements and ran the permuter for 180 iterations
 * without beating the base. */
void CalcNearCrossPointBox(MC_VECTOR *pStart, MC_VECTOR *pEnd,
                           CALC_MAP_UNIT *pUnit, MC_VECTOR *pOut)
{
    MC_VECTOR aCorner[4];
    MC_ALIGNED_VECTOR vPos;
    MC_ALIGNED_VECTOR vSize;
    MC_ALIGNED_VECTOR vHit;
    MC_MATRIX *pDir;

    vPos = pUnit->position;
    vSize = pUnit->bounds;
    pDir = pUnit->pDir;
    pOut->y = -1000.0f;
    vHit.value.y = pStart->y;

    aCorner[0].x = vPos.value.x
                 + (vSize.value.x * pDir->m[0][0] + vSize.value.z * pDir->m[2][0]);
    aCorner[0].y = vPos.value.y;
    aCorner[0].z = vPos.value.z
                 + (vSize.value.x * pDir->m[0][2] + vSize.value.z * pDir->m[2][2]);
    aCorner[1].x = vPos.value.x
                 - (vSize.value.z * pDir->m[0][2] + vSize.value.x * pDir->m[2][2]);
    aCorner[1].y = vPos.value.y;
    aCorner[1].z = vPos.value.z
                 + (vSize.value.z * pDir->m[0][0] + vSize.value.x * pDir->m[2][0]);
    aCorner[2].x = vPos.value.x
                 - (vSize.value.x * pDir->m[0][0] + vSize.value.z * pDir->m[2][0]);
    aCorner[2].y = vPos.value.y;
    aCorner[2].z = vPos.value.z
                 - (vSize.value.x * pDir->m[0][2] + vSize.value.z * pDir->m[2][2]);
    aCorner[3].x = vPos.value.x
                 + (vSize.value.z * pDir->m[0][2] + vSize.value.x * pDir->m[2][2]);
    aCorner[3].y = vPos.value.y;
    aCorner[3].z = vPos.value.z
                 - (vSize.value.z * pDir->m[0][0] + vSize.value.x * pDir->m[2][0]);

    if (CheckCrossLine(pStart, pEnd, &aCorner[0], &aCorner[1]) != 0) {
        CalcCrossPoint(pStart, pEnd, &aCorner[0], &aCorner[1], &vHit.value);
        *(MC_ALIGNED_VECTOR *)pOut = vHit;
    }
    if (CheckCrossLine(pStart, pEnd, &aCorner[1], &aCorner[2]) != 0) {
        CalcCrossPoint(pStart, pEnd, &aCorner[1], &aCorner[2], &vHit.value);
        if (pOut->y != -1000.0f) {
            CheckNearPoint(pStart, &vHit.value, pOut, pOut);
        } else {
            *(MC_ALIGNED_VECTOR *)pOut = vHit;
        }
    }
    if (CheckCrossLine(pStart, pEnd, &aCorner[2], &aCorner[3]) != 0) {
        CalcCrossPoint(pStart, pEnd, &aCorner[2], &aCorner[3], &vHit.value);
        if (pOut->y != -1000.0f) {
            CheckNearPoint(pStart, &vHit.value, pOut, pOut);
        } else {
            *(MC_ALIGNED_VECTOR *)pOut = vHit;
        }
    }
    if (CheckCrossLine(pStart, pEnd, &aCorner[3], &aCorner[0]) != 0) {
        CalcCrossPoint(pStart, pEnd, &aCorner[3], &aCorner[0], &vHit.value);
        if (pOut->y != -1000.0f) {
            CheckNearPoint(pStart, &vHit.value, pOut, pOut);
        } else {
            *(MC_ALIGNED_VECTOR *)pOut = vHit;
        }
    }
}
