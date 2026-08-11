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
