/* Basic matrix helpers used by the game-side transform code. */

void MATRIX_transpose4(float *out, const float *in)
{
    out[0] = in[0];
    out[1] = in[4];
    out[2] = in[8];
    out[3] = in[12];
    out[4] = in[1];
    out[5] = in[5];
    out[6] = in[9];
    out[7] = in[13];
    out[8] = in[2];
    out[9] = in[6];
    out[10] = in[10];
    out[11] = in[14];
    out[12] = in[3];
    out[13] = in[7];
    out[14] = in[11];
    out[15] = in[15];
}

void MATRIX_scale4s(float *m, float x, float y, float z)
{
    m[3] *= x;
    m[7] *= y;
    m[11] *= z;
}

void MATRIX_translate4(float *m, float x, float y, float z)
{
    m[12] = m[0] * x + m[4] * y + m[8] * z + m[12];
    m[13] = m[1] * x + m[5] * y + m[9] * z + m[13];
    m[14] = m[2] * x + m[6] * y + m[10] * z + m[14];
    m[15] = m[3] * x + m[7] * y + m[11] * z + m[15];
}

void MATRIX_scale4(float *m, float x, float y, float z)
{
    m[0] *= x;
    m[4] *= x;
    m[8] *= x;
    m[12] *= x;
    m[1] *= y;
    m[5] *= y;
    m[9] *= y;
    m[13] *= y;
    m[2] *= z;
    m[6] *= z;
    m[10] *= z;
    m[14] *= z;
}

void MATRIX_rotate4(float *m, float angle, float x, float y, float z);
void MATRIX_mul3x4(float *out, const float *left, const float *right);
void MATRIX_mul4sx3(float *out, const float *left, const float *right);

void MATRIX_rotX4(float *m, float angle)
{
    float rotate[16];
    MATRIX_rotate4(rotate, angle, 1.0f, 0.0f, 0.0f);
    MATRIX_mul3x4(m, m, rotate);
}

void MATRIX_rotY4(float *m, float angle)
{
    float rotate[16];
    MATRIX_rotate4(rotate, angle, 0.0f, 1.0f, 0.0f);
    MATRIX_mul3x4(m, m, rotate);
}

void MATRIX_rotZ4(float *m, float angle)
{
    float rotate[16];
    MATRIX_rotate4(rotate, angle, 0.0f, 0.0f, 1.0f);
    MATRIX_mul3x4(m, m, rotate);
}

void MATRIX_rotX4s(float *m, float angle)
{
    float rotate[16];
    MATRIX_rotate4(rotate, angle, 1.0f, 0.0f, 0.0f);
    MATRIX_mul4sx3(m, m, rotate);
}

void MATRIX_rotY4s(float *m, float angle)
{
    float rotate[16];
    MATRIX_rotate4(rotate, angle, 0.0f, 1.0f, 0.0f);
    MATRIX_mul4sx3(m, m, rotate);
}

void MATRIX_rotZ4s(float *m, float angle)
{
    float rotate[16];
    MATRIX_rotate4(rotate, angle, 0.0f, 0.0f, 1.0f);
    MATRIX_mul4sx3(m, m, rotate);
}

/* These entry points were reserved for combined Euler rotations but are
 * intentionally empty in the shipped game. */
void MATRIX_rotXYZ(void)
{
}

void MATRIX_rotZYX(void)
{
}

extern float sqrtf(float);

/* Convert a matrix whose rows carry a per-row scale in their w component:
 * scale each row's xyz by that row's own w, then zero it; row 3 (the
 * translation row) is copied unchanged. */
void MATRIX_convert4(float *out, const float *in)
{
    float zero = 0.0f;

    out[0] = in[3] * in[0];
    out[1] = in[3] * in[1];
    out[2] = in[3] * in[2];

    out[4] = in[7] * in[4];
    out[5] = in[7] * in[5];
    out[6] = in[7] * in[6];

    out[8] = in[11] * in[8];
    out[9] = in[11] * in[9];
    out[10] = in[11] * in[10];

    out[11] = zero;
    out[7] = zero;
    out[3] = zero;

    out[12] = in[12];
    out[13] = in[13];
    out[14] = in[14];
    out[15] = in[15];
}

/* Convert a matrix whose rows carry raw xyz plus an implied scale: copy
 * each row's xyz unchanged, and replace its w with the xyz vector's own
 * length (its scale magnitude); row 3 is copied unchanged. */
void MATRIX_convert4s(float *out, const float *in)
{
    out[0] = in[0];
    out[1] = in[1];
    out[2] = in[2];
    out[3] = sqrtf(in[0] * in[0] + in[1] * in[1] + in[2] * in[2]);

    out[4] = in[4];
    out[5] = in[5];
    out[6] = in[6];
    out[7] = sqrtf(in[4] * in[4] + in[5] * in[5] + in[6] * in[6]);

    out[8] = in[8];
    out[9] = in[9];
    out[10] = in[10];
    out[11] = sqrtf(in[8] * in[8] + in[9] * in[9] + in[10] * in[10]);

    out[12] = in[12];
    out[13] = in[13];
    out[14] = in[14];
    out[15] = in[15];
}

/* MATCHING NOTE (gas delay-slot priming) -- gcc emits the leaf return as a
 * bare `j $31` and leaves the delay slot for gas to fill in .set reorder
 * mode. gas can only do that if it has a valid "previous instruction", and
 * leaving .set noreorder invalidates it: the FIRST instruction assembled
 * after `.set reorder` only primes the tracker, the second is the first one
 * gas will actually move. So `.set reorder` has to come at least two
 * instructions before the end of the asm block -- with it on the last line
 * the final sqc2 is unmovable and gcc's nop stays. Placing it before the
 * last two sqc2s lets gas swap the final store into the jr delay slot,
 * exactly as the original. (Same fix should apply to xglMatrixUnit/Unit4s
 * in xglMatrix.c, where this was previously written off as unreachable.) */
/* VU0 macro mode: scale each of the first three rows of pMat by its own w
 * (then zero that w), multiply by pSrc (row-major), and write to pDst. */
void MATRIX_convert4MulMatrix(float *pDst, float *pMat, float *pSrc)
{
    __asm__ __volatile__(".set noreorder\n"
        "lqc2 $vf27, 0x0(%1)\n lqc2 $vf28, 0x10(%1)\n"
        "lqc2 $vf29, 0x20(%1)\n lqc2 $vf30, 0x30(%1)\n"
        "vmulw.xyz $vf27, $vf27, $vf27w\n"
        "vmulw.xyz $vf28, $vf28, $vf28w\n"
        "vmulw.xyz $vf29, $vf29, $vf29w\n"
        "vsub.w $vf27, $vf27, $vf27w\n"
        "vsub.w $vf28, $vf28, $vf28w\n"
        "vsub.w $vf29, $vf29, $vf29w\n"
        "lqc2 $vf2, 0x0(%2)\n lqc2 $vf3, 0x10(%2)\n"
        "lqc2 $vf4, 0x20(%2)\n lqc2 $vf5, 0x30(%2)\n"
        "sqc2 $vf27, 0x0(%1)\n sqc2 $vf28, 0x10(%1)\n"
        "sqc2 $vf29, 0x20(%1)\n sqc2 $vf30, 0x30(%1)\n"
        "vmulax.xyzw $ACC, $vf27, $vf2x\n"
        "vmadday.xyzw $ACC, $vf28, $vf2y\n"
        "vmaddaz.xyzw $ACC, $vf29, $vf2z\n"
        "vmaddw.xyzw $vf2, $vf30, $vf2w\n"
        "vmulax.xyzw $ACC, $vf27, $vf3x\n"
        "vmadday.xyzw $ACC, $vf28, $vf3y\n"
        "vmaddaz.xyzw $ACC, $vf29, $vf3z\n"
        "vmaddw.xyzw $vf3, $vf30, $vf3w\n"
        "vmulax.xyzw $ACC, $vf27, $vf4x\n"
        "vmadday.xyzw $ACC, $vf28, $vf4y\n"
        "vmaddaz.xyzw $ACC, $vf29, $vf4z\n"
        "vmaddw.xyzw $vf4, $vf30, $vf4w\n"
        "vmulax.xyzw $ACC, $vf27, $vf5x\n"
        "vmadday.xyzw $ACC, $vf28, $vf5y\n"
        "vmaddaz.xyzw $ACC, $vf29, $vf5z\n"
        "vmaddw.xyzw $vf30, $vf30, $vf5w\n"
        "sqc2 $vf2, 0x0(%0)\n sqc2 $vf3, 0x10(%0)\n"
        ".set reorder\n"
        "sqc2 $vf4, 0x20(%0)\n sqc2 $vf30, 0x30(%0)"
        : : "r"(pDst), "r"(pMat), "r"(pSrc));
}

/* Same gas delay-slot priming placement of `.set reorder` as
 * MATRIX_convert4MulMatrix above. */
/* VU0 macro mode: same row-scale prep on pMat as MATRIX_convert4MulMatrix,
 * but multiply the other way round (pSrc rows dotted against pMat columns). */
void MATRIX_convert4MulMatrixRev(float *pDst, float *pMat, float *pSrc)
{
    __asm__ __volatile__(".set noreorder\n"
        "lqc2 $vf27, 0x0(%1)\n lqc2 $vf28, 0x10(%1)\n"
        "lqc2 $vf29, 0x20(%1)\n lqc2 $vf30, 0x30(%1)\n"
        "vmulw.xyz $vf27, $vf27, $vf27w\n"
        "vmulw.xyz $vf28, $vf28, $vf28w\n"
        "vmulw.xyz $vf29, $vf29, $vf29w\n"
        "vsub.w $vf27, $vf27, $vf27w\n"
        "vsub.w $vf28, $vf28, $vf28w\n"
        "vsub.w $vf29, $vf29, $vf29w\n"
        "lqc2 $vf2, 0x0(%2)\n lqc2 $vf3, 0x10(%2)\n"
        "lqc2 $vf4, 0x20(%2)\n lqc2 $vf5, 0x30(%2)\n"
        "vmulax.xyzw $ACC, $vf2, $vf27x\n"
        "vmadday.xyzw $ACC, $vf3, $vf27y\n"
        "vmaddaz.xyzw $ACC, $vf4, $vf27z\n"
        "vmaddw.xyzw $vf27, $vf5, $vf27w\n"
        "vmulax.xyzw $ACC, $vf2, $vf28x\n"
        "vmadday.xyzw $ACC, $vf3, $vf28y\n"
        "vmaddaz.xyzw $ACC, $vf4, $vf28z\n"
        "vmaddw.xyzw $vf28, $vf5, $vf28w\n"
        "vmulax.xyzw $ACC, $vf2, $vf29x\n"
        "vmadday.xyzw $ACC, $vf3, $vf29y\n"
        "vmaddaz.xyzw $ACC, $vf4, $vf29z\n"
        "vmaddw.xyzw $vf29, $vf5, $vf29w\n"
        "vmulax.xyzw $ACC, $vf2, $vf30x\n"
        "vmadday.xyzw $ACC, $vf3, $vf30y\n"
        "vmaddaz.xyzw $ACC, $vf4, $vf30z\n"
        "vmaddw.xyzw $vf30, $vf5, $vf30w\n"
        "sqc2 $vf27, 0x0(%0)\n sqc2 $vf28, 0x10(%0)\n"
        ".set reorder\n"
        "sqc2 $vf29, 0x20(%0)\n sqc2 $vf30, 0x30(%0)"
        : : "r"(pDst), "r"(pMat), "r"(pSrc));
}

/* Multiply affine matrices while preserving the left matrix row scales. */
void MATRIX_mul4sx3(float *pDst, const float *pLeft, const float *pRight)
{
    const float *pScale;
    const float *pRow;
    float *pOut;
    float value[4];
    float scale;
    int i;

    pScale = pLeft + 3;
    pOut = pDst;
    pRow = pLeft;
    i = 2;
    do {
        value[0] = pRow[0];
        i--;
        value[1] = pRow[4];
        value[2] = pRow[8];
        value[3] = pRow[12];
        pRow++;
        pOut[0] = value[0] * pRight[0] + value[1] * pRight[1] + value[2] * pRight[2];
        pOut[4] = value[0] * pRight[4] + value[1] * pRight[5] + value[2] * pRight[6];
        pOut[8] = value[0] * pRight[8] + value[1] * pRight[9] + value[2] * pRight[10];
        scale = *pScale;
        pScale += 4;
        pOut[12] = value[0] * scale * pRight[12]
                 + value[1] * scale * pRight[13]
                 + value[2] * scale * pRight[14]
                 + value[3];
        pOut++;
    } while (i >= 0);

    pDst[3] = pLeft[3];
    pDst[7] = pLeft[7];
    pDst[11] = pLeft[11];
    pDst[15] = 1.0f;
}

/* Translate a matrix whose first three rows carry scale in w. */
void MATRIX_translate4s(float *pMat, float x, float y, float z)
{
    pMat[12] = pMat[0] * pMat[3] * x
             + pMat[4] * pMat[7] * y
             + pMat[8] * pMat[11] * z
             + pMat[12];
    pMat[13] = pMat[1] * pMat[3] * x
             + pMat[5] * pMat[7] * y
             + pMat[9] * pMat[11] * z
             + pMat[13];
    pMat[14] = pMat[2] * pMat[3] * x
             + pMat[6] * pMat[7] * y
             + pMat[10] * pMat[11] * z
             + pMat[14];
}
