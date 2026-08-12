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
    char data[0x12C0];
} TCAMERA_WORK;

extern TCAMERA_WORK tcamera[];

void SPL_getValueXYZ(float *pValue);

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

void TCAMERA_transSPL(TCAMERA_XGL_CAMERA *pCamera)
{
    SPL_getValueXYZ(pCamera->translate);
}

/* This hook was retained by the camera API but is empty in the shipped game. */
void TCAMERA_setFCurve(void)
{
}
