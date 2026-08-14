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

typedef struct {
    char pad00[0x28];
    unsigned short nButton;   /* 0x28 */
    unsigned short nPress;    /* 0x2A */
    char pad2C[0x68 - 0x2C];
} XGLPADDATA;

extern XGLPADDATA PadData[2];

typedef struct {
    unsigned short nUnk00;
    short nPsm;
    short nWidth;
    short nHeight;
    char pad08[0x54];        /* keeps the extern out of sdata */
} XGLRENDERPRE;

extern XGLRENDERPRE sRender;

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
/* TODO: VERIFIED FULL MATCH under -O2 -G8 -fno-strict-aliasing (as are
 * xglCameraTravelFocus and xglCameraScreenInit below -- all three
 * confirmed word-exact via ccpipe+masked_compare with that flag, and no
 * registered xglCamera.c function regresses under it). Needs
 * FILE_CFLAGS_OVERRIDE["xglCamera.c"] = "-O2 -G8 -fno-strict-aliasing"
 * in configure.py, which this agent was not permitted to edit. Do NOT
 * apply the flag globally: xglTask.c's registered matches regress. */
void xglCameraTravelReset(void *pCamera)
{
    static XGLCAMVEC sShootAngleA = {{ 0.0f, 0.0f, 0.0f, 1.0f }};
    static XGLCAMVEC sShootPlaceA = {{ 0.0f, 0.0f, 0.0f, 1.0f }};
    char *pBase = (char *)pCamera;

    *(int *)(pBase + 0x0C) = 0;
    __asm__ __volatile__(".set noreorder\n"
        "lq $2, 0x0(%1)\n sq $2, 0x0(%0)\n"
        ".set reorder" : : "r"(pBase + 0x30), "r"(&sShootAngleA)
        : "$2", "memory");
    __asm__ __volatile__(".set noreorder\n"
        "lq $2, 0x0(%1)\n sq $2, 0x0(%0)\n"
        ".set reorder" : : "r"(pBase + 0x60), "r"(&sShootPlaceA)
        : "$2", "memory");
}

/* Debug focus control: L/R tweak the focus distance, other buttons snap
 * the twist to +/- the fine-step constant */
void xglCameraTravelFocus(float *pFocus)
{
    if ((PadData[1].nPress & 0x20) != 0) {
        if (pFocus[0] < 9.0f) {
            pFocus[0] = pFocus[0] + 1.0f;
        }
    }
    if ((PadData[1].nPress & 0x80) != 0) {
        if (pFocus[0] > 1.0f) {
            pFocus[0] = pFocus[0] - 1.0f;
        }
    }
    if ((PadData[1].nButton & 8) != 0) {
        pFocus[3] = 0.001117010717f;
    }
    if ((PadData[1].nButton & 2) != 0) {
        pFocus[3] = -0.001117010717f;
    }
}

/* Reset a camera's screen block: clip range, port effect and the
 * screen-size floats mirrored from the render state */
void xglCameraScreenInit(void *pCamera)
{
    static XGLCAMVEC sClipRange = {{ 0.01f, 99072.0f, 0.0f, 0.0f }};
    static XGLCAMVEC sPortEffect = {{ 1.0f, 1.0f, 1.0f, 100.0f }};
    char *pScreen = (char *)pCamera + 0x10;

    *(int *)pScreen = 0xFFFFFF;
    *(int *)(pScreen + 4) = 0;
    __asm__ __volatile__(".set noreorder\n"
        "lq $2, 0x0(%1)\n sq $2, 0x0(%0)\n"
        ".set reorder" : : "r"((char *)pCamera + 0x20), "r"(&sClipRange)
        : "$2", "memory");
    __asm__ __volatile__(".set noreorder\n"
        "lq $2, 0x0(%1)\n sq $2, 0x0(%0)\n"
        ".set reorder" : : "r"((char *)pCamera + 0x30), "r"(&sPortEffect)
        : "$2", "memory");
    *(float *)(pScreen + 0x30) = sRender.nWidth;
    *(float *)(pScreen + 0x34) = sRender.nHeight;
    *(float *)(pScreen + 0x38) = sRender.nWidth;
    *(float *)(pScreen + 0x3C) = sRender.nHeight;
    *(int *)(pScreen + 0x50) = 0;
    *(int *)(pScreen + 0x54) = 0;
    *(float *)(pScreen + 0x58) = sRender.nWidth;
    *(float *)(pScreen + 0x5C) = sRender.nHeight;
}

extern void xglCameraTravelManual(void *pCamera);
extern void xglCameraTravelChase(void *pCamera);

/* TODO: VERIFIED FULL MATCH (164/164 bytes, masked_compare diffs == [])
 * under FILE_FIX_FLAGS["xglCamera.c"] = "--branch-likely
 * xglCameraTravelProc:3" (site found by 0..5 ccpipe sweep; only site 3
 * yields the original 50400008 beqzl). No FILE_FIX_FLAGS entry exists
 * for xglCamera.c, which this agent may not create -- flag request.
 * Levers already in-source: int (not void) return type suppresses the
 * sibcall `j` of xglCameraTravelManual back to the original jal+b; the
 * cross-jumped press-test tail is a literal goto into case 3; the
 * $3-pinned empty-asm passthrough gives both case blocks their own
 * in-place lui/addiu PadData materialization. */
/* Per-frame travel dispatch on the camera's travel mode: manual mode
 * (2) hands off to the pad-driven mover unless SELECT is held, chase
 * mode (3) follows the target unless SELECT debugging is active; in
 * both, SELECT+R3 rearms the travel state */
int xglCameraTravelProc(void *pCamera)
{
    XGLCAMERA *p = (XGLCAMERA *)pCamera;
    register XGLPADDATA *pPad __asm__("$3");

    switch (p->nUnk04) {
    case 0:
    case 1:
    case 4:
        break;
    case 2:
        __asm__("" : "=r"(pPad) : "0"(PadData));
        if ((pPad[1].nButton & 0x100) != 0) {
            goto press_check;
        }
        xglCameraTravelManual(pCamera);
        goto done;
    case 3:
        __asm__("" : "=r"(pPad) : "0"(PadData));
        if ((pPad[1].nButton & 0x100) != 0) {
press_check:
            if ((pPad[1].nPress & 0x10) == 0) {
                break;
            }
            xglCameraTravelInit(pCamera);
        } else {
            xglCameraTravelChase(pCamera);
        }
        break;
    }
done:;
}
