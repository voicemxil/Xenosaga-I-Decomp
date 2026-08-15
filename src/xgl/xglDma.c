#include "matching.h"

/* DMA transfer helpers for the xgl engine */

typedef unsigned int u_int;
typedef volatile u_int vu_int;

typedef struct {
    vu_int chcr;
    u_int pad0[3];
    vu_int madr;
    u_int pad1[3];
    vu_int qwc;
    u_int pad2[3];
    vu_int tadr;
} XGLDMACHAN;

typedef struct {
    u_int *pBase;
    u_int *pCur;
} XGLDMABUFF;

void FlushCache(int nMode);

static XGLDMACHAN *tbl_00490D60[3] = {
    (XGLDMACHAN *)0x10008000,
    (XGLDMACHAN *)0x10009000,
    (XGLDMACHAN *)0x1000A000,
};
XGLDMACHAN *mfifo_drain;

void xglDmaDirectSrcChain(u_int nCh, u_int nAddr);
void sceGsSyncPath(int nMode, int nTimeout);

/* Kick a normal-mode DMA transfer on the given channel */
void xglDmaDirectNormal(u_int nCh, u_int nAddr, u_int nQwc)
{
    XGLDMACHAN *pChan;

    if (nCh < 3) {
        pChan = tbl_00490D60[nCh];
        while (pChan->chcr & 0x100) {
        }
        pChan->qwc = nQwc;
        if ((nAddr >> 16) != 0x7000) {
            pChan->madr = nAddr;
        } else {
            pChan->madr = (nAddr & 0x3FFF) + 0x80000000;
        }
        *(vu_int *)0x1000E010 = 1 << nCh;
        pChan->chcr = 0x141;
    }
}

/* Kick a source-chain-mode DMA transfer on the given channel */
void xglDmaDirectSrcChain(u_int nCh, u_int nAddr)
{
    XGLDMACHAN *pChan;

    if (nCh < 3) {
        pChan = tbl_00490D60[nCh];
        while (pChan->chcr & 0x100) {
        }
        pChan->qwc = 0;
        if ((nAddr >> 16) != 0x7000) {
            pChan->tadr = nAddr;
        } else {
            pChan->tadr = (nAddr & 0x3FFF) + 0x80000000;
        }
        *(vu_int *)0x1000E010 = 1 << nCh;
        pChan->chcr = 0x145;
    }
}

/* Reset a DMA tag buffer to the given address */
void xglDmaBufferReset(XGLDMABUFF *pBuff, u_int *pAddr)
{
    pBuff->pBase = pAddr;
    pBuff->pCur = pAddr;
}

/* Append a CNT tag with inline data to the buffer */
u_int *xglDmaBufferCnt(XGLDMABUFF *pBuff, u_int *pData, u_int nQwc)
{
    u_int *pTag;
    u_int i;

    pTag = pBuff->pCur;
    pTag[0] = 0x10000000 + nQwc;
    pTag[1] = 0;
    pTag[2] = 0;
    pTag[3] = 0;
    pTag += 4;
    for (i = 0; i < nQwc * 4; i++) {
        *pTag++ = *pData++;
    }
    pBuff->pCur = pTag;
    return pTag;
}

/* Append a REF tag pointing at external data */
u_int *xglDmaBufferRef(XGLDMABUFF *pBuff, u_int nAddr, u_int nQwc)
{
    u_int *pTag;

    pTag = pBuff->pCur;
    pTag[0] = 0x30000000 + nQwc;
    pTag[1] = nAddr;
    pTag[2] = 0;
    pTag[3] = 0;
    pTag += 4;
    pBuff->pCur = pTag;
    return pTag;
}

/* Append a CALL tag to the buffer */
u_int *xglDmaBufferCall(XGLDMABUFF *pBuff, u_int nAddr)
{
    u_int *pTag;

    pTag = pBuff->pCur;
    pTag[0] = 0x50000000;
    pTag[1] = nAddr;
    pTag[2] = 0;
    pTag[3] = 0;
    pTag += 4;
    pBuff->pCur = pTag;
    return pTag;
}

/* Terminate the buffer with an END tag and kick it */
void xglDmaBufferRequest(XGLDMABUFF *pBuff, u_int nCh)
{
    u_int *pTag;

    pTag = pBuff->pCur;
    pTag[0] = 0x70000000;
    pTag[1] = 0;
    pTag[2] = 0;
    pTag[3] = 0;
    FlushCache(0);
    xglDmaDirectSrcChain(nCh, (u_int)pBuff->pBase);
    pBuff->pCur = pBuff->pBase;
}

/* Configure the DMA controller for memory FIFO transfers */
/* TODO: near-miss (4 words, down from 14, pure SCHEDULING): two adjacent
 * pair swaps remain -- orig orders (lui $a2 ; lw $v0) and (ori $v0,0xc ;
 * ori $a2,0xe050), ours schedules the lw/ori-v0 first. sched2 gives the
 * load priority and no source order, false-dependence asm, volatile
 * passthrough or hi/lo split changed the tie (each of those regressed to
 * 5-27). The MMIO register rotation itself is fully solved by zero-code
 * tied empty-asm passthroughs pinning nCtrl=$2, E040=$3, E000=$5,
 * E050=$6, plus $9 for the 0x104 chcr constant. */
void xglDmaMFIFOSetup(u_int nAddr, u_int nSize, int nCh)
{
    XGLDMACHAN *pChan;
    PIN(u_int nCtrl, "$2");
    PIN(vu_int *pE040, "$3");
    PIN(vu_int *pE000, "$5");
    PIN(vu_int *pE050, "$6");
    PIN(u_int nChcr, "$9");

    if (nCh < 3) {
        pChan = tbl_00490D60[nCh];
        __asm__("" : "=r"(pE000) : "0"((vu_int *)0x1000E000));
        __asm__("" : "=r"(pE050) : "0"((vu_int *)0x1000E050));
        nCtrl = *pE000;
        *pE050 = nAddr;
        *pE000 = nCtrl | 0xC;
        pE040 = (vu_int *)0x1000E040;
        *pE040 = nSize - 0x10;
        mfifo_drain = pChan;
        *(vu_int *)0x1000D010 = nAddr;
        pChan->tadr = nAddr;
        pChan->qwc = 0;
        nChcr = 0x104;
        pChan->chcr = nChcr;
    }
}

/* Submit data to the memory FIFO and wait for drain space */
/* TODO: near-miss (9 words, was 11; t0/t1 rotation solved by the
 * zero-code tied passthroughs pinning nMask=$8 / pChan=$9). Residue:
 * (a) the trailing D000 store is stolen into the jr delay slot -- fixing
 * it needs FILE_FIX_FLAGS["xglDma.c"] = "--barrier-return-store
 * xglDmaMFIFOKick" (VERIFIED: with that flag this drops to 7 diffs and
 * the length matches); (b) two scheduling windows: the same adjacent
 * (lui ; ori) pair swap seen in xglDmaMFIFOSetup, and the nMask addiu
 * hoisted above the drain lw/sw/sll group (false-dep asm inputs do not
 * move it). */
u_int xglDmaMFIFOKick(u_int nTadr, u_int nQwc)
{
    u_int nSize;
    PIN(u_int nMask, "$8");
    u_int nSpace;
    PIN(XGLDMACHAN *pChan, "$9");

    nSize = *(vu_int *)0x1000E040 + 0x10;
    while (*(vu_int *)0x1000D000 & 0x100) {
    }
    *(vu_int *)0x1000D080 = nTadr & 0x3FF0;
    PASSTHRU(pChan, mfifo_drain);
    *(vu_int *)0x1000D020 = nQwc;
    nQwc <<= 4;
    PASSTHRU(nMask, nSize - 0x10);
    do {
        nSpace = (pChan->tadr - *(vu_int *)0x1000D010 + nSize) & nMask;
        nSpace = nSpace ? nSpace : nSize;
    } while (nQwc >= nSpace);
    *(vu_int *)0x1000D000 = 0x100;
    return nSpace;
}

/* Stop memory FIFO mode after both DMA paths become idle */
/* TODO: Find the natural source shape for this matched scheduling scaffold. */
void xglDmaMFIFOLeave(void)
{
    PIN(u_int *pCtrl, "$3");
    PIN(u_int nCtrl, "$2");
    PIN(int nMask, "$4");
    vu_int *pWait;

    sceGsSyncPath(0, 0);
    pCtrl = (u_int *)0x1000E000;
    nMask = ~0xC;
    __asm__("" : "+r"(nMask) :: "memory");
    nCtrl = *pCtrl;
    __asm__("" ::: "$5", "memory");
    pWait = (vu_int *)0x1000D000;
    __asm__("" : "+r"(pCtrl), "+r"(nCtrl), "+r"(nMask), "+r"(pWait));
    nCtrl &= nMask;
    *pCtrl = nCtrl;
    while (*pWait & 0x100) {
    }
    while (mfifo_drain->chcr & 0x100) {
    }
}

/* Initialize the DMA subsystem (nothing to do) */
void xglDmaInitial(void)
{
}
