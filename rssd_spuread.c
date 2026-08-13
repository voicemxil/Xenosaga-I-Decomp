typedef struct {
    int nFlags;
    char pad004[0x1CC];
    void *pDmaAddr;
} RSSD_WORK;

typedef struct {
    int nHead[4];
    int nArg[4];
} RSSD_PACKET;

extern RSSD_WORK RssdWork;
void SsdCopyMemory(void *, const void *, int);

void RssdSpuRead(RSSD_PACKET *pPkt)
{
    int nSize;
    int nFlag;

    nSize = pPkt->nArg[1];
    nFlag = pPkt->nArg[2];
    SsdCopyMemory((char *)RssdWork.pDmaAddr, (char *)pPkt + 0x20, nSize);
    RssdWork.pDmaAddr = (char *)RssdWork.pDmaAddr + nSize;
    if (nFlag == 0) {
        RssdWork.nFlags &= ~4;
    }
}
