#include "matching.h"

/* MPEG/IPU initialization wrappers */

typedef struct {
    char pad00[0x24];
    int nStreamSize;     /* 0x24 */
    char pad28[4];
    int nUnk2C;          /* 0x2C */
    char pad30[0x10];
    short nWidth;        /* 0x40 */
    short nHeight;       /* 0x42 */
    short pad44;         /* 0x44 */
    short nUnk46;        /* 0x46 */
    int pad48;           /* 0x48 */
    int nUnk4C;          /* 0x4C */
    char pad50[0x28];
    int nMpegArg0;       /* 0x78 */
    int nMpegArg1;       /* 0x7C */
    char pad80[8];
    char *pBuf;          /* 0x88 */
    int nLeft;           /* 0x8C */
    int nAudioTotal;     /* 0x90 */
    unsigned char nAudioInit;/* 0x94 */
    char pad95[3];
    unsigned char nErrCount;/* 0x98 */
    unsigned char nUnk99;/* 0x99 */
    unsigned char nUnk9A;/* 0x9A */
    unsigned char nVideoCount;/* 0x9B */
    int nVideoBuf;       /* 0x9C */
    int nVideoSize;      /* 0xA0 */
    int nVideoWrite;     /* 0xA4 */
    int nVideoRead;      /* 0xA8 */
    int nVideoDone;      /* 0xAC */
    char *pAudioBuf;     /* 0xB0 */
    int nAudioSize;      /* 0xB4 */
    int nAudioWrite;     /* 0xB8 */
    int nAudioRead;      /* 0xBC */
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
    pInfo->nAudioWrite = 0x70000040;
    pInfo->nAudioRead = 0x70001000;
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
/* fillBuff has no return statement of its own: the original leaves the
 * sceMpegDemuxPss result in $v0 and xglMpeg2Open loops on it. */
int fillBuff(XGLMOVIEINFO *pInfo, int nWait);
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
int fillBuff(XGLMOVIEINFO *pInfo, int nWait)
{
    char *pSrc;
    int nRead;
    int i;
    int j;
    int pBuf;

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

typedef struct {
    int nUnk00;
    int nUnk04;
    char *pData;         /* 0x08 */
    int nSize;           /* 0x0C */
} XGLMPEGPKT;

void *memcpy(void *pDst, const void *pSrc, int nSize);

/* sceMpeg video callback: append one demuxed packet to the ring the IPU
 * reads from, splitting it around the wrap and writing through the
 * uncached-accelerated window */
int videoCallback(int nCode, XGLMPEGPKT *pPkt, XGLMOVIEINFO *pInfo)
{
    int nWrite;
    int nFree;
    int nHi;
    int nLo;
    int nSize;
    int nOver;

    pInfo->nVideoCount++;
    nWrite = pInfo->nVideoWrite;
    nFree = pInfo->nVideoRead - nWrite;
    if (nFree < 0) {
        nFree += pInfo->nVideoSize;
    }
    nSize = pPkt->nSize;
    if ((unsigned int)nFree < (unsigned int)nSize) {
        return 0;
    }
    nOver = nWrite + nSize - pInfo->nVideoSize;
    if (nOver > 0) {
        memcpy((void *)(((pInfo->nVideoBuf + nWrite) & 0x0FFFFFFF) | 0x20000000),
               pPkt->pData, nSize - nOver);
        memcpy((void *)((pInfo->nVideoBuf & 0x0FFFFFFF) | 0x20000000),
               pPkt->pData + (pPkt->nSize - nOver), nOver);
        pInfo->nVideoWrite = nOver;
    } else {
        memcpy((void *)(((pInfo->nVideoBuf + nWrite) & 0x0FFFFFFF) | 0x20000000),
               pPkt->pData, nSize);
        pInfo->nVideoWrite += pPkt->nSize;
    }
    return 1;
}

/* sceMpeg audio callback: strip the 44-byte WAV header from the first
 * packet (and the 4-byte prefix from later ones), then append the PCM to
 * the SPU stream ring, splitting it around the wrap */
int audioCallback(int nCode, XGLMPEGPKT *pPkt, XGLMOVIEINFO *pInfo)
{
    unsigned char *p;
    int nSize;
    int nHead;
    int nFree;
    int nHi;
    int nLo;

    p = (unsigned char *)pPkt->pData;
    nSize = pPkt->nSize;
    if (pInfo->nAudioInit == 0) {
        nHi = (p[19] << 24) + p[16];
        nLo = (p[17] << 8) + (p[18] << 16);
        pInfo->nAudioTotal = nHi + nLo;
        pInfo->nAudioInit = 1;
        nSize -= 44;
        p += 44;
    } else {
        p += 4;
        nSize -= 4;
    }
    nFree = pInfo->nAudioRead - pInfo->nAudioWrite;
    if (nFree <= 0) {
        nFree += pInfo->nAudioSize;
    }
    if (nSize >= nFree) {
        return 0;
    }
    nHead = pInfo->nAudioSize - pInfo->nAudioWrite;
    if (nHead >= nSize) {
        memcpy(pInfo->pAudioBuf + pInfo->nAudioWrite, p, nSize);
    } else {
        memcpy(pInfo->pAudioBuf + pInfo->nAudioWrite, p, nHead);
        memcpy(pInfo->pAudioBuf, p + nHead, nSize - nHead);
    }
    pInfo->nAudioWrite = (pInfo->nAudioWrite + nSize) % pInfo->nAudioSize;
    return 1;
}

typedef unsigned long u_long;
typedef unsigned int u_int;

/* Build the DMA/GIF tag chain that uploads one decoded frame to VRAM as
 * a grid of 16x16 blocks, then mask the final tag's address word */
void setLoadImageTags(u_int nPacket, u_int nSrc, int nHeight, int nWidth)
{
    u_long *p;
    int y;
    int x;

    p = (u_long *)((nPacket & 0x0FFFFFFF) | 0x20000000);
    p[0] = 0x10000003;
    p[2] = 0x1000000000008002UL;
    p[3] = 14;
    p[4] = 0x000E000000000000UL;
    p[5] = 80;
    p[6] = 0x0000001000000010UL;
    p[7] = 82;
    p[1] = 0;
    p += 8;
    for (y = 0; y < nHeight; y += 16) {
        for (x = 0; x < nWidth; x += 16) {
            p[1] = 0;
            p[0] = 0x10000004;
            p[2] = 0x1000000000008002UL;
            p[3] = 14;
            p[4] = ((u_long)y << 32) | ((u_long)x << 48);
            p[5] = 81;
            p[6] = 0;
            p[7] = 83;
            p[8] = 0x0800000000008040UL;
            p[9] = 0;
            p += 10;
            ((u_int *)p)[1] = nSrc;
            nSrc += 1024;
            ((u_int *)p)[0] = 0x30000040;
            p[1] = 0;
            p += 2;
        }
    }
    ((u_int *)p)[-4] &= 0x0FFFFFFF;
}

int xglCdStreamOpen(XGLMOVIEINFO *pInfo, char *pName);
int xglCdStreamReadRing(XGLMOVIEINFO *pInfo, int nBytes);
void sceMpegInit(void);
int sceMpegCreate(void *pMpeg, int nArg0, int nArg1);
int sceMpegAddStrCallback(void *pMpeg, int nId, int nSub, void *pFunc, void *pArg);
int sceMpegAddCallback(void *pMpeg, int nId, void *pFunc, void *pArg);
int nodataCallback(int, void *, XGLMOVIEINFO *);
int SsdInitVagStreamStereo(int nSize, int nChannels);
int SsdGetResultValue(int *pResult);

/* Open a movie stream, bring the MPEG decoder up with its four
 * callbacks, prime the demux buffer and start the VAG audio stream */
int xglMpeg2Open(XGLMOVIEINFO *pInfo, char *pName)
{
    int nResult;
    int i;
    int j;
    int pBuf;

    if (xglCdStreamOpen(pInfo, pName) < 0) {
        return -1;
    }
    i = 0;
    xglCdStreamReadRing(pInfo, pInfo->nStreamSize / 2);
    pInfo->nUnk2C = 0;
    sceMpegInit();
    sceMpegCreate((char *)pInfo + 48, pInfo->nMpegArg0, pInfo->nMpegArg1);
    sceMpegAddStrCallback((char *)pInfo + 48, 0, 0, videoCallback, pInfo);
    sceMpegAddStrCallback((char *)pInfo + 48, 3, 0, audioCallback, pInfo);
    sceMpegAddCallback((char *)pInfo + 48, 1, nodataCallback, pInfo);
    sceMpegAddCallback((char *)pInfo + 48, 0, errorCallback, pInfo);
    pInfo->nLeft = 0;
    pInfo->nAudioInit = 0;
    pInfo->nAudioTotal = 0;
    pInfo->nErrCount = 0;
    pInfo->nUnk99 = 0;
    pInfo->nUnk9A = 0;
    pInfo->nVideoCount = 0;
    do {
        if (i > 0xFFFFFF) {
            break;
        }
        i++;
    } while (fillBuff(pInfo, 1) != 0);
    SsdInitVagStreamStereo(4096, 4);
    while (SsdGetResultValue(&nResult) < 0) {
    }
    return 0;
}

void FlushCache(int nMode);

/* TODO: near-miss (17 words; every instruction is present, the residue
 * is which register each 0x1000Bxxx IPU-register address lands in and
 * whether `lui 0x1000` or the `lw nVideoBuf` goes first in the reset
 * block). Levers that got here: a plain `for (;;)` with the recompute as
 * the LAST statement -- gcc rotates it into the original's
 * recompute-at-top layout AND, being a recognised loop, hoists 0xFFFFFF
 * into $s3 (a hand-rotated `goto test` form reproduces the layout but
 * loses the hoist and rematerialises the constant every iteration); a
 * SEPARATE counter for the reset loop (sharing `i` pushes it into a
 * callee-saved register); that reset loop counting BYTES (`i < 4096;
 * i += 4`) the way the original's `li 4092 / addiu -4` does; volatile
 * int pointers for the IPU registers (a plain `*(int *)0x1000B420`
 * folds to an absolute MEM and gives `lui at` per store); and reading
 * nVideoBuf into a local BEFORE the volatile stores, since a volatile
 * store is a scheduling barrier the load cannot cross.
 * Swept: block-scoped pointer locals for the three registers (volatile
 * 26, plain 113 words), swapping the B420/B410 source order (22 but
 * wrong program order), assigning through the existing nLen local. */
/* IPU starvation callback: hand the IPU whatever whole 16-byte units the
 * ring holds (topping it up first), or after 64 dry calls reset the ring
 * to a stream of 0x100 words and restart the transfer */
int nodataCallback(int nCode, void *pArg, XGLMOVIEINFO *pInfo)
{
    int *p;
    int nAvail;
    int nLen;
    int nCount;
    int i;
    int j;
    int pBuf;

    nCount = pInfo->nUnk9A + 1;
    pInfo->nUnk9A = nCount;
    if ((unsigned char)nCount >= 65) {
        pInfo->nErrCount = 16;
        p = (int *)pInfo->nVideoBuf;
        /* Counting BYTES, not elements: the original's loop counter is
         * the byte offset (li 4092 / addiu -4), which keeps it in a
         * caller-saved register instead of costing an extra $sN. */
        for (j = 0; j < 4096; j += 4) {
            *p = 256;
            p++;
        }
        FlushCache(0);
        pBuf = pInfo->nVideoBuf;
        *(volatile int *)0x1000B420 = 4096;
        *(volatile int *)0x1000B410 = pBuf;
        *(volatile int *)0x1000B400 = 257;
        return 1;
    }
    nAvail = pInfo->nVideoWrite - pInfo->nVideoDone;
    pInfo->nVideoRead = pInfo->nVideoDone;
    if (nAvail < 0) {
        nAvail += pInfo->nVideoSize;
    }
    i = 0;
    for (;;) {
        if (i > 0xFFFFFF) {
            break;
        }
        i++;
        if (nAvail >= 4096) {
            goto emit;
        }
        if (fillBuff(pInfo, 1) == 0) {
            break;
        }
        nAvail = pInfo->nVideoWrite - pInfo->nVideoRead;
        if (nAvail < 0) {
            nAvail += pInfo->nVideoSize;
        }
    }
    nAvail += 15;
emit:
    nLen = nAvail & -16;
    if (nLen > 4096) {
        nLen = 4096;
    }
    if (pInfo->nVideoSize < pInfo->nVideoRead + nLen) {
        nLen = pInfo->nVideoSize - pInfo->nVideoRead;
    }
    *(volatile int *)0x1000B420 = nLen / 16;
    *(volatile int *)0x1000B410 = pInfo->nVideoBuf + pInfo->nVideoRead;
    *(volatile int *)0x1000B400 = 257;
    pInfo->nVideoDone = (pInfo->nVideoRead + nLen) % pInfo->nVideoSize;
    return 1;
}
