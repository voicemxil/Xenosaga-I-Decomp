/* tskUmnBgCube - ov02: the spinning cube behind the Umn menus.
 *
 * Its own translation unit because the rotation step adds 0.001f, whose
 * `li.s' macro only expands to the original's inline lui/ori/mtc1 when
 * the ASSEMBLER also sees -G0 (otherwise gas pools it in .lit4 and loads
 * it through $gp). tsk.c's other tasks reach FileWork/SeisanWork through
 * $gp and stop matching the moment gas loses its small-data threshold,
 * so the two cannot share a file. See configure.py FILE_ASFLAGS_OVERRIDE. */

typedef struct XGL_TASK {
    struct XGL_TASK *field0;
    struct XGL_TASK *field4;
    struct XGL_TASK *field8;
    void (*fieldC)(void);
} XGL_TASK;

typedef struct {
    char pad000[1];
    unsigned char nAbort;               /* 0x01 */
    char pad002[0x88 - 2];
} UMN_WORK;

extern UMN_WORK UmnWork;
extern void *xglTaskWaitRemove(XGL_TASK *node);

/* --- ov02: the spinning cube behind the Umn menus --- */

/* Eight-byte alignment on the vector is what picks the `ld'/`sd' copy
 * idiom over `lw'/`sw' for the whole-vector assignments below. */
typedef union {
    float f[4];
    long long ll[2];
} VEC;

typedef struct {
    VEC vPos;                       /* 0x00 */
    VEC vRot;                       /* 0x10 */
    VEC vSpin;                      /* 0x20 */
    VEC vScale;                     /* 0x30 */
    float fMatrix[16];              /* 0x40 */
} CUBEWORK;

typedef struct {
    char pad000[0x10];              /* 0x00 */
    unsigned char nState;           /* 0x10 */
    unsigned char bReady;           /* 0x11 */
    unsigned char nCamera;          /* 0x12 */
    char pad013[1];                 /* 0x13 */
    int nTexture;                   /* 0x14 */
    int nModel;                     /* 0x18 */
    CUBEWORK *pWork;                /* 0x1C */
} TSK_BGCUBE;

typedef struct {
    char pad000[0x70];              /* 0x00 */
    float fColor[16];               /* 0x70 */
    float fMatrix[16];              /* 0xB0 */
} XGLLIGHT;

typedef struct {
    VEC vAmbient;                   /* 0x00 */
    VEC vColor[3];                  /* 0x10 */
    VEC vDir[3];                    /* 0x40 */
} LIGHTSET;

typedef struct { int nDirty; } CUBECAMERA;

static VEC BgCubeOne = { { 1.0f, 1.0f, 1.0f, 1.0f } };
static LIGHTSET BgCubeLight = {
    { { 1.0f, 1.0f, 1.0f, 1.0f } },
    { { { 0.8f, 0.8f, 0.8f, 1.0f } },
      { { 0.8f, 0.8f, 0.8f, 1.0f } },
      { { 0.8f, 0.8f, 0.8f, 1.0f } } },
    { { { 1.0f, 1.0f, 0.5f, 1.0f } },
      { { -1.0f, 1.0f, 0.0f, 1.0f } },
      { { 0.0f, 1.0f, -1.0f, 1.0f } } }
};

extern void *memset(void *pDst, int nVal, unsigned int nSize);
extern float xglFRand(void);
extern CUBECAMERA *xglStudioGetCamera2(int nNo);
extern void xglMatrixStackUnit(void);
extern void xglMatrixStackScale(VEC *pScale);
extern void xglMatrixStackTrans(VEC *pTrans);
extern void xglMatrixStackRotX(float fAngle);
extern void xglMatrixStackRotY(float fAngle);
extern void xglMatrixStackRotZ(float fAngle);
extern void xglMatrixStackSave(float *pMatrix);
extern void xglLightIntensityAmbient(XGLLIGHT *pLight, float *pColor);
extern void xglLightIntensityParallel(XGLLIGHT *pLight, int nNo, float *pColor);
extern void xglLightDirection(XGLLIGHT *pLight, int nNo, float *pDir);
extern void xglLightCalcMatrix(XGLLIGHT *pLight);
extern void nmlModelSetLight(float *pMatrix, float *pColor);
extern void nmlModelSetTexture(int nTexture);
extern void nmlModelSetPlace(float *pMatrix);
extern void nmlModelEntry(int nModel);

/* Per-frame driver for the background cube: state 0 seeds the cube's
 * orientation and spin from xglFRand and falls straight into state 1,
 * which advances the rotation a thousandth of a radian per axis per
 * frame (written 0.0010000001f: the constant reaches the assembler as the
 * decimal ee-gcc prints for it, and both ends fold TOWARD ZERO, so a
 * plain 0.001f arrives one ulp low as 0x3A83126E); state 99 (armed by UmnWork's abort flag) tears the task down.
 *
 * The four vectors of the work object each keep their own pointer in a
 * callee-saved register, so they are locals here rather than repeated
 * `w->' member references. */
void tskUmnBgCubeMain(TSK_BGCUBE *pTask)
{
    VEC vZero;
    VEC vOne;
    XGLLIGHT light;
    LIGHTSET ls;
    CUBEWORK *w;
    VEC *pRot;
    VEC *pSpin;
    VEC *pScale;
    float *pMatrix;
    CUBECAMERA *pCam;
    int i;

    w = pTask->pWork;
    pRot = &w->vRot;
    pSpin = &w->vSpin;
    pScale = &w->vScale;
    pMatrix = w->fMatrix;
    if (UmnWork.nAbort == 0xFF) {
        pTask->nState = 99;
    }
    switch (pTask->nState) {
    case 0:
        pTask->nState = 1;
        pTask->bReady = 1;
        memset(&vZero, 0, sizeof(vZero));
        vZero.f[3] = 1.0f;
        vOne = BgCubeOne;
        w->vPos = vZero;
        *pRot = vZero;
        *pSpin = vZero;
        *pScale = vOne;
        pRot->f[0] = xglFRand();
        pRot->f[1] = xglFRand();
        pRot->f[2] = xglFRand();
        pSpin->f[0] = xglFRand();
        pSpin->f[1] = xglFRand();
        pSpin->f[2] = xglFRand();
        pScale->f[0] = pScale->f[1] = pScale->f[2] = 7.5f;
        /* fall through */
    case 1:
        for (i = 0; i < 3; i++) {
            pRot->f[i] += 0.0010000001f;
        }
        break;
    case 99:
        xglTaskWaitRemove((XGL_TASK *)pTask);
        return;
    }
    if (pTask->bReady == 0) {
        return;
    }
    pCam = xglStudioGetCamera2(pTask->nCamera);
    pCam->nDirty = 1;
    xglMatrixStackUnit();
    xglMatrixStackScale(pScale);
    xglMatrixStackTrans(&w->vPos);
    xglMatrixStackRotZ(pRot->f[2]);
    xglMatrixStackRotY(pRot->f[1]);
    xglMatrixStackRotX(pRot->f[0]);
    xglMatrixStackSave(pMatrix);
    ls = BgCubeLight;
    xglLightIntensityAmbient(&light, ls.vAmbient.f);
    xglLightIntensityParallel(&light, 0, ls.vColor[0].f);
    xglLightDirection(&light, 0, ls.vDir[0].f);
    xglLightIntensityParallel(&light, 1, ls.vColor[1].f);
    xglLightDirection(&light, 1, ls.vDir[1].f);
    xglLightIntensityParallel(&light, 2, ls.vColor[2].f);
    xglLightDirection(&light, 2, ls.vDir[2].f);
    xglLightCalcMatrix(&light);
    nmlModelSetLight(light.fMatrix, light.fColor);
    nmlModelSetTexture(pTask->nTexture);
    nmlModelSetPlace(pMatrix);
    nmlModelEntry(pTask->nModel);
}
