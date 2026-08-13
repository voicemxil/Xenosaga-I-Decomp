/* MMath - the math helper library: scalar float utilities plus VU0
 * macro-mode vector/matrix helpers built on top of the same quadword
 * conventions as xglMatrix.c (see that file for the .set noreorder
 * rationale on the inline asm blocks - the original ee-as neither
 * inserted COP2 hazard nops nor filled the return delay slot from
 * inside these blocks, so reordering must be disabled to reproduce it).
 */

typedef struct {
    float x;                        /* 0x00 */
    float y;                        /* 0x04 */
    float z;                        /* 0x08 */
    float w;                        /* 0x0C */
} MC_VECTOR;

typedef struct {
    float m[4][4];
} MC_MATRIX;

extern int xglSRand(void);
extern float fmodf(float x, float y);
extern float srsAtan2(float y, float x);

/* Uniform random float in [0, 2*PI/65536) */
float MMathMakeRandom(void)
{
    return (float)xglSRand() * 3.051850945e-05f;
}

/* Uniform random float in [0, 2*PI) */
float MMathMakeRandom2PI(void)
{
    return (float)xglSRand() * 0.0001917534391f;
}

/* Shortest angular delta from a to b, wrapped to (-PI, PI] */
float MMathCalcRotNear(float a, float b)
{
    float diff;

    diff = fmodf(b, 6.283185005f) - fmodf(a, 6.283185005f);
    if (3.141592741f < __builtin_fabsf(diff)) {
        if (diff < 0.0f) {
            diff += 6.283185005f;
        } else {
            diff -= 6.283185005f;
        }
    }
    return diff;
}

/* Angular delta from a to b, forced the "long way" around */
float MMathCalcRotFar(float a, float b)
{
    float diff;

    diff = fmodf(b, 6.283185005f) - fmodf(a, 6.283185005f);
    if (__builtin_fabsf(diff) < 3.141592741f) {
        if (diff < 0.0f) {
            diff += 6.283185005f;
        } else {
            diff -= 6.283185005f;
        }
    }
    return diff;
}

/* Scale a vector's XYZ from degrees to radians */
MC_VECTOR *MMathDeg2RadVector(MC_VECTOR *pDst, MC_VECTOR *pSrc)
{
    register float k __asm__("$f8") = 0.01745329238f;

    __asm__ __volatile__(".set noreorder\n"
        "mfc1 $t0, %2\n qmtc2.ni $t0, $vf1\n"
        "lqc2 $vf2, 0x0(%1)\n"
        "vmulx.xyz $vf2, $vf2, $vf1x\n"
        "sqc2 $vf2, 0x0(%0)\n"
        ".set reorder" : : "r"(pDst), "r"(pSrc), "f"(k));
    return pDst;
}

/* Convert an integer vector's XYZ from degrees to radians */
MC_VECTOR *MMathDeg2RadVectorI(MC_VECTOR *pDst, int *pSrc)
{
    register float k __asm__("$f8") = 0.01745329238f;

    __asm__ __volatile__(".set noreorder\n"
        "mfc1 $t0, %2\n qmtc2.ni $t0, $vf1\n"
        "lqc2 $vf2, 0x0(%1)\n"
        "vitof0.xyz $vf2, $vf2\n"
        "vmulx.xyz $vf2, $vf2, $vf1x\n"
        "sqc2 $vf2, 0x0(%0)\n"
        ".set reorder" : : "r"(pDst), "r"(pSrc), "f"(k));
    return pDst;
}

/* Scale a vector's XYZ from radians to degrees */
MC_VECTOR *MMathRad2DegVector(MC_VECTOR *pDst, MC_VECTOR *pSrc)
{
    register float k __asm__("$f8") = 57.29578018f;

    __asm__ __volatile__(".set noreorder\n"
        "mfc1 $t0, %2\n qmtc2.ni $t0, $vf1\n"
        "lqc2 $vf2, 0x0(%1)\n"
        "vmulx.xyz $vf2, $vf2, $vf1x\n"
        "sqc2 $vf2, 0x0(%0)\n"
        ".set reorder" : : "r"(pDst), "r"(pSrc), "f"(k));
    return pDst;
}

/* Convert a vector's XYZ from radians to degrees, truncated to integer */
int *MMathRad2DegVectorI(int *pDst, MC_VECTOR *pSrc)
{
    register float k __asm__("$f8") = 57.29578018f;

    __asm__ __volatile__(".set noreorder\n"
        "mfc1 $t0, %2\n qmtc2.ni $t0, $vf1\n"
        "lqc2 $vf2, 0x0(%1)\n"
        "vmulx.xyz $vf2, $vf2, $vf1x\n"
        "vftoi0.xyz $vf2, $vf2\n"
        "sqc2 $vf2, 0x0(%0)\n"
        ".set reorder" : : "r"(pDst), "r"(pSrc), "f"(k));
    return pDst;
}

/* Normalize a vector's XYZ using the VU0 reciprocal-sqrt pipe */
MC_VECTOR *MMathNormalizeVector2(MC_VECTOR *pDst, MC_VECTOR *pSrc)
{
    __asm__ __volatile__(".set noreorder\n"
        "lqc2 $vf1, 0x0(%1)\n"
        "vmul.xyz $vf2, $vf1, $vf1\n"
        "vaddy.x $vf2, $vf2, $vf2y\n"
        "vaddz.x $vf2, $vf2, $vf2z\n"
        "vrsqrt $Q, $vf0w, $vf2x\n"
        "vmove.w $vf2, $vf0\n"
        "vwaitq\n"
        "vmulq.xyz $vf2, $vf1, $Q\n"
        "sqc2 $vf2, 0x0(%0)\n"
        ".set reorder" : : "r"(pDst), "r"(pSrc));
    return pDst;
}

/* (a - b) * s, XYZ only */
MC_VECTOR *MMathSubVectorMulS(MC_VECTOR *pDst, MC_VECTOR *pA, MC_VECTOR *pB, float s)
{
    __asm__ __volatile__(".set noreorder\n"
        "mfc1 $t0, %3\n qmtc2.ni $t0, $vf3\n"
        "lqc2 $vf1, 0x0(%1)\n lqc2 $vf2, 0x0(%2)\n"
        "vsub.xyz $vf1, $vf1, $vf2\n"
        "vmulx.xyz $vf1, $vf1, $vf3x\n"
        "sqc2 $vf1, 0x0(%0)\n"
        ".set reorder" : : "r"(pDst), "r"(pA), "r"(pB), "f"(s));
    return pDst;
}

/* (a - b) / s, XYZ only */
MC_VECTOR *MMathSubVectorDivS(MC_VECTOR *pDst, MC_VECTOR *pA, MC_VECTOR *pB, float s)
{
    __asm__ __volatile__(".set noreorder\n"
        "mfc1 $t0, %3\n qmtc2.ni $t0, $vf3\n"
        "lqc2 $vf1, 0x0(%1)\n"
        "vdiv $Q, $vf0w, $vf3x\n"
        "lqc2 $vf2, 0x0(%2)\n"
        "vsub.xyz $vf1, $vf1, $vf2\n"
        "vwaitq\n"
        "vmulq.xyz $vf1, $vf1, $Q\n"
        "sqc2 $vf1, 0x0(%0)\n"
        ".set reorder" : : "r"(pDst), "r"(pA), "r"(pB), "f"(s));
    return pDst;
}

/* Componentwise a / b, XYZ only */
MC_VECTOR *MMathDivVector(MC_VECTOR *pDst, MC_VECTOR *pA, MC_VECTOR *pB)
{
    __asm__ __volatile__(".set noreorder\n"
        "lqc2 $vf1, 0x0(%1)\n lqc2 $vf2, 0x0(%2)\n"
        "vdiv $Q, $vf1x, $vf2x\n"
        "vmove.w $vf3, $vf1\n"
        "vwaitq\n"
        "vaddq.x $vf3, $vf0, $Q\n"
        "vdiv $Q, $vf1y, $vf2y\n"
        "vwaitq\n"
        "vaddq.y $vf3, $vf0, $Q\n"
        "vdiv $Q, $vf1z, $vf2z\n"
        "vwaitq\n"
        "vaddq.z $vf3, $vf0, $Q\n"
        "sqc2 $vf3, 0x0(%0)\n"
        ".set reorder" : : "r"(pDst), "r"(pA), "r"(pB));
    return pDst;
}

/* a / s, XYZ only */
MC_VECTOR *MMathDivVectorS(MC_VECTOR *pDst, MC_VECTOR *pA, float s)
{
    __asm__ __volatile__(".set noreorder\n"
        "mfc1 $t0, %2\n qmtc2.ni $t0, $vf2\n"
        "lqc2 $vf1, 0x0(%1)\n"
        "vdiv $Q, $vf0w, $vf2x\n"
        "vwaitq\n"
        "vmulq.xyz $vf1, $vf1, $Q\n"
        "sqc2 $vf1, 0x0(%0)\n"
        ".set reorder" : : "r"(pDst), "r"(pA), "f"(s));
    return pDst;
}

/* Componentwise a / b, all four components */
MC_VECTOR *MMathDivVector4(MC_VECTOR *pDst, MC_VECTOR *pA, MC_VECTOR *pB)
{
    __asm__ __volatile__(".set noreorder\n"
        "lqc2 $vf1, 0x0(%1)\n lqc2 $vf2, 0x0(%2)\n"
        "vdiv $Q, $vf1x, $vf2x\n"
        "vsub.w $vf3, $vf3, $vf3\n"
        "vwaitq\n"
        "vaddq.x $vf3, $vf0, $Q\n"
        "vdiv $Q, $vf1y, $vf2y\n"
        "vwaitq\n"
        "vaddq.y $vf3, $vf0, $Q\n"
        "vdiv $Q, $vf1z, $vf2z\n"
        "vwaitq\n"
        "vaddq.z $vf3, $vf0, $Q\n"
        "vdiv $Q, $vf1w, $vf2w\n"
        "vwaitq\n"
        "vaddq.w $vf3, $vf3, $Q\n"
        "sqc2 $vf3, 0x0(%0)\n"
        ".set reorder" : : "r"(pDst), "r"(pA), "r"(pB));
    return pDst;
}

/* a / s, all four components */
MC_VECTOR *MMathDivVectorS4(MC_VECTOR *pDst, MC_VECTOR *pA, float s)
{
    __asm__ __volatile__(".set noreorder\n"
        "mfc1 $t0, %2\n qmtc2.ni $t0, $vf2\n"
        "lqc2 $vf1, 0x0(%1)\n"
        "vdiv $Q, $vf0w, $vf2x\n"
        "vwaitq\n"
        "vmulq.xyzw $vf1, $vf1, $Q\n"
        "sqc2 $vf1, 0x0(%0)\n"
        ".set reorder" : : "r"(pDst), "r"(pA), "f"(s));
    return pDst;
}

/* XYZ dot product */
float MMathVectorDotProduct(MC_VECTOR *pA, MC_VECTOR *pB)
{
    float ret;

    __asm__ __volatile__(".set noreorder\n"
        "lqc2 $vf1, 0x0(%1)\n lqc2 $vf2, 0x0(%2)\n"
        "vmul.xyz $vf1, $vf1, $vf2\n"
        "vaddy.x $vf1, $vf1, $vf1y\n"
        "vaddz.x $vf1, $vf1, $vf1z\n"
        "qmfc2.ni $t0, $vf1\n"
        "mtc1 $t0, %0\n"
        ".set reorder" : "=f"(ret) : "r"(pA), "r"(pB));
    return ret;
}

/* XYZ cross product */
MC_VECTOR *MMathVectorCrossProduct(MC_VECTOR *pDst, MC_VECTOR *pA, MC_VECTOR *pB)
{
    __asm__ __volatile__(".set noreorder\n"
        "lqc2 $vf1, 0x0(%1)\n lqc2 $vf2, 0x0(%2)\n"
        "vopmula.xyz $ACC, $vf1, $vf2\n"
        "vopmsub.xyz $vf1, $vf2, $vf1\n"
        "vmove.w $vf1, $vf0\n"
        "sqc2 $vf1, 0x0(%0)\n"
        ".set reorder" : : "r"(pDst), "r"(pA), "r"(pB));
    return pDst;
}

/* Linear interpolation: a + (b - a) * t, XYZ only */
MC_VECTOR *MMathVectorInterpolation(MC_VECTOR *pDst, MC_VECTOR *pA, MC_VECTOR *pB, float t)
{
    __asm__ __volatile__(".set noreorder\n"
        "mfc1 $t0, %3\n qmtc2.ni $t0, $vf3\n"
        "vsubx.w $vf3, $vf0, $vf3x\n"
        "lqc2 $vf1, 0x0(%1)\n lqc2 $vf2, 0x0(%2)\n"
        "vmulaw.xyz $ACC, $vf1, $vf3w\n"
        "vmaddx.xyz $vf1, $vf2, $vf3x\n"
        "sqc2 $vf1, 0x0(%0)\n"
        ".set reorder" : : "r"(pDst), "r"(pA), "r"(pB), "f"(t));
    return pDst;
}

/* Apply a 4x4 matrix (row-major, w-normalized) to a point */
MC_VECTOR *MMathApplyMatrix(MC_VECTOR *pDst, MC_MATRIX *pMat, MC_VECTOR *pSrc)
{
    __asm__ __volatile__(".set noreorder\n"
        "lqc2 $vf1, 0x0(%2)\n"
        "lqc2 $vf2, 0x0(%1)\n lqc2 $vf3, 0x10(%1)\n"
        "lqc2 $vf4, 0x20(%1)\n lqc2 $vf5, 0x30(%1)\n"
        "vmulax.xyzw $ACC, $vf2, $vf1x\n"
        "vmadday.xyzw $ACC, $vf3, $vf1y\n"
        "vmaddaz.xyzw $ACC, $vf4, $vf1z\n"
        "vmaddw.xyzw $vf1, $vf5, $vf1w\n"
        "sqc2 $vf1, 0x0(%0)\n"
        ".set reorder" : : "r"(pDst), "r"(pMat), "r"(pSrc));
    return pDst;
}

/* Vector length (XYZ magnitude) */
float MMathCalcLength(MC_VECTOR *pA)
{
    float ret;

    __asm__ __volatile__(".set noreorder\n"
        "lqc2 $vf1, 0x0(%1)\n"
        "vmul.xyz $vf1, $vf1, $vf1\n"
        "vaddy.x $vf1, $vf1, $vf1y\n"
        "vaddz.x $vf1, $vf1, $vf1z\n"
        "vsqrt $Q, $vf1x\n"
        "vwaitq\n"
        "vaddq.x $vf1, $vf0, $Q\n"
        "qmfc2.ni $t0, $vf1\n"
        "mtc1 $t0, %0\n"
        ".set reorder" : "=f"(ret) : "r"(pA));
    return ret;
}

/* Vector length, XY plane only */
float MMathCalcLengthXY(MC_VECTOR *pA)
{
    float ret;

    __asm__ __volatile__(".set noreorder\n"
        "lqc2 $vf1, 0x0(%1)\n"
        "vmul.xy $vf1, $vf1, $vf1\n"
        "vaddy.x $vf1, $vf1, $vf1y\n"
        "vsqrt $Q, $vf1x\n"
        "vwaitq\n"
        "vaddq.x $vf1, $vf0, $Q\n"
        "qmfc2.ni $t0, $vf1\n"
        "mtc1 $t0, %0\n"
        ".set reorder" : "=f"(ret) : "r"(pA));
    return ret;
}

/* Vector length, XZ plane only */
float MMathCalcLengthXZ(MC_VECTOR *pA)
{
    float ret;

    __asm__ __volatile__(".set noreorder\n"
        "lqc2 $vf1, 0x0(%1)\n"
        "vmul.xz $vf1, $vf1, $vf1\n"
        "vaddz.x $vf1, $vf1, $vf1z\n"
        "vsqrt $Q, $vf1x\n"
        "vwaitq\n"
        "vaddq.x $vf1, $vf0, $Q\n"
        "qmfc2.ni $t0, $vf1\n"
        "mtc1 $t0, %0\n"
        ".set reorder" : "=f"(ret) : "r"(pA));
    return ret;
}

/* Vector length, YZ plane only */
float MMathCalcLengthYZ(MC_VECTOR *pA)
{
    float ret;

    __asm__ __volatile__(".set noreorder\n"
        "lqc2 $vf1, 0x0(%1)\n"
        "vmul.yz $vf1, $vf1, $vf1\n"
        "vaddz.y $vf1, $vf1, $vf1z\n"
        "vsqrt $Q, $vf1y\n"
        "vwaitq\n"
        "vaddq.x $vf1, $vf0, $Q\n"
        "qmfc2.ni $t0, $vf1\n"
        "mtc1 $t0, %0\n"
        ".set reorder" : "=f"(ret) : "r"(pA));
    return ret;
}

/* Distance between two points (XYZ) */
float MMathCalcDist(MC_VECTOR *pA, MC_VECTOR *pB)
{
    float ret;

    __asm__ __volatile__(".set noreorder\n"
        "lqc2 $vf1, 0x0(%1)\n lqc2 $vf2, 0x0(%2)\n"
        "vsub.xyz $vf1, $vf2, $vf1\n"
        "vmul.xyz $vf1, $vf1, $vf1\n"
        "vaddy.x $vf1, $vf1, $vf1y\n"
        "vaddz.x $vf1, $vf1, $vf1z\n"
        "vsqrt $Q, $vf1x\n"
        "vwaitq\n"
        "vaddq.x $vf1, $vf0, $Q\n"
        "qmfc2.ni $t0, $vf1\n"
        "mtc1 $t0, %0\n"
        ".set reorder" : "=f"(ret) : "r"(pA), "r"(pB));
    return ret;
}

/* Distance between two points, XZ plane only */
float MMathCalcDistXZ(MC_VECTOR *pA, MC_VECTOR *pB)
{
    float ret;

    __asm__ __volatile__(".set noreorder\n"
        "lqc2 $vf1, 0x0(%1)\n lqc2 $vf2, 0x0(%2)\n"
        "vsub.xz $vf1, $vf2, $vf1\n"
        "vmul.xz $vf1, $vf1, $vf1\n"
        "vaddz.x $vf1, $vf1, $vf1z\n"
        "vsqrt $Q, $vf1x\n"
        "vwaitq\n"
        "vaddq.x $vf1, $vf0, $Q\n"
        "qmfc2.ni $t0, $vf1\n"
        "mtc1 $t0, %0\n"
        ".set reorder" : "=f"(ret) : "r"(pA), "r"(pB));
    return ret;
}

/* Yaw angle (radians) from pFrom to pTo, XZ plane */
float MMathCalcDir(MC_VECTOR *pFrom, MC_VECTOR *pTo)
{
    float fRet;

    fRet = srsAtan2(pTo->x - pFrom->x, pTo->z - pFrom->z);
    __asm__ volatile("" ::: "memory");
    return fRet;
}

/* TODO: not decompiled -- sibling-call blocker RESISTS the trailing-barrier
   fix (see resume prompt). Two attempts: (1) local + trailing
   asm("":::"memory") before return -- still `j MMathNormalizeVector2`, no
   change; (2) register-pinning the return pointer to $2 (v0) to reproduce
   the original's early `move v0,a0` -- gcc satisfies the "r" constraint by
   using a0 directly (same value) instead of respecting the pin, so the
   early move never materializes and the tail call still sibcalls. This is
   a genuine two-part near-miss: the v0/a0 register tie-break predates the
   sibcall codegen and the barrier idiom alone can't force it. Logic is
   correct. */
/* Direction vector (unnormalized diff, then normalized) from pFrom to pTo */
MC_VECTOR *MMathCalcDirVector(MC_VECTOR *pDst, MC_VECTOR *pFrom, MC_VECTOR *pTo)
{
    __asm__ __volatile__(".set noreorder\n"
        "lqc2 $vf1, 0x0(%1)\n lqc2 $vf2, 0x0(%2)\n"
        "vsub.xyz $vf2, $vf2, $vf1\n"
        "sqc2 $vf2, 0x0(%0)\n"
        ".set reorder" : : "r"(pDst), "r"(pFrom), "r"(pTo));
    return MMathNormalizeVector2(pDst, pDst);
}

/* Hermite tangents: (pC - pA) * 0.5, (pD - pB) * 0.5 */
void MMathCalcHermitePrm(MC_VECTOR *pOut1, MC_VECTOR *pOut2, MC_VECTOR *pA, MC_VECTOR *pB, MC_VECTOR *pC, MC_VECTOR *pD)
{
    __asm__ __volatile__(".set noreorder\n"
        "lqc2 $vf1, 0x0(%2)\n"
        "lqc2 $vf2, 0x0(%3)\n"
        "lqc2 $vf3, 0x0(%4)\n"
        "lqc2 $vf4, 0x0(%5)\n"
        "viaddi $vi5, $vi0, 8\n"
        "vmfir.x $vf5x, $vi5\n"
        "vitof4.x $vf5x, $vf5x\n"
        "vsub.xyz $vf6, $vf3, $vf1\n"
        "vsub.xyz $vf7, $vf4, $vf2\n"
        "vmulx.xyz $vf6, $vf6, $vf5x\n"
        "vmulx.xyz $vf7, $vf7, $vf5x\n"
        "sqc2 $vf6, 0x0(%0)\n"
        "sqc2 $vf7, 0x0(%1)\n"
        ".set reorder" : : "r"(pOut1), "r"(pOut2), "r"(pA), "r"(pB), "r"(pC), "r"(pD));
}

/* Multiply two 4x4 matrices (row-major): dst = left * right */
MC_MATRIX *MMathMulMatrix(MC_MATRIX *pDst, MC_MATRIX *pLeft, MC_MATRIX *pRight)
{
    __asm__ __volatile__(".set noreorder\n"
        "lqc2 $vf2, 0x0(%1)\n lqc2 $vf3, 0x10(%1)\n"
        "lqc2 $vf4, 0x20(%1)\n lqc2 $vf5, 0x30(%1)\n"
        "lqc2 $vf1, 0x0(%2)\n"
        "vmulax.xyzw $ACC, $vf2, $vf1x\n"
        "vmadday.xyzw $ACC, $vf3, $vf1y\n"
        "vmaddaz.xyzw $ACC, $vf4, $vf1z\n"
        "vmaddw.xyzw $vf6, $vf5, $vf1w\n"
        "sqc2 $vf6, 0x0(%0)\n"
        "lqc2 $vf1, 0x10(%2)\n"
        "vmulax.xyzw $ACC, $vf2, $vf1x\n"
        "vmadday.xyzw $ACC, $vf3, $vf1y\n"
        "vmaddaz.xyzw $ACC, $vf4, $vf1z\n"
        "vmaddw.xyzw $vf6, $vf5, $vf1w\n"
        "sqc2 $vf6, 0x10(%0)\n"
        "lqc2 $vf1, 0x20(%2)\n"
        "vmulax.xyzw $ACC, $vf2, $vf1x\n"
        "vmadday.xyzw $ACC, $vf3, $vf1y\n"
        "vmaddaz.xyzw $ACC, $vf4, $vf1z\n"
        "vmaddw.xyzw $vf6, $vf5, $vf1w\n"
        "sqc2 $vf6, 0x20(%0)\n"
        "lqc2 $vf1, 0x30(%2)\n"
        "vmulax.xyzw $ACC, $vf2, $vf1x\n"
        "vmadday.xyzw $ACC, $vf3, $vf1y\n"
        "vmaddaz.xyzw $ACC, $vf4, $vf1z\n"
        "vmaddw.xyzw $vf6, $vf5, $vf1w\n"
        "sqc2 $vf6, 0x30(%0)\n"
        ".set reorder" : : "r"(pDst), "r"(pLeft), "r"(pRight));
    return pDst;
}

/* Identity matrix -- default `pMat` for the MMathRotateMatrix* family below
 * when the caller passes NULL. */
unsigned int McMathUnitMatrix[16] = {
    0x3F800000, 0x00000000, 0x00000000, 0x00000000,
    0x00000000, 0x3F800000, 0x00000000, 0x00000000,
    0x00000000, 0x00000000, 0x3F800000, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x3F800000,
};

/* Rotate pMat (or identity, if NULL) about X by `angle` radians */
MC_MATRIX *MMathRotateMatrixX(MC_MATRIX *pDst, MC_MATRIX *pMat, float angle)
{
    register MC_MATRIX *r __asm__("$2");
    __asm__ __volatile__("daddu $2, %1, $0" : "=r"(r) : "r"(pDst));
    {
        register MC_MATRIX *pDef __asm__("$8") = (MC_MATRIX *)&McMathUnitMatrix;
        pMat = pMat ? pMat : pDef;
    }
    __asm__ __volatile__(".set noreorder\n"
        "mfc1 $t0, %2\n"
        "qmtc2 $t0, $vf4\n"
        "vcallms 0xE8\n"
        "vmove.x $vf5, $vf1\n"
        "qmtc2 $t0, $vf4\n"
        "vcallms 0x20\n"
        "vmove.x $vf6, $vf1\n"
        "lqc2 $vf1, 0x0(%1)\n"
        "lqc2 $vf2, 0x10(%1)\n"
        "lqc2 $vf3, 0x20(%1)\n"
        "lqc2 $vf4, 0x30(%1)\n"
        "sqc2 $vf1, 0x0(%0)\n"
        "sqc2 $vf4, 0x30(%0)\n"
        "vmulax.xyzw $ACC, $vf2, $vf5x\n"
        "vmaddx.xyzw $vf1, $vf3, $vf6x\n"
        "sqc2 $vf1, 0x10(%0)\n"
        "vmulax.xyzw $ACC, $vf3, $vf5x\n"
        "vmsubx.xyzw $vf1, $vf2, $vf6x\n"
        "sqc2 $vf1, 0x20(%0)\n"
        ".set reorder" : : "r"(r), "r"(pMat), "f"(angle));
    return r;
}

/* Rotate pMat (or identity, if NULL) about Y by `angle` radians */
MC_MATRIX *MMathRotateMatrixY(MC_MATRIX *pDst, MC_MATRIX *pMat, float angle)
{
    register MC_MATRIX *r __asm__("$2");
    __asm__ __volatile__("daddu $2, %1, $0" : "=r"(r) : "r"(pDst));
    {
        register MC_MATRIX *pDef __asm__("$8") = (MC_MATRIX *)&McMathUnitMatrix;
        pMat = pMat ? pMat : pDef;
    }
    __asm__ __volatile__(".set noreorder\n"
        "mfc1 $t0, %2\n"
        "qmtc2 $t0, $vf4\n"
        "vcallms 0xE8\n"
        "vmove.x $vf5, $vf1\n"
        "qmtc2 $t0, $vf4\n"
        "vcallms 0x20\n"
        "vmove.x $vf6, $vf1\n"
        "lqc2 $vf1, 0x0(%1)\n"
        "lqc2 $vf2, 0x10(%1)\n"
        "lqc2 $vf3, 0x20(%1)\n"
        "lqc2 $vf4, 0x30(%1)\n"
        "sqc2 $vf2, 0x10(%0)\n"
        "sqc2 $vf4, 0x30(%0)\n"
        "vmulax.xyzw $ACC, $vf1, $vf5x\n"
        "vmsubx.xyzw $vf2, $vf3, $vf6x\n"
        "sqc2 $vf2, 0x0(%0)\n"
        "vmulax.xyzw $ACC, $vf3, $vf5x\n"
        "vmaddx.xyzw $vf2, $vf1, $vf6x\n"
        "sqc2 $vf2, 0x20(%0)\n"
        ".set reorder" : : "r"(r), "r"(pMat), "f"(angle));
    return r;
}

/* Rotate pMat (or identity, if NULL) about Z by `angle` radians */
MC_MATRIX *MMathRotateMatrixZ(MC_MATRIX *pDst, MC_MATRIX *pMat, float angle)
{
    register MC_MATRIX *r __asm__("$2");
    __asm__ __volatile__("daddu $2, %1, $0" : "=r"(r) : "r"(pDst));
    {
        register MC_MATRIX *pDef __asm__("$8") = (MC_MATRIX *)&McMathUnitMatrix;
        pMat = pMat ? pMat : pDef;
    }
    __asm__ __volatile__(".set noreorder\n"
        "mfc1 $t0, %2\n"
        "qmtc2 $t0, $vf4\n"
        "vcallms 0xE8\n"
        "vmove.x $vf5, $vf1\n"
        "qmtc2 $t0, $vf4\n"
        "vcallms 0x20\n"
        "vmove.x $vf6, $vf1\n"
        "lqc2 $vf1, 0x0(%1)\n"
        "lqc2 $vf2, 0x10(%1)\n"
        "lqc2 $vf3, 0x20(%1)\n"
        "lqc2 $vf4, 0x30(%1)\n"
        "sqc2 $vf3, 0x20(%0)\n"
        "sqc2 $vf4, 0x30(%0)\n"
        "vmulax.xyzw $ACC, $vf1, $vf5x\n"
        "vmaddx.xyzw $vf3, $vf2, $vf6x\n"
        "sqc2 $vf3, 0x0(%0)\n"
        "vmulax.xyzw $ACC, $vf2, $vf5x\n"
        "vmsubx.xyzw $vf3, $vf1, $vf6x\n"
        "sqc2 $vf3, 0x10(%0)\n"
        ".set reorder" : : "r"(r), "r"(pMat), "f"(angle));
    return r;
}

/* Rotate pMat (or identity, if NULL) about X then Y, pAngle = {angleX, angleY} */
MC_MATRIX *MMathRotateMatrixXY(MC_MATRIX *pDst, MC_MATRIX *pMat, MC_VECTOR *pAngle)
{
    register MC_MATRIX *r __asm__("$2");
    __asm__ __volatile__("daddu $2, %1, $0" : "=r"(r) : "r"(pDst));
    {
        register MC_MATRIX *pDef __asm__("$8") = (MC_MATRIX *)&McMathUnitMatrix;
        pMat = pMat ? pMat : pDef;
    }
    __asm__ __volatile__(".set noreorder\n"
        "lqc2 $vf7, 0x0(%2)\n"
        "vaddx.x $vf4, $vf0, $vf7\n"
        "vcallms 0xE8\n"
        "vaddx.x $vf5, $vf0, $vf1\n"
        "vaddy.x $vf4, $vf0, $vf7\n"
        "vcallms 0xE8\n"
        "vaddx.y $vf5, $vf0, $vf1\n"
        "vaddx.x $vf4, $vf0, $vf7\n"
        "vcallms 0x20\n"
        "vaddx.x $vf6, $vf0, $vf1\n"
        "vaddy.x $vf4, $vf0, $vf7\n"
        "vcallms 0x20\n"
        "vaddx.y $vf6, $vf0, $vf1\n"
        "lqc2 $vf1, 0x0(%1)\n"
        "lqc2 $vf2, 0x10(%1)\n"
        "lqc2 $vf3, 0x20(%1)\n"
        "lqc2 $vf4, 0x30(%1)\n"
        "vmulax.xyzw $ACC, $vf2, $vf5x\n"
        "vmaddx.xyzw $vf7, $vf3, $vf6x\n"
        "vmulax.xyzw $ACC, $vf3, $vf5x\n"
        "vmsubx.xyzw $vf3, $vf2, $vf6x\n"
        "vmulay.xyzw $ACC, $vf1, $vf5y\n"
        "vmsuby.xyzw $vf8, $vf3, $vf6y\n"
        "vmulay.xyzw $ACC, $vf3, $vf5y\n"
        "vmaddy.xyzw $vf3, $vf1, $vf6y\n"
        "sqc2 $vf8, 0x0(%0)\n"
        "sqc2 $vf7, 0x10(%0)\n"
        "sqc2 $vf3, 0x20(%0)\n"
        "sqc2 $vf4, 0x30(%0)\n"
        ".set reorder" : : "r"(r), "r"(pMat), "r"(pAngle));
    return r;
}

/* Rotate pMat (or identity, if NULL) about Y then X, pAngle = {angleX, angleY} */
MC_MATRIX *MMathRotateMatrixYX(MC_MATRIX *pDst, MC_MATRIX *pMat, MC_VECTOR *pAngle)
{
    register MC_MATRIX *r __asm__("$2");
    __asm__ __volatile__("daddu $2, %1, $0" : "=r"(r) : "r"(pDst));
    {
        register MC_MATRIX *pDef __asm__("$8") = (MC_MATRIX *)&McMathUnitMatrix;
        pMat = pMat ? pMat : pDef;
    }
    __asm__ __volatile__(".set noreorder\n"
        "lqc2 $vf7, 0x0(%2)\n"
        "vaddx.x $vf4, $vf0, $vf7\n"
        "vcallms 0xE8\n"
        "vaddx.x $vf5, $vf0, $vf1\n"
        "vaddy.x $vf4, $vf0, $vf7\n"
        "vcallms 0xE8\n"
        "vaddx.y $vf5, $vf0, $vf1\n"
        "vaddx.x $vf4, $vf0, $vf7\n"
        "vcallms 0x20\n"
        "vaddx.x $vf6, $vf0, $vf1\n"
        "vaddy.x $vf4, $vf0, $vf7\n"
        "vcallms 0x20\n"
        "vaddx.y $vf6, $vf0, $vf1\n"
        "lqc2 $vf1, 0x0(%1)\n"
        "lqc2 $vf2, 0x10(%1)\n"
        "lqc2 $vf3, 0x20(%1)\n"
        "lqc2 $vf4, 0x30(%1)\n"
        "vmulay.xyzw $ACC, $vf1, $vf5y\n"
        "vmsuby.xyzw $vf7, $vf3, $vf6y\n"
        "vmulay.xyzw $ACC, $vf3, $vf5y\n"
        "vmaddy.xyzw $vf3, $vf1, $vf6y\n"
        "vmulax.xyzw $ACC, $vf2, $vf5x\n"
        "vmaddx.xyzw $vf8, $vf3, $vf6x\n"
        "vmulax.xyzw $ACC, $vf3, $vf5x\n"
        "vmsubx.xyzw $vf3, $vf2, $vf6x\n"
        "sqc2 $vf7, 0x0(%0)\n"
        "sqc2 $vf8, 0x10(%0)\n"
        "sqc2 $vf3, 0x20(%0)\n"
        "sqc2 $vf4, 0x30(%0)\n"
        ".set reorder" : : "r"(r), "r"(pMat), "r"(pAngle));
    return r;
}

/* Rotate pMat (or identity, if NULL) about X, Y, then Z; pAngle = {angleX, angleY, angleZ} */
MC_MATRIX *MMathRotateMatrixXYZ(MC_MATRIX *pDst, MC_MATRIX *pMat, MC_VECTOR *pAngle)
{
    register MC_MATRIX *r __asm__("$2");
    __asm__ __volatile__("daddu $2, %1, $0" : "=r"(r) : "r"(pDst));
    {
        register MC_MATRIX *pDef __asm__("$8") = (MC_MATRIX *)&McMathUnitMatrix;
        pMat = pMat ? pMat : pDef;
    }
    __asm__ __volatile__(".set noreorder\n"
        "lqc2 $vf7, 0x0(%2)\n"
        "vaddx.x $vf4, $vf0, $vf7\n"
        "vcallms 0xE8\n"
        "vaddx.x $vf5, $vf0, $vf1\n"
        "vaddy.x $vf4, $vf0, $vf7\n"
        "vcallms 0xE8\n"
        "vaddx.y $vf5, $vf0, $vf1\n"
        "vaddz.x $vf4, $vf0, $vf7\n"
        "vcallms 0xE8\n"
        "vaddx.z $vf5, $vf0, $vf1\n"
        "vaddx.x $vf4, $vf0, $vf7\n"
        "vcallms 0x20\n"
        "vaddx.x $vf6, $vf0, $vf1\n"
        "vaddy.x $vf4, $vf0, $vf7\n"
        "vcallms 0x20\n"
        "vaddx.y $vf6, $vf0, $vf1\n"
        "vaddz.x $vf4, $vf0, $vf7\n"
        "vcallms 0x20\n"
        "vaddx.z $vf6, $vf0, $vf1\n"
        "lqc2 $vf1, 0x0(%1)\n"
        "lqc2 $vf2, 0x10(%1)\n"
        "lqc2 $vf3, 0x20(%1)\n"
        "lqc2 $vf4, 0x30(%1)\n"
        "vmulax.xyzw $ACC, $vf2, $vf5x\n"
        "vmaddx.xyzw $vf7, $vf3, $vf6x\n"
        "vmulax.xyzw $ACC, $vf3, $vf5x\n"
        "vmsubx.xyzw $vf3, $vf2, $vf6x\n"
        "vmulay.xyzw $ACC, $vf1, $vf5y\n"
        "vmsuby.xyzw $vf8, $vf3, $vf6y\n"
        "vmulay.xyzw $ACC, $vf3, $vf5y\n"
        "vmaddy.xyzw $vf3, $vf1, $vf6y\n"
        "vmulaz.xyzw $ACC, $vf8, $vf5z\n"
        "vmaddz.xyzw $vf1, $vf7, $vf6z\n"
        "vmulaz.xyzw $ACC, $vf7, $vf5z\n"
        "vmsubz.xyzw $vf2, $vf8, $vf6z\n"
        "sqc2 $vf1, 0x0(%0)\n"
        "sqc2 $vf2, 0x10(%0)\n"
        "sqc2 $vf3, 0x20(%0)\n"
        "sqc2 $vf4, 0x30(%0)\n"
        ".set reorder" : : "r"(r), "r"(pMat), "r"(pAngle));
    return r;
}

/* Rotate pMat (or identity, if NULL) about Y, X, then Z; pAngle = {angleX, angleY, angleZ} */
MC_MATRIX *MMathRotateMatrixYXZ(MC_MATRIX *pDst, MC_MATRIX *pMat, MC_VECTOR *pAngle)
{
    register MC_MATRIX *r __asm__("$2");
    __asm__ __volatile__("daddu $2, %1, $0" : "=r"(r) : "r"(pDst));
    {
        register MC_MATRIX *pDef __asm__("$8") = (MC_MATRIX *)&McMathUnitMatrix;
        pMat = pMat ? pMat : pDef;
    }
    __asm__ __volatile__(".set noreorder\n"
        "lqc2 $vf7, 0x0(%2)\n"
        "vaddx.x $vf4, $vf0, $vf7\n"
        "vcallms 0xE8\n"
        "vaddx.x $vf5, $vf0, $vf1\n"
        "vaddy.x $vf4, $vf0, $vf7\n"
        "vcallms 0xE8\n"
        "vaddx.y $vf5, $vf0, $vf1\n"
        "vaddz.x $vf4, $vf0, $vf7\n"
        "vcallms 0xE8\n"
        "vaddx.z $vf5, $vf0, $vf1\n"
        "vaddx.x $vf4, $vf0, $vf7\n"
        "vcallms 0x20\n"
        "vaddx.x $vf6, $vf0, $vf1\n"
        "vaddy.x $vf4, $vf0, $vf7\n"
        "vcallms 0x20\n"
        "vaddx.y $vf6, $vf0, $vf1\n"
        "vaddz.x $vf4, $vf0, $vf7\n"
        "vcallms 0x20\n"
        "vaddx.z $vf6, $vf0, $vf1\n"
        "lqc2 $vf1, 0x0(%1)\n"
        "lqc2 $vf2, 0x10(%1)\n"
        "lqc2 $vf3, 0x20(%1)\n"
        "lqc2 $vf4, 0x30(%1)\n"
        "vmulay.xyzw $ACC, $vf1, $vf5y\n"
        "vmsuby.xyzw $vf7, $vf3, $vf6y\n"
        "vmulay.xyzw $ACC, $vf3, $vf5y\n"
        "vmaddy.xyzw $vf3, $vf1, $vf6y\n"
        "vmulax.xyzw $ACC, $vf2, $vf5x\n"
        "vmaddx.xyzw $vf8, $vf3, $vf6x\n"
        "vmulax.xyzw $ACC, $vf3, $vf5x\n"
        "vmsubx.xyzw $vf3, $vf2, $vf6x\n"
        "vmulaz.xyzw $ACC, $vf7, $vf5z\n"
        "vmaddz.xyzw $vf1, $vf8, $vf6z\n"
        "vmulaz.xyzw $ACC, $vf8, $vf5z\n"
        "vmsubz.xyzw $vf2, $vf7, $vf6z\n"
        "sqc2 $vf1, 0x0(%0)\n"
        "sqc2 $vf2, 0x10(%0)\n"
        "sqc2 $vf3, 0x20(%0)\n"
        "sqc2 $vf4, 0x30(%0)\n"
        ".set reorder" : : "r"(r), "r"(pMat), "r"(pAngle));
    return r;
}
