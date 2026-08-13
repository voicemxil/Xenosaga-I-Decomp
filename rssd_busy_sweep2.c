
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

void RssdBusy_0(RSSD_ASYNC_ARG *pArg0)
{
    register RSSD_WORK *pWork __asm__("$3");
    register RSSD_ASYNC_ARG *pArg __asm__("$6");
    int nSema;

    pArg = pArg0;
    pWork = &RssdWork;
    pWork->nFlags |= 2;
    nSema = pWork->nSemaId;
    pWork->nUnk008 = pArg->nValue;
    pWork->nResolution = pArg->nRes;
    pWork->nUnk012 = pArg->nUnk1A;
    pWork->nUnk014 = pArg->nUnk1C;
    SignalSema(nSema);
}

void RssdBusy_1(RSSD_ASYNC_ARG *pArg0)
{
    register RSSD_WORK *pWork __asm__("$3");
    register RSSD_ASYNC_ARG *pArg __asm__("$6");
    int nSema;

    pArg = pArg0;
    pWork = &RssdWork;
    pWork->nFlags |= 2;
    nSema = pWork->nSemaId;
    pWork->nUnk008 = pArg->nValue;
    pWork->nResolution = pArg->nRes;
    pWork->nUnk014 = pArg->nUnk1C;
    pWork->nUnk012 = pArg->nUnk1A;
    SignalSema(nSema);
}

void RssdBusy_2(RSSD_ASYNC_ARG *pArg0)
{
    register RSSD_WORK *pWork __asm__("$3");
    register RSSD_ASYNC_ARG *pArg __asm__("$6");
    int nSema;

    pArg = pArg0;
    pWork = &RssdWork;
    pWork->nFlags |= 2;
    nSema = pWork->nSemaId;
    pWork->nUnk008 = pArg->nValue;
    pWork->nUnk012 = pArg->nUnk1A;
    pWork->nResolution = pArg->nRes;
    pWork->nUnk014 = pArg->nUnk1C;
    SignalSema(nSema);
}

void RssdBusy_3(RSSD_ASYNC_ARG *pArg0)
{
    register RSSD_WORK *pWork __asm__("$3");
    register RSSD_ASYNC_ARG *pArg __asm__("$6");
    int nSema;

    pArg = pArg0;
    pWork = &RssdWork;
    pWork->nFlags |= 2;
    nSema = pWork->nSemaId;
    pWork->nUnk008 = pArg->nValue;
    pWork->nUnk012 = pArg->nUnk1A;
    pWork->nUnk014 = pArg->nUnk1C;
    pWork->nResolution = pArg->nRes;
    SignalSema(nSema);
}

void RssdBusy_4(RSSD_ASYNC_ARG *pArg0)
{
    register RSSD_WORK *pWork __asm__("$3");
    register RSSD_ASYNC_ARG *pArg __asm__("$6");
    int nSema;

    pArg = pArg0;
    pWork = &RssdWork;
    pWork->nFlags |= 2;
    nSema = pWork->nSemaId;
    pWork->nUnk008 = pArg->nValue;
    pWork->nUnk014 = pArg->nUnk1C;
    pWork->nResolution = pArg->nRes;
    pWork->nUnk012 = pArg->nUnk1A;
    SignalSema(nSema);
}

void RssdBusy_5(RSSD_ASYNC_ARG *pArg0)
{
    register RSSD_WORK *pWork __asm__("$3");
    register RSSD_ASYNC_ARG *pArg __asm__("$6");
    int nSema;

    pArg = pArg0;
    pWork = &RssdWork;
    pWork->nFlags |= 2;
    nSema = pWork->nSemaId;
    pWork->nUnk008 = pArg->nValue;
    pWork->nUnk014 = pArg->nUnk1C;
    pWork->nUnk012 = pArg->nUnk1A;
    pWork->nResolution = pArg->nRes;
    SignalSema(nSema);
}
