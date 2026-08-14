/* xgl camera system: view/travel setup for the 3D camera rig */

typedef int TI __attribute__((mode(TI)));

typedef union {
    float f[4];
    TI q;
} XGLCAMVEC;

typedef struct {
    int nUnk00;
    int nUnk04;
    char pad08[4];     /* 0x08 */
    int nUnk0C;        /* 0x0C */
    char pad10[0x10];
    float fClipNear;   /* 0x20 */
    float fClipFar;    /* 0x24 */
    char pad28[8];
    XGLCAMVEC vShootAngle;  /* 0x30 */
    char pad40[0x20];
    XGLCAMVEC vShootPlace;  /* 0x60 */
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

/* Reset the travel state: clear the mode word and restore the default
 * shoot angle/place vectors */
/* TODO: near-miss. Body shape is right (TI copies, first dest staged
 * through a scratch reg, second through the reassigned parameter) but
 * (a) our gas steals the last sq into the jr delay slot where the
 * original keeps sq-before-jr + nop -- needs
 * FILE_FIX_FLAGS["xglCamera.c"] = "--barrier-return-store
 * xglCameraTravelReset"; (b) the second dest folds back to 96($a0)
 * instead of the original's addiu $a0,$a0,96, and dest/symbol register
 * pairs are swapped. */
void xglCameraTravelReset(void *pCamera)
{
    static XGLCAMVEC sShootAngleA = {{ 0.0f, 0.0f, 0.0f, 1.0f }};
    static XGLCAMVEC sShootPlaceA = {{ 0.0f, 0.0f, 0.0f, 1.0f }};
    char *pBase = (char *)pCamera;
    TI *pAngle = (TI *)(pBase + 0x30);

    *(int *)(pBase + 0x0C) = 0;
    pCamera = pBase + 0x60;
    *pAngle = sShootAngleA.q;
    *(TI *)pCamera = sShootPlaceA.q;
}
