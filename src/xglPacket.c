/* VIF1 packet double-buffer management for the xgl engine */

typedef unsigned int u_int;

typedef struct XGLPACKET {
    u_int *pCur;
    u_int *pBase;
    u_int nState;
    u_int *pOpenDirect;
    u_int nUnk10;
    u_int *pOpenGif;
    u_int nUnk18;
    u_int nUnk1C;
    u_int *pEnd;
    u_int *pLimit;
} XGLPACKET;

void sceVif1PkInit(XGLPACKET *pPk, u_int *pBuff);
void sceVif1PkEnd(XGLPACKET *pPk, u_int nFlags);
u_int sceVif1PkTerminate(XGLPACKET *pPk);
void sceVif1PkReset(XGLPACKET *pPk);
void FlushCache(int nMode);
void xglDmaDirectSrcChain(u_int nCh, u_int nAddr);

XGLPACKET asPacketSource[2];
XGLPACKET *pCurrentPacket;
XGLPACKET *pSendPacket;

/* Return the packet currently being built */
XGLPACKET *xglPacketGetCurrent(void)
{
    return pCurrentPacket;
}

/* Set up the double-buffered packet builders */
void xglPacketInit(void)
{
    sceVif1PkInit(&asPacketSource[0], (u_int *)0xC00000);
    sceVif1PkInit(&asPacketSource[1], (u_int *)0xE00000);
    asPacketSource[0].pEnd = (u_int *)0xE00000;
    asPacketSource[0].pLimit = (u_int *)0xE00000;
    asPacketSource[1].pEnd = (u_int *)0x1000000;
    asPacketSource[1].pLimit = (u_int *)0x1000000;
    pCurrentPacket = &asPacketSource[0];
    pSendPacket = 0;
}

/* Finish the current packet and switch to the other build buffer */
void xglPacketMove(void)
{
    sceVif1PkEnd(pCurrentPacket, 0);
    sceVif1PkTerminate(pCurrentPacket);
    FlushCache(0);
    ((u_int *)pCurrentPacket)[9] = ((u_int *)pCurrentPacket)[8];
    pSendPacket = pCurrentPacket;
    if (pCurrentPacket == &asPacketSource[1]) {
        pCurrentPacket = &asPacketSource[0];
    } else {
        pCurrentPacket = &asPacketSource[1];
    }
    sceVif1PkReset(pCurrentPacket);
}

/* Kick the finished packet to VIF1 via source-chain DMA */
void xglPacketSend(void)
{
    if (pSendPacket != 0) {
        xglDmaDirectSrcChain(1, (u_int)pSendPacket->pBase);
    }
}
