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
