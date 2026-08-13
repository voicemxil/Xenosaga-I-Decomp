typedef struct {
    int nFlags;                     /* 0x000 */
    char pad004[0x4];               /* 0x004 */
    int nUnk008;                    /* 0x008 */
    char pad00C[0x4];               /* 0x00C */
    unsigned short nResolution;     /* 0x010 */
    short nUnk012;                  /* 0x012 */
    int nUnk014;                    /* 0x014 */
    char pad018[0x8];               /* 0x018 */
    void *pServerFunc;              /* 0x020 */
    void *pServerArg;               /* 0x024 */
    void *pFuncFunc;                /* 0x028 */
    void *pFuncArg;                 /* 0x02C */
    char pad030[0x50];              /* 0x030 */
    short nResultCode;              /* 0x080 */
    char pad082[0x6];               /* 0x082 */
    int nResultValue;               /* 0x088 */
    char pad08C[0x120];             /* 0x08C */
    int nUnk1AC;                    /* 0x1AC */
    char pad1B0[0x8];               /* 0x1B0 */
    int nSemaId;                    /* 0x1B8 */
    char pad1BC[0x4];               /* 0x1BC */
    char *pMemStart;                /* 0x1C0 */
    char *pMemEnd;                  /* 0x1C4 */
    int nMemSize;                   /* 0x1C8 */
    int nDmaSize;                   /* 0x1CC */
    void *pDmaAddr;                 /* 0x1D0 */
    int nDmaId;                     /* 0x1D4 */
    char pad1D8[0x4];               /* 0x1D8 */
    short nDmaFlag;                 /* 0x1DC */
    char pad1DE[0xA];               /* 0x1DE */
    void *pSampleDmaFunc;           /* 0x1E8 */
    void *pSampleDmaArg;            /* 0x1EC */
    void *pSampleKeyoffFunc;        /* 0x1F0 */
    void *pSampleKeyoffArg;         /* 0x1F4 */
    void *pStreamEndFunc;           /* 0x1F8 */
    void *pStreamEndArg;            /* 0x1FC */
} RSSD_WORK;

typedef struct {
    int nHead[4];                   /* 0x00 */
    int nArg[4];                    /* 0x10 */
} RSSD_PACKET;

typedef struct {
    char pad00[0x10];               /* 0x00 */
    int nValue;                     /* 0x10 */
    char pad14[0x4];                /* 0x14 */
    unsigned short nRes;            /* 0x18 */
    short nUnk1A;                   /* 0x1A */
    int nUnk1C;                     /* 0x1C */
} RSSD_ASYNC_ARG;

extern RSSD_WORK RssdWork;

void SignalSema(int);
int WakeupThread(int);
void SsdCopyMemory(void *, const void *, int);
int RssdCallFunc(int, RSSD_PACKET *, void *, int);

/* Fetch the completion flag of the last posted driver function call */
int RssdGetCallCompletedCode(void)
{
    return (RssdWork.nFlags >> 3) & 1;
}

/* Block (optionally) until the last driver function call completes */
void RssdFuncCallCompleted(int bWait)
{
    int nCode;

    do {
        nCode = RssdGetCallCompletedCode();
    } while (bWait != 0 && nCode != 0);
}

/* Latch the driver's busy-notify parameters and release the caller */
void RssdBusy(RSSD_ASYNC_ARG *pArg)
{
    RssdWork.nFlags |= 2;
    RssdWork.nUnk008 = pArg->nValue;
    RssdWork.nResolution = pArg->nRes;
    RssdWork.nUnk012 = pArg->nUnk1A;
    RssdWork.nUnk014 = pArg->nUnk1C;
    SignalSema(RssdWork.nSemaId);
}

/* Copy a completed SPU read result into the DMA staging area */
void RssdSpuRead(RSSD_PACKET *pPkt)
{
    SsdCopyMemory((char *)RssdWork.pDmaAddr, (char *)pPkt + 0x20, pPkt->nArg[1]);
    RssdWork.pDmaAddr = (char *)RssdWork.pDmaAddr + pPkt->nArg[1];
    if (pPkt->nArg[2] == 0) {
        RssdWork.nFlags &= ~4;
    }
}

/* Wake the background-wave thread once its data has arrived */
void RssdBackgroundNextWave(RSSD_ASYNC_ARG *pArg)
{
    RssdWork.nFlags &= ~4;
    if (pArg->nValue >= 0) {
        WakeupThread(RssdWork.nUnk1AC);
    }
}
