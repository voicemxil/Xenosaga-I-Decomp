/* Normal-map model rendering global state accessors */

typedef struct {
    int nR;                 /* 0x00 */
    int nG;                 /* 0x04 */
    int nB;                 /* 0x08 */
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

typedef struct {
    char pad0[0x1D0];
    float aFogDist[20];     /* 0x1D0 */
    float fTransparency2;   /* 0x220 */
    float fReflTransparency;/* 0x224 */
    char pad228[0x8];
    union {
        float f;
        int n;
    } uAxis;                /* 0x230 */
    char pad234[0x4];
    int nPixelAlpha;        /* 0x238 */
    void *pMatrix;          /* 0x23C */
    void *pTexture;         /* 0x240 */
    int nTexfunc;           /* 0x244 */
    long nAlpha;            /* 0x248 */
    int nStatus;            /* 0x250 */
    char pad254[0x28];
    int nWindow;            /* 0x27C */
    char pad280[0x28];
    int nShadowHeightOn;    /* 0x2A8 */
    float fShadowHeight;    /* 0x2AC */
    int nStatus2;           /* 0x2B0 */
    int nPartsPixelAlphaNum;/* 0x2B4 */
    char pad2B8[0x8];
    float fTransparency;    /* 0x2C0 */
    char pad2C4[0xC];
    union {
        PIXEL_ALPHA aParts[4];
        int aInt[4];
    } uPartsPixelAlpha;     /* 0x2D0 */
    char pad2E0[0x10];
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

void *s_pMapLast;
int s_nToumeiNum;
int s_nMapShadowParts;
float s_fSortOffsetEntry;
int s_nParent;
int s_nClip;
int s_nShapeNum;
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

void INIT_BACK_BUFFER(void);
void nmlModelFogPara(float *pPara, float fNear, float fFar, float fMin, float fMax);

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

/* Set the MPEG2 cross-fade duration */
void nmlModelSetMpeg2CrossFadeTime(int time)
{
    s_inBackBuffer.nCrossFadeTime = time;
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

/* Set the fog distance parameters for the current model */
void nmlModelSetFogDist(float fNear, float fFar, float fMin, float fMax)
{
    nmlModelFogPara(s_inLayout.aFogDist, fNear, fFar, fMin, fMax);
    s_inLayout.nStatus |= 0x2;
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
