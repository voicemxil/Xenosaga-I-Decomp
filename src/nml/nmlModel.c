#include "matching.h"

/* Steering, in the same family as matching.h's LAUNDER/SCHED_NOP: an empty
 * volatile asm emits nothing at all, but it stops gcc 2.96's sibling-call
 * pass from rewriting a preceding `jal callee` + return into `j callee`.
 * The original build only sibcall-converted the switch arms that end in an
 * explicit jump to the epilogue, not the last arm that falls into it.
 * Vanishes in a portable build. */
#ifdef MATCHING
#define NO_SIBCALL() __asm__ __volatile__("")
#else
#define NO_SIBCALL() ((void) 0)
#endif

/* Normal-map model rendering global state accessors */

typedef int TI __attribute__((mode(TI)));
typedef union {
    float f[4];
    TI q;
} VEC4;

typedef union {
    int n;
    float f;
} FADE_COL;

typedef struct {
    FADE_COL uR;            /* 0x00 */
    FADE_COL uG;            /* 0x04 */
    FADE_COL uB;            /* 0x08 */
    int nUnkC;              /* 0x0C */
    int nTime;              /* 0x10 */
    int nTimeMax;           /* 0x14 */
    int nUnk18;             /* 0x18 */
    int nUnk1C;             /* 0x1C */
    int nUnk20;             /* 0x20 */
    int nDispose;           /* 0x24 */
    int nCancel;            /* 0x28 */
    int nUnk2C;             /* 0x2C */
    int nLock;              /* 0x30 */
    int pad[3];             /* 0x34 */
} FADE_CONTROL;

typedef struct {
    int nRequest;           /* 0x00 */
    int nUnk04;             /* 0x04 */
    int nUnk08;             /* 0x08 */
    int nUnk0C;             /* 0x0C */
    int nUnk10;             /* 0x10 */
    int nUnk14;             /* 0x14 */
    int nUnk18;             /* 0x18 */
    int nSignal;            /* 0x1C */
    int nUnk20;             /* 0x20 */
    int nCrossFadeTime;     /* 0x24 */
} BACK_BUFFER;

typedef struct {
    int nActive;            /* 0x00 */
    int pad[3];             /* 0x04 */
} MAP_HANDLE;

typedef struct {
    short nAlpha;           /* 0x00 */
    short nParts;           /* 0x02 */
} PIXEL_ALPHA;

/* One entry of the stealth-filter list sorted at s_inLayout+0x280 */
typedef struct {
    char pad00[0xC];
    float fDepth;           /* 0x0C */
    char pad10[0x10];
    float fRange;           /* 0x20 */
} STEALTH;

typedef struct {
    VEC4 aLightP[4];        /* 0x000 */
    VEC4 aLightC[4];        /* 0x040 */
    VEC4 inPlace[4];        /* 0x080 */
    VEC4 inPlaceInv[4];     /* 0x0C0 */
    VEC4 aPointP[4];        /* 0x100 */
    VEC4 aPointC[4];        /* 0x140 */
    VEC4 aShadowMtx[4];   /* 0x180 */
    VEC4 uFogCol1;           /* 0x1C0 */
    float aFogDist[20];     /* 0x1D0 */
    float fTransparency2;   /* 0x220 */
    float fReflTransparency;/* 0x224 */
    char pad228[0x8];
    union {
        float f;
        int n;
    } uAxis;                /* 0x230 */
    int nFilterFlag;        /* 0x234 */
    int nPixelAlpha;        /* 0x238 */
    void *pMatrix;          /* 0x23C */
    void *pTexture;         /* 0x240 */
    int nTexfunc;           /* 0x244 */
    long nAlpha;            /* 0x248 */
    int nStatus;            /* 0x250 */
    char pad254[0x14];
    union {
        int n;
        long l;
    } uTexMap;              /* 0x268 */
    int nTexMapAlpha;       /* 0x270 */
    float fTexMapZ;         /* 0x274 */
    char pad278[0x4];
    int nWindow;            /* 0x27C */
    STEALTH *aStealth[10];  /* 0x280 */
    int nShadowHeightOn;    /* 0x2A8 */
    float fShadowHeight;    /* 0x2AC */
    int nStatus2;           /* 0x2B0 */
    int nPartsPixelAlphaNum;/* 0x2B4 */
    char pad2B8[0x8];
    float fTransparency;    /* 0x2C0 */
    int pad2C4;
    int nStealthNum;        /* 0x2C8 */
    int pad2CC;
    union {
        PIXEL_ALPHA aParts[4];
        int aInt[4];
    } uPartsPixelAlpha;     /* 0x2D0 */
    char pad2E0[0x30];
    int aClip[4];           /* 0x310 */
} LAYOUT;

LAYOUT s_inLayout;
BACK_BUFFER s_inBackBuffer;
FADE_CONTROL s_inFadeOut;
FADE_CONTROL s_inFadeIn;
FADE_CONTROL s_inActiveFadeOut;
FADE_CONTROL s_inActiveFadeIn;
MAP_HANDLE s_inMapHandle;
float s_inGblFogPara[20];
int g_aSubWindow[4];
int s_aToumeiId[16];
float s_aToumei[16];
int s_aShapeId[32];
float s_aShapeWeight[32];
int s_aMapLast[12];
short s_aMapShadowParts[8];

VEC4 s_inShadowVec;
VEC4 s_inGblPos;
VEC4 s_inGblFogCol;
int s_nShadowVec;

void *s_pMapLast;
int s_nToumeiNum;
int s_nMapShadowParts;
float s_fSortOffsetEntry;
int s_nParent;
int s_nClip;
int s_nShapeNum;
int s_nDispVisible;
int s_nMapClip;
int s_nPacketSignal;
int s_nUseBackBuffer;
int s_nEffectWrite;
int s_nMapLast;
int s_nFadeDoit;
int s_nPause;
int s_nMenu;
int s_nFrameLockOff;
int s_nRenderCancelOld;
extern int s_nAlphaGroup;
extern int s_nNonAlphaGroup;
extern int s_nParentBuf;

void INIT_BACK_BUFFER(void);
void nmlModelFogPara(float *pPara, float fNear, float fFar, float fMin, float fMax);
void INIT_FADE_CONTROL(void *pFade);
void CLEAR_LAYOUT_MODEL(void *pLayout);
/* Componentwise sort of two vectors: pMin gets the elementwise minimum
 * and pMax the elementwise maximum (w untouched). */
void _MinMaxSort(void *pMin, void *pMax)
{
    PS2_ASM(".set noreorder\n"
            "lqc2 $vf20, 0x0(%0)\n"
            "lqc2 $vf21, 0x0(%1)\n"
            "vmini.xyz $vf22, $vf20, $vf21\n"
            "vmax.xyz $vf23, $vf20, $vf21\n"
            "sqc2 $vf22, 0x0(%0)\n"
            "sqc2 $vf23, 0x0(%1)\n"
            ".set reorder"
            : : "r"(pMin), "r"(pMax) : "memory");
}

void CLEAR_PROREAL(void *pProReal)
{
    *(int *)((char *)pProReal + 0x1C0) = 0;
}
void CLEAR_MAP_HANDLE(void *pMapHandle)
{
    *(int *)pMapHandle = 0;
}
extern char s_aMatName[];
extern char s_aTexName[];

/* Byte-exact.  The chained zero assignment makes gcc prepare the two
 * symbol addresses in the retail order while retaining separate stores. */
/* Reset the per-frame model entry state. */
void CLEAR_MODEL_ENTRY(void)
{
    s_nDispVisible = -1;
    s_aMatName[0] = s_aTexName[0] = 0;
    s_nMapShadowParts = 0;
    s_nShadowVec = 0;
    s_nToumeiNum = 0;
    s_fSortOffsetEntry = 0.0f;
    s_nParent = 0;
    s_nClip = 0;
    s_nShapeNum = 0;
    s_nMapClip = 0;
}
void FLUSH_MODELSYSTEM(void);
void FLUSH_ALPHA_GROUP(void)
{
    s_nAlphaGroup = 0;
    s_nNonAlphaGroup = 0;
}
void FLUSH_MAP_HANDLE(void *pMapHandle)
{
    *(int *)((char *)pMapHandle + 4) = 0;
}
void FLUSH_PARENT_BUF(void)
{
    s_nParentBuf = 0;
}
void FLUSH_BACK_BUFFER(void)
{
    s_inBackBuffer.nRequest = 0;
}

extern char s_inProReal[];

VEC4 s_inGblPointP[3];
VEC4 s_inGblPointC[4];

/* Reset the back-buffer request state */
void nmlModelSetBackBufferClear(void)
{
    INIT_BACK_BUFFER();
}

/* Flag the current model as a face model */
void nmlModelSetFaceModel(int flag)
{
    if (flag != 0) {
        s_inLayout.nStatus |= 0x100000;
    }
}

/* Flag the current model as a human model */
void nmlModelSetHumanModel(int flag)
{
    if (flag != 0) {
        s_inLayout.nStatus |= 0x40;
    }
}

/* Request map clipping */
void nmlModelSetMapClip(int flag)
{
    if (flag != 0) {
        s_nMapClip = 1;
    }
}

/* Request a render cancel (no-op) */
void nmlModelSendRenderCancel(void)
{
}

/* Return the previous render-cancel state */
int nmlModelGetRenderCancel(void)
{
    return s_nRenderCancelOld;
}

/* Request use of the back buffer */
void nmlModelSendBackBufferSignal(void)
{
    s_nUseBackBuffer = 1;
}

/* Return whether a back-buffer request is pending */
int nmlModelIsBackBufferRequest(void)
{
    return s_inBackBuffer.nSignal;
}

/* Set the pause state */
void nmlModelSendPauseSignal(int pause)
{
    s_nPause = pause;
}

/* Signal that the menu has opened */
void nmlModelSendMenuStart(void)
{
    s_nMenu = 1;
}

/* Signal that the menu has closed */
void nmlModelSendMenuEnd(void)
{
    s_nMenu = 0;
}

/* Return whether the menu is open */
int nmlModelGetMenuStatus(void)
{
    return s_nMenu;
}

/* Signal that a battle is starting */
void nmlModelSendSignalStartBattle(void)
{
    s_nFrameLockOff = 1;
}

/* Signal that an event has finished */
void nmlModelSendSignalEventFinish(void)
{
    s_nFrameLockOff = 1;
}

/* Clear the packet-change and back-buffer signals */
void nmlModelSendPacketChangeSignal(void)
{
    s_nPacketSignal = 0;
    s_inBackBuffer.nSignal = 0;
}

/* Reset the back buffer and all four fade controllers */
void nmlModelSendSignalMovieStart(void)
{
    INIT_BACK_BUFFER();
    INIT_FADE_CONTROL(&s_inFadeIn);
    INIT_FADE_CONTROL(&s_inFadeOut);
    INIT_FADE_CONTROL(&s_inActiveFadeIn);
    INIT_FADE_CONTROL(&s_inActiveFadeOut);
}

/* Set the MPEG2 cross-fade duration */
void nmlModelSetMpeg2CrossFadeTime(int time)
{
    s_inBackBuffer.nCrossFadeTime = time;
}

/* Add a render-level flag to the layout status2 word */
void nmlModelSetRenderLevel(int level)
{
    s_inLayout.nStatus2 |= level;
}

/* Lock fade-outs */
void nmlModelSetFadeOutLock(void)
{
    s_inFadeOut.nLock = 1;
}

/* Unlock fade-outs */
void nmlModelSetFadeOutLockOff(void)
{
    s_inFadeOut.nLock = 0;
}

/* Cancel the active fade-in after the given time, clamped to >= 0 */
void nmlModelSetActiveFadeInCancel(int time)
{
    if (time < 0) {
        time = 0;
    }
    s_inActiveFadeIn.nCancel = time;
}

/* Cancel the fade-in after the given time, clamped to >= 0 */
void nmlModelSetFadeInCancel(int time)
{
    if (time < 0) {
        time = 0;
    }
    s_inFadeIn.nCancel = time;
}

/* Cancel the active fade-out after the given time, clamped to >= 0 */
void nmlModelSetActiveFadeOutCancel(int time)
{
    if (time < 0) {
        time = 0;
    }
    s_inActiveFadeOut.nCancel = time;
}

/* Cancel the fade-out after the given time, clamped to >= 0 */
void nmlModelSetFadeOutCancel(int time)
{
    if (time < 0) {
        time = 0;
    }
    s_inFadeOut.nCancel = time;
}

/* Tick down the cancel timers of every fade controller */
void nmlModelFadeDoit(void)
{
    if (s_inFadeIn.nCancel != 0) {
        s_inFadeIn.nCancel--;
    }
    if (s_inFadeOut.nCancel != 0) {
        s_inFadeOut.nCancel--;
    }
    if (s_inActiveFadeIn.nCancel != 0) {
        s_inActiveFadeIn.nCancel--;
    }
    if (s_inActiveFadeOut.nCancel != 0) {
        s_inActiveFadeOut.nCancel--;
    }
}

/* Request fade processing */
void nmlModelSetFadeDoit(void)
{
    s_nFadeDoit = 1;
}

/* Mark the fade-out for disposal */
void nmlModelSetFadeOutDispose(void)
{
    s_inFadeOut.nDispose = 1;
}

/* Mark the fade-in for disposal */
void nmlModelSetFadeInDispose(void)
{
    s_inFadeIn.nDispose = 1;
}

/* Start a fade-in over the given time with the given color (x255 scale) */
/* gas's automatic delay-slot fill wants the final swc1 in the jr $ra delay
 * slot; the original issues it before the branch and leaves a genuine nop.
 * Held off by --barrier-return-store in this file's FILE_FIX_FLAGS. */
void nmlModelSetFadeInInterrupt(int time, float r, float g, float b)
{
    s_inFadeIn.nUnkC = 0;
    s_inFadeIn.nTimeMax = time;
    s_inFadeIn.nTime = time;
    s_inFadeIn.uR.n = (int)(r * 255.0f);
    s_inFadeIn.uG.n = (int)(g * 255.0f);
    s_inFadeIn.uB.n = (int)(b * 255.0f);
}

/* Return whether none of the four fade controllers is currently active */
int nmlModelGetFadeLevel(void)
{
    int level;

    level = 0;
    level = (s_inFadeIn.nTime < 0) ? level : 1;
    level = (s_inFadeOut.nTime < 0) ? level : 1;
    level = (s_inActiveFadeIn.nTime < 0) ? level : 1;
    level = (s_inActiveFadeOut.nTime < 0) ? level : 1;
    return level;
}

/* Copy the global position into pOut, forcing w = 1.0.  The lq/sq pair
 * is inline asm: compiled TI copies always hoist the lq above the two
 * address lui's; an asm block with the address as an operand pins the
 * original order (same trick for the other lq/sq copies below). */
void nmlModelGetGblPosition(void *pOut)
{
    __asm__ __volatile__("lq $2, 0x0(%1)\n sq $2, 0x0(%0)"
        : : "r"(pOut), "r"(&s_inGblPos) : "$2", "memory");
    ((float *)pOut)[3] = 1.0f;
}

/* Configure the back buffer for a battle transition */
void nmlModelSetBackBufferToBattle(int time)
{
    s_inBackBuffer.nUnk14 = 1;
    s_inBackBuffer.nRequest = 1;
    s_inBackBuffer.nUnk10 = time;
    s_inBackBuffer.nUnk04 = 0;
    s_inBackBuffer.nUnk20 = 0;
    s_inBackBuffer.nUnk18 = 0x28;
}

/* Configure the back buffer for an event skip */
void nmlModelSetBackBufferToEventSkip(int time)
{
    s_inBackBuffer.nUnk14 = 1;
    s_inBackBuffer.nRequest = 1;
    s_inBackBuffer.nUnk10 = time;
    s_inBackBuffer.nUnk04 = 0;
    s_inBackBuffer.nUnk20 = 0;
    s_inBackBuffer.nUnk18 = 0x28;
}

/* Configure the back buffer, forcing a minimum duration of one frame */
void nmlModelSetBackBuffer(int mode, int time, int inter, int frame)
{
    s_inBackBuffer.nUnk08 = mode;
    s_inBackBuffer.nUnk10 = inter;
    s_inBackBuffer.nUnk20 = frame;
    s_inBackBuffer.nUnk0C = time;
    s_inBackBuffer.nUnk14 = 0;
    s_inBackBuffer.nRequest = 0;
    s_inBackBuffer.nUnk04 = 0;
    if (time <= 0) {
        s_inBackBuffer.nUnk0C = 1;
    }
}

/* Set the effect-write flag */
void nmlModelSetEffectWrite(int flag)
{
    s_nEffectWrite = flag;
}

/* Enable or disable one of the sub-windows */
void nmlModelUseSubWindow(int no, int flag)
{
    if ((unsigned int)no < 4U) {
        g_aSubWindow[no] = flag;
    }
}

/* Select the active window */
void nmlModelSetWindow(int no)
{
    if ((unsigned int)no < 4U) {
        s_inLayout.nWindow = no;
    }
}

/* Set the sort offset applied at entry */
void nmlModelSetSortOffset(float offset)
{
    s_fSortOffsetEntry = offset;
}

/* Disable specular lighting for the current model */
void nmlModelSetSpecularOff(int flag)
{
    if (flag != 0) {
        s_inLayout.nStatus2 |= 0x2;
    }
}

/* Set the shadow-map id (no-op) */
void nmlModelSetShadowMapId(void)
{
}

/* Set the drop-shadow height */
void nmlModelSetShadowHeight(float height)
{
    s_inLayout.fShadowHeight = height;
    s_inLayout.nShadowHeightOn = 1;
}

/* Set the shadow direction vector */
void nmlModelSetShadowVec(void *vec)
{
    __asm__ __volatile__("lq $2, 0x0(%1)\n sq $2, 0x0(%0)"
        : : "r"(&s_inShadowVec), "r"(vec) : "$2", "memory");
    s_nShadowVec = 1;
}


/* Flag the current model as a proreal texture target */
void nmlModelSetTexProreal(void)
{
    s_inLayout.nStatus |= 0x800;
}

/* Set the axis-symmetry plane */
void nmlModelSetAxisSymmetry(float axis)
{
    s_inLayout.uAxis.f = axis * 0.0f;
    s_inLayout.uAxis.n |= 0x1;
}

/* Set the parent model id */
void nmlModelSetParent(int parent)
{
    s_nParent = parent;
}

/* Merge a render-status flag into the current model */
void nmlModelSetRenderStatus(int status)
{
    if (status == 0x100) {
        s_inLayout.nStatus |= 0x100;
    } else if (status == 0x200) {
        s_inLayout.nStatus |= 0x200;
    }
}

/* Register a shape id and weight for the current model */
void nmlModelSetShapeId(int id, float weight)
{
    if (s_nShapeNum < 32) {
        s_aShapeId[s_nShapeNum] = id;
        s_aShapeWeight[s_nShapeNum++] = weight;
    }
}

/* Register a translucent part for the current model */
void nmlModelSetToumeiParts(int parts, float toumei)
{
    if (s_nToumeiNum < 16) {
        s_aToumeiId[s_nToumeiNum] = parts;
        s_aToumei[s_nToumeiNum++] = toumei;
    }
}

/* Set the pixel alpha, clamped to [1, 127] */
void nmlModelSetPixelAlpha(int alpha)
{
    if (alpha <= 0) {
        alpha = 1;
    }
    alpha = (alpha < 0x80) ? alpha : 0x7F;
    s_inLayout.nPixelAlpha = alpha;
}

/* Register a per-part pixel alpha, clamped to [1, 127] */
void nmlModelSetPartsPixelAlpha(int parts, int alpha)
{
    if (alpha <= 0) {
        alpha = 1;
    }
    alpha = (alpha < 0x80) ? alpha : 0x7F;
    if (s_inLayout.nPartsPixelAlphaNum < 4) {
        s_inLayout.uPartsPixelAlpha.aParts[s_inLayout.nPartsPixelAlphaNum].nAlpha = alpha;
        s_inLayout.uPartsPixelAlpha.aParts[s_inLayout.nPartsPixelAlphaNum].nParts = parts;
        s_inLayout.nPartsPixelAlphaNum++;
    }
}

/* Set the clip mode */
void nmlModelSetClip(int clip)
{
    s_nClip = clip;
}

/* Set the alpha blend value */
void nmlModelSetAlpha(long alpha)
{
    s_inLayout.nAlpha = alpha;
}

/* Enable stencil writes for the current model */
void nmlModelSetStencil(int flag)
{
    if (flag != 0) {
        s_inLayout.nStatus |= 0x4;
    }
}

/* Enable Z writes for the current model */
void nmlModelSetZwrite(int flag)
{
    if (flag != 0) {
        s_inLayout.nStatus |= 0x8;
    }
}

/* Set the model transparency, clamped to [0, 1] */
void nmlModelSetTransparency(float trans)
{
    if (trans < 0.0f) {
        trans = 0.0f;
    }
    if (trans > 1.0f) {
        trans = 1.0f;
    }
    s_inLayout.fTransparency2 = trans;
    s_inLayout.fTransparency = trans;
}

/* Set the reflection transparency, clamped to [0, 1] */
void nmlModelSetReflTransparency(float trans)
{
    if (trans < 0.0f) {
        trans = 0.0f;
    }
    if (trans > 1.0f) {
        trans = 1.0f;
    }
    s_inLayout.fReflTransparency = trans;
}

/* Flag the current model as translucent */
void nmlModelSetToumei(int flag)
{
    if (flag != 0) {
        s_inLayout.nStatus |= 0x20;
    }
}

/* Set the current texture after validating its address and XTX header */
void nmlModelSetTexture(void *pTex)
{
    char *p = (char *)pTex;

    if (pTex != (void *)0 && (unsigned int)((int)pTex - 0x200000) <= 0x1DFFFFF && (((int)pTex & 0xF) == 0)) {
        if (p[0] == 'X' && p[1] == 'T' && p[2] == p[0]) {
            s_inLayout.pTexture = pTex;
        }
    }
}

/* Set the texture function */
void nmlModelSetTexfunc(int func)
{
    s_inLayout.nTexfunc = func;
}

/* Set the current model matrix */
void nmlModelSetMatrix(void *matrix)
{
    s_inLayout.pMatrix = matrix;
}

/* Copy a multiplier color into the layout (aFogDist overlay) */
void nmlModelSetMulColor(void *color)
{
    __asm__ __volatile__("lq $2, 0x0(%1)\n sq $2, 0x0(%0)\n nop"
        : : "r"(&s_inLayout.aFogDist[12]), "r"(color) : "$2", "memory");
}

/* Set the fog color for the current model */
void nmlModelSetFogCol(void *color)
{
    __asm__ __volatile__("lq $2, 0x0(%1)\n sq $2, 0x0(%0)"
        : : "r"(&s_inLayout.uFogCol1), "r"(color) : "$2", "memory");
    s_inLayout.nStatus |= 0x2;
}

/* Set the fog distance parameters for the current model */
void nmlModelSetFogDist(float fNear, float fFar, float fMin, float fMax)
{
    nmlModelFogPara(s_inLayout.aFogDist, fNear, fFar, fMin, fMax);
    s_inLayout.nStatus |= 0x2;
}

/* Set the global fog color */
void nmlModelSetGlobalFogCol(void *color)
{
    __asm__ __volatile__("lq $2, 0x0(%1)\n sq $2, 0x0(%0)"
        : : "r"(&s_inGblFogCol), "r"(color) : "$2", "memory");
    s_inLayout.nStatus |= 0x1000000;
}

/* Set the global fog distance parameters */
void nmlModelSetGlobalFogDist(float fNear, float fFar, float fMin, float fMax)
{
    nmlModelFogPara(s_inGblFogPara, fNear, fFar, fMin, fMax);
    s_inLayout.nStatus |= 0x1000000;
}

/* Clear the global fog flag */
void nmlModelSetGlobalFogReset(void)
{
    s_inLayout.nStatus &= ~0x1000000;
}

/* Cancel fog for the current model */
void nmlModelSetFogCancel(void)
{
    s_inLayout.nStatus |= 0x2000;
}

/* Register the current model as the map and flag the map handle */
void nmlModelSetMapEntry(void)
{
    s_inMapHandle.nActive = 1;
    s_inLayout.nStatus |= 0x10000;
}

/* Flag the current model as a shadow-map entry */
void nmlModelSetShadowMapEntry(void)
{
    s_inLayout.nStatus |= 0x10000;
}

/* Register a map part that receives shadows */
void nmlModelSetMapShadowParts(int parts)
{
    if (s_nMapShadowParts < 8) {
        s_aMapShadowParts[s_nMapShadowParts++] = parts;
    }
}

/* Register data to render after the map */
void nmlModelSetMapLastEntry(void *pData, int no)
{
    if (s_nMapLast < 12) {
        s_pMapLast = pData;
        s_aMapLast[s_nMapLast++] = no;
    }
}

/* Reset the map-last entry list */
void nmlModelSetMapLastInit(void)
{
    s_nMapLast = 0;
}

/* Clear the layout, proreal, map-handle and model-entry state */
void nmlModelClear(void)
{
    CLEAR_LAYOUT_MODEL(&s_inLayout);
    CLEAR_PROREAL(s_inProReal);
    CLEAR_MAP_HANDLE(&s_inMapHandle);
    CLEAR_MODEL_ENTRY();
}

/* Flush every render subsystem, then clear the model state */
void nmlModelFlushClear(void)
{
    FLUSH_MODELSYSTEM();
    FLUSH_ALPHA_GROUP();
    FLUSH_MAP_HANDLE(&s_inMapHandle);
    FLUSH_PARENT_BUF();
    FLUSH_BACK_BUFFER();
    nmlModelClear();
}

void CONSTRUCT_MODELSYSTEM(void);
void CONSTRUCT_CIRCLR_SHADOW(void);
void CONSTRUCT_BACK_BUFFER(void);
void CONSTRUCT_ALPHA_GROUP(void);
void CONSTRUCT_PARENT_BUF(void);
void CONSTRUCT_MAP_HANDLE(void *pMapHandle);
void CONSTRUCT_FADE_CONTROL(void *pFade);
extern int D_004A9430[];

/* Construct every model subsystem, then clear model state */
void nmlModelConstruct(void)
{
    CONSTRUCT_MODELSYSTEM();
    CONSTRUCT_CIRCLR_SHADOW();
    CONSTRUCT_BACK_BUFFER();
    CONSTRUCT_ALPHA_GROUP();
    CONSTRUCT_PARENT_BUF();
    CONSTRUCT_MAP_HANDLE(&s_inMapHandle);
    CONSTRUCT_FADE_CONTROL(&s_inFadeIn);
    CONSTRUCT_FADE_CONTROL(&s_inFadeOut);
    CONSTRUCT_FADE_CONTROL(&s_inActiveFadeIn);
    CONSTRUCT_FADE_CONTROL(&s_inActiveFadeOut);
    nmlModelClear();
    D_004A9430[0] = 0;
}

extern unsigned short D_0033868C[];
int SCRIPT_getCfTime(void);
int s_nUseStealth;

/* TODO: near-miss (25/35 words, LOGIC/register) - clamp logic, field
 * offsets (nFilterFlag@0x234, aFogDist[4]@0x1E0, aFogDist[5]@0x1E4) and
 * the stealth-timer gate are all verified against asm. Original caches
 * &s_inLayout in a single register ($v0) used throughout; every local
 * pointer / direct-field-access variant tried still splits it into two
 * registers (an extra `addiu v1,v0,0` copy) once the (flag&2) branch is
 * added. Length now matches (35 words); remaining diffs are pure
 * instruction/register shuffling within that block. */
void *xglStudioGetActiveCamera(void);
void _ModelCalcClipInit(void *pCamera);
int _ModelCalcClip(void *pPos);
int xglCullingCheck(void *p1, void *p2);

/* Transform pPos by pMatrix through the VU0 point-transform pipe, then
 * clip-test it against the active camera and the culling map; returns
 * 0 if there's no active camera */
int nmlModelCalcClipMat1(void *pPos, void *pMatrix)
{
    void *pCamera;
    VEC4 vTemp;
    int n1, n2;

    pCamera = xglStudioGetActiveCamera();
    if (pCamera != 0) {
        __asm__ __volatile__(".set noreorder\n"
            "lqc2 $vf31, 0x0(%1)\n"
            "lqc2 $vf27, 0x0(%2)\n lqc2 $vf28, 0x10(%2)\n"
            "lqc2 $vf29, 0x20(%2)\n lqc2 $vf30, 0x30(%2)\n"
            "vmulax.xyz $ACC, $vf27, $vf31x\n vmadday.xyz $ACC, $vf28, $vf31y\n"
            "vmaddaz.xyz $ACC, $vf29, $vf31z\n vmaddw.xyz $vf31, $vf30, $vf0w\n"
            "sqc2 $vf31, 0x0(%0)\n"
            ".set reorder" : : "r"(&vTemp), "r"(pPos), "r"(pMatrix));
        _ModelCalcClipInit((char *)pCamera + 0x4F0);
        n1 = _ModelCalcClip(&vTemp);
        n2 = xglCullingCheck(pCamera, &vTemp);
        return n1 | n2;
    }
    return (int)pCamera;
}

/* Transform pPos by pMatrix1 then pMatrix2 (chained) through the VU0
 * point-transform pipe, then clip-test against the active camera and
 * the culling map; returns 0 if there's no active camera */
int nmlModelCalcClipMat2(void *pPos, void *pMatrix1, void *pMatrix2)
{
    void *pCamera;
    VEC4 vTemp;
    int n1, n2;

    pCamera = xglStudioGetActiveCamera();
    if (pCamera != 0) {
        __asm__ __volatile__(".set noreorder\n"
            "lqc2 $vf31, 0x0(%1)\n"
            "lqc2 $vf27, 0x0(%2)\n lqc2 $vf28, 0x10(%2)\n"
            "lqc2 $vf29, 0x20(%2)\n lqc2 $vf30, 0x30(%2)\n"
            "vmulax.xyz $ACC, $vf27, $vf31x\n vmadday.xyz $ACC, $vf28, $vf31y\n"
            "vmaddaz.xyz $ACC, $vf29, $vf31z\n vmaddw.xyz $vf31, $vf30, $vf0w\n"
            "lqc2 $vf27, 0x0(%3)\n lqc2 $vf28, 0x10(%3)\n"
            "lqc2 $vf29, 0x20(%3)\n lqc2 $vf30, 0x30(%3)\n"
            "vmulax.xyz $ACC, $vf27, $vf31x\n vmadday.xyz $ACC, $vf28, $vf31y\n"
            "vmaddaz.xyz $ACC, $vf29, $vf31z\n vmaddw.xyz $vf31, $vf30, $vf0w\n"
            "sqc2 $vf31, 0x0(%0)\n"
            ".set reorder" : : "r"(&vTemp), "r"(pPos), "r"(pMatrix1), "r"(pMatrix2));
        _ModelCalcClipInit((char *)pCamera + 0x4F0);
        n1 = _ModelCalcClip(&vTemp);
        n2 = xglCullingCheck(pCamera, &vTemp);
        return n1 | n2;
    }
    return (int)pCamera;
}

void *xglStudioSelectGetActiveCamera(int nStudio);

/* Transform pPos by pMatrix and clip/cull-test it against every active
 * sub-window camera (skipping disabled windows); returns 0 as soon as
 * any camera shows it visible, or 1 if every camera clips/culls it (or
 * immediately 0 if a window has no camera at all) */
int nmlModelCalcClipMat1AllCam(void *pPos, void *pMatrix)
{
    int i;
    void *pCamera;
    VEC4 vTemp;
    int n1, n2;
    int nRet;

    nRet = 1;
    for (i = 0; i < 4; i++) {
        pCamera = xglStudioSelectGetActiveCamera(i);
        if (pCamera == 0) {
            return (int)pCamera;
        }
        if (g_aSubWindow[i] != 0) {
            __asm__ __volatile__(".set noreorder\n"
                "lqc2 $vf31, 0x0(%1)\n"
                "lqc2 $vf27, 0x0(%2)\n lqc2 $vf28, 0x10(%2)\n"
                "lqc2 $vf29, 0x20(%2)\n lqc2 $vf30, 0x30(%2)\n"
                "vmulax.xyz $ACC, $vf27, $vf31x\n vmadday.xyz $ACC, $vf28, $vf31y\n"
                "vmaddaz.xyz $ACC, $vf29, $vf31z\n vmaddw.xyz $vf31, $vf30, $vf0w\n"
                "sqc2 $vf31, 0x0(%0)\n"
                ".set reorder" : : "r"(&vTemp), "r"(pPos), "r"(pMatrix));
            _ModelCalcClipInit((char *)pCamera + 0x4F0);
            n1 = _ModelCalcClip(&vTemp);
            n2 = xglCullingCheck(pCamera, &vTemp);
            if ((n1 | n2) == 0) {
                nRet = 0;
                break;
            }
        }
    }
    return nRet;
}

/* Transform pPos by pMatrix1 then pMatrix2 and clip-test it (no culling
 * check) against sub-windows 0-2; returns a per-window clip bitmask,
 * with bit 0x80 additionally set when every checked window clipped it
 * (or immediately 0 if an enabled window has no camera) */
int nmlModelCalcClipMat2AllCam(void *pPos, void *pMatrix1, void *pMatrix2)
{
    int i;
    void *pCamera;
    VEC4 vTemp;
    int nResult;
    int nMask;
    int nCount;
    int nSum;

    nMask = 0;
    nCount = 0;
    nSum = 0;
    i = 0;
    do {
        if (g_aSubWindow[i] != 0) {
            pCamera = xglStudioSelectGetActiveCamera(i);
            if (pCamera == 0) {
                return (int)pCamera;
            }
            _ModelCalcClipInit((char *)pCamera + 0x4F0);
            __asm__ __volatile__(".set noreorder\n"
                "lqc2 $vf31, 0x0(%1)\n"
                "lqc2 $vf27, 0x0(%2)\n lqc2 $vf28, 0x10(%2)\n"
                "lqc2 $vf29, 0x20(%2)\n lqc2 $vf30, 0x30(%2)\n"
                "vmulax.xyz $ACC, $vf27, $vf31x\n vmadday.xyz $ACC, $vf28, $vf31y\n"
                "vmaddaz.xyz $ACC, $vf29, $vf31z\n vmaddw.xyz $vf31, $vf30, $vf0w\n"
                "lqc2 $vf27, 0x0(%3)\n lqc2 $vf28, 0x10(%3)\n"
                "lqc2 $vf29, 0x20(%3)\n lqc2 $vf30, 0x30(%3)\n"
                "vmulax.xyz $ACC, $vf27, $vf31x\n vmadday.xyz $ACC, $vf28, $vf31y\n"
                "vmaddaz.xyz $ACC, $vf29, $vf31z\n vmaddw.xyz $vf31, $vf30, $vf0w\n"
                "sqc2 $vf31, 0x0(%0)\n"
                ".set reorder" : : "r"(&vTemp), "r"(pPos), "r"(pMatrix1), "r"(pMatrix2));
            nResult = _ModelCalcClip(&vTemp);
            nCount++;
            nMask |= nResult << i;
            nSum += nResult;
        }
        i++;
    } while (i < 3);
    if (nCount != 0 && nSum == nCount) {
        nMask |= 0x80;
    }
    return nMask;
}

void nmlModelSetFilter(int flag, float a, float b)
{
    LAYOUT *p = &s_inLayout;

    p->nFilterFlag = flag;
    if (a < 0.0f) {
        a = 0.0f;
    }
    if (128.0f < a) {
        a = 128.0f;
    }
    p->aFogDist[4] = a;
    p->aFogDist[5] = b;
    if ((flag & 2) != 0) {
        /* Unconditional inside the flag test: the original puts this store
         * in the delay slot of the D_0033868C bne, which only works if it
         * runs on both arms. */
        s_nUseStealth = 1;
        if (D_0033868C[0] == 2) {
            if (SCRIPT_getCfTime() < 2) {
                s_nUseStealth = 0;
            }
        }
    }
}

/* --- Fade set/write helpers --- */

typedef struct {
    char pad00[0x58];
    int nOutTime;           /* 0x58 */
    int nInTime;            /* 0x5C */
    char pad60[0x30];
    FADE_COL afOutColor[4]; /* 0x90 */
    FADE_COL afInColor[4];  /* 0xA0 */
} FADE_PRESET;

extern FADE_PRESET D_00338680;
extern int D_004A9124[];
void fade_render(void *pPk, FADE_CONTROL *pFade);
void nmlFadePacketWrite(void *pPk);

/* Prime a fade controller: time, color (x255 scale) and control words,
 * then point the fade packet writer at nmlFadePacketWrite */
static void fade_set(FADE_CONTROL *pFade, FADE_COL *puCol, int nUnk18, int nTime,
                     int nUnk1C, int nUnk20, int nUnk2C)
{
    pFade->nTime = nTime;
    pFade->nTimeMax = nTime;
    pFade->uR.n = (int)(puCol[0].f * 255.0f);
    pFade->uG.n = (int)(puCol[1].f * 255.0f);
    pFade->uB.n = (int)(puCol[2].f * 255.0f);
    pFade->nUnk18 = nUnk18;
    pFade->nUnk1C = nUnk1C;
    pFade->nUnk20 = nUnk20;
    pFade->nUnk2C = nUnk2C;
    pFade->nUnkC = 0;
    pFade->nDispose = 0;
    if (nTime < 0) {
        pFade->nTime = -1;
    }
    D_004A9124[0] = (int)nmlFadePacketWrite;
}

/* Render all four fade controllers into the packet and clear the
 * fade-doit request */
void nmlFadePacketWrite(void *pPk)
{
    fade_render(pPk, &s_inActiveFadeIn);
    fade_render(pPk, &s_inActiveFadeOut);
    fade_render(pPk, &s_inFadeIn);
    fade_render(pPk, &s_inFadeOut);
    s_nFadeDoit = 0;
}

/* Start a fade-out over time+2 frames using the preset fade-out color */
void nmlModelSetFadeOut(int time, int n20)
{
    fade_set(&s_inFadeOut, D_00338680.afOutColor, D_00338680.nOutTime,
             time + 2, 0, n20, 0);
}

/* Start a fade-in using the preset fade-in color, then reset the presets */
/* TODO: near-miss (12 diffs, 33 orig vs 32 built) -- all stores now match
 * in kind (FPR zero via mtc1 after the FADE_COL union conversion); the
 * original leaves a genuine nop after the two mtc1's (gas mtc1->swc1
 * hazard nop?) and keeps the stores in pure source order, while our
 * schedule hoists the last two afOutColor stores into that slot. Same
 * hazard-nop class as nmlModelFogPara / nmlPacketAddGsFogCol. */
void nmlModelSetFadeIn(int time, int n20)
{
    float fZero;
    float f30;

    fade_set(&s_inFadeIn, D_00338680.afInColor, D_00338680.nInTime,
             time, 1, n20, 0);
    fZero = 0.0f;
    f30 = 30.0f;
    /* Statement order is load-bearing: sched2 hoists trailing stores
     * forward, and only this rotation of the store list reproduces the
     * original's emission order (rotation sweep 2026-08-14), together
     * with --mtc1-nop nmlModelSetFadeIn:1 for the ee-as COP1 stall pad. */
    D_00338680.nOutTime = 0;
    D_00338680.afInColor[2].f = fZero;
    D_00338680.afInColor[1].f = fZero;
    D_00338680.afInColor[0].f = fZero;
    D_00338680.afInColor[3].f = f30;
    D_00338680.nInTime = 0;
    D_00338680.afOutColor[2].f = fZero;
    D_00338680.afOutColor[1].f = fZero;
    D_00338680.afOutColor[0].f = fZero;
    D_00338680.afOutColor[3].f = f30;
}

/* --- Active (script) fades --- */

/* Both nmlModelSetActive* below MATCH. The only difference from stock
 * gcc output was padding in the three unsigned-int-to-float conversion
 * blocks: gas fills the `b` delay slot after each `cvt.s.w`, where the
 * original leaves it empty. Pinning a nop into those three slots per
 * function (--pin-slot-nop nmlModelSetActiveFadeOut:0,1,2 and the same
 * for FadeIn, in FILE_FIX_FLAGS) is the whole fix: the trailing pad nop
 * that also differed is emitted by gcc's own `.p2align 3,,7` after the
 * branch, so it appears by itself once the slot holds a nop.
 * Note the unsigned->float dance (bltz / andi 1 / srl 1 / or / add.s) is
 * gcc's UNSIGNED conversion: the colour argument must stay `unsigned
 * int` or the whole block collapses to a plain cvt.s.w. */

/* Start an active fade-out over time+2 frames from a packed 0x00BBGGRR
 * colour, unless one is already running */
void nmlModelSetActiveFadeOut(int nTime, unsigned int nColor, int nUnk2C)
{
    FADE_COL aCol[3];

    if (s_inActiveFadeOut.nTime >= 0) {
        return;
    }
    aCol[0].f = (float)(nColor & 0xFF) * 0.0078125f;
    aCol[1].f = (float)((nColor >> 8) & 0xFF) * 0.0078125f;
    aCol[2].f = (float)((nColor >> 16) & 0xFF) * 0.0078125f;
    fade_set(&s_inActiveFadeOut, aCol, 0, nTime + 2, 0, 1, nUnk2C);
}

/* Start an active fade-in from a packed 0x00BBGGRR colour */
void nmlModelSetActiveFadeIn(int nTime, unsigned int nColor, int nUnk2C)
{
    FADE_COL aCol[3];

    aCol[0].f = (float)(nColor & 0xFF) * 0.0078125f;
    aCol[1].f = (float)((nColor >> 8) & 0xFF) * 0.0078125f;
    aCol[2].f = (float)((nColor >> 16) & 0xFF) * 0.0078125f;
    fade_set(&s_inActiveFadeIn, aCol, 0, nTime, 1, 1, nUnk2C);
}

/* --- Global point lights --- */

void xglVectorScaleXYZ(void *pDst, void *pSrc, float fScale);

/* Clear the global point-light flag and colors */
void nmlModelSetGlobalPointLightReset(void)
{
    s_inLayout.nStatus &= ~0x2000000;
    __asm__ __volatile__(
        "sq $0, 0x0(%0)\n sq $0, 0x10(%0)\n"
        "sq $0, 0x20(%0)\n sq $0, 0x30(%0)\n nop"
        : : "r"(s_inGblPointC) : "memory");
}

/* Set one global point-light position.
 * The original leaves the beqz delay slot empty and issues the array-index
 * `addu` before the branch; gcc fills the slot with it. Held off by
 * --pin-slot-nop nmlModelSetGlobalPointLightPos:0 in FILE_FIX_FLAGS. */
void nmlModelSetGlobalPointLightPos(int no, void *pPos)
{
    VEC4 *p;

    if ((unsigned int)no < 3U) {
        p = &s_inGblPointP[no];
        __asm__ __volatile__("lq $2, 0x0(%1)\n sq $2, 0x0(%0)"
            : : "r"(p), "r"(pPos) : "$2", "memory");
        s_inLayout.nStatus |= 0x2000000;
    }
}

/* Set one global point-light color (x255 scale) */
void nmlModelSetGlobalPointLightCol(int no, void *pCol)
{
    if ((unsigned int)no < 3U) {
        xglVectorScaleXYZ(&s_inGblPointC[no], pCol, 128.0f);
        s_inLayout.nStatus |= 0x2000000;
    }
}

/* Set one global point-light position and color in one call */
void nmlModelSetGlobalPointLight(int no, void *pPos, void *pCol)
{
    if ((unsigned int)no < 3U) {
        __asm__ __volatile__("lq $2, 0x0(%1)\n sq $2, 0x0(%0)"
            : : "r"(&s_inGblPointP[no]), "r"(pPos) : "$2", "memory");
        xglVectorScaleXYZ(&s_inGblPointC[no], pCol, 128.0f);
        s_inLayout.nStatus |= 0x2000000;
    }
}

/* Set one per-model point light (position and color, x255 scale) */
void nmlModelSetPointLight(int no, void *pPos, void *pCol)
{
    VEC4 *pP;
    VEC4 *pC;

    if ((unsigned int)no < 3U) {
        pP = s_inLayout.aPointP;
        __asm__ __volatile__("lq $2, 0x0(%1)\n sq $2, 0x0(%0)"
            : : "r"(&pP[no]), "r"(pPos) : "$2", "memory");
        pC = s_inLayout.aPointC;
        xglVectorScaleXYZ(&pC[no], pCol, 128.0f);
        s_inLayout.nStatus |= 0x1000;
    }
}

/* Copy the parallel-light direction and color blocks into the layout */
void nmlModelSetLight(void *pDir, void *pCol)
{
    VEC4 *p;
    VEC4 *q;

    p = s_inLayout.aLightP;
    __asm__ __volatile__(
        "lq $2, 0x0(%1)\n sq $2, 0x0(%0)\n"
        "lq $2, 0x10(%1)\n sq $2, 0x10(%0)\n"
        "lq $2, 0x20(%1)\n sq $2, 0x20(%0)\n"
        "lq $2, 0x30(%1)\n sq $2, 0x30(%0)"
        : : "r"(p), "r"(pDir) : "$2", "memory");
    q = s_inLayout.aLightC;
    __asm__ __volatile__(
        "lq $2, 0x0(%1)\n sq $2, 0x0(%0)\n"
        "lq $2, 0x10(%1)\n sq $2, 0x10(%0)\n"
        "lq $2, 0x20(%1)\n sq $2, 0x20(%0)\n"
        "lq $2, 0x30(%1)\n sq $2, 0x30(%0)"
        : : "r"(q), "r"(pCol) : "$2", "memory");
    s_inLayout.nStatus |= 0x10;
}

/* --- Place / scale / offsets --- */

void xglMatrixInverse(void *pDst, void *pSrc);

char s_aTexName[32];
char s_aMatName[32];
float s_afTexOffset[2];
VEC4 s_inMatOffset;
VEC4 s_inScale;
extern float D_004A940C[];

/* Set the model place matrix and cache its inverse */
void nmlModelSetPlace(void *pMtx)
{
    VEC4 *p;

    p = s_inLayout.inPlace;
    __asm__ __volatile__(
        "lq $2, 0x0(%1)\n sq $2, 0x0(%0)\n"
        "lq $2, 0x10(%1)\n sq $2, 0x10(%0)\n"
        "lq $2, 0x20(%1)\n sq $2, 0x20(%0)\n"
        "lq $2, 0x30(%1)\n sq $2, 0x30(%0)"
        : : "r"(p), "r"(pMtx) : "$2", "memory");
    xglMatrixInverse(s_inLayout.inPlaceInv, pMtx);
    s_inLayout.nStatus |= 0x1;
}

/* Set the model scale, promoting the X slot to the largest component
 * (min 1.0) and recording that maximum */
void nmlModelSetScale(void *pScale)
{
    VEC4 *p;
    float fMax;
    float f;
    float fOne;

    p = &s_inScale;
    __asm__ __volatile__("lq $2, 0x0(%1)\n sq $2, 0x0(%0)"
        : : "r"(p), "r"(pScale) : "$2", "memory");
    fMax = p->f[0];
    f = p->f[1];
    fOne = 1.0f;
    if (fMax < f) {
        p->f[0] = f;
        fMax = f;
    }
    f = p->f[2];
    if (fMax < f) {
        p->f[0] = f;
        fMax = f;
    }
    if (fMax < fOne) {
        p->f[0] = fOne;
        fMax = fOne;
    }
    D_004A940C[0] = fMax;
}

void *memcpy(void *, const void *, unsigned int);

/* Register a texture-offset target by name with a UV offset */
void nmlModelSetTexOffset(void *pName, float u, float v)
{
    memcpy(s_aTexName, pName, 32);
    s_aTexName[31] = 0;
    s_afTexOffset[0] = u;
    s_afTexOffset[1] = v;
}

/* Register a material-offset target by name with an offset vector */
typedef struct {
    char a[32];
} NAMEBUF;

void nmlModelSetMatOffset(void *pName, void *pOffset)
{
    *(NAMEBUF *)s_aMatName = *(NAMEBUF *)pName;
    s_aMatName[31] = 0;
    /* Real quadword move, and the source of this function's register
     * shape: a TImode struct assignment lets gcc pick the scratch, which
     * lands the pair on $v1 and pushes the name-buffer pointer to $v0. */
    PS2_ASM("lq $2, 0x0(%1)\n sq $2, 0x0(%0)"
        : : "r"(&s_inMatOffset), "r"(pOffset) : "$2", "memory");
    /* The original left the jr delay slot empty here; our gas is in
     * reorder mode and hoists the sq above the jr unless something
     * unmovable sits between them. */
    SCHED_NOP();
}

/* Set the texture-map mode flags, alpha (clamped to [0,128]) and Z */
void nmlModelSetTexMap(int nFlag, int nAlpha, float fZ)
{
    s_inLayout.nTexMapAlpha = nAlpha;
    s_inLayout.uTexMap.n = nFlag;
    s_inLayout.fTexMapZ = fZ;
    if (nAlpha < 0) {
        s_inLayout.nTexMapAlpha = 0;
    }
    if (s_inLayout.nTexMapAlpha > 128) {
        s_inLayout.nTexMapAlpha = 128;
    }
    if ((s_inLayout.uTexMap.l & 0x10001) == 0x10001) {
        s_inLayout.nStatus |= 0x40000;
    } else if ((s_inLayout.uTexMap.l & 0x20001) == 0x20001) {
        s_inLayout.nStatus |= 0x200000;
    } else if ((s_inLayout.uTexMap.l & 0x30000) == 0x30000) {
        s_inLayout.nStatus |= 0x400000;
    } else {
        s_inLayout.nStatus |= 0x4000;
    }
}

/* --- Init / parts visibility / direct send --- */

void INIT_MODELSYSTEM(void);
void INIT_ALPHA_GROUP(void);
void INIT_PARENT_BUF(void);
void INIT_MAP_HANDLE(void *pMapHandle);
extern int D_0095BB44[];
int nmlModelLexDataCheck(void *pData);
void nmlModelDirectXtxSub(void *pTex, int no, int no2);

/* Initialize every model subsystem and reset the model state */
void nmlModelInit(void)
{
    INIT_MODELSYSTEM();
    INIT_BACK_BUFFER();
    INIT_ALPHA_GROUP();
    INIT_PARENT_BUF();
    INIT_MAP_HANDLE(&s_inMapHandle);
    nmlModelClear();
    D_004A9430[0] = 0;
    D_0095BB44[0] = 30;
}

/* Show or hide every part of a lex model in one pass */
void nmlModelInitPartsVisible(void *pData, int nVisible)
{
    int i;
    int *pTop;
    int *pOfs;
    char *q;

    if (nmlModelLexDataCheck(pData) == 0) {
        pTop = (int *)((char *)pData + 0xB0);
        i = 0;
        if (*(int *)((char *)pData + 0x44) > 0) {
            pOfs = pTop;
            do {
                q = (char *)pData + *pOfs++;
                if (nVisible != 0) {
                    *(int *)(q + 32) &= ~8;
                } else {
                    *(int *)(q + 32) |= 8;
                }
                i++;
            } while (i < *(int *)((char *)pData + 0x44));
        }
    }
}

/* Validate an XTX texture and send it directly (unless a packet change
 * is pending) */
void nmlModelDirectSendXtx(int no, void *pTex)
{
    char *p = (char *)pTex;

    if (s_nPacketSignal == 0) {
        if ((unsigned int)((int)pTex - 0x200000) <= 0x1DFFFFF && (((int)pTex & 0xF) == 0)) {
            if (p[0] == 'X' && p[1] == 'T' && p[2] == p[0]) {
                nmlModelDirectXtxSub(pTex, no, no + 1);
            }
        }
    }
}

/* --- Fog parameter helpers --- */

void _CurSetMatrix(void *pMtx);
void _CurRotTransPersFog(void *pOut, void *pPos, float *pFog);

/* Build the 4-float fog parameter block from near/far distances and
 * min/max densities */
void nmlModelFogPara(float *pPara, float fNear, float fFar, float fMin, float fMax)
{
    float fA;
    float fB;

    fA = (1.0f - fMax) * 255.0f;
    fB = (1.0f - fMin) * 255.0f;
    if (fA < 0.0f) {
        fA = 0.0f;
    }
    if (255.0f < fA) {
        fA = 255.0f;
    }
    if (fB < 0.0f) {
        fB = 0.0f;
    }
    if (255.0f < fB) {
        fB = 255.0f;
    }
    pPara[0] = fA;
    pPara[1] = fB;
    pPara[2] = ((fA - fB) * (fFar + fNear) / (fFar - fNear) + (fA + fB)) * 0.5f;
    pPara[3] = fFar * fNear * (fB - fA) / (fFar - fNear);
}

/* Compute the fog coefficient of a point through the active window's
 * camera; 255 when there is no camera */
int nmlModelGetFogPara(void *pPos)
{
    VEC4 vTemp;
    float *pFog;
    void *pCamera;
    unsigned int nFog;

    nFog = 255;
    pFog = s_inGblFogPara;
    pFog = ((s_inLayout.nStatus & 0x2) != 0) ? s_inLayout.aFogDist : pFog;
    pCamera = xglStudioSelectGetActiveCamera(s_inLayout.nWindow);
    if (pCamera != 0) {
        _CurSetMatrix((char *)pCamera + 0x470);
        _CurRotTransPersFog(&vTemp, pPos, pFog);
        nFog = (unsigned int)vTemp.f[3];
    }
    return nFog;
}

/* Compute the fog coefficient of a point through a specific studio's
 * camera; 255 when there is no camera */
int nmlModelGetFogParaStudio(void *pPos, int nStudio)
{
    VEC4 vTemp;
    float *pFog;
    void *pCamera;
    unsigned int nFog;

    nFog = 255;
    pFog = s_inGblFogPara;
    pFog = ((s_inLayout.nStatus & 0x2) != 0) ? s_inLayout.aFogDist : pFog;
    pCamera = xglStudioSelectGetActiveCamera(nStudio);
    if (pCamera != 0) {
        _CurSetMatrix((char *)pCamera + 0x470);
        _CurRotTransPersFog(&vTemp, pPos, pFog);
        nFog = (unsigned int)vTemp.f[3];
    }
    return nFog;
}

/* --- Clip test entry points --- */

/* Clip-test and cull-test a point against the active camera; 0 when
 * there is no active camera */
int nmlModelCalcClip(void *pPos)
{
    void *pCamera;
    int n1, n2;

    pCamera = xglStudioGetActiveCamera();
    if (pCamera != 0) {
        _ModelCalcClipInit((char *)pCamera + 0x4F0);
        n1 = _ModelCalcClip(pPos);
        n2 = xglCullingCheck(pCamera, pPos);
        return n1 | n2;
    }
    return (int)pCamera;
}

/* Clip-test a point against an explicit camera */
int nmlModelCalcClipCam(void *pPos, void *pCamera)
{
    _ModelCalcClipInit((char *)pCamera + 0x4F0);
    return _ModelCalcClip(pPos);
}

/* Clip-test a point against the active camera without the culling-map
 * check; 0 when there is no active camera */
int nmlModelCalcClipNoCulling(void *pPos)
{
    void *pCamera;

    pCamera = xglStudioGetActiveCamera();
    if (pCamera != 0) {
        _ModelCalcClipInit((char *)pCamera + 0x4F0);
        return _ModelCalcClip(pPos);
    }
    return (int)pCamera;
}

int _ModelCalcClipMat1(void *pPos, void *pMatrix);
int _ModelCalcClipMat2(void *pPos, void *pMatrix1, void *pMatrix2);

/* Transform a point by one matrix and clip-test it against an explicit
 * camera */
int nmlModelCalcClipMat1Cam(void *pPos, void *pMatrix, void *pCamera)
{
    _ModelCalcClipInit((char *)pCamera + 0x4F0);
    return _ModelCalcClipMat1(pPos, pMatrix);
}

/* Transform a point by two chained matrices and clip-test it against an
 * explicit camera */
int nmlModelCalcClipMat2Cam(void *pPos, void *pMatrix1, void *pMatrix2, void *pCamera)
{
    _ModelCalcClipInit((char *)pCamera + 0x4F0);
    return _ModelCalcClipMat2(pPos, pMatrix1, pMatrix2);
}

/* Clip-test and cull-test a point against a specific studio's camera;
 * 0 when that studio has no camera */
int nmlModelCalcClipStudio(void *pPos, int nStudio)
{
    void *pCamera;
    int nRet;

    nRet = 0;
    pCamera = xglStudioSelectGetActiveCamera(nStudio);
    if (pCamera != 0) {
        _ModelCalcClipInit((char *)pCamera + 0x4F0);
        nRet = _ModelCalcClip(pPos);
        nRet |= xglCullingCheck(pCamera, pPos);
    }
    return nRet;
}

/* --- Movie-finish signal / non-linear camera ask --- */

extern unsigned short D_004B9102[];

/* Configure the back buffer and fade cancels for each movie-finish mode */
void nmlModelSendSignalMovieFinish(int mode)
{
    switch (mode) {
    case 1:
        nmlModelSetBackBufferToBattle(D_004B9102[0]);
        nmlModelSetFadeInCancel(30);
        nmlModelSetFadeOutCancel(30);
        break;
    case 2:
        nmlModelSetBackBuffer(38, 1, D_004B9102[0], 0);
        nmlModelSetFadeInCancel(30);
        break;
    case 5:
        nmlModelSetBackBuffer(38, 1, D_004B9102[0], 0);
        nmlModelSetFadeOutCancel(30);
        nmlModelSetFadeInCancel(30);
        break;
    case 3:
        nmlModelSetBackBuffer(3, 1, D_004B9102[0], 0);
        nmlModelSetFadeInCancel(30);
        break;
    case 4:
        nmlModelSetBackBuffer(D_0095BB44[0], 1, D_004B9102[0], 1);
        nmlModelSetFadeInCancel(30);
        NO_SIBCALL();
        break;
    default:
        break;
    }
}

float xglPointLength(void *pPos, void *pPos2);
float fabsf(float);
int s_nNonLinearCamera;
VEC4 s_inNonLinearPos;
VEC4 s_inNonLinearEye;
extern float s_fNonLinearMin;
extern float s_fNonLinearMax;

/* Decide (once) whether the active camera is a non-linear one; returns
 * 1 while the cached answer is "non-linear" */
int nmlModelAskNonLinearCamera(void)
{
    VEC4 vTemp;
    void *pCamera;
    float fAbs;

    if (s_nNonLinearCamera == 0) {
        pCamera = xglStudioSelectGetActiveCamera(0);
        if (pCamera != 0) {
            s_nNonLinearCamera = 1;
            if (1.0f <= xglPointLength(&s_inNonLinearPos, (char *)pCamera + 0xD0)) {
                s_nNonLinearCamera = 2;
            } else {
                __asm__ __volatile__(".set noreorder\n"
                    "lqc2 $vf3, 0x0(%1)\n"
                    "lqc2 $vf2, 0x0(%2)\n"
                    "vsub.xyz $vf2xyz, $vf2xyz, $vf3xyz\n"
                    "sqc2 $vf2, 0x0(%0)\n"
                    ".set reorder"
                    : : "r"(&vTemp), "r"((char *)pCamera + 0xA0), "r"(&s_inNonLinearEye));
                fAbs = fabsf(vTemp.f[1]);
                if (s_fNonLinearMin < fAbs) {
                    if (fAbs < s_fNonLinearMax) {
                        s_nNonLinearCamera = 2;
                    }
                }
            }
        }
    }
    return (s_nNonLinearCamera ^ 2) == 0;
}

/* --- Lex data validation / parts visibility / direct DMA send --- */

char *strstr(const char *, const char *);

/* Validate a lex model block: address range, alignment, "lex" magic and
 * (for named variants) the MagicCarpetCome marker; 0 means valid */
int nmlModelLexDataCheck(void *pData)
{
    char *p;
    int nRet;
    int n;
    int nOfs;
    int *aOfs;
    char *q;
    char *pStr;

    p = (char *)pData;
    nRet = 1;
    if ((unsigned int)((int)p - 0x200000) <= 0x1DFFFFF && (((int)p & 0xF) == 0)) {
        if ((*(long *)p & 0xFFFFFF) != 0x78656C) {
            goto out;
        }
        LAUNDER_V(nRet);
        if (p[63] != 0 && *(int *)(p + 0x6C) == 0) {
            n = *(int *)(p + 0x44);
            /* Binding the table base to its own pointer before indexing
             * is what emits `addu v0,s0,v0` (base first); folded into one
             * address expression gcc canonicalises the multiply first and
             * emits `addu v0,v0,s0`. */
            aOfs = (int *)(p + 0xAC);
            nOfs = aOfs[n];
            q = p + nOfs;
            pStr = q + *(int *)(q + 0x24);
            if (*(int *)(p + 0xA0) != 0) {
                pStr += n * 16;
            }
            if (strstr(pStr + *(int *)(q + 0x28), "MagicCarpetCome") == 0) {
                goto out;
            }
        }
        if (*(int *)(p + 0x44) > 0) {
            nRet = 0;
        }
    }
out:
    return nRet;
}

typedef struct {
    int nData;              /* 0x0 */
    short nSize;            /* 0x4 */
    short nType;            /* 0x6 */
} DIRECT_ENTRY;

unsigned int nmlPacketSetAttributeData16N(void *pData, int nNum);
DIRECT_ENTRY s_aDirectEntry[5500];
int s_nDirectNum;

/* Queue one direct-send entry, copying packet-attribute data for the
 * odd channels and referencing it for the even ones */
void nmlModelDirectSend(int no, void *pData, int nSize)
{
    DIRECT_ENTRY *p;

    if (s_nPacketSignal == 0) {
        if (s_nDirectNum < 5500) {
            p = &s_aDirectEntry[s_nDirectNum];
            p->nSize = nSize;
            p->nType = 1;
            switch (no) {
            case 1: case 3: case 5: case 7: case 9: case 11:
                p->nData = nmlPacketSetAttributeData16N(pData, nSize);
                break;
            case 2: case 4: case 6: case 8: case 10: case 12:
                p->nData = (int)pData;
                break;
            }
            switch (no) {
            case 1: case 2:
                p->nType = 1;
                break;
            case 3: case 4:
                p->nType = 2;
                break;
            case 5: case 6:
                p->nType = 3;
                break;
            case 7: case 8:
                p->nType = 4;
                break;
            case 9: case 10:
                p->nType = 5;
                break;
            case 11: case 12:
                p->nType = 6;
                break;
            }
            s_nDirectNum++;
        }
    }
}

typedef union {
    unsigned long l;
    unsigned int w[2];
} XTXREG;

typedef struct {
    char pad00[0x20];
    XTXREG uUnk20;          /* 0x20 */
    char pad28[0x18];
    XTXREG uUnk40;          /* 0x40 */
} DIRECT_XTX;

extern DIRECT_XTX D_004B9170;

/* Send every mip block of an XTX texture: a transfer-descriptor entry
 * followed by the texel data itself */
void nmlModelDirectXtxSub(void *pTex, int no1, int no2)
{
    char *p;
    int i;
    unsigned int nHead;
    unsigned int nLow;
    unsigned int nHigh;

    i = 0;
    p = (char *)pTex + *(int *)((char *)pTex + 12);
    if (*(int *)((char *)pTex + 8) > 0) {
        do {
            nHead = *(unsigned int *)p;
            nLow = nHead & 0xFFFF;
            nHigh = nHead >> 16;
            if (nHead == nLow) {
                nHigh = ((int)nLow + 63) >> 6;
            }
            i++;
            D_004B9170.uUnk20.l = ((unsigned long)((*(unsigned int *)(p + 8) >> 6) + 0x3800) << 32)
                | ((unsigned long)nHigh << 48);
            D_004B9170.uUnk40.l = ((unsigned long)*(unsigned int *)(p + 4) << 32) | nLow;
            nmlModelDirectSend(no1, &D_004B9170, 6);
            nmlModelDirectSend(no2, (char *)pTex + *(int *)(p + 16), *(int *)(p + 12));
            p += 20;
        } while (i < *(int *)((char *)pTex + 8));
    }
}

char s_aHideParts[1024];
int s_nHidePartsNum;

/* Show or hide one named part of a lex model; parts hidden before the
 * model exists are queued for later */
void nmlModelSetPartsVisible(void *pData, int nParts, int nVisible)
{
    int i;
    int *pOfs;
    int nQ;
    int nSet;
    int nMask;
    int nCnt;

    if (nmlModelLexDataCheck(pData) != 0) {
        return;
    }
    if (nParts >= *(int *)((char *)pData + 0x40)) {
        return;
    }
    pOfs = (int *)((char *)pData + 0xB0);
    if ((*(int *)((char *)pData + pOfs[0] + 0xC0) & 0x10) != 0) {
        nSet = 8;
        nMask = -1;
        if (nVisible != 0) {
            nSet = 0;
            nMask = ~8;
        }
        i = 0;
        /* The original reloads *pOfs for the loop preheader instead of
         * reusing the value the 0xC0 test already loaded. Left to CSE,
         * gcc keeps that value live in a register across the test, which
         * costs a register and forces an extra b/addu loop-entry
         * rotation; laundering pOfs here makes the two loads distinct. */
        LAUNDER(pOfs);
        if (*(int *)((char *)pData + 0x44) > 0) {
            do {
                /* Accumulating into one variable is what makes the loaded
                 * offset and the resulting address share $v1 as in the
                 * original. But with nQ as both destination and second
                 * operand gcc commutes the add to reuse the target, giving
                 * `addu v1,v1,s0`; the tied empty asm gives the add a fresh
                 * destination (so pData stays first) and then puts it back
                 * in nQ's register for free. */
                nQ = *pOfs;
                PASSTHRU(nQ, (int)pData + nQ);
                if (*(int *)(nQ + 0x2C) == nParts) {
                    pOfs++;
                    *(int *)(nQ + 0x20) = (*(int *)(nQ + 0x20) | nSet) & nMask;
                } else {
                    pOfs++;
                }
                i++;
            } while (i < *(int *)((char *)pData + 0x44));
        }
        return;
    }
    if (nVisible == 0) {
        if (s_nHidePartsNum < 1023) {
            nCnt = s_nHidePartsNum + 1;
            s_aHideParts[nCnt] = nParts;
            s_nHidePartsNum = nCnt;
        }
    }
}

/* --- Drop-shadow circle rendering --- */

int s_aCircleShadow[16];
extern LAYOUT *D_009550B0[];
extern unsigned short D_00952410[];
extern char D_00955900[];
extern char D_00956920[];
void nmlPacketSendCircleTexture(void *p1, void *p2);
void nmlPacketMakeCircleTexture(void *pLayout, int nArg);

/* Fill the circle-shadow alpha ramp from a 0-128 ratio */
static void set_circle_shadow_ratio(int nRatio)
{
    float fStep;
    int i;
    int j;

    fStep = (float)nRatio * 0.0625f;
    for (i = 0; i < 16; i++) {
        j = (i < 8) ? i : i + 8;
        s_aCircleShadow[j] = (int)(128.0f - (float)i * fStep) << 24;
    }
}

int nmlModelRenderProreal(LAYOUT *pM, int nArg);

/* Render the circle shadow for the first visible model in the given index
 * ring, then draw it through the pro-real path */
int nmlModelRenderCircle(void *pHdr, int nStep, int nArg)
{
    LAYOUT *pM;
    int i;
    int nRet;

    pM = 0;
    nRet = 0;
    if (*(float *)(*(char **)((char *)pHdr + 0xC) + 0x26C) <= 0.001f) {
        goto done;
    }
    i = *(short *)pHdr - nStep;
    goto test;
    do {
        i += nStep;
        pM = D_009550B0[D_00952410[i]];
        if ((pM->nStatus & 0x100000) == 0) {
            break;
        }
        pM = 0;
test:   ;
    } while (i != (short)*(unsigned short *)pHdr
                  + (short)*(unsigned short *)((char *)pHdr + 2) * nStep - nStep);
    if (pM != 0) {
        set_circle_shadow_ratio((int)((float)pM->nTexMapAlpha * pM->fTransparency));
        nmlPacketSendCircleTexture(D_00955900, D_00956920);
        nRet = nmlModelRenderProreal(pM, nArg);
    }
done:
    return nRet;
}

/* Render the drop-shadow circle for the first visible model in the
 * given index ring */
/* TODO: near-miss, 15/52 words (REGISTER + scheduling; instruction counts
 * now agree). Logic verified against the disasm: ring walk, face-model
 * skip bit 0x100000, alpha*transparency ratio. Computing nEnd BEFORE i is
 * what fixed the length -- it lets gcc fill the loop-test beqz delay slot
 * with `i += nStep` (dead on the exit path), which was the missing word.
 * What remains is a register cascade: the original puts nStep in $a3, nC
 * in $a2, D_009550B0 in $t1 and the 0x100000 mask in $t0; gcc shifts all
 * four one slot up ($t0/$a3/$t2/$t1) because it gives $a2 to nEnd alone
 * instead of letting nC and nEnd share it.
 * Best found so far, 8/52: PIN(int nStep2, "$7") + LAUNDER_V on a local
 * copy of nStep, with the nested D_009550B0[D_00952410[i]] kept as one
 * expression (an `idx` temp flips which table gets $a1). That fixes every
 * register except the multiply destination, and leaves a 10-instruction
 * scheduling permutation in the header: the original issues `lh nC` first
 * and the mult last, gcc hoists `lh cnt` and the mult to the front of the
 * block. Because gcc then still has $v1 live for a %hi it writes the
 * product to $v0 instead of the original's $v1, so no reordering flag can
 * close it -- --rotate cannot rename a destination register. Left as plain
 * C rather than shipping the pin, since the pin does not reach a match.
 * Swept: nEnd/i statement order, nCnt temp, idx temp, for/continue loop
 * forms, single shared nC/nEnd variable, (cnt-1)*nStep grouping, explicit
 * pTbl/pIdx table locals, LAUNDER/LAUNDER_V fences on nC, mask and tables,
 * PIN of nC to $6 and of the product to $3. */
int nmlModelRenderDropCircle(void *pHdr, int nStep, int nArg)
{
    LAYOUT *pM;
    int i;
    int nEnd;
    int nC;

    pM = 0;
    nC = *(short *)pHdr;
    nEnd = *(short *)((char *)pHdr + 2) * nStep + nC - nStep;
    i = nC - nStep;
    while (i != nEnd) {
        i += nStep;
        pM = D_009550B0[D_00952410[i]];
        if ((pM->nStatus & 0x100000) == 0) {
            break;
        }
    }
    set_circle_shadow_ratio((int)((float)pM->nTexMapAlpha * pM->fTransparency));
    nmlPacketSendCircleTexture(D_00955900, D_00956920);
    nmlPacketMakeCircleTexture(pM, nArg);
    return 0;
}

float s_fShadowOfs = 0.005f;

/* TODO: near-miss, 11 diffs, NOT registered. Every instruction and every
 * register matches; the whole remaining difference is one sched2 decision, and
 * the swept space is recorded here so nobody re-opens it blind:
 *   - gcc fills the `beq s_nShadowVec` delay slot with `l.s $f5,
 *     s_fShadowOfs` (pulled from below); the original puts the f5 load
 *     BEFORE the three aVec stores and `s.s $f2,4($sp)` in the slot.
 *     Same insns, different filler choice. No fixer flag expresses it:
 *     --swap-into-slot is jal-only, and even extended it would land the
 *     load one slot too late.
 *   - the two %hi/%lo materialisations (&s_inLayout for the tail,
 *     &s_inShadowVec) interleave with the aVec loads differently.
 * Swept and REJECTED: fOfs assignment at all four positions among the
 * aVec initialisers (all 11 or worse); no fOfs local at all (72 words --
 * gcc then loses a div hazard nop and gains a .p2align pad); a
 * `LAYOUT *p = &s_inLayout` for the tail (69 words, much worse).
 * Locked in and NOT to be re-derived: aVec[0]/aVec[1]/aVec[2] in that
 * source order (any other order swaps $f0/$f1); the else arm and both
 * tail blocks write f[0] BEFORE f[2] -- gcc reverses each pair, so the
 * source order is the mirror of the emitted order.
 * tools/permute.py cannot run on this file (raw inline asm defeats
 * pycparser); a stripped copy would be the next move.
 */

/* Build the drop-shadow projection matrix from the light direction.
 * The light vector defaults to (aLightC[0].x, 1.5, aLightC[2].x) and is
 * overridden by the explicit s_inShadowVec when one has been set -- note
 * only components 0 and 2 are overridden, 1.5 always survives. */
void nmlModelCalcDropShadow(void)
{
    float aVec[3];
    float fOfs;

    xglMatrixUnit(s_inLayout.aShadowMtx);
    aVec[0] = s_inLayout.aLightC[0].f[0];
    aVec[1] = 1.5f;
    fOfs = s_fShadowOfs;
    aVec[2] = s_inLayout.aLightC[2].f[0];
    if (s_nShadowVec != 0) {
        aVec[0] = s_inShadowVec.f[0];
        aVec[2] = s_inShadowVec.f[2];
    }
    if (aVec[1] != 0.0f) {
        s_inLayout.aShadowMtx[1].f[0] = -aVec[0] / aVec[1];
        s_inLayout.aShadowMtx[1].f[2] = -aVec[2] / aVec[1];
    } else {
        s_inLayout.aShadowMtx[1].f[0] = 0.0f;
        s_inLayout.aShadowMtx[1].f[2] = 0.0f;
    }
    s_inLayout.aShadowMtx[1].f[1] = 0.0f;
    s_inLayout.aShadowMtx[3].f[1] = s_inLayout.inPlace[3].f[1] + fOfs;
    s_inLayout.aShadowMtx[3].f[0] =
        -s_inLayout.inPlace[3].f[1] * s_inLayout.aShadowMtx[1].f[0];
    s_inLayout.aShadowMtx[3].f[2] =
        -s_inLayout.inPlace[3].f[1] * s_inLayout.aShadowMtx[1].f[2];
    if (s_inLayout.nShadowHeightOn != 0) {
        s_inLayout.aShadowMtx[3].f[1] = s_inLayout.fShadowHeight + fOfs;
        s_inLayout.aShadowMtx[3].f[0] =
            -s_inLayout.fShadowHeight * s_inLayout.aShadowMtx[1].f[0];
        s_inLayout.aShadowMtx[3].f[2] =
            -s_inLayout.fShadowHeight * s_inLayout.aShadowMtx[1].f[2];
    }
}

void _CurSetViewScaleTrans(void *pScale, void *pTrans);
void _CurRotTransPersClip(void *pOut, float *pClip);
void nmlFilterStealthMake(float fSize, int nIndex, void *pCamera, LAYOUT *pLayout);
float tanf(float);

float s_fStealthScale;
float s_fStealthMinRange;
float s_fStealthBias;

/* TODO: near-miss, 32 diffs, NOT registered. Right length (93 words),
 * right instructions, right control flow -- only 2 words differ in
 * OPCODE (the outer sort loop comes out `bnezl` where the original has
 * a plain `bnez`, and the original reuses its layout pointer where we
 * rematerialise `lui %hi`). Everything else is register naming: the
 * original keeps pCamera in $s2 and &s_inLayout in $s1 (we have them the
 * other way round), and the whole bubble-sort body lands on
 * $s0/$v1/$a3/$a0/$t0 where we get $a1/$a0/$v1/$v0/$a2.
 * Swept and REJECTED: block-scoping the sort temporaries (no change);
 * a function-scope `LAYOUT *pL = &s_inLayout` (91 words); the same
 * pointer scoped to the if-arm (96 words); the sort as
 * `do { nSwap = 0; ... } while (nSwap)` instead of
 * `nSwap = 1; while (nSwap) { nSwap = 0; ... }` (92 words -- the while
 * form is the one that gives the right length, and gcc folds the
 * initial 1 away by itself).
 * The layout base is what to attack next: the original hoists the FULL
 * address out of the outer loop (`move $a2,$s1`), we hoist only the
 * `lui %hi` -- so in the original the sort's base is derived from a
 * POINTER pseudo, not from the symbol_ref that `&s_inLayout.aStealth[1]`
 * gives us.
 *
 * Also names LAYOUT+0x280 as aStealth[10] and +0x2C8 as nStealthNum.
 */

/* Queue the stealth (heat-haze) filter entries for this model. When the
 * model wants per-studio entries all ten slots are built and then bubble
 * sorted back to front by depth; otherwise a single entry is made. */
void nmlModelStealthEntry(void *pModel, void *pCamera)
{
    int aClip[4];
    float fSize;
    int i;

    _CurSetMatrix((char *)pCamera + 0x470);
    _CurSetViewScaleTrans((char *)pCamera + 0x80, (char *)pCamera + 0x70);
    _CurRotTransPersClip(aClip, &s_inLayout.aFogDist[8]);
    fSize = (float)aClip[2] * s_fStealthScale
            / tanf(*(float *)((char *)pCamera + 0x94) * 0.5f);
    if (*(int *)((char *)pModel + 0x48) != 0) {
        for (i = 0; i < 10; i++) {
            nmlFilterStealthMake(fSize, i, pCamera, &s_inLayout);
            s_inLayout.nStealthNum++;
        }
        {
            int nSwap;

            nSwap = 1;
            while (nSwap != 0) {
                STEALTH **pp;
                int n;

                nSwap = 0;
                pp = &s_inLayout.aStealth[1];
                for (n = 8; n >= 0; n--) {
                    STEALTH *pA;

                    pA = pp[-1];
                    if (s_fStealthMinRange < pA->fRange) {
                        STEALTH *pB;

                        pB = pp[0];
                        if (pA->fDepth + s_fStealthBias < pB->fDepth) {
                            pp[-1] = pB;
                            nSwap = 1;
                            pp[0] = pA;
                        }
                    }
                    pp++;
                }
            }
        }
    } else {
        nmlFilterStealthMake(fSize, 0, pCamera, &s_inLayout);
        s_inLayout.nStealthNum++;
    }
}

float s_fClipScale = 1.3f;

/* Clip-test the model's entry position against every enabled sub-window
 * camera.  When only window 0 is in use a single test is made; otherwise
 * each enabled window's result is recorded in aClip[] and the model is
 * marked fully clipped (0x20000) only when every tested window clipped
 * it. */
/* TODO: near-miss, 17 diffs of 85 words, NOT registered.  Right length,
 * right control flow, both loops and the whole tail are word-for-word
 * correct; the ONLY problem is the entry block's schedule, and every
 * later diff is that shift cascading.
 *   orig: addiu sp / li v1,1 / 6x sd / lw v0,s_nClip(gp) / beq
 *   ours: lw v1,s_nClip(gp) / addiu sp / li v0,1 / 5x sd / beq / sd ra
 * i.e. the original has the constant ready first and the gp load late,
 * and does not fill the beq slot with `sd ra`.  Downstream that also
 * costs `lw v0,72(a0)` vs `lw a0,72(a0)` (gcc reuses the dying argument
 * register only because it schedules that load early) and swaps the two
 * lwc1's.
 * Swept and REJECTED: a named local for s_nClip; `if (s_nClip == 1) goto
 * done;` (both exactly 17); four spellings of the qword source address
 * (&aFogDist[8], (char *)&s_inLayout + 0x1F0, a VEC4 * local, the
 * aFogDist+8 pointer form -- all 17, the addiu is a masked %lo reloc and
 * was never a diff); `vPos = *(VEC4 *)...` as a struct assign (87 words,
 * much worse).
 * Note LAYOUT really is 0x320 bytes (from the ELF symbol table), with an
 * int aClip[4] at 0x310, and the entry position is the VEC4 overlaying
 * aFogDist[8..11] at 0x1F0.
 */
int nmlModelCalcEntryClip(void *pModel)
{
    VEC4 vPos;
    int i;
    int nUse;
    int nSum;
    int nCount;

    if (s_nClip != 1) {
        vPos.q = *(TI *)&s_inLayout.aFogDist[8];
        vPos.f[3] *= s_fClipScale;
        if (*(int *)((char *)pModel + 0x48) != 0) {
            nUse = 0;
            for (i = 1; i < 4; i++) {
                if (g_aSubWindow[i] != 0) {
                    nUse = 1;
                }
            }
            if (nUse != 0) {
                nSum = 0;
                nCount = 0;
                for (i = 0; i < 4; i++) {
                    if (g_aSubWindow[i] != 0) {
                        int n;

                        n = nmlModelCalcClipStudio(&vPos, i);
                        nCount++;
                        s_inLayout.aClip[i] = n;
                        nSum += n;
                    }
                }
                if (nSum == nCount) {
                    s_inLayout.nStatus |= 0x20000;
                }
            } else if (nmlModelCalcClipStudio(&vPos, 0) != 0) {
                s_inLayout.aClip[0] = 1;
                s_inLayout.nStatus |= 0x20000;
            }
        }
    }
    return 0;
}

/* The "current" matrix lives in VU0 registers vf27-vf30 for the whole
 * transform pipeline; these three move it in, out, and compose it. */
void _CurMatrixSet(void *pMtx)
{
    PS2_ASM(".set noreorder\n"
            "lqc2 $vf27, 0x0(%0)\n"
            "lqc2 $vf28, 0x10(%0)\n"
            "lqc2 $vf29, 0x20(%0)\n"
            "lqc2 $vf30, 0x30(%0)\n"
            ".set reorder" : : "r"(pMtx));
}

void _CurMatrixGet(void *pMtx)
{
    PS2_ASM(".set noreorder\n"
            "sqc2 $vf27, 0x0(%0)\n"
            "sqc2 $vf28, 0x10(%0)\n"
            "sqc2 $vf29, 0x20(%0)\n"
            "sqc2 $vf30, 0x30(%0)\n"
            ".set reorder" : : "r"(pMtx) : "memory");
}

/* Post-multiply the current matrix by pMtx. */
void _CurMatrixMul(void *pMtx)
{
    PS2_ASM(".set noreorder\n"
            "lqc2 $vf2, 0x0(%0)\n"
            "lqc2 $vf3, 0x10(%0)\n"
            "lqc2 $vf4, 0x20(%0)\n"
            "lqc2 $vf5, 0x30(%0)\n"
            "vmulax.xyzw $ACC, $vf27, $vf2x\n"
            "vmadday.xyzw $ACC, $vf28, $vf2y\n"
            "vmaddaz.xyzw $ACC, $vf29, $vf2z\n"
            "vmaddw.xyzw $vf2, $vf30, $vf2w\n"
            "vmulax.xyzw $ACC, $vf27, $vf3x\n"
            "vmadday.xyzw $ACC, $vf28, $vf3y\n"
            "vmaddaz.xyzw $ACC, $vf29, $vf3z\n"
            "vmaddw.xyzw $vf3, $vf30, $vf3w\n"
            "vmulax.xyzw $ACC, $vf27, $vf4x\n"
            "vmadday.xyzw $ACC, $vf28, $vf4y\n"
            "vmaddaz.xyzw $ACC, $vf29, $vf4z\n"
            "vmaddw.xyzw $vf4, $vf30, $vf4w\n"
            "vmulax.xyzw $ACC, $vf27, $vf5x\n"
            "vmadday.xyzw $ACC, $vf28, $vf5y\n"
            "vmaddaz.xyzw $ACC, $vf29, $vf5z\n"
            "vmaddw.xyzw $vf30, $vf30, $vf5w\n"
            "vmulw.xyzw $vf27, $vf2, $vf0w\n"
            "vmulw.xyzw $vf28, $vf3, $vf0w\n"
            "vmulw.xyzw $vf29, $vf4, $vf0w\n"
            ".set reorder" : : "r"(pMtx));
}

/* Multiply the 3x3 part of pMtx by the current matrix and renormalise
 * each resulting column; result written to pDst as three quadwords. */
void _CurMatrixMul33norm(void *pDst, void *pMtx)
{
    PS2_ASM(".set noreorder\n"
            "lqc2 $vf20, 0x0(%1)\n"
            "lqc2 $vf21, 0x10(%1)\n"
            "lqc2 $vf22, 0x20(%1)\n"
            "vmulax.xyz $ACC, $vf27, $vf20x\n"
            "vmadday.xyz $ACC, $vf28, $vf20y\n"
            "vmaddz.xyz $vf20, $vf29, $vf20z\n"
            "vmulax.xyz $ACC, $vf27, $vf21x\n"
            "vmadday.xyz $ACC, $vf28, $vf21y\n"
            "vmaddz.xyz $vf21, $vf29, $vf21z\n"
            "vmulax.xyz $ACC, $vf27, $vf22x\n"
            "vmadday.xyz $ACC, $vf28, $vf22y\n"
            "vmaddz.xyz $vf22, $vf29, $vf22z\n"
            "vaddz.x $vf25, $vf0, $vf20\n"
            "vaddy.x $vf24, $vf0, $vf20\n"
            "vaddx.x $vf23, $vf0, $vf20\n"
            "vaddz.y $vf25, $vf0, $vf21\n"
            "vaddy.y $vf24, $vf0, $vf21\n"
            "vaddx.y $vf23, $vf0, $vf21\n"
            "vaddz.z $vf25, $vf0, $vf22\n"
            "vaddy.z $vf24, $vf0, $vf22\n"
            "vaddx.z $vf23, $vf0, $vf22\n"
            "vmula.xyz $ACC, $vf23, $vf23\n"
            "vmadda.xyz $ACC, $vf24, $vf24\n"
            "vmadd.xyz $vf10, $vf25, $vf25\n"
            "vrsqrt $Q, $vf0w, $vf10x\n"
            "vwaitq\n"
            "vmulq.x $vf23, $vf23, $Q\n"
            "vmulq.x $vf24, $vf24, $Q\n"
            "vmulq.x $vf25, $vf25, $Q\n"
            "vnop\n"
            "vnop\n"
            "vrsqrt $Q, $vf0w, $vf10y\n"
            "vwaitq\n"
            "vmulq.y $vf23, $vf23, $Q\n"
            "vmulq.y $vf24, $vf24, $Q\n"
            "vmulq.y $vf25, $vf25, $Q\n"
            "vnop\n"
            "vnop\n"
            "vrsqrt $Q, $vf0w, $vf10z\n"
            "vwaitq\n"
            "vmulq.z $vf23, $vf23, $Q\n"
            "vmulq.z $vf24, $vf24, $Q\n"
            "vmulq.z $vf25, $vf25, $Q\n"
            "sqc2 $vf23, 0x0(%0)\n"
            "sqc2 $vf24, 0x10(%0)\n"
            "sqc2 $vf25, 0x20(%0)\n"
            ".set reorder" : : "r"(pDst), "r"(pMtx) : "memory");
}

/* Occlusion-cell debug draw: retail ships it empty. */
void culling_cell_disp(void)
{
}

/* Load the eight clip-plane quadwords at pMat into $vf12-$vf19, where the
 * _ModelCalcClip* routines below expect them. */
void _ModelCalcClipInit(void *pMat)
{
    PS2_ASM(".set noreorder\n"
            "lqc2 $vf12, 0x0(%0)\n"
            "lqc2 $vf13, 0x10(%0)\n"
            "lqc2 $vf14, 0x20(%0)\n"
            "lqc2 $vf15, 0x30(%0)\n"
            "lqc2 $vf16, 0x40(%0)\n"
            "lqc2 $vf17, 0x50(%0)\n"
            "lqc2 $vf18, 0x60(%0)\n"
            "lqc2 $vf19, 0x70(%0)\n"
            ".set reorder" : : "r"(pMat));
}

/* pDst = 3x3 part of pMat * pSrc (no translation). */
void _ApplyMatrix33(void *pDst, void *pMat, void *pSrc)
{
    PS2_ASM(".set noreorder\n"
            "lqc2 $vf31, 0x0(%2)\n"
            "lqc2 $vf27, 0x0(%1)\n"
            "lqc2 $vf28, 0x10(%1)\n"
            "lqc2 $vf29, 0x20(%1)\n"
            "lqc2 $vf30, 0x30(%1)\n"
            "vmulax.xyz $ACC, $vf27, $vf31x\n"
            "vmadday.xyz $ACC, $vf28, $vf31y\n"
            "vmaddz.xyz $vf31, $vf29, $vf31z\n"
            "sqc2 $vf31, 0x0(%0)\n"
            ".set reorder"
            : : "r"(pDst), "r"(pMat), "r"(pSrc) : "memory");
}

/* pDst = pMat * pSrc, translation included. */
void _ApplyMatrix(void *pDst, void *pMat, void *pSrc)
{
    PS2_ASM(".set noreorder\n"
            "lqc2 $vf31, 0x0(%2)\n"
            "lqc2 $vf27, 0x0(%1)\n"
            "lqc2 $vf28, 0x10(%1)\n"
            "lqc2 $vf29, 0x20(%1)\n"
            "lqc2 $vf30, 0x30(%1)\n"
            "vmulax.xyz $ACC, $vf27, $vf31x\n"
            "vmadday.xyz $ACC, $vf28, $vf31y\n"
            "vmaddaz.xyz $ACC, $vf29, $vf31z\n"
            "vmaddw.xyz $vf31, $vf30, $vf0w\n"
            "sqc2 $vf31, 0x0(%0)\n"
            ".set reorder"
            : : "r"(pDst), "r"(pMat), "r"(pSrc) : "memory");
}

/* pDst = pMat2 * pMat1 * pSrc. */
void _ApplyMatrix2Mat(void *pDst, void *pMat1, void *pMat2, void *pSrc)
{
    PS2_ASM(".set noreorder\n"
            "lqc2 $vf31, 0x0(%3)\n"
            "lqc2 $vf27, 0x0(%1)\n"
            "lqc2 $vf28, 0x10(%1)\n"
            "lqc2 $vf29, 0x20(%1)\n"
            "lqc2 $vf30, 0x30(%1)\n"
            "vmulax.xyz $ACC, $vf27, $vf31x\n"
            "vmadday.xyz $ACC, $vf28, $vf31y\n"
            "vmaddaz.xyz $ACC, $vf29, $vf31z\n"
            "vmaddw.xyz $vf31, $vf30, $vf0w\n"
            "lqc2 $vf27, 0x0(%2)\n"
            "lqc2 $vf28, 0x10(%2)\n"
            "lqc2 $vf29, 0x20(%2)\n"
            "lqc2 $vf30, 0x30(%2)\n"
            "vmulax.xyz $ACC, $vf27, $vf31x\n"
            "vmadday.xyz $ACC, $vf28, $vf31y\n"
            "vmaddaz.xyz $ACC, $vf29, $vf31z\n"
            "vmaddw.xyz $vf31, $vf30, $vf0w\n"
            "sqc2 $vf31, 0x0(%0)\n"
            ".set reorder"
            : : "r"(pDst), "r"(pMat1), "r"(pMat2), "r"(pSrc) : "memory");
}

/* Intersection of the line pP0->pP1 with the plane pPlane (pPlane->w is the
 * plane offset); pOut receives the intersection point.  Whole-body VU0. */
void _FacePoint(void *pOut, void *pP0, void *pP1, void *pPlane, void *pTmp)
{
    PS2_ASM(".set noreorder\n"
            "lqc2 $vf22, 0(%3)\n"
            "lqc2 $vf20, 0(%1)\n"
            "lqc2 $vf21, 0(%2)\n"
            "lqc2 $vf23, 0(%4)\n"
            "vmulw.xyz $vf14, $vf22, $vf22w\n"
            "vmulw.xyz $vf14, $vf14, $vf20w\n"
            "vadd.xyz $vf23, $vf23, $vf14\n"
            "vaddx.x $vf10, $vf0, $vf22\n"
            "vaddy.x $vf11, $vf0, $vf22y\n"
            "vaddz.x $vf12, $vf0, $vf22z\n"
            "vsub.xyz $vf19, $vf21, $vf20\n"
            "vmulax.x $ACC, $vf10, $vf21\n"
            "vmadday.x $ACC, $vf11, $vf21y\n"
            "vmaddz.x $vf17, $vf12, $vf21z\n"
            "vmulax.x $ACC, $vf10, $vf19\n"
            "vmadday.x $ACC, $vf11, $vf19y\n"
            "vmaddz.x $vf18, $vf12, $vf19z\n"
            "vmulax.x $ACC, $vf10, $vf23\n"
            "vmadday.x $ACC, $vf11, $vf23y\n"
            "vmaddz.x $vf16, $vf12, $vf23z\n"
            "vdiv $Q, $vf0w, $vf18x\n"
            "vsub.x $vf16, $vf17, $vf16\n"
            "vwaitq\n"
            "vmulq.x $vf16, $vf16, $Q\n"
            "vmulx.xyz $vf15, $vf19, $vf16x\n"
            "vsub.xyz $vf15, $vf21, $vf15\n"
            "sqc2 $vf15, 0(%0)\n"
            ".set reorder"
            : : "r"(pOut), "r"(pP0), "r"(pP1), "r"(pPlane), "r"(pTmp)
            : "memory");
}

/* Clip a point against the six frustum planes held in $vf12-$vf19 by
 * _ModelCalcClipInit.  Returns 1 when the point is inside all of them. */
int _ModelCalcClip(void *pPos)
{
    int nRet;
    PS2_ASM(".set noreorder\n"
            "lqc2 $vf10, 0(%1)\n"
            "vaddw.x $vf2, $vf0, $vf10w\n"
            "qmfc2 $2, $vf2\n"
            "mtc1 $2, $f1\n"
            "vmulax.xyzw $ACC, $vf16, $vf10x\n"
            "vmadday.xyzw $ACC, $vf17, $vf10y\n"
            "vmaddaz.xyzw $ACC, $vf18, $vf10z\n"
            "vmaddw.xyzw $vf9, $vf19, $vf0w\n"
            "qmfc2 $2, $vf9\n"
            "mtc1 $2, $f0\n"
            "nop\n"
            "c.lt.s $f0, $f1\n"
            "nop\n"
            "bc1f _$mcc_set_end\n"
            "nop\n"
            "vmr32.xyzw $vf9, $vf9\n"
            "qmfc2 $2, $vf9\n"
            "mtc1 $2, $f0\n"
            "nop\n"
            "c.lt.s $f0, $f1\n"
            "nop\n"
            "bc1f _$mcc_set_end\n"
            "nop\n"
            "vmr32.xyzw $vf9, $vf9\n"
            "qmfc2 $2, $vf9\n"
            "mtc1 $2, $f0\n"
            "nop\n"
            "c.lt.s $f0, $f1\n"
            "nop\n"
            "bc1f _$mcc_set_end\n"
            "nop\n"
            "vmulax.xyzw $ACC, $vf12, $vf10x\n"
            "vmadday.xyzw $ACC, $vf13, $vf10y\n"
            "vmaddaz.xyzw $ACC, $vf14, $vf10z\n"
            "vmaddw.xyzw $vf9, $vf15, $vf0w\n"
            "qmfc2 $2, $vf9\n"
            "mtc1 $2, $f0\n"
            "nop\n"
            "c.lt.s $f0, $f1\n"
            "nop\n"
            "bc1f _$mcc_set_end\n"
            "nop\n"
            "vmr32.xyzw $vf9, $vf9\n"
            "qmfc2 $2, $vf9\n"
            "mtc1 $2, $f0\n"
            "nop\n"
            "c.lt.s $f0, $f1\n"
            "nop\n"
            "bc1f _$mcc_set_end\n"
            "nop\n"
            "vmr32.xyzw $vf9, $vf9\n"
            "qmfc2 $2, $vf9\n"
            "mtc1 $2, $f0\n"
            "nop\n"
            "c.lt.s $f0, $f1\n"
            "nop\n"
            "bc1f _$mcc_set_end\n"
            "nop\n"
            "sub $2, $2, $2\n"
            ".globl _$mcc_exit\n"
            "j _$mcc_exit\n"
            "nop\n"
            "_$mcc_set_end:\n"
            "sub $2, $2, $2\n"
            "addi $2, $2, 1\n"
            "_$mcc_exit:\n"
            "daddu %0, $2, $0\n"
            ".set reorder"
            : "=r"(nRet) : "r"(pPos) : "$2", "$f0", "$f1");
    return nRet;
}

/* As _ModelCalcClip, with the point first transformed by pMat (rotation
 * rows w-cleared). */
int _ModelCalcClipMat1(void *pPos, void *pMat)
{
    int nRet;
    PS2_ASM(".set noreorder\n"
            "lqc2 $vf10, 0(%1)\n"
            "lqc2 $vf27, 0(%2)\n"
            "lqc2 $vf28, 16(%2)\n"
            "lqc2 $vf29, 32(%2)\n"
            "lqc2 $vf30, 48(%2)\n"
            "vsubw.w $vf27, $vf27, $vf27\n"
            "vsubw.w $vf28, $vf28, $vf28\n"
            "vsubw.w $vf29, $vf29, $vf29\n"
            "vmulax.xyzw $ACC, $vf27, $vf10x\n"
            "vmadday.xyzw $ACC, $vf28, $vf10y\n"
            "vmaddaz.xyzw $ACC, $vf29, $vf10z\n"
            "vmaddw.xyzw $vf20, $vf30, $vf0w\n"
            "vaddw.x $vf2, $vf0, $vf10w\n"
            "qmfc2 $2, $vf2\n"
            "mtc1 $2, $f1\n"
            "vmulax.xyzw $ACC, $vf16, $vf20x\n"
            "vmadday.xyzw $ACC, $vf17, $vf20y\n"
            "vmaddaz.xyzw $ACC, $vf18, $vf20z\n"
            "vmaddw.xyzw $vf9, $vf19, $vf0w\n"
            "qmfc2 $2, $vf9\n"
            "mtc1 $2, $f0\n"
            "nop\n"
            "c.lt.s $f0, $f1\n"
            "nop\n"
            "bc1f _$mcc1_set_end\n"
            "nop\n"
            "vmr32.xyzw $vf9, $vf9\n"
            "qmfc2 $2, $vf9\n"
            "mtc1 $2, $f0\n"
            "nop\n"
            "c.lt.s $f0, $f1\n"
            "nop\n"
            "bc1f _$mcc1_set_end\n"
            "nop\n"
            "vmr32.xyzw $vf9, $vf9\n"
            "qmfc2 $2, $vf9\n"
            "mtc1 $2, $f0\n"
            "nop\n"
            "c.lt.s $f0, $f1\n"
            "nop\n"
            "bc1f _$mcc1_set_end\n"
            "nop\n"
            "vmulax.xyzw $ACC, $vf12, $vf20x\n"
            "vmadday.xyzw $ACC, $vf13, $vf20y\n"
            "vmaddaz.xyzw $ACC, $vf14, $vf20z\n"
            "vmaddw.xyzw $vf9, $vf15, $vf0w\n"
            "qmfc2 $2, $vf9\n"
            "mtc1 $2, $f0\n"
            "nop\n"
            "c.lt.s $f0, $f1\n"
            "nop\n"
            "bc1f _$mcc1_set_end\n"
            "nop\n"
            "vmr32.xyzw $vf9, $vf9\n"
            "qmfc2 $2, $vf9\n"
            "mtc1 $2, $f0\n"
            "nop\n"
            "c.lt.s $f0, $f1\n"
            "nop\n"
            "bc1f _$mcc1_set_end\n"
            "nop\n"
            "vmr32.xyzw $vf9, $vf9\n"
            "qmfc2 $2, $vf9\n"
            "mtc1 $2, $f0\n"
            "nop\n"
            "c.lt.s $f0, $f1\n"
            "nop\n"
            "bc1f _$mcc1_set_end\n"
            "nop\n"
            "sub $2, $2, $2\n"
            ".globl _$mcc1_exit\n"
            "j _$mcc1_exit\n"
            "nop\n"
            "_$mcc1_set_end:\n"
            "sub $2, $2, $2\n"
            "addi $2, $2, 1\n"
            "_$mcc1_exit:\n"
            "daddu %0, $2, $0\n"
            ".set reorder"
            : "=r"(nRet) : "r"(pPos), "r"(pMat) : "$2", "$f0", "$f1");
    return nRet;
}

/* As _ModelCalcClipMat1, through the concatenation of pMat1 and pMat2. */
int _ModelCalcClipMat2(void *pPos, void *pMat1, void *pMat2)
{
    int nRet;
    PS2_ASM(".set noreorder\n"
            "lqc2 $vf10, 0(%1)\n"
            "lqc2 $vf27, 0(%2)\n"
            "lqc2 $vf28, 16(%2)\n"
            "lqc2 $vf29, 32(%2)\n"
            "lqc2 $vf30, 48(%2)\n"
            "vsubw.w $vf27, $vf27, $vf27\n"
            "vsubw.w $vf28, $vf28, $vf28\n"
            "vsubw.w $vf29, $vf29, $vf29\n"
            "lqc2 $vf20, 0(%3)\n"
            "lqc2 $vf21, 16(%3)\n"
            "lqc2 $vf22, 32(%3)\n"
            "lqc2 $vf23, 48(%3)\n"
            "vsubw.w $vf20, $vf20, $vf20\n"
            "vsubw.w $vf21, $vf21, $vf21\n"
            "vsubw.w $vf22, $vf22, $vf22\n"
            "vmulax.xyzw $ACC, $vf27, $vf20x\n"
            "vmadday.xyzw $ACC, $vf28, $vf20y\n"
            "vmaddaz.xyzw $ACC, $vf29, $vf20z\n"
            "vmaddw.xyzw $vf20, $vf30, $vf20w\n"
            "vmulax.xyzw $ACC, $vf27, $vf21x\n"
            "vmadday.xyzw $ACC, $vf28, $vf21y\n"
            "vmaddaz.xyzw $ACC, $vf29, $vf21z\n"
            "vmaddw.xyzw $vf21, $vf30, $vf21w\n"
            "vmulax.xyzw $ACC, $vf27, $vf22x\n"
            "vmadday.xyzw $ACC, $vf28, $vf22y\n"
            "vmaddaz.xyzw $ACC, $vf29, $vf22z\n"
            "vmaddw.xyzw $vf22, $vf30, $vf22w\n"
            "vmulax.xyzw $ACC, $vf27, $vf23x\n"
            "vmove.xyzw $vf27, $vf20\n"
            "vmadday.xyzw $ACC, $vf28, $vf23y\n"
            "vmove.xyzw $vf28, $vf21\n"
            "vmaddaz.xyzw $ACC, $vf29, $vf23z\n"
            "vmove.xyzw $vf29, $vf22\n"
            "vmaddw.xyzw $vf30, $vf30, $vf23w\n"
            "vmulax.xyzw $ACC, $vf27, $vf10x\n"
            "vmadday.xyzw $ACC, $vf28, $vf10y\n"
            "vmaddaz.xyzw $ACC, $vf29, $vf10z\n"
            "vmaddw.xyzw $vf20, $vf30, $vf0w\n"
            "vaddw.x $vf2, $vf0, $vf10w\n"
            "qmfc2 $2, $vf2\n"
            "mtc1 $2, $f1\n"
            "vmulax.xyzw $ACC, $vf16, $vf20x\n"
            "vmadday.xyzw $ACC, $vf17, $vf20y\n"
            "vmaddaz.xyzw $ACC, $vf18, $vf20z\n"
            "vmaddw.xyzw $vf9, $vf19, $vf0w\n"
            "qmfc2 $2, $vf9\n"
            "mtc1 $2, $f0\n"
            "nop\n"
            "c.lt.s $f0, $f1\n"
            "nop\n"
            "bc1f _$mcc2_set_end\n"
            "nop\n"
            "vmr32.xyzw $vf9, $vf9\n"
            "qmfc2 $2, $vf9\n"
            "mtc1 $2, $f0\n"
            "nop\n"
            "c.lt.s $f0, $f1\n"
            "nop\n"
            "bc1f _$mcc2_set_end\n"
            "nop\n"
            "vmr32.xyzw $vf9, $vf9\n"
            "qmfc2 $2, $vf9\n"
            "mtc1 $2, $f0\n"
            "nop\n"
            "c.lt.s $f0, $f1\n"
            "nop\n"
            "bc1f _$mcc2_set_end\n"
            "nop\n"
            "vmulax.xyzw $ACC, $vf12, $vf20x\n"
            "vmadday.xyzw $ACC, $vf13, $vf20y\n"
            "vmaddaz.xyzw $ACC, $vf14, $vf20z\n"
            "vmaddw.xyzw $vf9, $vf15, $vf0w\n"
            "qmfc2 $2, $vf9\n"
            "mtc1 $2, $f0\n"
            "nop\n"
            "c.lt.s $f0, $f1\n"
            "nop\n"
            "bc1f _$mcc2_set_end\n"
            "nop\n"
            "vmr32.xyzw $vf9, $vf9\n"
            "qmfc2 $2, $vf9\n"
            "mtc1 $2, $f0\n"
            "nop\n"
            "c.lt.s $f0, $f1\n"
            "nop\n"
            "bc1f _$mcc2_set_end\n"
            "nop\n"
            "vmr32.xyzw $vf9, $vf9\n"
            "qmfc2 $2, $vf9\n"
            "mtc1 $2, $f0\n"
            "nop\n"
            "c.lt.s $f0, $f1\n"
            "nop\n"
            "bc1f _$mcc2_set_end\n"
            "nop\n"
            "sub $2, $2, $2\n"
            ".globl _$mcc2_exit\n"
            "j _$mcc2_exit\n"
            "nop\n"
            "_$mcc2_set_end:\n"
            "sub $2, $2, $2\n"
            "addi $2, $2, 1\n"
            "_$mcc2_exit:\n"
            "daddu %0, $2, $0\n"
            ".set reorder"
            : "=r"(nRet) : "r"(pPos), "r"(pMat1), "r"(pMat2) : "$2", "$f0", "$f1");
    return nRet;
}

/* Occlusion volume: five side planes plus a near distance, built by
 * setup_occlusion from the camera. */
typedef struct {
    VEC4 vTrans;            /* 0x00 */
    VEC4 vAngle;            /* 0x10 */
    VEC4 vScale;            /* 0x20 */
    VEC4 vDir;              /* 0x30 */
    VEC4 aMtx[4];           /* 0x40 */
    VEC4 aMtxInv[4];        /* 0x80 */
    VEC4 aCorner[4];        /* 0xC0 */
    float aPlane[5][4];     /* 0x100 */
    float fClip;            /* 0x150 */
} CULLCELL;

/* True when pPos is inside the occlusion volume pOcc (pCam+0x1B0 is the
 * camera's view matrix). */
int check_occlusion(CULLCELL *pOcc, void *pCam, void *pPos)
{
    VEC4 v;
    float fZ, fNW;
    int nRet;

    PS2_ASM(".set noreorder\n"
            "lqc2 $vf31, 0x0(%2)\n"
            "lqc2 $vf27, 0x0(%1)\n"
            "lqc2 $vf28, 0x10(%1)\n"
            "lqc2 $vf29, 0x20(%1)\n"
            "lqc2 $vf30, 0x30(%1)\n"
            "vmulax.xyz $ACC, $vf27, $vf31x\n"
            "vmadday.xyz $ACC, $vf28, $vf31y\n"
            "vmaddaz.xyz $ACC, $vf29, $vf31z\n"
            "vmaddw.xyz $vf31, $vf30, $vf0w\n"
            "sqc2 $vf31, 0x0(%0)\n"
            ".set reorder"
            : : "r"(&v), "r"((char *)pCam + 0x1B0), "r"(pPos) : "memory");

    nRet = 0;
    fZ = -v.f[2];
    v.f[2] = fZ;
    if (fZ - v.f[3] < pOcc->fClip) goto done;
    fNW = -v.f[3];
    if (fNW < v.f[0] * pOcc->aPlane[0][0] + v.f[1] * pOcc->aPlane[0][1]
          + fZ * pOcc->aPlane[0][2] + pOcc->aPlane[0][3]) goto done;
    if (fNW < v.f[0] * pOcc->aPlane[1][0] + v.f[1] * pOcc->aPlane[1][1]
          + fZ * pOcc->aPlane[1][2] + pOcc->aPlane[1][3]) goto done;
    if (fNW < v.f[0] * pOcc->aPlane[2][0] + v.f[1] * pOcc->aPlane[2][1]
          + fZ * pOcc->aPlane[2][2] + pOcc->aPlane[2][3]) goto done;
    if (fNW < v.f[0] * pOcc->aPlane[3][0] + v.f[1] * pOcc->aPlane[3][1]
          + fZ * pOcc->aPlane[3][2] + pOcc->aPlane[3][3]) goto done;
    if (fNW < v.f[0] * pOcc->aPlane[4][0] + v.f[1] * pOcc->aPlane[4][1]
          + fZ * pOcc->aPlane[4][2] + pOcc->aPlane[4][3]) goto done;
    nRet = 1;
done:
    return nRet;
}

extern void xglVectorOuter(void *pDest, void *pA, void *pB);
extern void xglVectorNormal(void *pDest, void *pSource);

/* Plane through the three points pP0/pP1/pP2: pPlane gets the unit normal
 * in xyz and the plane offset in w. */
void plane_from_points(float *pP0, float *pP1, float *pP2, float *pPlane)
{
    VEC4 vA;
    VEC4 vB;
    VEC4 vN;

    vA.f[0] = pP1[0] - pP0[0];
    vA.f[1] = pP1[1] - pP0[1];
    vA.f[2] = pP1[2] - pP0[2];
    vB.f[0] = pP2[0] - pP0[0];
    vB.f[1] = pP2[1] - pP0[1];
    vB.f[2] = pP2[2] - pP0[2];
    xglVectorOuter(&vN, &vA, &vB);
    xglVectorNormal(&vN, &vN);
    pPlane[0] = vN.f[0];
    pPlane[1] = vN.f[1];
    pPlane[2] = vN.f[2];
    pPlane[3] = -(vN.f[0] * pP0[0] + vN.f[1] * pP0[1] + vN.f[2] * pP0[2]);
}


#define VEC_SUB(d, a, b)                                        \
    PS2_ASM(".set noreorder\n"                                  \
            "lqc2 $vf3, 0x0(%2)\n"                              \
            "lqc2 $vf2, 0x0(%1)\n"                              \
            "vsub.xyz $vf2, $vf2, $vf3\n"                       \
            "sqc2 $vf2, 0x0(%0)\n"                              \
            ".set reorder"                                      \
            : : "r"(d), "r"(a), "r"(b) : "memory")

#define VEC_ADD(d, a, b)                                        \
    PS2_ASM(".set noreorder\n"                                  \
            "lqc2 $vf3, 0x0(%2)\n"                              \
            "lqc2 $vf2, 0x0(%1)\n"                              \
            "vadd.xyz $vf2, $vf2, $vf3\n"                       \
            "sqc2 $vf2, 0x0(%0)\n"                              \
            ".set reorder"                                      \
            : : "r"(d), "r"(a), "r"(b) : "memory")

/* Dot product of the two edge vectors of the swept-sphere segment
 * pP0->pP1 shortened by the two radii; negative when the segment points
 * away from pSphere. */
float _CheckLine(float *pV0, float *pV1, float *pP0, float *pP1,
                 float *pSphere, float *pDir)
{
    VEC4 vA;
    VEC4 vB;
    VEC4 vC;
    VEC4 vD;
    VEC4 vE;

    xglVectorScaleXYZ(&vE, pDir, pDir[3] * pSphere[3] * 0.5f);
    xglVectorScaleXYZ(&vA, pV0, pSphere[3] * 0.5f);
    xglVectorScaleXYZ(&vB, pV1, pSphere[3] * 0.5f);
    VEC_SUB(&vA, pP0, &vA);
    VEC_SUB(&vB, pP1, &vB);
    VEC_ADD(&vA, &vA, &vE);
    VEC_ADD(&vB, &vB, &vE);
    VEC_SUB(&vC, &vB, &vA);
    VEC_SUB(&vD, pSphere, &vA);
    return vC.f[0] * vD.f[0] + vC.f[1] * vD.f[1] + vC.f[2] * vD.f[2];
}

void xglMatrixStackUnit(void);
void xglMatrixStackTrans(void *pVec);
void xglMatrixStackRotX(float fAngle);
void xglMatrixStackRotY(float fAngle);
void xglMatrixStackRotZ(float fAngle);
void xglMatrixStackScale(void *pVec);
void xglMatrixStackSave(void *pMtx);
void xglMatrixStackInverse(void);

#define VEC_ZERO4(d)                                            \
    PS2_ASM("sq $0, 0x0(%0)\n"                                  \
            "sq $0, 0x10(%0)\n"                                 \
            "sq $0, 0x20(%0)\n"                                 \
            "sq $0, 0x30(%0)" : : "r"(d) : "memory")

#define VEC_COPY(d, s)                                          \
    PS2_ASM("lq $2, 0x0(%1)\n"                                  \
            "sq $2, 0x0(%0)" : : "r"(d), "r"(s) : "$2", "memory")

#define VEC_ZERO(d)                                             \
    PS2_ASM(".set noreorder\n"                                  \
            "sq $0, 0x0(%0)\n"                                  \
            ".set reorder" : : "r"(d) : "memory")

#define APPLY_MATRIX33(d, m, s)                                 \
    PS2_ASM(".set noreorder\n"                                  \
            "lqc2 $vf31, 0x0(%2)\n"                             \
            "lqc2 $vf27, 0x0(%1)\n"                             \
            "lqc2 $vf28, 0x10(%1)\n"                            \
            "lqc2 $vf29, 0x20(%1)\n"                            \
            "lqc2 $vf30, 0x30(%1)\n"                            \
            "vmulax.xyz $ACC, $vf27, $vf31x\n"                  \
            "vmadday.xyz $ACC, $vf28, $vf31y\n"                 \
            "vmaddz.xyz $vf31, $vf29, $vf31z\n"                 \
            "sqc2 $vf31, 0x0(%0)\n"                             \
            ".set reorder"                                      \
            : : "r"(d), "r"(m), "r"(s) : "memory")

#define APPLY_MATRIX(d, m, s)                                   \
    PS2_ASM(".set noreorder\n"                                  \
            "lqc2 $vf31, 0x0(%2)\n"                             \
            "lqc2 $vf27, 0x0(%1)\n"                             \
            "lqc2 $vf28, 0x10(%1)\n"                            \
            "lqc2 $vf29, 0x20(%1)\n"                            \
            "lqc2 $vf30, 0x30(%1)\n"                            \
            "vmulax.xyz $ACC, $vf27, $vf31x\n"                  \
            "vmadday.xyz $ACC, $vf28, $vf31y\n"                 \
            "vmaddaz.xyz $ACC, $vf29, $vf31z\n"                 \
            "vmaddw.xyz $vf31, $vf30, $vf0w\n"                  \
            "sqc2 $vf31, 0x0(%0)\n"                             \
            ".set reorder"                                      \
            : : "r"(d), "r"(m), "r"(s) : "memory")

/* Five separate objects, not an array: retail rematerialises a %hi/%lo
 * pair for each one, which an array base held in a register would not. */
static const VEC4 s_vCullFront = {{ 0.0f,  0.0f, 1.0f, 1.0f}};
static const VEC4 s_vCullLT    = {{-1.0f, -1.0f, 0.0f, 1.0f}};
static const VEC4 s_vCullRT    = {{ 1.0f, -1.0f, 0.0f, 1.0f}};
static const VEC4 s_vCullRB    = {{ 1.0f,  1.0f, 0.0f, 1.0f}};
static const VEC4 s_vCullLB    = {{-1.0f,  1.0f, 0.0f, 1.0f}};

/* Build a cell's local matrix, its inverse, its facing direction and the
 * four corner directions of its view volume. */
void culling_matrix(CULLCELL *pCell)
{
    VEC4 v0;
    VEC4 v1;
    VEC4 v2;
    VEC4 v3;
    VEC4 v4;

    xglMatrixStackUnit();
    xglMatrixStackTrans(&pCell->vTrans);
    xglMatrixStackRotY(pCell->vAngle.f[1]);
    xglMatrixStackRotX(pCell->vAngle.f[0]);
    xglMatrixStackRotZ(pCell->vAngle.f[2]);
    xglMatrixStackScale(&pCell->vScale);
    xglMatrixStackSave(pCell->aMtx);
    xglMatrixStackInverse();
    xglMatrixStackSave(pCell->aMtxInv);

    v0 = s_vCullFront;
    APPLY_MATRIX33(&pCell->vDir, pCell->aMtx, &v0);
    xglVectorNormal(&pCell->vDir, &pCell->vDir);

    v1 = s_vCullLT;
    v2 = s_vCullRT;
    v3 = s_vCullRB;
    v4 = s_vCullLB;
    APPLY_MATRIX(&pCell->aCorner[0], pCell->aMtx, &v1);
    APPLY_MATRIX(&pCell->aCorner[1], pCell->aMtx, &v2);
    APPLY_MATRIX(&pCell->aCorner[2], pCell->aMtx, &v3);
    APPLY_MATRIX(&pCell->aCorner[3], pCell->aMtx, &v4);
}

/* Build the cell's five clipping planes and near distance from its four
 * corner directions, seen from the camera pCam. */
void setup_occlusion(CULLCELL *pCell, void *pCam)
{
    VEC4 v0;
    VEC4 v1;
    VEC4 v2;
    VEC4 v3;
    VEC4 vZero;
    float *pPlane;

    APPLY_MATRIX(&v0, (char *)pCam + 0x1B0, &pCell->aCorner[0]);
    APPLY_MATRIX(&v1, (char *)pCam + 0x1B0, &pCell->aCorner[1]);
    APPLY_MATRIX(&v2, (char *)pCam + 0x1B0, &pCell->aCorner[2]);
    APPLY_MATRIX(&v3, (char *)pCam + 0x1B0, &pCell->aCorner[3]);

    v0.f[2] = -v0.f[2];
    v1.f[2] = -v1.f[2];
    v2.f[2] = -v2.f[2];
    v3.f[2] = -v3.f[2];
    pCell->fClip = v0.f[2];
    if (v1.f[2] < pCell->fClip) pCell->fClip = v1.f[2];
    if (v2.f[2] < pCell->fClip) pCell->fClip = v2.f[2];
    if (v3.f[2] < pCell->fClip) pCell->fClip = v3.f[2];

    VEC_ZERO(&vZero);
    pPlane = pCell->aPlane[0];
    plane_from_points(v0.f, v1.f, v2.f, pPlane);
    if (pCell->aPlane[0][3] > 0.0f) {
        plane_from_points(vZero.f, v0.f, v1.f, pCell->aPlane[1]);
        plane_from_points(vZero.f, v1.f, v2.f, pCell->aPlane[2]);
        plane_from_points(vZero.f, v2.f, v3.f, pCell->aPlane[3]);
        plane_from_points(vZero.f, v3.f, v0.f, pCell->aPlane[4]);
    } else {
        plane_from_points(v2.f, v1.f, v0.f, pPlane);
        plane_from_points(vZero.f, v1.f, v0.f, pCell->aPlane[1]);
        plane_from_points(vZero.f, v2.f, v1.f, pCell->aPlane[2]);
        plane_from_points(vZero.f, v3.f, v2.f, pCell->aPlane[3]);
        plane_from_points(vZero.f, v0.f, v3.f, pCell->aPlane[4]);
    }
}

/* One entry of the parent-buffer list at s_aParentBuf. */
typedef struct {
    void *pOwner;
    int nUnk04;
    int nUnk08;
    int nUnk0C;
} PARENT_BUF;

extern PARENT_BUF s_aParentBuf[];

/* The shadow-map parts list carried by a model layout. */
typedef struct {
    char pad000[0x278];
    int nNum;               /* 0x278 */
    char pad27C[0x64];
    unsigned short aParts[24];  /* 0x2E0 */
} SHADOW_PARTS;

/* True when nParts is one of the parts registered for the shadow map on
 * this layout. */
int is_shadow_map_parts(SHADOW_PARTS *pLayout, int nParts)
{
    int nNum = pLayout->nNum;
    int nRet = 0;
    int i;

    if (nNum != 0) {
        for (i = 0; i < nNum; i++) {
            if (pLayout->aParts[i] == nParts) {
                nRet = 1;
                break;
            }
        }
    }
    return nRet;
}

/* True when this model's parts id is in the transparency list. */
int is_parts_transparency(void *pModel)
{
    int nRet = 0;
    int i;

    for (i = 0; i < s_nToumeiNum; i++) {
        if (s_aToumeiId[i] == *(int *)((char *)pModel + 0x2C)) {
            nRet = 1;
            break;
        }
    }
    return nRet;
}

/* True when this model's parts id is registered as a map last-entry. */
int is_block_last_entry(void *pModel)
{
    int nRet = 0;
    int i;

    for (i = 0; i < s_nMapLast; i++) {
        if (*(int *)((char *)pModel + 0x2C) == s_aMapLast[i]) {
            nRet = 1;
            break;
        }
    }
    return nRet;
}

/* Take the next free parent-buffer entry, or NULL when the list is full.
 *
 * TODO: near-miss, 4 words SHORT, NOT registered. Retail re-loads
 * s_nParentBuf after the first store and recomputes the entry address a
 * second time (with the two `addu` operands in the opposite order), i.e.
 * its CSE treated the store through the entry pointer as possibly
 * aliasing the counter. Swept: one pointer local, two pointer locals,
 * indexed `s_aParentBuf[s_nParentBuf].f` for every store, the increment
 * before/after the last store, and `s_nParentBuf = s_nParentBuf + 1`
 * spelled out -- every form lets gcc 2.96 prove non-aliasing and CSE both
 * the load and the address, so the function comes out 15 words instead of
 * 19. No source spelling found that defeats it. */
PARENT_BUF *parent_buf_entry(void)
{
    PARENT_BUF *p = 0;

    if (s_nParentBuf < 512) {
        p = &s_aParentBuf[s_nParentBuf];
        p->pOwner = 0;
        s_aParentBuf[s_nParentBuf].nUnk08 = 0;
        s_aParentBuf[s_nParentBuf].nUnk0C = 0;
        s_nParentBuf++;
    }
    return p;
}

int s_nCount;
int s_nModel;
int s_nBlocks;
int s_nDirect;
int s_nBlocksAlpha;
int s_nBlocksAlphaLast;
int s_nAnotherStudio;
int s_nUseStealth;
int s_nUseGnosys;
int s_nUseZwrite;
int s_nMapAlphaEntry;
int s_nMainCameraWarp;
int s_nRenderCancel;
VEC4 s_inGblPointC[4];
VEC4 s_inMainCameraPos;
VEC4 s_inMainCameraAng;

void *xglStudioSelectGetActiveCamera(int nWindow);

/* Find the parent-buffer entry whose owner points at pParent. */
PARENT_BUF *parent_buf_search(void *pParent)
{
    PARENT_BUF *pRet = 0;
    int i;

    for (i = s_nParentBuf - 1; i >= 0; i--) {
        PARENT_BUF *p = &s_aParentBuf[i];
        if (p->pOwner != 0
            && *(void **)((char *)p->pOwner + 0x254) == pParent
            && pParent != 0) {
            pRet = p;
            break;
        }
    }
    return pRet;
}

/* One-time construction of the model system's global state. The original
 * scheduler separates the four independent g_aSubWindow stores around the
 * gp-relative block; configure.py restores that exact pure ordering. */
void CONSTRUCT_MODELSYSTEM(void)
{
    g_aSubWindow[0] = 2;
    g_aSubWindow[3] = 0;
    s_nEffectWrite = 1;
    s_nModel = 0;
    s_nBlocks = 1;
    s_nDirect = 0;
    s_nBlocksAlpha = 0;
    s_nBlocksAlphaLast = 0;
    s_nAnotherStudio = 0;
    s_nUseStealth = 0;
    s_nUseGnosys = 0;
    s_nUseZwrite = 0;
    s_nMapAlphaEntry = 0;
    s_nMapLast = 0;
    s_nMainCameraWarp = 0;
    s_nRenderCancel = 0;
    s_nRenderCancelOld = 0;
    s_nFrameLockOff = 0;
    s_nPause = 0;
    s_nMenu = 0;
    g_aSubWindow[2] = 0;
    g_aSubWindow[1] = 0;
    VEC_ZERO4(s_inGblPointC);
    VEC_ZERO(&s_inGblFogCol);
    VEC_ZERO(s_inGblFogPara);
}

/* Per-frame reset of the model system.
 *
 * TODO: near-miss, 3 diffs, NOT registered. The only difference is that
 * retail issues the final `s_nMainCameraWarp = 0` store BEFORE the
 * epilogue's `ld ra`, where gcc schedules the restore first. Swept:
 * statement order, `if (pCam)` vs `if (pCam != 0)`, struct-assignment vs
 * VEC_COPY for the two camera copies, and a held source pointer. */
void FLUSH_MODELSYSTEM(void)
{
    void *pCam;

    s_nPacketSignal++;
    s_nModel = 0;
    s_nBlocks = 1;
    s_nDirect = 0;
    s_nBlocksAlpha = 0;
    s_nBlocksAlphaLast = 0;
    s_nAnotherStudio = 0;
    s_nUseStealth = 0;
    s_nUseGnosys = 0;
    s_nUseZwrite = 0;
    s_nUseBackBuffer = 0;
    s_nMapAlphaEntry = 0;
    s_nFrameLockOff = 0;
    pCam = xglStudioSelectGetActiveCamera(0);
    if (pCam != 0) {
        VEC_COPY(&s_inMainCameraPos, (char *)pCam + 0xD0);
        VEC_COPY(&s_inMainCameraAng, (char *)pCam + 0xA0);
    }
    s_nMainCameraWarp = 0;
}

float s_fSortOffset;

/* One entry of the alpha-sorted block list. */
typedef struct {
    int nUnk00;
    float fDepth;
    int nUnk08;
    int nUnk0C;
} ALPHA_GROUP;

extern ALPHA_GROUP s_aAlphaGroup[];

void _CurSetMatrix(void *pMtx);
void _CurSetViewScaleTrans(void *pScale, void *pTrans);
float _VectorLengthSQ(void *pA, void *pB);
float tanf(float x);

/* Record the current model's sort depth in the alpha-group list. */
void AlphaGroupSortEntry(void *pModel)
{
    void *pCam = xglStudioSelectGetActiveCamera(s_inLayout.nWindow);
    float fLen = 0.0f;

    if (pCam != 0) {
        fLen = _VectorLengthSQ((char *)pCam + 208, pModel);
    }
    s_aAlphaGroup[s_nAlphaGroup].fDepth = fLen + s_fSortOffset;
}

/* Screen-space depth of pPos in the studio camera nWindow.
 *
 * TODO: near-miss, 3 words SHORT, NOT registered. Retail keeps TWO
 * callee-saved FP registers -- $f21 for the fScale argument and $f20 for
 * the 0.0 default -- and rematerialises the zero into $f20 after the
 * first call; gcc here keeps only fScale callee-saved and rematerialises
 * the zero straight into $f0, so the frame is 48 instead of 64 and the
 * two latency nops before the `div.s` vanish. Swept: the zero in the
 * declaration initialiser, assigned after the call, and hoisted into its
 * own named local (the constant-range lever) -- all three give the same
 * 37 words. */
float deapth_for_studio(int nWindow, void *pPos, float fScale)
{
    int aClip[4];
    float fRet = 0.0f;
    void *pCam = xglStudioSelectGetActiveCamera(nWindow);

    if (pCam != 0) {
        _CurSetMatrix((char *)pCam + 1136);
        _CurSetViewScaleTrans((char *)pCam + 128, (char *)pCam + 112);
        _CurRotTransPersClip(aClip, (float *)pPos);
        fRet = (float)aClip[2] * fScale
             / tanf(*(float *)((char *)pCam + 148) * 0.5f);
    }
    return fRet;
}
