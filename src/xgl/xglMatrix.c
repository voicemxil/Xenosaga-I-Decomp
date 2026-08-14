/* 4x4 matrix helpers - EE-side quadword math plus the VU0 matrix-stack stubs.
 *
 * The xglMatrixStack* entry points are all VU0 macro-mode: they poke a
 * microprogram entry address into $vi27, hand over operands in $vf4..$vf7 and
 * fire vcallmsr.  Every one of those blocks has to be assembled with reordering
 * off, because the original ee-as neither inserted COP2 hazard nops nor filled
 * the return delay slot from inside the block.
 */

typedef int TI __attribute__((mode(TI)));

typedef union {
    float f[16];
    TI q[4];
} XGL_MATRIX;

typedef struct {
    float x;                        /* 0x00 */
    float y;                        /* 0x04 */
    float z;                        /* 0x08 */
    float w;                        /* 0x0C */
} XGL_VECTOR;

extern unsigned int Vu0CallMatrixStackUnit[];
extern unsigned int Vu0CallMatrixStackMul[];
extern unsigned int Vu0CallMatrixStackReverse[];
extern unsigned int Vu0CallMatrixStackInverse[];
extern unsigned int Vu0CallMatrixStackScale[];
extern unsigned int Vu0CallMatrixStackTrans[];
extern unsigned int Vu0CallMatrixStackRotV[];
extern unsigned int Vu0CallMatrixStackRotX[];
extern unsigned int Vu0CallMatrixStackRotY[];
extern unsigned int Vu0CallMatrixStackRotZ[];
extern unsigned int Vu0CallMatrixStackFrustum[];
extern unsigned int Vu0CallMatrixStackPushUnit[];
extern unsigned int Vu0CallMatrixStackPush[];
extern unsigned int Vu0CallMatrixStackPop[];
extern unsigned int Vu0CallMatrixStackMulVector[];
extern unsigned int Vu0CallMatrixStackRTPS[];

/* Store the identity matrix (quadword copy through the $2 scratch reg,
 * matching the hand-rolled copy macro shape of the original) */
void xglMatrixUnit(XGL_MATRIX *pDst)
{
    static XGL_MATRIX unit = {{
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    }};

    __asm__ __volatile__(".set noreorder\n"
        "lq $2, 0x0(%1)\n sq $2, 0x0(%0)\n"
        "lq $2, 0x10(%1)\n sq $2, 0x10(%0)\n"
        "lq $2, 0x20(%1)\n sq $2, 0x20(%0)\n"
        "lq $2, 0x30(%1)\n sq $2, 0x30(%0)\n"
        ".set reorder" : : "r"(pDst), "r"(&unit) : "$2", "memory");
}

/* Store the identity matrix with every row scale factor set to 1.0 */
void xglMatrixUnit4s(XGL_MATRIX *pDst)
{
    static XGL_MATRIX unit = {{
        1.0f, 0.0f, 0.0f, 1.0f,
        0.0f, 1.0f, 0.0f, 1.0f,
        0.0f, 0.0f, 1.0f, 1.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    }};

    __asm__ __volatile__(".set noreorder\n"
        "lq $2, 0x0(%1)\n sq $2, 0x0(%0)\n"
        "lq $2, 0x10(%1)\n sq $2, 0x10(%0)\n"
        "lq $2, 0x20(%1)\n sq $2, 0x20(%0)\n"
        "lq $2, 0x30(%1)\n sq $2, 0x30(%0)\n"
        ".set reorder" : : "r"(pDst), "r"(&unit) : "$2", "memory");
}

/* Multiply two 4x4 matrices through the VU0 macro-mode multiply-accumulate */
void xglMatrixMul(void *pDst, void *pLeft, void *pRight)
{
    __asm__ __volatile__(".set noreorder\n"
        "lqc2 $vf2, 0x0(%2)\n lqc2 $vf3, 0x10(%2)\n"
        "lqc2 $vf4, 0x20(%2)\n lqc2 $vf5, 0x30(%2)\n"
        "lqc2 $vf27, 0x0(%1)\n lqc2 $vf28, 0x10(%1)\n"
        "lqc2 $vf29, 0x20(%1)\n lqc2 $vf30, 0x30(%1)\n"
        "vmulax.xyzw $ACC, $vf27, $vf2x\n vmadday.xyzw $ACC, $vf28, $vf2y\n"
        "vmaddaz.xyzw $ACC, $vf29, $vf2z\n vmaddw.xyzw $vf2, $vf30, $vf2w\n"
        "vmulax.xyzw $ACC, $vf27, $vf3x\n vmadday.xyzw $ACC, $vf28, $vf3y\n"
        "vmaddaz.xyzw $ACC, $vf29, $vf3z\n vmaddw.xyzw $vf3, $vf30, $vf3w\n"
        "vmulax.xyzw $ACC, $vf27, $vf4x\n vmadday.xyzw $ACC, $vf28, $vf4y\n"
        "vmaddaz.xyzw $ACC, $vf29, $vf4z\n vmaddw.xyzw $vf4, $vf30, $vf4w\n"
        "vmulax.xyzw $ACC, $vf27, $vf5x\n vmadday.xyzw $ACC, $vf28, $vf5y\n"
        "vmaddaz.xyzw $ACC, $vf29, $vf5z\n vmaddw.xyzw $vf30, $vf30, $vf5w\n"
        "sqc2 $vf2, 0x0(%0)\n sqc2 $vf3, 0x10(%0)\n"
        "sqc2 $vf4, 0x20(%0)\n sqc2 $vf30, 0x30(%0)\n"
        ".set reorder" : : "r"(pDst), "r"(pLeft), "r"(pRight));
}

/* TODO: near-match - the inner copy loop swaps $v0/$v1/$a2 and the addu operand order */
/* Transpose a 4x4 matrix into the destination */
void xglMatrixReverse(float *pDst, float *pSrc)
{
    float aTmp[16];
    float *pd;
    float *ps;
    TI t;
    int i;
    int j;
    int n;

    for (i = 0, n = 0; i < 4; i++, n += 4) {
        pd = &aTmp[n];
        ps = &pSrc[i];
        for (j = 3; j >= 0; j--) {
            *pd = *ps;
            ps += 4;
            pd++;
        }
    }
    t = ((TI *)aTmp)[0];
    ((TI *)pDst)[0] = t;
    t = ((TI *)aTmp)[1];
    ((TI *)pDst)[1] = t;
    t = ((TI *)aTmp)[2];
    ((TI *)pDst)[2] = t;
    t = ((TI *)aTmp)[3];
    ((TI *)pDst)[3] = t;
}

/* Scale each column of a 4x4 matrix by one component of a vector */
void xglMatrixScale(float *pDst, float *pSrc, float *pScale)
{
    int i;

    for (i = 3; i >= 0; i--) {
        pDst[0] = pSrc[0] * pScale[0];
        pDst[4] = pSrc[4] * pScale[1];
        pDst[8] = pSrc[8] * pScale[2];
        pDst[12] = pSrc[12] * pScale[3];
        pSrc++;
        pDst++;
    }
}

/* Copy a 4x4 matrix while folding a translation into its last row */
void xglMatrixTrans(float *pDst, float *pSrc, float *pTrans)
{
    int i;

    for (i = 3; i >= 0; i--) {
        pDst[0] = pSrc[0];
        pDst[4] = pSrc[4];
        pDst[8] = pSrc[8];
        pDst[12] = pSrc[0] * pTrans[0] + pSrc[4] * pTrans[1] + pSrc[8] * pTrans[2] + pSrc[12] * pTrans[3];
        pSrc++;
        pDst++;
    }
}

/* Reset the VU0 matrix stack top to the identity */
void xglMatrixStackUnit(void)
{
    __asm__ __volatile__(".set noreorder\n"
        "ctc2.i %0, $vi27\n vnop\n vnop\n vcallmsr $vi27\n"
        ".set reorder" : : "r"((unsigned int)Vu0CallMatrixStackUnit >> 3));
}

/* Multiply the VU0 matrix stack top by a 4x4 matrix from memory */
void xglMatrixStackMul(void *pMtx)
{
    __asm__ __volatile__(".set noreorder\n"
        "ctc2.i %0, $vi27\n"
        "lqc2 $vf4, 0x0(%1)\n lqc2 $vf5, 0x10(%1)\n"
        "lqc2 $vf6, 0x20(%1)\n lqc2 $vf7, 0x30(%1)\n"
        "vcallmsr $vi27\n"
        ".set reorder" : : "r"((unsigned int)Vu0CallMatrixStackMul >> 3), "r"(pMtx));
}

/* Transpose the VU0 matrix stack top in place */
void xglMatrixStackReverse(void)
{
    __asm__ __volatile__(".set noreorder\n"
        "ctc2.i %0, $vi27\n vnop\n vnop\n vcallmsr $vi27\n"
        ".set reorder" : : "r"((unsigned int)Vu0CallMatrixStackReverse >> 3));
}

/* Invert the VU0 matrix stack top in place */
void xglMatrixStackInverse(void)
{
    __asm__ __volatile__(".set noreorder\n"
        "ctc2.i %0, $vi27\n vnop\n vnop\n vcallmsr $vi27\n"
        ".set reorder" : : "r"((unsigned int)Vu0CallMatrixStackInverse >> 3));
}

/* Scale the VU0 matrix stack top by a vector */
void xglMatrixStackScale(void *pVec)
{
    __asm__ __volatile__(".set noreorder\n"
        "ctc2.i %0, $vi27\n vnop\n lqc2 $vf4, 0x0(%1)\n vcallmsr $vi27\n"
        ".set reorder" : : "r"((unsigned int)Vu0CallMatrixStackScale >> 3), "r"(pVec));
}

/* Translate the VU0 matrix stack top by a vector */
void xglMatrixStackTrans(void *pVec)
{
    __asm__ __volatile__(".set noreorder\n"
        "ctc2.i %0, $vi27\n vnop\n lqc2 $vf4, 0x0(%1)\n vcallmsr $vi27\n"
        ".set reorder" : : "r"((unsigned int)Vu0CallMatrixStackTrans >> 3), "r"(pVec));
}

/* Rotate the VU0 matrix stack top about an arbitrary axis */
void xglMatrixStackRotV(void *pVec, float fAngle)
{
    __asm__ __volatile__(".set noreorder\n"
        "ctc2.i %0, $vi27\n lqc2 $vf5, 0x0(%1)\n qmtc2.ni %2, $vf4\n vcallmsr $vi27\n"
        ".set reorder" : : "r"((unsigned int)Vu0CallMatrixStackRotV >> 3), "r"(pVec), "r"(fAngle));
}

/* Rotate the VU0 matrix stack top about X */
void xglMatrixStackRotX(float fAngle)
{
    __asm__ __volatile__(".set noreorder\n"
        "ctc2.i %0, $vi27\n vnop\n qmtc2.ni %1, $vf4\n vcallmsr $vi27\n"
        ".set reorder" : : "r"((unsigned int)Vu0CallMatrixStackRotX >> 3), "r"(fAngle));
}

/* Rotate the VU0 matrix stack top about Y */
void xglMatrixStackRotY(float fAngle)
{
    __asm__ __volatile__(".set noreorder\n"
        "ctc2.i %0, $vi27\n vnop\n qmtc2.ni %1, $vf4\n vcallmsr $vi27\n"
        ".set reorder" : : "r"((unsigned int)Vu0CallMatrixStackRotY >> 3), "r"(fAngle));
}

/* Rotate the VU0 matrix stack top about Z */
void xglMatrixStackRotZ(float fAngle)
{
    __asm__ __volatile__(".set noreorder\n"
        "ctc2.i %0, $vi27\n vnop\n qmtc2.ni %1, $vf4\n vcallmsr $vi27\n"
        ".set reorder" : : "r"((unsigned int)Vu0CallMatrixStackRotZ >> 3), "r"(fAngle));
}

/* Post-multiply the VU0 matrix stack top by a frustum projection */
void xglMatrixStackFrustum(float fLeft, float fRight, float fBottom, float fTop, float fNear, float fFar)
{
    XGL_VECTOR *p;

    p = (XGL_VECTOR *)0x11004800;
    __asm__ __volatile__(".set noreorder\n"
        "ctc2.i %0, $vi27\n"
        ".set reorder" : : "r"((unsigned int)Vu0CallMatrixStackFrustum >> 3));
    p[0].x = fLeft;
    p[0].y = fBottom;
    p[0].z = fNear;
    p[1].x = fRight;
    p[1].y = fTop;
    p[1].z = fFar;
    p[0].w = 0.0f;
    p[1].w = 0.0f;
    __asm__ __volatile__(".set noreorder\n"
        "sync\n vcallmsr $vi27\n"
        ".set reorder");
}

/* Copy the resident VU0 matrix registers out to memory */
void xglMatrixStackSave(void *pMtx)
{
    __asm__ __volatile__(".set noreorder\n"
        "ctc2.i %0, $vi0\n"
        "sqc2 $vf28, 0x0(%0)\n sqc2 $vf29, 0x10(%0)\n"
        "sqc2 $vf30, 0x20(%0)\n sqc2 $vf31, 0x30(%0)\n"
        ".set reorder" : : "r"(pMtx));
}

/* Reload the resident VU0 matrix registers from memory */
void xglMatrixStackLoad(void *pMtx)
{
    __asm__ __volatile__(".set noreorder\n"
        "ctc2.i %0, $vi0\n"
        "lqc2 $vf28, 0x0(%0)\n lqc2 $vf29, 0x10(%0)\n"
        "lqc2 $vf30, 0x20(%0)\n lqc2 $vf31, 0x30(%0)\n"
        ".set reorder" : : "r"(pMtx));
}

/* Push the identity matrix onto the VU0 matrix stack */
void xglMatrixStackPushUnit(void)
{
    __asm__ __volatile__(".set noreorder\n"
        "ctc2.i %0, $vi27\n vnop\n vnop\n vcallmsr $vi27\n"
        ".set reorder" : : "r"((unsigned int)Vu0CallMatrixStackPushUnit >> 3));
}

/* Push a copy of the current matrix onto the VU0 matrix stack */
void xglMatrixStackPush(void)
{
    __asm__ __volatile__(".set noreorder\n"
        "ctc2.i %0, $vi27\n vnop\n vnop\n vcallmsr $vi27\n"
        ".set reorder" : : "r"((unsigned int)Vu0CallMatrixStackPush >> 3));
}

/* Pop the VU0 matrix stack */
void xglMatrixStackPop(void *pMtx)
{
    __asm__ __volatile__(".set noreorder\n"
        "ctc2.i %0, $vi27\n vnop\n ctc2.ni %1, $vi1\n vcallmsr $vi27\n"
        ".set reorder" : : "r"((unsigned int)Vu0CallMatrixStackPop >> 3), "r"(pMtx));
}

/* Transform one vector by the VU0 matrix stack top */
void xglMatrixStackMulVector(void *pDst, void *pSrc)
{
    __asm__ __volatile__(".set noreorder\n"
        "ctc2.i %0, $vi27\n vnop\n lqc2 $vf4, 0x0(%2)\n vcallmsr $vi27\n"
        "cfc2.i $0, $vi0\n sqc2 $vf5, 0x0(%1)\n"
        ".set reorder" : : "r"((unsigned int)Vu0CallMatrixStackMulVector >> 3), "r"(pDst), "r"(pSrc));
}

/* Rotate, translate and perspective-transform a vector on VU0 */
void xglMatrixStackRTPS(void *pDst, void *pA, void *pB, void *pC)
{
    __asm__ __volatile__(".set noreorder\n"
        "ctc2.i %0, $vi27\n"
        "lqc2 $vf4, 0x0(%2)\n lqc2 $vf5, 0x0(%3)\n lqc2 $vf6, 0x0(%4)\n"
        "vcallmsr $vi27\n cfc2.i $0, $vi0\n sqc2 $vf7, 0x0(%1)\n"
        ".set reorder" : : "r"((unsigned int)Vu0CallMatrixStackRTPS >> 3), "r"(pDst), "r"(pA), "r"(pB), "r"(pC));
}

/* --- Quaternion -> rotation matrix (VU0 macro mode) --- */
void xglQuaternion2Matrix(void *pMtx, void *pQuat)
{
    __asm__ __volatile__(".set noreorder\n"
        "lqc2 $vf31, 0x0(%1)\n"
        "vaddw.x $vf27x, $vf0x, $vf0w\n"
        "vaddw.y $vf28y, $vf0y, $vf0w\n"
        "vaddw.z $vf29z, $vf0z, $vf0w\n"
        "vadd.xyzw $vf14xyzw, $vf31xyzw, $vf31xyzw\n"
        "vsub.w $vf27w, $vf27w, $vf27w\n"
        "vsub.w $vf28w, $vf28w, $vf28w\n"
        "vsub.w $vf29w, $vf29w, $vf29w\n"
        "vmulx.xyzw $vf15xyzw, $vf31xyzw, $vf14x\n"
        "vmuly.xyzw $vf16xyzw, $vf31xyzw, $vf14y\n"
        "vmulz.xyzw $vf17xyzw, $vf31xyzw, $vf14z\n"
        "vnop\n"
        "vnop\n"
        "vaddy.x $vf20x, $vf15x, $vf16y\n"
        "vaddz.y $vf20y, $vf16y, $vf17z\n"
        "vaddx.z $vf20z, $vf17z, $vf15x\n"
        "vnop\n"
        "vsubw.z $vf27z, $vf15z, $vf16w\n"
        "vaddw.y $vf27y, $vf15y, $vf17w\n"
        "vsuby.x $vf27x, $vf27x, $vf20y\n"
        "vsubw.x $vf28x, $vf16x, $vf17w\n"
        "vaddw.z $vf28z, $vf16z, $vf15w\n"
        "vsubz.y $vf28y, $vf28y, $vf20z\n"
        "vsubw.y $vf29y, $vf17y, $vf15w\n"
        "vaddw.x $vf29x, $vf17x, $vf16w\n"
        "vsubx.z $vf29z, $vf29z, $vf20x\n"
        "sqc2 $vf0, 0x30(%0)\n"
        "sqc2 $vf27, 0x0(%0)\n"
        "sqc2 $vf28, 0x10(%0)\n"
        "sqc2 $vf29, 0x20(%0)\n"
        ".set reorder" : : "r"(pMtx), "r"(pQuat) : "memory");
}

/* --- Rotate/translate/perspective transforms through the studio camera --- */

typedef struct {
    char pad00[0x70];
    int nUnk70;          /* 0x70 */
    char pad74[0xC];
    int nUnk80;          /* 0x80 */
    char pad84[0x470 - 0x84];
    int nUnk470;         /* 0x470 */
} XGLSTUDIOCAM;

XGLSTUDIOCAM *xglStudioGetCamera2(int nCamera);

float xglRotTransPers(XGL_VECTOR *pOut, void *pMtx, XGL_VECTOR *pIn, int nCamera)
{
    XGLSTUDIOCAM *pCam;

    xglMatrixStackPush();
    pCam = xglStudioGetCamera2(nCamera);
    xglMatrixStackLoad(&pCam->nUnk470);
    if (pMtx != 0) {
        xglMatrixStackMul(pMtx);
    }
    xglMatrixStackRTPS(pOut, pIn, &pCam->nUnk80, &pCam->nUnk70);
    xglMatrixStackPop((void *)1);
    return pOut->w;
}

void xglRotTransPersN(XGL_VECTOR *pOut, void *pMtx, XGL_VECTOR *pIn, int nCount, int nCamera)
{
    XGLSTUDIOCAM *pCam;

    xglMatrixStackPush();
    pCam = xglStudioGetCamera2(nCamera);
    xglMatrixStackLoad(&pCam->nUnk470);
    if (pMtx != 0) {
        xglMatrixStackMul(pMtx);
    }
    while (nCount > 0) {
        nCount--;
        xglMatrixStackRTPS(pOut, pIn, &pCam->nUnk80, &pCam->nUnk70);
        pOut++;
        pIn++;
    }
    xglMatrixStackPop((void *)1);
}
/* --- Euler rotation constructors (VU0 sin/cos + hand-scheduled row mix) --- */

extern unsigned int Vu0CallSin[];
extern unsigned int Vu0CallCos[];

void xglMatrixRotX(XGL_MATRIX *pDst, XGL_MATRIX *pSrc, float fAngle)
{
    int nSin;
    int nCos;
    float aSC[4];
    register float *pSC __asm__("$7") = aSC;
    float fSin;
    float fCos;

    __asm__ __volatile__(".set noreorder\n"
        "ctc2.i %1, $vi27\n"
        "vnop\n"
        "qmtc2 %2, $vf4\n"
        "vcallmsr $vi27\n"
        "qmfc2.i %0, $vf1\n"
        ".set reorder"
        : "=r"(nSin)
        : "r"((unsigned int)Vu0CallSin >> 3), "r"(fAngle));
    fSin = *(float *)&nSin;
    aSC[1] = fSin;
    __asm__ __volatile__(".set noreorder\n"
        "ctc2.i %1, $vi27\n"
        "vnop\n"
        "qmtc2 %2, $vf4\n"
        "vcallmsr $vi27\n"
        "qmfc2.i %0, $vf1\n"
        ".set reorder"
        : "=r"(nCos)
        : "r"((unsigned int)Vu0CallCos >> 3), "r"(fAngle));
    fCos = *(float *)&nCos;
    aSC[0] = fCos;
    __asm__ __volatile__(".set noreorder\n"
        "lq $2, 0x0(%1)\n"
        "lqc2 $vf28, 0x10(%1)\n"
        "lqc2 $vf29, 0x20(%1)\n"
        "lqc2 $vf1, 0x0(%2)\n"
        "sq $2, 0x0(%0)\n"
        "lq $2, 0x30(%1)\n"
        "sq $2, 0x30(%0)\n"
        "vmulax.xyzw $ACC, $vf28, $vf1x\n"
        "vmaddy.xyzw $vf27, $vf29, $vf1y\n"
        "vmulax.xyzw $ACC, $vf29, $vf1x\n"
        "vmsuby.xyzw $vf30, $vf28, $vf1y\n"
        "sqc2 $vf27, 0x10(%0)\n"
        "sqc2 $vf30, 0x20(%0)\n"
        ".set reorder" : : "r"(pDst), "r"(pSrc), "r"(pSC) : "$2", "memory");
}

void xglMatrixRotY(XGL_MATRIX *pDst, XGL_MATRIX *pSrc, float fAngle)
{
    int nSin;
    int nCos;
    float aSC[4];
    register float *pSC __asm__("$7") = aSC;
    float fSin;
    float fCos;

    __asm__ __volatile__(".set noreorder\n"
        "ctc2.i %1, $vi27\n"
        "vnop\n"
        "qmtc2 %2, $vf4\n"
        "vcallmsr $vi27\n"
        "qmfc2.i %0, $vf1\n"
        ".set reorder"
        : "=r"(nSin)
        : "r"((unsigned int)Vu0CallSin >> 3), "r"(fAngle));
    fSin = *(float *)&nSin;
    aSC[1] = fSin;
    __asm__ __volatile__(".set noreorder\n"
        "ctc2.i %1, $vi27\n"
        "vnop\n"
        "qmtc2 %2, $vf4\n"
        "vcallmsr $vi27\n"
        "qmfc2.i %0, $vf1\n"
        ".set reorder"
        : "=r"(nCos)
        : "r"((unsigned int)Vu0CallCos >> 3), "r"(fAngle));
    fCos = *(float *)&nCos;
    aSC[0] = fCos;
    __asm__ __volatile__(".set noreorder\n"
        "lq $2, 0x10(%1)\n"
        "lqc2 $vf27, 0x0(%1)\n"
        "lqc2 $vf29, 0x20(%1)\n"
        "lqc2 $vf1, 0x0(%2)\n"
        "sq $2, 0x10(%0)\n"
        "lq $2, 0x30(%1)\n"
        "sq $2, 0x30(%0)\n"
        "vmulax.xyzw $ACC, $vf27, $vf1x\n"
        "vmsuby.xyzw $vf28, $vf29, $vf1y\n"
        "vmulax.xyzw $ACC, $vf29, $vf1x\n"
        "vmaddy.xyzw $vf30, $vf27, $vf1y\n"
        "sqc2 $vf28, 0x0(%0)\n"
        "sqc2 $vf30, 0x20(%0)\n"
        ".set reorder" : : "r"(pDst), "r"(pSrc), "r"(pSC) : "$2", "memory");
}

void xglMatrixRotZ(XGL_MATRIX *pDst, XGL_MATRIX *pSrc, float fAngle)
{
    int nSin;
    int nCos;
    float aSC[4];
    register float *pSC __asm__("$7") = aSC;
    float fSin;
    float fCos;

    __asm__ __volatile__(".set noreorder\n"
        "ctc2.i %1, $vi27\n"
        "vnop\n"
        "qmtc2 %2, $vf4\n"
        "vcallmsr $vi27\n"
        "qmfc2.i %0, $vf1\n"
        ".set reorder"
        : "=r"(nSin)
        : "r"((unsigned int)Vu0CallSin >> 3), "r"(fAngle));
    fSin = *(float *)&nSin;
    aSC[1] = fSin;
    __asm__ __volatile__(".set noreorder\n"
        "ctc2.i %1, $vi27\n"
        "vnop\n"
        "qmtc2 %2, $vf4\n"
        "vcallmsr $vi27\n"
        "qmfc2.i %0, $vf1\n"
        ".set reorder"
        : "=r"(nCos)
        : "r"((unsigned int)Vu0CallCos >> 3), "r"(fAngle));
    fCos = *(float *)&nCos;
    aSC[0] = fCos;
    __asm__ __volatile__(".set noreorder\n"
        "lq $2, 0x20(%1)\n"
        "lqc2 $vf27, 0x0(%1)\n"
        "lqc2 $vf28, 0x10(%1)\n"
        "lqc2 $vf1, 0x0(%2)\n"
        "sq $2, 0x20(%0)\n"
        "lq $2, 0x30(%1)\n"
        "sq $2, 0x30(%0)\n"
        "vmulax.xyzw $ACC, $vf27, $vf1x\n"
        "vmaddy.xyzw $vf29, $vf28, $vf1y\n"
        "vmulax.xyzw $ACC, $vf28, $vf1x\n"
        "vmsuby.xyzw $vf30, $vf27, $vf1y\n"
        "sqc2 $vf29, 0x0(%0)\n"
        "sqc2 $vf30, 0x10(%0)\n"
        ".set reorder" : : "r"(pDst), "r"(pSrc), "r"(pSC) : "$2", "memory");
}