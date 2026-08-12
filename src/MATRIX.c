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
