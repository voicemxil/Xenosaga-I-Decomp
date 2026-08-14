/* Studio (camera/light set) management for the xgl engine */

typedef struct {
    int nActive;            /* 0x000 */
    char pad4[0x5EC];       /* 0x004 */
} XGLCAMERA;

typedef struct {
    char pad0[0xF0];        /* 0x000 */
    XGLCAMERA aCamera[8];   /* 0x0F0 */
} XGLSTUDIO;

XGLSTUDIO asStudioSource[4];
XGLSTUDIO *pCurrentStudio;
int StudioEntrySkip;

XGLCAMERA *xglStudioGetCamera2(int nCamera);
XGLCAMERA *xglStudioSelectGetActiveCamera(int nStudio);
void xglSleep(void);
void xglLightInit(void *pLight);
void xglCameraInit(XGLCAMERA *pCamera);
void xglCameraMove(XGLCAMERA *pCamera);
void xglCameraTravelProc(XGLCAMERA *pCamera);

/* Make the given studio slot the current one */
void xglStudioChange(int nStudio)
{
    pCurrentStudio = &asStudioSource[nStudio];
}

/* Store the current studio's light block through the given pointer */
void xglStudioGetLight(void **ppLight)
{
    *ppLight = pCurrentStudio;
}

/* Store one of the current studio's cameras through the given pointer */
void xglStudioGetCamera(XGLCAMERA **ppCamera, int nCamera)
{
    *ppCamera = &pCurrentStudio->aCamera[nCamera];
}

/* Return the current studio's first camera that is switched on */
XGLCAMERA *xglStudioGetActiveCamera(void)
{
    XGLCAMERA *pCamera;
    int i;

    for (i = 0; i < 8; i++) {
        pCamera = xglStudioGetCamera2(i);
        if (pCamera->nActive != 0) {
            return pCamera;
        }
    }
    return 0;
}

/* Return the given studio's first camera that is switched on */
XGLCAMERA *xglStudioSelectGetActiveCamera(int nStudio)
{
    XGLCAMERA *pCamera;
    int i;

    pCamera = asStudioSource[nStudio].aCamera;
    for (i = 0; i < 8; i++) {
        if (pCamera->nActive != 0) {
            return pCamera;
        }
        pCamera++;
    }
    return 0;
}

/* Run the active camera of every studio through a movement update */
void xglStudioFlushActiveCamera(void)
{
    XGLCAMERA *pCamera;
    int i;

    for (i = 0; i < 4; i++) {
        pCamera = xglStudioSelectGetActiveCamera(i);
        if (pCamera != 0) {
            xglCameraMove(pCamera);
        }
    }
}

/* Return the current studio's light block */
void *xglStudioGetLight2(void)
{
    return pCurrentStudio;
}

/* Return one of the current studio's cameras */
XGLCAMERA *xglStudioGetCamera2(int nCamera)
{
    return &pCurrentStudio->aCamera[nCamera];
}

/* Switch camera 0 of every studio on and leave studio 0 selected */
void xglStudioMainCameraInit(void)
{
    XGLCAMERA *pCamera;
    int i;

    for (i = 0; i < 4; i++) {
        xglStudioChange(i);
        pCamera = xglStudioGetCamera2(0);
        pCamera->nActive = 1;
        ((int *)pCamera)[1] = 1;
    }
    xglStudioChange(0);
}

/* Select studio 0 and return its main camera */
XGLCAMERA *xglStudioMainCameraMove(void)
{
    xglStudioChange(0);
    return xglStudioGetCamera2(0);
}

/* Reset every studio's light and cameras */
void xglStudioInit(void)
{
    int i;
    int j;

    StudioEntrySkip = 0;
    for (j = 0; j < 4; j++) {
        xglStudioChange(j);
        xglLightInit(xglStudioGetLight2());
        for (i = 0; i < 8; i++) {
            xglCameraInit(xglStudioGetCamera2(i));
        }
    }
    xglStudioChange(0);
}

/* Update every switched-on camera of every studio */
void xglStudioMove(void)
{
    XGLCAMERA *pCamera;
    int i;
    int j;

    for (j = 0; j < 4; j++) {
        xglStudioChange(j);
        for (i = 0; i < 8; i++) {
            pCamera = xglStudioGetCamera2(i);
            if (pCamera->nActive == 1) {
                xglCameraTravelProc(pCamera);
                xglCameraMove(pCamera);
            }
        }
    }
    xglStudioChange(0);
}

/* Studio thread: initialise, then update the studios once per frame */
void xglStudioEntry(void)
{
    xglSleep();
    xglStudioInit();
    xglStudioMainCameraInit();
    while (1) {
        if (StudioEntrySkip == 0) {
            xglStudioMainCameraMove();
            xglStudioMove();
        }
        xglSleep();
    }
}
