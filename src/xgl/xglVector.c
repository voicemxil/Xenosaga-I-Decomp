/* Four-component vector helpers */

void xglVectorDiv(float *pDest, const float *pLeft, const float *pRight)
{
    pDest[0] = pLeft[0] / pRight[0];
    pDest[1] = pLeft[1] / pRight[1];
    pDest[2] = pLeft[2] / pRight[2];
    pDest[3] = pLeft[3] / pRight[3];
}

void xglVectorDivXYZ(float *pDest, const float *pLeft, const float *pRight)
{
    pDest[0] = pLeft[0] / pRight[0];
    pDest[1] = pLeft[1] / pRight[1];
    pDest[2] = pLeft[2] / pRight[2];
    pDest[3] = pLeft[3];
}

void xglVectorMulAdd(float *pDest, const float *pLeft,
                     const float *pRight, const float *pAdd)
{
    pDest[0] = pLeft[0] * pRight[0] + pAdd[0];
    pDest[1] = pLeft[1] * pRight[1] + pAdd[1];
    pDest[2] = pLeft[2] * pRight[2] + pAdd[2];
    pDest[3] = pLeft[3] * pRight[3] + pAdd[3];
}

void xglVectorMulAddXYZ(float *pDest, const float *pLeft,
                        const float *pRight, const float *pAdd)
{
    pDest[0] = pLeft[0] * pRight[0] + pAdd[0];
    pDest[1] = pLeft[1] * pRight[1] + pAdd[1];
    pDest[2] = pLeft[2] * pRight[2] + pAdd[2];
    pDest[3] = pLeft[3];
}

void xglVectorMulSub(float *pDest, const float *pLeft,
                     const float *pRight, const float *pSub)
{
    pDest[0] = pLeft[0] * pRight[0] - pSub[0];
    pDest[1] = pLeft[1] * pRight[1] - pSub[1];
    pDest[2] = pLeft[2] * pRight[2] - pSub[2];
    pDest[3] = pLeft[3] * pRight[3] - pSub[3];
}

void xglVectorMulSubXYZ(float *pDest, const float *pLeft,
                        const float *pRight, const float *pSub)
{
    pDest[0] = pLeft[0] * pRight[0] - pSub[0];
    pDest[1] = pLeft[1] * pRight[1] - pSub[1];
    pDest[2] = pLeft[2] * pRight[2] - pSub[2];
    pDest[3] = pLeft[3];
}

void xglVectorDivAdd(float *pDest, const float *pLeft,
                     const float *pRight, const float *pAdd)
{
    pDest[0] = pLeft[0] / pRight[0] + pAdd[0];
    pDest[1] = pLeft[1] / pRight[1] + pAdd[1];
    pDest[2] = pLeft[2] / pRight[2] + pAdd[2];
    pDest[3] = pLeft[3] / pRight[3] + pAdd[3];
}

void xglVectorDivAddXYZ(float *pDest, const float *pLeft,
                        const float *pRight, const float *pAdd)
{
    pDest[0] = pLeft[0] / pRight[0] + pAdd[0];
    pDest[1] = pLeft[1] / pRight[1] + pAdd[1];
    pDest[2] = pLeft[2] / pRight[2] + pAdd[2];
    pDest[3] = pLeft[3];
}

void xglVectorDivSub(float *pDest, const float *pLeft,
                     const float *pRight, const float *pSub)
{
    pDest[0] = pLeft[0] / pRight[0] - pSub[0];
    pDest[1] = pLeft[1] / pRight[1] - pSub[1];
    pDest[2] = pLeft[2] / pRight[2] - pSub[2];
    pDest[3] = pLeft[3] / pRight[3] - pSub[3];
}

void xglVectorDivSubXYZ(float *pDest, const float *pLeft,
                        const float *pRight, const float *pSub)
{
    pDest[0] = pLeft[0] / pRight[0] - pSub[0];
    pDest[1] = pLeft[1] / pRight[1] - pSub[1];
    pDest[2] = pLeft[2] / pRight[2] - pSub[2];
    pDest[3] = pLeft[3];
}

void xglVectorScale(float *pDest, const float *pSource, float fScale)
{
    pDest[0] = pSource[0] * fScale;
    pDest[1] = pSource[1] * fScale;
    pDest[2] = pSource[2] * fScale;
    pDest[3] = pSource[3] * fScale;
}

void xglVectorScaleXYZ(float *pDest, const float *pSource, float fScale)
{
    pDest[0] = pSource[0] * fScale;
    pDest[1] = pSource[1] * fScale;
    pDest[2] = pSource[2] * fScale;
    pDest[3] = pSource[3];
}

void xglVectorScaleAdd(float *pDest, const float *pSource,
                       const float *pAdd, float fScale)
{
    pDest[0] = pSource[0] * fScale + pAdd[0];
    pDest[1] = pSource[1] * fScale + pAdd[1];
    pDest[2] = pSource[2] * fScale + pAdd[2];
    pDest[3] = pSource[3] * fScale + pAdd[3];
}

void xglVectorScaleAddXYZ(float *pDest, const float *pSource,
                          const float *pAdd, float fScale)
{
    pDest[0] = pSource[0] * fScale + pAdd[0];
    pDest[1] = pSource[1] * fScale + pAdd[1];
    pDest[2] = pSource[2] * fScale + pAdd[2];
    pDest[3] = pSource[3];
}

void xglVectorScaleSub(float *pDest, const float *pSource,
                       const float *pSub, float fScale)
{
    pDest[0] = pSource[0] * fScale - pSub[0];
    pDest[1] = pSource[1] * fScale - pSub[1];
    pDest[2] = pSource[2] * fScale - pSub[2];
    pDest[3] = pSource[3] * fScale - pSub[3];
}

void xglVectorScaleSubXYZ(float *pDest, const float *pSource,
                          const float *pSub, float fScale)
{
    pDest[0] = pSource[0] * fScale - pSub[0];
    pDest[1] = pSource[1] * fScale - pSub[1];
    pDest[2] = pSource[2] * fScale - pSub[2];
    pDest[3] = pSource[3];
}

void xglVectorInter(float *pDest, const float *pStart,
                    const float *pEnd, float fRatio)
{
    pDest[0] = (pEnd[0] - pStart[0]) * fRatio + pStart[0];
    pDest[1] = (pEnd[1] - pStart[1]) * fRatio + pStart[1];
    pDest[2] = (pEnd[2] - pStart[2]) * fRatio + pStart[2];
    pDest[3] = (pEnd[3] - pStart[3]) * fRatio + pStart[3];
}

void xglVectorInterXYZ(float *pDest, const float *pStart,
                       const float *pEnd, float fRatio)
{
    pDest[0] = (pEnd[0] - pStart[0]) * fRatio + pStart[0];
    pDest[1] = (pEnd[1] - pStart[1]) * fRatio + pStart[1];
    pDest[2] = (pEnd[2] - pStart[2]) * fRatio + pStart[2];
    pDest[3] = pEnd[3];
}

void xglVectorInner(float *pDest, const float *pLeft, const float *pRight)
{
    *pDest = pLeft[0] * pRight[0];
    *pDest += pLeft[1] * pRight[1];
    *pDest += pLeft[2] * pRight[2];
}

float xglVectorInner4(const float *pLeft, const float *pRight)
{
    return pLeft[0] * pRight[0] + pLeft[1] * pRight[1]
         + pLeft[2] * pRight[2] + pLeft[3] * pRight[3];
}

void xglVectorOuter(float *pDest, const float *pLeft, const float *pRight)
{
    pDest[0] = pLeft[1] * pRight[2] - pLeft[2] * pRight[1];
    pDest[1] = pLeft[2] * pRight[0] - pLeft[0] * pRight[2];
    pDest[2] = pLeft[0] * pRight[1] - pLeft[1] * pRight[0];
    pDest[3] = pLeft[3];
}

void xglVectorClamp(float *pDest, const float *pSource,
                    float fMinimum, float fMaximum)
{
    pDest[0] = pSource[0] < fMinimum ? fMinimum
             : fMaximum < pSource[0] ? fMaximum : pSource[0];
    pDest[1] = pSource[1] < fMinimum ? fMinimum
             : fMaximum < pSource[1] ? fMaximum : pSource[1];
    pDest[2] = pSource[2] < fMinimum ? fMinimum
             : fMaximum < pSource[2] ? fMaximum : pSource[2];
    pDest[3] = pSource[3] < fMinimum ? fMinimum
             : fMaximum < pSource[3] ? fMaximum : pSource[3];
}

void xglVectorClampXYZ(float *pDest, const float *pSource,
                       float fMinimum, float fMaximum)
{
    pDest[0] = pSource[0] < fMinimum ? fMinimum
             : fMaximum < pSource[0] ? fMaximum : pSource[0];
    pDest[1] = pSource[1] < fMinimum ? fMinimum
             : fMaximum < pSource[1] ? fMaximum : pSource[1];
    pDest[2] = pSource[2] < fMinimum ? fMinimum
             : fMaximum < pSource[2] ? fMaximum : pSource[2];
    pDest[3] = pSource[3];
}

/* Normalize a 3-component vector (w carried through unnormalized) via the
 * VU0 macro-mode reciprocal-square-root pipeline */
void xglVectorNormal(void *pDest, void *pSource)
{
    __asm__ __volatile__(".set noreorder\n"
        "lqc2 $vf2, 0x0(%1)\n"
        "vmul.xyz $vf3, $vf2, $vf2\n"
        "vaddy.x $vf3, $vf3, $vf3y\n"
        "vaddz.x $vf3, $vf3, $vf3z\n"
        "vrsqrt $Q, $vf0w, $vf3x\n"
        "vwaitq\n"
        "vmulq.xyz $vf2, $vf2, $Q\n"
        "sqc2 $vf2, 0x0(%0)\n"
        ".set reorder" : : "r"(pDest), "r"(pSource));
}

/* Transform a 3-component point by a 4x4 matrix through the VU0
 * macro-mode multiply-accumulate pipeline (w = 1) */
void xglVectorMulMat(void *pDest, void *pMtx, void *pVec)
{
    __asm__ __volatile__(".set noreorder\n"
        "lqc2 $vf1, 0x0(%2)\n"
        "lqc2 $vf27, 0x0(%1)\n lqc2 $vf28, 0x10(%1)\n"
        "lqc2 $vf29, 0x20(%1)\n lqc2 $vf30, 0x30(%1)\n"
        "vmulax.xyzw $ACC, $vf27, $vf1x\n vmadday.xyzw $ACC, $vf28, $vf1y\n"
        "vmaddaz.xyzw $ACC, $vf29, $vf1z\n vmaddw.xyzw $vf31, $vf30, $vf1w\n"
        "sqc2 $vf31, 0x0(%0)\n"
        ".set reorder" : : "r"(pDest), "r"(pMtx), "r"(pVec));
}

/* --- VU0 macro-mode length helpers --- */

/* Length of the xyz part of a vector, stored through pDest */
void xglVectorLength(float *pDest, const float *pVector)
{
    __asm__ __volatile__(".set noreorder\n"
        "lqc2 $vf3, 0x0(%1)\n"
        "vmul.xyz $vf3xyz, $vf3xyz, $vf3xyz\n"
        "vaddy.x $vf3x, $vf3x, $vf3y\n"
        "vaddz.x $vf3x, $vf3x, $vf3z\n"
        "vsqrt $Q, $vf3x\n"
        "vwaitq\n"
        "vaddq.x $vf4x, $vf0x, $Q\n"
        "qmfc2 $2, $vf4\n"
        "sw $2, 0x0(%0)\n"
        ".set reorder" : : "r"(pDest), "r"(pVector) : "$2", "memory");
}

/* Distance between two points (xyz) */
float xglPointLength(const float *pFrom, const float *pTo)
{
    int nRet;

    __asm__ __volatile__(".set noreorder\n"
        "lqc2 $vf2, 0x0(%1)\n"
        "lqc2 $vf3, 0x0(%2)\n"
        "vsub.xyz $vf2xyz, $vf2xyz, $vf3xyz\n"
        "vmul.xyz $vf3xyz, $vf2xyz, $vf2xyz\n"
        "vaddy.x $vf3x, $vf3x, $vf3y\n"
        "vaddz.x $vf3x, $vf3x, $vf3z\n"
        "vsqrt $Q, $vf3x\n"
        "vwaitq\n"
        "vaddq.x $vf4x, $vf0x, $Q\n"
        "qmfc2 %0, $vf4\n"
        ".set reorder" : "=r"(nRet) : "r"(pFrom), "r"(pTo));
    return *(float *)&nRet;
}

/* Distance between two points in the XZ plane only */
float xglPointLengthXZ(const float *pFrom, const float *pTo)
{
    int nRet;

    __asm__ __volatile__(".set noreorder\n"
        "lqc2 $vf2, 0x0(%1)\n"
        "lqc2 $vf3, 0x0(%2)\n"
        "vsub.xyz $vf2xyz, $vf2xyz, $vf3xyz\n"
        "vmul.xyz $vf3xyz, $vf2xyz, $vf2xyz\n"
        "vaddz.x $vf3x, $vf3x, $vf3z\n"
        "vsqrt $Q, $vf3x\n"
        "vwaitq\n"
        "vaddq.x $vf4x, $vf0x, $Q\n"
        "qmfc2 %0, $vf4\n"
        ".set reorder" : "=r"(nRet) : "r"(pFrom), "r"(pTo));
    return *(float *)&nRet;
}
