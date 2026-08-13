/* Small draw-command wrappers. */

extern int D_003626B0[];
extern void sceVif1PkAddDirectDataN(void *packet, void *data, int count);
extern float D_0099C800[];
extern void xglMatrixStackUnit(void);
extern void xglMatrixStackTrans(float *translation);
extern void xglMatrixStackSave(float *matrix);
extern void drawAxis(float *matrix, float scale);

void DrawShadow(void *draw)
{
    sceVif1PkAddDirectDataN(*(void **)draw, D_003626B0, 7);
}

void drawCursor(void)
{
    float matrix[16];

    D_0099C800[3] = 1.0f;
    xglMatrixStackUnit();
    xglMatrixStackTrans(D_0099C800);
    xglMatrixStackSave(matrix);
    drawAxis(matrix, 1.0f);
}
