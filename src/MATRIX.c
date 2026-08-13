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

/* TODO: near-match (2 diffs, SCHEDULING) -- instruction multiset is exactly
 * right but the original schedules the final sqc2 (pDst+0x30) into the
 * leaf return's jr delay slot; our build emits it before jr with a plain
 * trailing nop. Same wall already documented on xglMatrixUnit/Unit4s in
 * xglMatrix.c: that delay-slot fill comes from gcc's own codegen for a
 * non-opaque store and is not reachable when the store lives inside an
 * inline-asm block (opaque to the scheduler). Not registered. */
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
        "sqc2 $vf4, 0x20(%0)\n sqc2 $vf30, 0x30(%0)\n"
        ".set reorder" : : "r"(pDst), "r"(pMat), "r"(pSrc));
}

/* TODO: near-match (2 diffs, SCHEDULING) -- same delay-slot-fill wall as
 * MATRIX_convert4MulMatrix above. Not registered. */
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
        "sqc2 $vf29, 0x20(%0)\n sqc2 $vf30, 0x30(%0)\n"
        ".set reorder" : : "r"(pDst), "r"(pMat), "r"(pSrc));
}
