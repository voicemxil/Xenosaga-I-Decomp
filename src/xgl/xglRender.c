#include "matching.h"

/* Frame rendering environment management for the xgl engine */

typedef unsigned char u_char;
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
    u_char nUnk58;
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
    PIN(u_short nTmp, "$7");
    PIN(u_short nFront, "$6");
    PIN(u_short nDisp, "$3");
    PIN(int nSum, "$4");
    PIN(int nDrop, "$5");
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

/* Set the global fade colour: four clamped [0,1] channels packed into
 * one 0xAABBGGRR word (alpha keeps the PS2 0-128 scale, so it is scaled
 * by 128 and not masked). */
void xglRenderGlobalFadeSet(float fR, float fG, float fB, float fA)
{
    if (fR < 0.0f) {
        fR = 0.0f;
    }
    if (fG < 0.0f) {
        fG = 0.0f;
    }
    if (fB < 0.0f) {
        fB = 0.0f;
    }
    if (fA < 0.0f) {
        fA = 0.0f;
    }
    if (fR > 1.0f) {
        fR = 1.0f;
    }
    if (fG > 1.0f) {
        fG = 1.0f;
    }
    if (fB > 1.0f) {
        fB = 1.0f;
    }
    if (fA > 1.0f) {
        fA = 1.0f;
    }
    s_nGblFade = ((int)(fR * 255.0f) & 0xFF)
               + (((int)(fG * 255.0f) & 0xFF) << 8)
               + (((int)(fB * 255.0f) & 0xFF) << 16)
               + ((int)(fA * 128.0f) << 24);
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

/* TODO: near-miss (38/49 words). Logic, field offsets, and constants
 * verified against asm. The 0x47 test-addr constant needs to live in a
 * callee-saved register across the ClearFrame/ClearColor calls (orig
 * uses s1), but every source ordering tried lets 2.96 schedule the
 * constant load past the calls into a caller-saved temp instead.
 * Parked per budget rule after 2 attempts. */
/* Reinitialize the clear-environment packet (giftag, test reg, and the
 * screen-space scissor rectangle derived from the render width/height) */
/* TODO: near-miss (9 words of 49; was 36 with a length mismatch).
 * Solved here: LAUNDER on the 0x47 test-address constant is what makes
 * gcc keep it in a callee-saved register across the two calls, which is
 * what gives the original's 32-byte frame with s0+s1+ra (without it gcc
 * rematerializes the constant afterwards and the frame is 16); the
 * constant must be u_long or the sd needs a dsll32/dsrl32
 * zero-extension round-trip; nWidth is read BEFORE nUnk06 and as a
 * plain field (an explicit (short)*(u_short*) cast or a short* alias
 * both re-base the address at &sRender+4 and shift the lhu offsets);
 * and a trailing memory barrier keeps sched2 from pulling `ld $ra`
 * below the scissor store.
 * Residue: (a) the original puts `sd $s1,24($s0)` in the
 * xglRenderClearColor delay slot and issues `lui $a0,0x8000` before the
 * jal, ours the other way round; (b) two adjacent scheduling swaps
 * around the &sRender materialization; (c) the scissor store still
 * issues 17 words too early -- every barrier position over the whole
 * tail block was swept (12 variants) and only the trailing one helps. */
void xglRenderClearEnvInit(void)
{
    u_int *p;
    u_long nTestAddr;
    u_int nScissorAddr;
    int nHalf1, nHalf2;

    p = (u_int *)&ClearEnv;
    p[0] = 0x8001;
    p[1] = 0x50034000;
    p[2] = 0xE551E;
    p[3] = 0;
    nTestAddr = 0x47;
    /* The original keeps 71 in a callee-saved register across the two
     * calls (32-byte frame, s0+s1+ra); without the launder gcc
     * rematerializes the constant afterwards and the frame is 16. */
    LAUNDER(nTestAddr);
    xglRenderClearFrame();
    xglRenderClearColor(0x80000000);
    ClearEnv.nTestAddr = nTestAddr;

    nHalf2 = sRender.nWidth >> 1;
    nScissorAddr = 0x50000;
    nHalf1 = (short)sRender.nUnk06 >> 1;
    ClearEnv.aUnk30[5] = nTestAddr;
    p[12] = (2048 - nHalf2) << 4;
    p[13] = (2048 - nHalf1) << 4;
    p[16] = (nHalf2 + 2048) << 4;
    p[17] = (nHalf1 + 2048) << 4;
    ClearEnv.aUnk30[4] = nScissorAddr;
    /* Fence the epilogue: without it sched2 pulls the `ld $ra` down past
     * the scissor store and hoists the store itself above the four
     * window words. */
    __asm__ __volatile__("" : : : "memory");
}

void sceGsResetPath(void);
void sceGsResetGraph(int a0, int a1, int a2, int a3);
void nmlModelConstruct(void);
void xglRenderSetReso(int nMode);

int s_nRenderReset1;
int s_nRenderReset2;
int s_nRenderReset3;

/* TODO: near-miss (length mismatch, 50 vs 52 words -- 2 missing words,
 * likely a hazard/scheduling nop the source shape hasn't reproduced).
 * Loop bounds, field offsets and call sequence verified against asm.
 * Parked per budget rule after 2 attempts. */
/* One-time render subsystem bring-up: reset the GS path/graph, init
 * vsync + resolution, and clear the per-frame render state */
void xglRenderInit(void)
{
    int i;

    sceGsResetPath();
    *(volatile u_int *)0x10003820 |= 1;
    *(volatile u_int *)0x10003c20 |= 1;
    sceGsResetGraph(0, 1, 2, 0);
    xglRenderSyncInit();
    xglRenderSetReso(1);
    s_nRenderReset1 = 0;
    s_nRenderReset2 = 1;
    s_nRenderReset3 = 0;
    sRender.nFade = 0;
    sRender.nUnk48 = 0;
    sRender.nDrop = 0;
    sRender.nUnk50 = 0;
    sRender.nUnk58 = 0;
    for (i = 7; i >= 0; i--) {
        sRender.aUnk24[i] = 0;
    }
    xglRenderGlobalFadeInit();
    xglRenderClearOff();
    nmlModelConstruct();
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

void FlushCache(int nMode);

/* TODO: near-miss (23 words, pure SCHEDULING -- same multiset shifted
 * 1-2 slots). Levers that got here: volatile pRender for the fbp
 * loads/stores with a plain-cast pPlain for the signed lh nWidth/nPsm
 * reads (reproduces the store-then-reload of nFrontFbp without andi
 * masks), $5/$8 pins for nDisp/nFront, scratch-store source order
 * 14/tag/76/77 fixing the t3/t2/t1/a2 constant allocation. Residue:
 * sched2 hoists li 14 above the tag lui triple and delays lhu t0/sh
 * by two slots; sd 40($a3) sits late instead of splitting the or pair. */
/* Swap the display/draw frame pointers and kick the scratchpad FRAME/
 * DISPFB flip packet through DMA channel 2 */
void xglRenderDrawFlip(void)
{
    volatile XGLRENDER *pRender = &sRender;
    XGLRENDER *pPlain = (XGLRENDER *)&sRender;
    u_long *p = (u_long *)0x70000000;
    int nFbw;
    PIN(u_short nDisp, "$5");
    PIN(u_short nFront, "$8");
    u_long nFrame;

    nFbw = pPlain->nWidth;
    nDisp = pRender->nDrawFbp;
    nFront = pRender->nFrontFbp;
    pRender->nFrontFbp = nDisp;
    if (nFbw < 0) {
        nFbw += 63;
    }
    nFbw >>= 6;
    p[1] = 14;
    p[0] = 0x1000000000008002;
    p[3] = 76;
    p[5] = 77;
    nFrame = pRender->nFrontFbp | ((u_long)nFbw << 16)
           | ((u_long)pPlain->nPsm << 24);
    pRender->nDrawFbp = nFront;
    p[4] = nFrame;
    p[2] = nFrame;
    __asm__ __volatile__("sync" : : : "memory");
    FlushCache(0);
    xglDmaDirectNormal(2, 0x70000000, 3);
}

typedef struct { u_char a[8]; } PK8;
typedef struct { u_char a[24]; } PK24;
extern PK8 asDispReso[];
extern PK24 asVramBase[];
void xglRenderDrawEnvInit(void);
void xglRenderDispEnvInit(void);
void xglRenderClearEnvInit(void);
void xglStudioInit(void);
void xglStudioMainCameraInit(void);

/* TODO: near-miss (18 words). Block copies via packed PK8/PK24 structs
 * reproduce the ldl/ldr+sdl/sdr macro shape with symbol+offset(idx)
 * operands (per-element PK8 copies CSE the addresses into pointers
 * instead); a memory barrier keeps the fbp mirror reads below the
 * copies. Residue: 8 of the words are gas expanding the address macros
 * with daddu $at where the original build used addu (needs ldl/ldr
 * support in fix_cc_asm's --expand-sym-loads or a gas-level fix -- an
 * xglRender.c FILE_FIX_FLAGS entry does not exist to extend), plus
 * head/tail scheduling around the barrier.
 * Wave-4 finding on the addu/daddu half: it is purely a gas mode bit.
 * Assembling with -mgp32 (or -32) makes gas expand the ldl/ldr
 * symbol+offset(reg) macro with `addu $at,$at,$X` exactly as the
 * original does -- verified on a standalone .s.  It is NOT usable as a
 * FILE_ASFLAGS_OVERRIDE for this TU because xglRenderDrawFlip's
 * `dli $3,0x1000000000008002` then fails to assemble ("number larger
 * than 32 bits").  So either that one constant has to stop being a dli,
 * or fix_cc_asm needs to expand ldl/ldr itself the way
 * --expand-sym-loads already does for the narrower integer loads. */
void xglRenderSetReso(int nReso)
{
    char *p = (char *)&sRender;

    *(PK8 *)p = asDispReso[nReso];
    *(PK24 *)(p + 8) = asVramBase[nReso];
    __asm__ __volatile__("" : : : "memory");
    sRender.nFrontFbp = sRender.nDispFbp;
    sRender.nUnk22 = sRender.nUnk12;
    xglRenderDrawEnvInit();
    xglRenderDispEnvInit();
    xglRenderClearEnvInit();
    xglStudioInit();
    xglStudioMainCameraInit();
}
