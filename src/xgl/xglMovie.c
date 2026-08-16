#include "matching.h"

/* MPEG/IPU initialization wrappers */

typedef struct {
    char pad00[0x40];
    short nWidth;        /* 0x40 */
    short nHeight;       /* 0x42 */
    short pad44;         /* 0x44 */
    short nUnk46;        /* 0x46 */
    int pad48;           /* 0x48 */
    int nUnk4C;          /* 0x4C */
    char pad50[0x38];
    char *pBuf;          /* 0x88 */
    int nLeft;           /* 0x8C */
    char pad90[8];
    unsigned char nErrCount;/* 0x98 */
    unsigned char nUnk99;/* 0x99 */
    unsigned char nUnk9A;/* 0x9A */
    char pad9B[0x1D];
    void *pIpuBuf;       /* 0xB8 */
    void *pSpuBuf;       /* 0xBC */
    char padC0[9];
    char nUnkC9;         /* 0xC9 */
    unsigned char nUnkCA;/* 0xCA */
    unsigned char nUnkCB;/* 0xCB */
} XGLMOVIEINFO;

void *xglMpeg2InfoInit2(void *pInfo, void *pData, int nSize);

typedef struct {
    char pad00[0x20];
    int nBuf20;          /* 0x20 */
    int nSize24;         /* 0x24 */
    char pad28[0x50];
    int nBase78;         /* 0x78 */
    int nLen7C;          /* 0x7C */
    int nBuf80;          /* 0x80 */
    int nBuf84;          /* 0x84 */
    int nBuf88;          /* 0x88 */
    char pad8C[0x10];
    int nBuf9C;          /* 0x9C */
    int nLenA0;          /* 0xA0 */
    int nLenA4;          /* 0xA4 */
    int nPosA8;          /* 0xA8 */
    int nLenAC;          /* 0xAC */
    char padB0[0];
    int nBufB0;          /* 0xB0 */
    int nLenB4;          /* 0xB4 */
    int nPosB8;          /* 0xB8 */
    int nPosBC;          /* 0xBC */
} XGLMPEG2INIT;
void sceIpuInit(void);
void xglCdStreamParamInit(void *);
int xglCdStreamClose(void *);
void setD4_CHCR(int nVal);
int sceIpuSync(int nMode, int nTimeout);
int sceMpegReset(void *);
int sceMpegDelete(void *);
int SsdStopVagStream(int);
int SsdDisposeVagStream(void);
int SsdGetResultValue(int *);

void *xglMpeg2InfoInit(void *pInfo, void *pData)
{
    return xglMpeg2InfoInit2(pInfo, pData, 0x100000);
}

void xglMovieInit(void)
{
    sceIpuInit();
}

/* Reset a movie-info block: stream params, scratchpad buffer pointers
 * and the playback bookkeeping fields */
void xglMovieInfoInit(XGLMOVIEINFO *pInfo)
{
    int nInit = -1;

    xglCdStreamParamInit(pInfo);
    pInfo->pIpuBuf = (void *)0x70000040;
    pInfo->pSpuBuf = (void *)0x70001000;
    pInfo->nUnk46 = nInit;
    pInfo->nUnk9A = 0;
    pInfo->nUnkC9 = nInit;
    pInfo->nUnkCA = 0;
    pInfo->nUnkCB = 0;
}

/* TODO: near-miss (~2 words) blocked on a fixer flag: our gas steals the
 * volatile `sw $0,0($v1)` (IPU_CMD reset) into the second sceIpuSync
 * jal's delay slot; the original keeps store-then-jal with a genuine
 * nop. Needs FILE_FIX_FLAGS["xglMovie.c"] = "--barrier-branch-move
 * xglMovieClose". Everything else (branch layout, movz tail with nRet
 * assigned AFTER the call) already lines up. */
/* Stop the IPU and its DMA channel, waiting for the decoder to idle */
int xglMovieClose(XGLMOVIEINFO *pInfo)
{
    int nRet;
    int nSync;

    if (pInfo->nUnk99 != 0) {
        xglCdStreamClose(pInfo);
    }
    setD4_CHCR(0);
    *(volatile unsigned int *)0x10002010 = 0x40000000;
    if (sceIpuSync(0, 64) < 0) {
        return -1;
    }
    *(volatile unsigned int *)0x10002000 = 0;
    nSync = sceIpuSync(0, 64);
    nRet = -1;
    if (!(nSync < 0)) {
        nRet = 0;
    }
    return nRet;
}

/* TODO: near-miss (2 words, SCHEDULING). Everything matches except the
 * final zero stores: ours emits `sw $0,0($t2)` grouped with the earlier
 * t2-based stores, the original keeps it after the sd 40/48 pair. Every
 * source permutation of the zero stores leaves the sw hoisted; barrier
 * and passthrough attempts fix the order but cascade a v0/v1 allocation
 * swap through the whole body. swap-adjacent can't help (two memory
 * ops are ineligible). Levers that got this far: hand-written 5-op
 * li/dsll/ori asm for the 0x0050005800005458 GIF tag (gas's own dli
 * synthesizes the shorter lui form), $4-pinned empty-asm passthrough
 * for the 0x0000001000000001 constant. */
/* Build the GS transfer header (BITBLTBUF/TRXPOS/TRXREG/TRXDIR + image
 * GIF tag) for one decoded movie frame, returning the payload address */
void *xglMovieMakeXtxHeader(XGLMOVIEINFO *pInfo, char *pBuf)
{
    long long *p = (long long *)pBuf;
    int *q;
    int nBase = pInfo->nUnk4C;
    int nWidth = pInfo->nWidth;
    int nHeight = pInfo->nHeight;
    PIN(long long nTag, "$8");
    PIN(long long nGif, "$4");

    __asm__("li %0, 80\n\t"
            "dsll %0, %0, 16\n\t"
            "ori %0, %0, 0x58\n\t"
            "dsll %0, %0, 16\n\t"
            "ori %0, %0, 0x5458" : "=r"(nTag));
    PASSTHRU(nGif, 0x0000001000000001LL);
    q = (int *)(pBuf + 16);
    q[3] = nBase + 2;
    q[0] = 0x40000 + nWidth;
    q[1] = nHeight;
    q[2] = 0;
    q = (int *)(pBuf + 56);
    q[3] = 0x8000000;
    p[0] = nTag;
    p[1] = nGif;
    p[4] = 48;
    q[1] = 0x50000001 + nBase;
    q[2] = 0x8000 + nBase;
    p[6] = 0;
    q[0] = 0;
    p[5] = 0;
    p[9] = 0;
    /* The three zero stores at +40/+48/+56 come out of gcc's sched2 as
     * (+56, +48, +40); the original build issued (+40, +48, +56).  All
     * 720 source orderings of this store group were compiled
     * (tools/permute.py) and none reproduces it, so it is a pure
     * scheduler tie-break, restored by
     * --rotate-seq xglMovieMakeXtxHeader:28:3,xglMovieMakeXtxHeader:29:2.
     * Safe by construction: all three store zero to distinct offsets. */
    return pBuf + 80;
}

/* Tear down the MPEG decoder and VAG audio stream for a movie */
int xglMpeg2Close(XGLMOVIEINFO *pInfo)
{
    int nResult;

    sceMpegReset((char *)pInfo + 48);
    sceMpegDelete((char *)pInfo + 48);
    SsdStopVagStream(0);
    while (SsdGetResultValue(&nResult) < 0) {
    }
    SsdDisposeVagStream();
    while (SsdGetResultValue(&nResult) < 0) {
    }
    xglCdStreamClose(pInfo);
    return 0;
}

/* Carve the fixed MPEG/IPU work buffers out of one linear block, each
 * rounded up to a 64-byte boundary, and hand back the end of the block. */
void *xglMpeg2InfoInit2(void *pInfo, void *pData, int nSize)
{
    XGLMPEG2INIT *p = (XGLMPEG2INIT *)pInfo;
    int nAddr = (int)pData;
    int nPtr;

    nPtr = (nAddr + 0xFD7A7) & ~0x3F;
    p->nBuf80 = nPtr;
    nPtr = (nPtr + 0xE003F) & ~0x3F;
    p->nBuf84 = nPtr;
    nPtr = (nPtr + 0x1507F) & ~0x3F;
    p->nBuf88 = nPtr;
    nPtr = (nPtr + 0x403F) & ~0x3F;
    p->nBuf9C = nPtr;
    nPtr = (nPtr + 0x8003F) & ~0x3F;
    p->nLen7C = 0xFD768;
    p->nLenA0 = 0x80000;
    p->nLenAC = 0x10;
    p->nLenB4 = 0x20000;
    p->nBufB0 = nPtr;
    nPtr = (nPtr + 0x2003F) & ~0x3F;
    p->nBase78 = nAddr;
    p->nLenA4 = 0x10;
    p->nPosA8 = 0;
    p->nPosB8 = 0;
    p->nPosBC = 0;
    xglCdStreamParamInit(p);
    p->nBuf20 = nPtr;
    p->nSize24 = nSize;
    return (void *)((nPtr + nSize + 0x3F) & ~0x3F);
}

int printf(const char *pFmt, ...);

/* sceMpeg error callback: print the message the library handed us, bump
 * the error counter and clear the stall flag */
int errorCallback(int nCode, char **ppMsg, XGLMOVIEINFO *pInfo)
{
    printf("%s\n", ppMsg[1]);
    pInfo->nErrCount++;
    pInfo->nUnk99 = 0;
    return 1;
}

int fileRead(XGLMOVIEINFO *pInfo);
int sceMpegDemuxPss(void *pMpeg, void *pSrc, int nSize);

/* TODO: near-miss (42 words built vs 44; every instruction present is
 * correct and in the right place). Missing: the original's `move v1,v0`
 * copying fileRead's result out of $v0, and the alignment `nop` before
 * the shared tail. The copy means nRead is a MULTI-BLOCK pseudo in the
 * original (global_alloc gave it $v1); every shape that puts the
 * `pInfo->nLeft = nRead` store into both successors of the test lets
 * gcc constant-fold the zero arm into `sw zero,140(s0)` instead.
 * Swept: for / while / while(1)+break / do-while+goto loop forms (the
 * do-while is the one that reproduces the block layout -- a `for` or
 * `while` header gets rotated by duplicate_loop_exit_test and sinks the
 * nWait test to the bottom, 3 words short); re-reading pInfo->nLeft for
 * the test; storing through the call expression; pre-initialising
 * nRead; duplicating the store into both arms (10 diffs, wrong code).
 */
/* Top the demux up: if the last read still has bytes left carry on from
 * where it stopped, otherwise pull fresh data (optionally spinning until
 * some arrives), then feed the PSS demuxer */
void fillBuff(XGLMOVIEINFO *pInfo, int nWait)
{
    char *pSrc;
    int nRead;
    int i;

    if (pInfo->nLeft != 0) {
        pSrc = pInfo->pBuf - pInfo->nLeft + 0x4000;
    } else {
        i = 0;
        do {
            if (i > 0xFFFFFF) {
                goto ready;
            }
            i++;
            nRead = fileRead(pInfo);
            pInfo->nLeft = nRead;
            if (nRead != 0) {
                goto ready;
            }
        } while (nWait != 0);
        return;
ready:
        pSrc = pInfo->pBuf;
    }
    pInfo->nLeft -= sceMpegDemuxPss((char *)pInfo + 48, pSrc, pInfo->nLeft);
}
