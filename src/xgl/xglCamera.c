#include "matching.h"

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
extern void xglVectorScaleXYZ(float *pDest, const float *pSource, float fScale);
extern unsigned short D_00490E20;

/* TODO: near-miss (word count now matches at 60/60; ~13 real diffs
 * remain, mostly SCHEDULING). Structure/offsets/constants are verified:
 * 0x30=vShootAngle, 0x60=vShootPlace, 0xC=scale field, 0x0=X field;
 * D_004D8868/004D886C are the .lit4 constants 0.08f/0.24f (write
 * inline, don't extern them). Two things took real work to reproduce:
 * (1) the calls only pass `factor` (or `factor2`) as fScale -- the
 * *pScale = *pScale * factor store is a SEPARATE side effect on the
 * scale field, not fed into the call; (2) an empty asm memory barrier
 * in the else-arm (not both arms) is needed to stop gcc from merging
 * the if/else branches' identical `x = *(float*)pBase` loads into one
 * post-join load, which is what the original's redundant
 * `lwc1 $f12,0x0($s0)` twice (once in the branch-taken `b`'s delay
 * slot, once at the join label) actually is. Residual diffs: the
 * leading `lui $v1,%hi(D_00490E20)` sits BEFORE the stack frame setup
 * in the original (i.e. before `move s0,a0` even) -- no C-level
 * reordering moved it earlier, this may need a $v1 pin or is a
 * genuine 2.96 prologue-scheduling artifact; and the second factor's
 * `x - 5.0f` computes into $f12 in the original but $f20 here (swap
 * likely needs `factor2` recomputed via a *pAngle-styled pinned temp
 * rather than a plain local -- not yet tried). */
/* When the travel-rig "wide" mode bit is set, quadruple the shoot-angle
 * scale and the angle/place vectors; then rescale both vectors by a
 * focus-distance-dependent factor derived from the camera's X field */
void xglCameraTravelScale(void *pCamera)
{
    int flag;
    char *pBase;
    float *pAngle, *pPlace;
    float x;

    flag = D_00490E20 & 0x40;
    pBase = (char *)pCamera;
    pAngle = (float *)(pBase + 0x30);
    pPlace = (float *)(pBase + 0x60);

    if (flag != 0) {
        *(float *)(pBase + 0xC) = *(float *)(pBase + 0xC) * 4.0f;
        xglVectorScaleXYZ(pAngle, pAngle, 4.0f);
        xglVectorScaleXYZ(pPlace, pPlace, 4.0f);
        x = *(float *)pBase;
    } else {
        __asm__ __volatile__("" : : : "memory");
        x = *(float *)pBase;
    }
    {
        float factor = (x - 5.0f) * 0.08f + 1.0f;
        *(float *)(pBase + 0xC) = *(float *)(pBase + 0xC) * factor;
        xglVectorScaleXYZ(pAngle, pAngle, factor);
    }
    x = *(float *)pBase;
    {
        float factor2 = (x - 5.0f) * 0.24f + 1.0f;
        xglVectorScaleXYZ(pPlace, pPlace, factor2);
    }
}

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
    PIN(XGLPADDATA *pPad, "$3");

    switch (p->nUnk04) {
    case 0:
    case 1:
    case 4:
        break;
    case 2:
        PASSTHRU(pPad, PadData);
        if ((pPad[1].nButton & 0x100) != 0) {
            goto press_check;
        }
        xglCameraTravelManual(pCamera);
        goto done;
    case 3:
        PASSTHRU(pPad, PadData);
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

/* TODO: near-miss (53/54 words). TI-cast copies give the lq/sq pairs
 * (plain XGLCAMVEC assignment degrades to ld/sd), per-copy src/dst
 * empty-asm passthroughs stop gcc addressing the adjacent statics as
 * one-symbol+16*n and folding the sq destination offsets. Residue: one
 * word short and the head scheduling (orig hoists the first static's
 * lui/addiu above the f0/f1 stores); register alternation a2/v1/a1
 * unpinned. Data verified from 0x4A8C40. */
/* Reset the full travel block: focus scalars, the default rig vectors,
 * the manual-mode mirrors of the twist/height fields, and the chase
 * reference/interpolation positions */
void xglCameraTravelInit(void *pCamera)
{
    static XGLCAMVEC sShootAngleR = {{ 0.0f, 0.0f, 0.0f, 1.0f }};
    static XGLCAMVEC sShootAngleV = {{ 0.0f, 0.0f, 0.0f, 1.0f }};
    static XGLCAMVEC sShootAngleA2 = {{ 0.0f, 0.0f, 0.0f, 1.0f }};
    static XGLCAMVEC sShootPlaceR = {{ 0.0f, 1.0f, 4.0f, 1.0f }};
    static XGLCAMVEC sShootPlaceV = {{ 0.0f, 0.0f, 0.0f, 1.0f }};
    static XGLCAMVEC sShootPlaceA2 = {{ 0.0f, 0.0f, 0.0f, 1.0f }};
    static XGLCAMVEC sChaseRefPos = {{ 0.0f, 1.0f, 0.0f, 1.0f }};
    static XGLCAMVEC sChaseIntPos = {{ 0.0f, 1.0f, 0.0f, 1.0f }};
    char *pBase = (char *)pCamera;
    PIN(float *pTravel, "$7");
    XGLCAMVEC *pSrc;
    char *pDst;

    __asm__("" : "=r"(pTravel) : "0"((float *)(pBase + 0x90)));
    pTravel[0] = 5.0f;
    pTravel[1] = 0.69813174f;
    ((int *)pTravel)[2] = 0;
    ((int *)pTravel)[3] = 0;
    PASSTHRU(pSrc, &sShootAngleR);
    PASSTHRU(pDst, pBase + 0xA0);
    *(TI *)pDst = *(TI *)pSrc;
    PASSTHRU(pSrc, &sShootAngleV);
    PASSTHRU(pDst, pBase + 0xB0);
    *(TI *)pDst = *(TI *)pSrc;
    PASSTHRU(pSrc, &sShootAngleA2);
    PASSTHRU(pDst, pBase + 0xC0);
    *(TI *)pDst = *(TI *)pSrc;
    PASSTHRU(pSrc, &sShootPlaceR);
    PASSTHRU(pDst, pBase + 0xD0);
    *(TI *)pDst = *(TI *)pSrc;
    PASSTHRU(pSrc, &sShootPlaceV);
    PASSTHRU(pDst, pBase + 0xE0);
    *(TI *)pDst = *(TI *)pSrc;
    PASSTHRU(pSrc, &sShootPlaceA2);
    PASSTHRU(pDst, pBase + 0xF0);
    *(TI *)pDst = *(TI *)pSrc;
    pTravel[28] = pTravel[5];
    pTravel[29] = pTravel[18];
    PASSTHRU(pSrc, &sChaseRefPos);
    PASSTHRU(pDst, pBase + 0x110);
    *(TI *)pDst = *(TI *)pSrc;
    PASSTHRU(pSrc, &sChaseIntPos);
    PASSTHRU(pDst, pBase + 0x120);
    *(TI *)pDst = *(TI *)pSrc;
}

/* TODO: near-miss (12 words). Natural C matches the whole shape except
 * (a) the four upper-clamp slt/movz pairs come out in x0-first order
 * against the original's x1-first (source reordering shifts which cond
 * lands in $a2 but not the emission order), and (b) $5-$8-pinned cond
 * passthroughs fix the cond registers but then gcc rewrites the entry
 * corner-swaps as conditional moves from the original args (shorter by
 * two words). Pinning x0/y0/x1/y1 to $10/$11/$12/$9 keeps the arg
 * copies but loses the 3-move rotate. */
/* Set a camera's clip window: order the corners, clamp them to the
 * current display size and store them as floats in the screen block */
void xglCameraSetWindow(void *pCamera, int nX0, int nY0, int nX1, int nY1)
{
    char *pScreen;
    int n;

    if (pCamera == 0) {
        return;
    }
    pScreen = (char *)pCamera + 0x10;
    if (nX1 < nX0) {
        n = nX1;
        nX1 = nX0;
        nX0 = n;
    }
    if (nY1 < nY0) {
        n = nY1;
        nY1 = nY0;
        nY0 = n;
    }
    if (nX0 < 0) {
        nX0 = 0;
    }
    if (nY0 < 0) {
        nY0 = 0;
    }
    if (nX1 < 0) {
        nX1 = 0;
    }
    if (nY1 < 0) {
        nY1 = 0;
    }
    if (!(nX1 < sRender.nWidth)) {
        nX1 = sRender.nWidth - 1;
    }
    if (!(nY1 < sRender.nHeight)) {
        nY1 = sRender.nHeight - 1;
    }
    if (!(nX0 < sRender.nWidth)) {
        nX0 = sRender.nWidth - 1;
    }
    if (!(nY0 < sRender.nHeight)) {
        nY0 = sRender.nHeight - 1;
    }
    *(float *)(pScreen + 0x50) = nX0;
    *(float *)(pScreen + 0x54) = nY0;
    *(float *)(pScreen + 0x58) = nX1;
    *(float *)(pScreen + 0x5C) = nY1;
}
