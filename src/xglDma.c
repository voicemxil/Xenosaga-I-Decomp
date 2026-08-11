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

void xglDmaDirectSrcChain(u_int nCh, u_int nAddr);

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

/* Initialize the DMA subsystem (nothing to do) */
void xglDmaInitial(void)
{
}
