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

XGLRENDER sRender;
XGLCLEARENV ClearEnv;
XGLDISPENV DispEnv;
int s_nClearFrame;
int s_nGblFadeInit;
int s_nGblFade;
volatile u_int VSyncCount;

void SyncDCache(void *pStart, void *pEnd);
void xglDmaDirectNormal(u_int nCh, u_int nAddr, u_int nQwc);
void sceGsSyncVCallback(int (*pFunc)(int));

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
    register u_short nTmp __asm__("$7");
    register u_short nFront __asm__("$6");
    register u_short nDisp __asm__("$3");
    register int nSum __asm__("$4");
    register int nDrop __asm__("$5");

    nSum = sRender.nUnk50;
    nDrop = sRender.nDrop;
    nTmp = sRender.nUnk22;
    __asm__("" ::: "memory");
    nFront = sRender.nFrontFbp;
    nSum += nDrop;
    nDisp = sRender.nDispFbp;
    __asm__("" : "+r"(nSum), "+r"(nDrop), "+r"(nTmp), "+r"(nFront), "+r"(nDisp) : : "memory");
    sRender.nFrontFbp = nTmp;
    __asm__("" ::: "memory");
    sRender.nUnk12 = nDisp;
    __asm__("" ::: "memory");
    sRender.nUnk22 = nFront;
    __asm__("" ::: "memory");
    sRender.nUnk50 = nSum;
    sRender.nUnk54 = 0;
    __asm__("" ::: "memory");
    sRender.nDispFbp = nTmp;
    sRender.nUnk48 = 0;
    __asm__("" ::: "memory");
    sRender.nDrop = 0;
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
