typedef struct {
    int nFlags;
    char pad004[0x4];
    int nUnk008;
    char pad00C[0x4];
    unsigned short nResolution;
    short nUnk012;
    int nUnk014;
    char pad018[0x8];
    void *pServerFunc;
    void *pServerArg;
    void *pFuncFunc;
    void *pFuncArg;
    char pad030[0x50];
    short nResultCode;
    char pad082[0x6];
    int nResultValue;
    char pad08C[0x120];
    int nUnk1AC;
    char pad1B0[0x8];
    int nSemaId;
    char pad1BC[0x4];
    char *pMemStart;
} RSSD_WORK;

typedef struct {
    char pad00[0x10];
    int nValue;
    char pad14[0x4];
    unsigned short nRes;
    short nUnk1A;
    int nUnk1C;
} RSSD_ASYNC_ARG;

extern RSSD_WORK RssdWork;
void SignalSema(int);

void RssdBusy(RSSD_ASYNC_ARG *pArg)
{
    register RSSD_WORK *pWork __asm__("$3");
    int nSema;

    pWork = &RssdWork;
    pWork->nFlags |= 2;
    nSema = pWork->nSemaId;
    pWork->nUnk008 = pArg->nValue;
    pWork->nResolution = pArg->nRes;
    pWork->nUnk012 = pArg->nUnk1A;
    pWork->nUnk014 = pArg->nUnk1C;
    SignalSema(nSema);
}
