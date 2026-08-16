/* Timeline camera helpers.  A timeline camera owns four animation channels
 * and drives the active XGL camera's transform fields. */

typedef struct {
    char pad000[0x94];
    float fov;                 /* 0x094 */
    char pad098[0x8];
    float rotate[3];           /* 0x0A0 */
    char pad0AC[0x24];
    float translate[3];        /* 0x0D0 */
} TCAMERA_XGL_CAMERA;

typedef struct {
    char pad000[0xC];
    int aChannel[4];           /* 0x00C: the four animation channel modes */
    char pad01C[0x12C0 - 0x1C];
} TCAMERA_WORK;

extern TCAMERA_WORK tcamera[];

void SPL_getValueXYZ(float *pValue);
float SPL_getValue(void *pSpline, int nComponent);

TCAMERA_WORK *TCAMERA_get(int nCamera)
{
    return &tcamera[nCamera];
}

/* The MPack view channel is unused in the final game. */
void TCAMERA_viewMPack(void)
{
}

void TCAMERA_setTranslate(TCAMERA_XGL_CAMERA *pCamera,
                          float x, float y, float z)
{
    float *pTranslate;

    pCamera = (TCAMERA_XGL_CAMERA *)((char *)pCamera + 0xD0);
    pTranslate = (float *)pCamera;
    pTranslate[0] = x;
    pTranslate[1] = y;
    pTranslate[2] = z;
}

void TCAMERA_setFov(TCAMERA_XGL_CAMERA *pCamera, float degrees)
{
    pCamera->fov = degrees / 180.0f * 3.141592741f;
}

void TCAMERA_setRoll(TCAMERA_XGL_CAMERA *pCamera, float degrees)
{
    pCamera->rotate[2] = degrees / 180.0f * 3.141592741f;
}

void TCAMERA_setRotate(TCAMERA_XGL_CAMERA *pCamera,
                       float x, float y, float z)
{
    float *pRotate;

    pCamera = (TCAMERA_XGL_CAMERA *)((char *)pCamera + 0xA0);
    pRotate = (float *)pCamera;
    pRotate[0] = x / 180.0f * 3.141592741f;
    pRotate[1] = y / 180.0f * 3.141592741f;
    pRotate[2] = z / 180.0f * 3.141592741f;
}

void TCAMERA_transSPL(TCAMERA_XGL_CAMERA *pCamera)
{
    SPL_getValueXYZ(pCamera->translate);
}

void TCAMERA_rotateSPL(TCAMERA_XGL_CAMERA *pCamera)
{
    float value[4];
    float *pRotate = pCamera->rotate;

    SPL_getValueXYZ(value);
    pRotate[0] = value[0] / 180.0f * 3.141592741f;
    pRotate[1] = value[1] / 180.0f * 3.141592741f;
    pRotate[2] = value[2] / 180.0f * 3.141592741f;
}

void TCAMERA_rollSPL(TCAMERA_XGL_CAMERA *pCamera, void *pSpline)
{
    float *pRotate;

    pCamera = (TCAMERA_XGL_CAMERA *)((char *)pCamera + 0xA0);
    pRotate = (float *)pCamera;
    pRotate[2] = SPL_getValue(pSpline, 0) / 180.0f * 3.141592741f;
}

void TCAMERA_fovSPL(TCAMERA_XGL_CAMERA *pCamera, void *pSpline)
{
    pCamera->fov = SPL_getValue(pSpline, 0) / 180.0f * 3.141592741f;
}

/* This hook was retained by the camera API but is empty in the shipped game. */
void TCAMERA_setFCurve(void)
{
}

/* 128-bit quantities move as a unit through lq/sq on the EE. */
typedef int TCAMERA_QUAD __attribute__((mode(TI)));

/* The interest point the MPack (pre-baked motion) camera path aims at. */
extern TCAMERA_QUAD mpack_interest;

void TCAMERA_mpackGetInterest(TCAMERA_QUAD *pInterest)
{
    *pInterest = mpack_interest;
}

/* Reset every timeline camera's four animation channels to mode 16. */
void TCAMERA_init(void)
{
    TCAMERA_WORK *pCam;
    int i;
    int j;

    pCam = tcamera;
    for (i = 0; i < 8; i++) {
        for (j = 3; j >= 0; j--) {
            pCam->aChannel[j] = 16;
        }
        pCam++;
    }
}
