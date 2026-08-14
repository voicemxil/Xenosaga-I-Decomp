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

/* TODO: near-match - $v0/$v1 are swapped and gcc fills the return delay slot with the last sq */
/* Store the identity matrix */
void xglMatrixUnit(XGL_MATRIX *pDst)
{
    static XGL_MATRIX unit = {{
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    }};
    TI t;

    t = unit.q[0];
    pDst->q[0] = t;
    t = unit.q[1];
    pDst->q[1] = t;
    t = unit.q[2];
    pDst->q[2] = t;
    t = unit.q[3];
    pDst->q[3] = t;
}

/* TODO: near-match - $v0/$v1 are swapped and gcc fills the return delay slot with the last sq */
/* Store the identity matrix with every row scale factor set to 1.0 */
void xglMatrixUnit4s(XGL_MATRIX *pDst)
{
    static XGL_MATRIX unit = {{
        1.0f, 0.0f, 0.0f, 1.0f,
        0.0f, 1.0f, 0.0f, 1.0f,
        0.0f, 0.0f, 1.0f, 1.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    }};
    TI t;

    t = unit.q[0];
    pDst->q[0] = t;
    t = unit.q[1];
    pDst->q[1] = t;
    t = unit.q[2];
    pDst->q[2] = t;
    t = unit.q[3];
    pDst->q[3] = t;
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
