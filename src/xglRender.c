/* Frame rendering environment management for the xgl engine */

typedef unsigned int u_int;
typedef unsigned short u_short;
typedef unsigned long u_long;

typedef struct {
    u_short nUnk00;
    short nPsm;
    short nWidth;
    u_short nUnk06;
    u_short nUnk08;
    u_short nUnk0A;
    u_short nUnk0C;
    u_short nUnk0E;
    u_short nDispFbp;
    u_short nUnk12;
    u_short nDrawFbp;
    u_short nUnk16;
    u_short nUnk18;
    u_short nTexBase;
    u_short nUnk1C;
    u_short nUnk1E;
    u_short nFrontFbp;
    u_short nUnk22;
    u_int aUnk24[8];
    int nFade;
    int nUnk48;
    int nDrop;
    int nUnk50;
    int nUnk54;
} XGLRENDER;

typedef struct {
    u_long nTag0;
    u_long nTag1;
    u_long nTest;
    u_long nTestAddr;
    u_int nR;
    u_int nG;
    u_int nB;
    u_int nA;
    u_long aUnk30[6];
} XGLCLEARENV;

typedef struct {
    u_long nPMode;
    u_long aUnk08[15];
} XGLDISPENV;

typedef union {
    u_long l[32];
} XGLDRAWENV;

XGLRENDER sRender;
XGLCLEARENV ClearEnv;
XGLDISPENV DispEnv;
XGLDRAWENV DrawEnv;
int s_nClearFrame;
int s_nGblFadeInit;
int s_nGblFade;
volatile u_int VSyncCount;
char FLAG_FRAME_60;
int D_004A912C __attribute__((section(".data")));

void SyncDCache(void *pStart, void *pEnd);
void xglDmaDirectNormal(u_int nCh, u_int nAddr, u_int nQwc);
void sceGsSyncVCallback(int (*pFunc)(int));
int sceGsSyncV(int nMode);
void xglSleep(void);
void xglRenderInit(void);
void xglRenderMove(void);
void xglSendSePacket(void);
void xglPadRead(void);

/* Vertical-sync interrupt callback: count syncs and check DMA activity */
int xglRenderVSyncCallback(int nId)
{
    VSyncCount++;
    if (VSyncCount == 2) {
        if ((*(volatile u_int *)0x10003020 & 0xC00) || (*(volatile u_int *)0x10003C00 & 3)) {
            sRender.nUnk54 = 1;
        }
    }
    return 0;
}

/* Install the vsync callback and reset the sync counter */
void xglRenderSyncInit(void)
{
    sceGsSyncVCallback(xglRenderVSyncCallback);
    VSyncCount = 0;
}

/* Synchronize rendering to the current vertical-refresh mode */
void xglRenderSyncMove(void)
{
    if (FLAG_FRAME_60 == 1) {
        sceGsSyncV(0);
        return;
    }
    while (sceGsSyncV(0) != 0) {
    }
    /* TODO: Confirm the original source form of this polling delay. */
    while (({
        int waiting = VSyncCount < 2;
        __asm__("nop\n\tnop\n\tnop");
        waiting;
    })) {
    }
    if (VSyncCount >= 3) {
        D_004A912C = 1;
    }
    VSyncCount = 0;
}

/* Configure the clear packet to clear depth only */
void xglRenderClearDepth(void)
{
    ClearEnv.nTest = 0x32001;
}

/* Configure the clear packet to clear the frame buffer */
void xglRenderClearFrame(void)
{
    ClearEnv.nTest = 0x30003;
}

/* Set the frame clear color */
void xglRenderClearColor(u_int nColor)
{
    ClearEnv.nR = nColor & 0xFF;
    ClearEnv.nG = (nColor >> 8) & 0xFF;
    ClearEnv.nB = (nColor >> 16) & 0xFF;
    ClearEnv.nA = nColor >> 24;
}

/* Send the clear environment packet through DMA channel 2 */
void xglRenderClearEnvMove(void)
{
    SyncDCache(&ClearEnv, (char *)&ClearEnv + 0x60);
    xglDmaDirectNormal(2, (u_int)&ClearEnv, 6);
}

/* Rotate the display buffers and clear the per-frame swap state */
void xglRenderSwapBase(void)
{
    /* TODO: Find the natural source shape for this register allocation. */
    register u_short nTmp __asm__("$7");
    register u_short nFront __asm__("$6");
    register u_short nDisp __asm__("$3");
    register int nSum __asm__("$4");
    register int nDrop __asm__("$5");
    volatile XGLRENDER *pRender;

    pRender = &sRender;
    nSum = pRender->nUnk50;
    nDrop = pRender->nDrop;
    nTmp = pRender->nUnk22;
    nFront = pRender->nFrontFbp;
    nSum += nDrop;
    nDisp = pRender->nDispFbp;
    pRender->nFrontFbp = nTmp;
    pRender->nUnk12 = nDisp;
    pRender->nUnk22 = nFront;
    pRender->nUnk50 = nSum;
    pRender->nUnk54 = 0;
    pRender->nDispFbp = nTmp;
    pRender->nUnk48 = 0;
    pRender->nDrop = 0;
}

/* Hide the display circuit */
void xglRenderDispOff(void)
{
    DispEnv.nPMode &= ~2;
}

/* Show the display circuit */
void xglRenderDispOn(void)
{
    DispEnv.nPMode |= 2;
}

/* Reset the global fade state */
void xglRenderGlobalFadeInit(void)
{
    s_nGblFadeInit = 1;
    sRender.nFade = 0;
    s_nGblFade = 0;
}

/* Enable frame clearing for the next two frames */
void xglRenderClearOn(void)
{
    s_nClearFrame = 2;
}

/* Disable frame clearing */
void xglRenderClearOff(void)
{
    s_nClearFrame = 0;
}

/* Main render task entry point: init once, then loop forever */
void xglRenderEntry(void)
{
    xglSleep();
    xglRenderInit();
    for (;;) {
        xglRenderMove();
        xglSendSePacket();
        xglPadRead();
        xglSleep();
    }
}

typedef struct {
    char pad[0x30];
    u_int nR;
    u_int nG;
    u_int nB;
    u_int nA;
    char pad2[0x40];
} XGLTESTENV;

u_int *xglPacketGetCurrent(void);
void sceVif1PkRef(u_int *pPk, u_int nAddr, u_int nSize, u_int a3, u_int t0, u_int t1);

XGLTESTENV TestEnv;

/* Copy the current clear color into TestEnv and reference the packet
 * through the VIF1 packet builder */
void xglRenderClear(void)
{
    u_int *pPacket;

    TestEnv.nR = ClearEnv.nR;
    TestEnv.nG = ClearEnv.nG;
    TestEnv.nB = ClearEnv.nB;
    TestEnv.nA = 0x80;
    pPacket = xglPacketGetCurrent();
    sceVif1PkRef(pPacket, (u_int)&TestEnv, 6, 0, 0, 0);
}

void sceGsSetDefDispEnv(void *pEnv, int nPsm, int nWidth, int nMagic, u_int t0, u_int t1);

/* Initialize the display environment from the current render state */
void xglRenderDispEnvInit(void)
{
    u_long nPMode;

    sceGsSetDefDispEnv(&DispEnv, sRender.nPsm, sRender.nWidth, *(short *)&sRender.nUnk06, 0, 0);
    nPMode = DispEnv.nPMode;
    nPMode &= ~2;
    nPMode &= ~0x40;
    DispEnv.nPMode = nPMode;
    DispEnv.aUnk08[4] = DispEnv.aUnk08[1];
    DispEnv.aUnk08[5] = DispEnv.aUnk08[2];
    DispEnv.aUnk08[6] = 0;
    DispEnv.aUnk08[7] = 0;
    DispEnv.aUnk08[8] = 0;
}

void sceGsPutDrawEnv(void *pEnv);

/* Update the draw environment's frame/zbuf registers from the render
 * state and flush it to the GS */
void xglRenderDrawEnvMove(void)
{
    u_long nMask, nZbp, nFbp;

    nMask = 0x1FF;
    nFbp = sRender.nUnk12 & nMask;
    nZbp = sRender.nUnk08 & nMask;
    DrawEnv.l[2] = (DrawEnv.l[2] & ~nMask) | nFbp;
    DrawEnv.l[12] = (DrawEnv.l[12] & ~nMask) | nFbp;
    DrawEnv.l[4] = (DrawEnv.l[4] & ~nMask) | nZbp;
    DrawEnv.l[14] = (DrawEnv.l[14] & ~nMask) | nZbp;
    SyncDCache(&DrawEnv, (char *)&DrawEnv + 0x100);
    sceGsPutDrawEnv(&DrawEnv);
}

int nmlModelIsBackBufferRequest(void);

/* Call each registered per-frame final-packet callback with the current
 * packet pointer, unless a back-buffer request is pending */
void xglRenderFinalPacket(void)
{
    u_int *pPacket;
    XGLRENDER *pRender;
    void (**pCallback)(u_int *);
    int i;

    pPacket = xglPacketGetCurrent();
    if (nmlModelIsBackBufferRequest()) {
        return;
    }
    pRender = &sRender;
    pCallback = (void (**)(u_int *))pRender->aUnk24;
    for (i = 7; i >= 0; i--) {
        if (*pCallback != 0) {
            (*pCallback)(pPacket);
        }
        pCallback++;
    }
}
