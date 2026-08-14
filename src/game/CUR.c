/* CUR_MATRIX - the "current matrix" kept resident in VU0 registers vf27..vf30.
 *
 * vf27/vf28/vf29 hold the three rotation/scale rows and vf30 the translation.
 * Every entry point is VU0 macro-mode, so the bodies are inline assembly; the
 * "4s" variants keep a per-row scale factor in the w component.
 */

/* Load the identity matrix into the current matrix */
void CUR_MATRIX_Unit(void)
{
    __asm__ __volatile__("vmove.xyzw $vf30, $vf0");
    __asm__ __volatile__("vmr32.xyzw $vf29, $vf0");
    __asm__ __volatile__("vmr32.xyzw $vf28, $vf29");
    __asm__ __volatile__("vmr32.xyzw $vf27, $vf28");
}

/* Load the identity matrix and reset the per-row scale factors to 1.0 */
void CUR_MATRIX_Unit4s(void)
{
    __asm__ __volatile__("vmove.xyzw $vf30, $vf0");
    __asm__ __volatile__("vmr32.xyzw $vf29, $vf0");
    __asm__ __volatile__("vmr32.xyzw $vf28, $vf29");
    __asm__ __volatile__("vmr32.xyzw $vf27, $vf28");
    __asm__ __volatile__("vmove.w $vf27, $vf0");
    __asm__ __volatile__("vmove.w $vf28, $vf0");
    __asm__ __volatile__("vmove.w $vf29, $vf0");
}

/* Replace the current matrix with a 4x4 matrix from memory */
void CUR_MATRIX_Set(void *pMtx)
{
    __asm__ __volatile__("lqc2 $vf27, 0x0(%0)" : : "r"(pMtx));
    __asm__ __volatile__("lqc2 $vf28, 0x10(%0)" : : "r"(pMtx));
    __asm__ __volatile__("lqc2 $vf29, 0x20(%0)" : : "r"(pMtx));
    __asm__ __volatile__("lqc2 $vf30, 0x30(%0)" : : "r"(pMtx));
}

/* Store the current matrix out to a 4x4 matrix in memory */
void CUR_MATRIX_Get(void *pMtx)
{
    __asm__ __volatile__("sqc2 $vf27, 0x0(%0)" : : "r"(pMtx));
    __asm__ __volatile__("sqc2 $vf28, 0x10(%0)" : : "r"(pMtx));
    __asm__ __volatile__("sqc2 $vf29, 0x20(%0)" : : "r"(pMtx));
    __asm__ __volatile__("sqc2 $vf30, 0x30(%0)" : : "r"(pMtx));
}

/* Store just the translation row, with w forced to 1.0 */
void CUR_MATRIX_GetT(void *pVec)
{
    __asm__ __volatile__("vmove.xyz $vf20, $vf30");
    __asm__ __volatile__("vmove.w $vf20, $vf0");
    __asm__ __volatile__("sqc2 $vf20, 0x0(%0)" : : "r"(pVec));
}

/* Replace just the translation row */
void CUR_MATRIX_SetT(void *pVec)
{
    __asm__ __volatile__("lqc2 $vf1, 0x0(%0)" : : "r"(pVec));
    __asm__ __volatile__("vmove.xyz $vf30, $vf1");
}

/* Replace the three rotation rows, leaving translation and scales alone */
void CUR_MATRIX_SetR(void *pMtx)
{
    __asm__ __volatile__("lqc2 $vf20, 0x0(%0)" : : "r"(pMtx));
    __asm__ __volatile__("lqc2 $vf21, 0x10(%0)" : : "r"(pMtx));
    __asm__ __volatile__("lqc2 $vf22, 0x20(%0)" : : "r"(pMtx));
    __asm__ __volatile__("vmove.xyz $vf27, $vf20");
    __asm__ __volatile__("vmove.xyz $vf28, $vf21");
    __asm__ __volatile__("vmove.xyz $vf29, $vf22");
}

/* Concatenate a 4x4 matrix onto the current matrix, folding in the scales */
void CUR_MATRIX_4s3(void *pMtx)
{
    __asm__ __volatile__("lqc2 $vf20, 0x0(%0)" : : "r"(pMtx));
    __asm__ __volatile__("lqc2 $vf21, 0x10(%0)" : : "r"(pMtx));
    __asm__ __volatile__("lqc2 $vf22, 0x20(%0)" : : "r"(pMtx));
    __asm__ __volatile__("lqc2 $vf23, 0x30(%0)" : : "r"(pMtx));
    __asm__ __volatile__("vmulax.xyz $ACC, $vf27, $vf20x");
    __asm__ __volatile__("vmadday.xyz $ACC, $vf28, $vf20y");
    __asm__ __volatile__("vmaddz.xyz $vf20, $vf29, $vf20z");
    __asm__ __volatile__("vmulax.xyz $ACC, $vf27, $vf21x");
    __asm__ __volatile__("vmadday.xyz $ACC, $vf28, $vf21y");
    __asm__ __volatile__("vmaddz.xyz $vf21, $vf29, $vf21z");
    __asm__ __volatile__("vmulax.xyz $ACC, $vf27, $vf22x");
    __asm__ __volatile__("vmadday.xyz $ACC, $vf28, $vf22y");
    __asm__ __volatile__("vmaddz.xyz $vf22, $vf29, $vf22z");
    __asm__ __volatile__("vmulw.xyz $vf24, $vf27, $vf27w");
    __asm__ __volatile__("vmulw.xyz $vf25, $vf28, $vf28w");
    __asm__ __volatile__("vmulw.xyz $vf26, $vf29, $vf29w");
    __asm__ __volatile__("vmove.w $vf30, $vf0");
    __asm__ __volatile__("vmove.xyz $vf27, $vf20");
    __asm__ __volatile__("vmove.xyz $vf28, $vf21");
    __asm__ __volatile__("vmove.xyz $vf29, $vf22");
    __asm__ __volatile__("vmulax.xyz $ACC, $vf24, $vf23x");
    __asm__ __volatile__("vmadday.xyz $ACC, $vf25, $vf23y");
    __asm__ __volatile__("vmaddaz.xyz $ACC, $vf26, $vf23z");
    __asm__ __volatile__("vmaddw.xyz $vf30, $vf30, $vf0w");
}

/* Translate the current matrix, scaling each axis by its row scale */
void CUR_MATRIX_Txyz4s(void *pVec)
{
    __asm__ __volatile__("lqc2 $vf4, 0x0(%0)" : : "r"(pVec));
    __asm__ __volatile__("vmulx.w $vf20, $vf27, $vf4x");
    __asm__ __volatile__("vmuly.w $vf21, $vf28, $vf4y");
    __asm__ __volatile__("vmulz.w $vf22, $vf29, $vf4z");
    __asm__ __volatile__("vmulaw.xyz $ACC, $vf27, $vf20w");
    __asm__ __volatile__("vmaddaw.xyz $ACC, $vf28, $vf21w");
    __asm__ __volatile__("vmaddaw.xyz $ACC, $vf29, $vf22w");
    __asm__ __volatile__("vmaddw.xyz $vf30, $vf30, $vf0w");
}

/* Rotate the current matrix about X by a fixed-point angle */
void CUR_MATRIX_Rx4s(int *pAngle)
{
    int nAngle;

    nAngle = pAngle[0];
    __asm__ __volatile__("qmtc2.ni %0, $vf4" : : "r"(nAngle));
    __asm__ __volatile__("vcallms 0xE8");
    __asm__ __volatile__("vmove.x $vf20, $vf1");
    __asm__ __volatile__("qmtc2.ni %0, $vf4" : : "r"(nAngle));
    __asm__ __volatile__("vcallms 0x20");
    __asm__ __volatile__("vmulax.xyz $ACC, $vf28, $vf20x");
    __asm__ __volatile__("vmaddx.xyz $vf9, $vf29, $vf1x");
    __asm__ __volatile__("vmove.w $vf30, $vf0");
    __asm__ __volatile__("vmulax.xyz $ACC, $vf29, $vf20x");
    __asm__ __volatile__("vmsubx.xyz $vf29, $vf28, $vf1x");
    __asm__ __volatile__("vmove.xyz $vf28, $vf9");
}

/* Rotate the current matrix about Y by a fixed-point angle */
void CUR_MATRIX_Ry4s(int *pAngle)
{
    int nAngle;

    nAngle = pAngle[0];
    __asm__ __volatile__("qmtc2.ni %0, $vf4" : : "r"(nAngle));
    __asm__ __volatile__("vcallms 0xE8");
    __asm__ __volatile__("vmove.x $vf20, $vf1");
    __asm__ __volatile__("qmtc2.ni %0, $vf4" : : "r"(nAngle));
    __asm__ __volatile__("vcallms 0x20");
    __asm__ __volatile__("vmulax.xyz $ACC, $vf27, $vf20x");
    __asm__ __volatile__("vmsubx.xyz $vf9, $vf29, $vf1x");
    __asm__ __volatile__("vmove.w $vf30, $vf0");
    __asm__ __volatile__("vmulax.xyz $ACC, $vf29, $vf20x");
    __asm__ __volatile__("vmaddx.xyz $vf29, $vf27, $vf1x");
    __asm__ __volatile__("vmove.xyz $vf27, $vf9");
}

/* Rotate the current matrix about Z by a fixed-point angle */
void CUR_MATRIX_Rz4s(int *pAngle)
{
    int nAngle;

    nAngle = pAngle[0];
    __asm__ __volatile__("qmtc2.ni %0, $vf4" : : "r"(nAngle));
    __asm__ __volatile__("vcallms 0xE8");
    __asm__ __volatile__("vmove.x $vf22, $vf1");
    __asm__ __volatile__("qmtc2.ni %0, $vf4" : : "r"(nAngle));
    __asm__ __volatile__("vcallms 0x20");
    __asm__ __volatile__("vmulax.xyz $ACC, $vf27, $vf22x");
    __asm__ __volatile__("vmaddx.xyz $vf9, $vf28, $vf1x");
    __asm__ __volatile__("vmove.w $vf30, $vf0");
    __asm__ __volatile__("vmulax.xyz $ACC, $vf28, $vf22x");
    __asm__ __volatile__("vmsubx.xyz $vf28, $vf27, $vf1x");
    __asm__ __volatile__("vmove.xyz $vf27, $vf9");
}

/* Rotate the current matrix by Z, then Y, then X from an angle triple */
void CUR_MATRIX_Rzyx4s(int *pAngle)
{
    int nAngle;

    nAngle = pAngle[2];
    __asm__ __volatile__("qmtc2.ni %0, $vf4" : : "r"(nAngle));
    __asm__ __volatile__("vcallms 0xE8");
    __asm__ __volatile__("vmove.x $vf20, $vf1");
    __asm__ __volatile__("qmtc2.ni %0, $vf4" : : "r"(nAngle));
    __asm__ __volatile__("vcallms 0x20");
    __asm__ __volatile__("vaddx.y $vf20, $vf0, $vf1x");
    nAngle = pAngle[1];
    __asm__ __volatile__("qmtc2.ni %0, $vf4" : : "r"(nAngle));
    __asm__ __volatile__("vcallms 0xE8");
    __asm__ __volatile__("vmove.x $vf21, $vf1");
    __asm__ __volatile__("qmtc2.ni %0, $vf4" : : "r"(nAngle));
    __asm__ __volatile__("vcallms 0x20");
    __asm__ __volatile__("vaddx.y $vf21, $vf0, $vf1x");
    nAngle = pAngle[0];
    __asm__ __volatile__("qmtc2.ni %0, $vf4" : : "r"(nAngle));
    __asm__ __volatile__("vcallms 0xE8");
    __asm__ __volatile__("vmove.x $vf22, $vf1");
    __asm__ __volatile__("qmtc2.ni %0, $vf4" : : "r"(nAngle));
    __asm__ __volatile__("vcallms 0x20");
    __asm__ __volatile__("vmulax.xyz $ACC, $vf27, $vf22x");
    __asm__ __volatile__("vmaddx.xyz $vf9, $vf28, $vf1x");
    __asm__ __volatile__("vmulax.xyz $ACC, $vf28, $vf22x");
    __asm__ __volatile__("vmsubx.xyz $vf28, $vf27, $vf1x");
    __asm__ __volatile__("vmulax.xyz $ACC, $vf9, $vf21x");
    __asm__ __volatile__("vmsuby.xyz $vf8, $vf29, $vf21y");
    __asm__ __volatile__("vmulax.xyz $ACC, $vf29, $vf21x");
    __asm__ __volatile__("vmaddy.xyz $vf29, $vf9, $vf21y");
    __asm__ __volatile__("vmove.xyz $vf27, $vf8");
    __asm__ __volatile__("vmulax.xyz $ACC, $vf28, $vf20x");
    __asm__ __volatile__("vmaddy.xyz $vf9, $vf29, $vf20y");
    __asm__ __volatile__("vmove.w $vf30, $vf0");
    __asm__ __volatile__("vmulax.xyz $ACC, $vf29, $vf20x");
    __asm__ __volatile__("vmsuby.xyz $vf29, $vf28, $vf20y");
    __asm__ __volatile__("vmove.xyz $vf28, $vf9");
}

/* Scale each row of the current matrix by the matching component */
void CUR_MATRIX_Sxyz4s(void *pVec)
{
    __asm__ __volatile__("lqc2 $vf19, 0x0(%0)" : : "r"(pVec));
    __asm__ __volatile__("vmulx.w $vf27, $vf27, $vf19x");
    __asm__ __volatile__("vmuly.w $vf28, $vf28, $vf19y");
    __asm__ __volatile__("vmulz.w $vf29, $vf29, $vf19z");
}
