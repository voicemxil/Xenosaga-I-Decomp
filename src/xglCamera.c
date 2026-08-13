/* xgl camera system: view/travel setup for the 3D camera rig */

typedef struct {
    int nUnk00;
    int nUnk04;
    char pad08[0x18];
    float fClipNear;   /* 0x20 */
    float fClipFar;    /* 0x24 */
} XGLCAMERA;

extern void xglCameraScreenInit(void *pCamera);
extern void xglCameraTravelInit(void *pCamera);
extern void xglCameraViewScreen(void *pCamera, void *pWindow);
extern void xglCameraViewTravel(void *pCamera, void *pWindow);
extern void xglCameraViewVolume(void *pCamera, void *pWindow);
extern void *memset(void *pDst, int nVal, unsigned int nSize);

/* Zero the control/mode fields of a camera */
void xglCameraControlInit(void *pCamera)
{
    XGLCAMERA *p = (XGLCAMERA *)pCamera;
    p->nUnk00 = 0;
    p->nUnk04 = 0;
}

/* Reset the near/far clip planes to their default range */
void xglCameraClipRangeDefault(void *pCamera)
{
    XGLCAMERA *p = (XGLCAMERA *)pCamera;
    p->fClipNear = 0.01f;
    p->fClipFar = 99000.0f;
}

/* Initialize control state, screen params and travel state for a camera */
void xglCameraInit(void *pCamera)
{
    xglCameraControlInit(pCamera);
    xglCameraScreenInit(pCamera);
    xglCameraTravelInit(pCamera);
}

/* Apply a window offset by feeding it through the screen/travel/volume updaters */
void xglCameraMoveOffset(void *pCamera, void *pWindow)
{
    xglCameraViewScreen(pCamera, pWindow);
    xglCameraViewTravel(pCamera, pWindow);
    xglCameraViewVolume(pCamera, pWindow);
}

/* Update the camera view using a zeroed (default) window */
void xglCameraMove(void *pCamera)
{
    int window[4];
    memset(window, 0, sizeof(window));
    xglCameraViewScreen(pCamera, window);
    xglCameraViewTravel(pCamera, window);
    xglCameraViewVolume(pCamera, window);
}
