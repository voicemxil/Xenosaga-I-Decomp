
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

void RssdBusy_0(RSSD_ASYNC_ARG *pArg)
{
    RssdWork.nFlags |= 2;
    RssdWork.nUnk008 = pArg->nValue;
    RssdWork.nResolution = pArg->nRes;
    RssdWork.nUnk012 = pArg->nUnk1A;
    RssdWork.nUnk014 = pArg->nUnk1C;
    SignalSema(RssdWork.nSemaId);
}

void RssdBusy_1(RSSD_ASYNC_ARG *pArg)
{
    RssdWork.nFlags |= 2;
    RssdWork.nUnk008 = pArg->nValue;
    RssdWork.nResolution = pArg->nRes;
    RssdWork.nUnk014 = pArg->nUnk1C;
    RssdWork.nUnk012 = pArg->nUnk1A;
    SignalSema(RssdWork.nSemaId);
}

void RssdBusy_2(RSSD_ASYNC_ARG *pArg)
{
    RssdWork.nFlags |= 2;
    RssdWork.nUnk008 = pArg->nValue;
    RssdWork.nUnk012 = pArg->nUnk1A;
    RssdWork.nResolution = pArg->nRes;
    RssdWork.nUnk014 = pArg->nUnk1C;
    SignalSema(RssdWork.nSemaId);
}

void RssdBusy_3(RSSD_ASYNC_ARG *pArg)
{
    RssdWork.nFlags |= 2;
    RssdWork.nUnk008 = pArg->nValue;
    RssdWork.nUnk012 = pArg->nUnk1A;
    RssdWork.nUnk014 = pArg->nUnk1C;
    RssdWork.nResolution = pArg->nRes;
    SignalSema(RssdWork.nSemaId);
}

void RssdBusy_4(RSSD_ASYNC_ARG *pArg)
{
    RssdWork.nFlags |= 2;
    RssdWork.nUnk008 = pArg->nValue;
    RssdWork.nUnk014 = pArg->nUnk1C;
    RssdWork.nResolution = pArg->nRes;
    RssdWork.nUnk012 = pArg->nUnk1A;
    SignalSema(RssdWork.nSemaId);
}

void RssdBusy_5(RSSD_ASYNC_ARG *pArg)
{
    RssdWork.nFlags |= 2;
    RssdWork.nUnk008 = pArg->nValue;
    RssdWork.nUnk014 = pArg->nUnk1C;
    RssdWork.nUnk012 = pArg->nUnk1A;
    RssdWork.nResolution = pArg->nRes;
    SignalSema(RssdWork.nSemaId);
}

void RssdBusy_6(RSSD_ASYNC_ARG *pArg)
{
    RssdWork.nFlags |= 2;
    RssdWork.nResolution = pArg->nRes;
    RssdWork.nUnk008 = pArg->nValue;
    RssdWork.nUnk012 = pArg->nUnk1A;
    RssdWork.nUnk014 = pArg->nUnk1C;
    SignalSema(RssdWork.nSemaId);
}

void RssdBusy_7(RSSD_ASYNC_ARG *pArg)
{
    RssdWork.nFlags |= 2;
    RssdWork.nResolution = pArg->nRes;
    RssdWork.nUnk008 = pArg->nValue;
    RssdWork.nUnk014 = pArg->nUnk1C;
    RssdWork.nUnk012 = pArg->nUnk1A;
    SignalSema(RssdWork.nSemaId);
}

void RssdBusy_8(RSSD_ASYNC_ARG *pArg)
{
    RssdWork.nFlags |= 2;
    RssdWork.nResolution = pArg->nRes;
    RssdWork.nUnk012 = pArg->nUnk1A;
    RssdWork.nUnk008 = pArg->nValue;
    RssdWork.nUnk014 = pArg->nUnk1C;
    SignalSema(RssdWork.nSemaId);
}

void RssdBusy_9(RSSD_ASYNC_ARG *pArg)
{
    RssdWork.nFlags |= 2;
    RssdWork.nResolution = pArg->nRes;
    RssdWork.nUnk012 = pArg->nUnk1A;
    RssdWork.nUnk014 = pArg->nUnk1C;
    RssdWork.nUnk008 = pArg->nValue;
    SignalSema(RssdWork.nSemaId);
}

void RssdBusy_10(RSSD_ASYNC_ARG *pArg)
{
    RssdWork.nFlags |= 2;
    RssdWork.nResolution = pArg->nRes;
    RssdWork.nUnk014 = pArg->nUnk1C;
    RssdWork.nUnk008 = pArg->nValue;
    RssdWork.nUnk012 = pArg->nUnk1A;
    SignalSema(RssdWork.nSemaId);
}

void RssdBusy_11(RSSD_ASYNC_ARG *pArg)
{
    RssdWork.nFlags |= 2;
    RssdWork.nResolution = pArg->nRes;
    RssdWork.nUnk014 = pArg->nUnk1C;
    RssdWork.nUnk012 = pArg->nUnk1A;
    RssdWork.nUnk008 = pArg->nValue;
    SignalSema(RssdWork.nSemaId);
}

void RssdBusy_12(RSSD_ASYNC_ARG *pArg)
{
    RssdWork.nFlags |= 2;
    RssdWork.nUnk012 = pArg->nUnk1A;
    RssdWork.nUnk008 = pArg->nValue;
    RssdWork.nResolution = pArg->nRes;
    RssdWork.nUnk014 = pArg->nUnk1C;
    SignalSema(RssdWork.nSemaId);
}

void RssdBusy_13(RSSD_ASYNC_ARG *pArg)
{
    RssdWork.nFlags |= 2;
    RssdWork.nUnk012 = pArg->nUnk1A;
    RssdWork.nUnk008 = pArg->nValue;
    RssdWork.nUnk014 = pArg->nUnk1C;
    RssdWork.nResolution = pArg->nRes;
    SignalSema(RssdWork.nSemaId);
}

void RssdBusy_14(RSSD_ASYNC_ARG *pArg)
{
    RssdWork.nFlags |= 2;
    RssdWork.nUnk012 = pArg->nUnk1A;
    RssdWork.nResolution = pArg->nRes;
    RssdWork.nUnk008 = pArg->nValue;
    RssdWork.nUnk014 = pArg->nUnk1C;
    SignalSema(RssdWork.nSemaId);
}

void RssdBusy_15(RSSD_ASYNC_ARG *pArg)
{
    RssdWork.nFlags |= 2;
    RssdWork.nUnk012 = pArg->nUnk1A;
    RssdWork.nResolution = pArg->nRes;
    RssdWork.nUnk014 = pArg->nUnk1C;
    RssdWork.nUnk008 = pArg->nValue;
    SignalSema(RssdWork.nSemaId);
}

void RssdBusy_16(RSSD_ASYNC_ARG *pArg)
{
    RssdWork.nFlags |= 2;
    RssdWork.nUnk012 = pArg->nUnk1A;
    RssdWork.nUnk014 = pArg->nUnk1C;
    RssdWork.nUnk008 = pArg->nValue;
    RssdWork.nResolution = pArg->nRes;
    SignalSema(RssdWork.nSemaId);
}

void RssdBusy_17(RSSD_ASYNC_ARG *pArg)
{
    RssdWork.nFlags |= 2;
    RssdWork.nUnk012 = pArg->nUnk1A;
    RssdWork.nUnk014 = pArg->nUnk1C;
    RssdWork.nResolution = pArg->nRes;
    RssdWork.nUnk008 = pArg->nValue;
    SignalSema(RssdWork.nSemaId);
}

void RssdBusy_18(RSSD_ASYNC_ARG *pArg)
{
    RssdWork.nFlags |= 2;
    RssdWork.nUnk014 = pArg->nUnk1C;
    RssdWork.nUnk008 = pArg->nValue;
    RssdWork.nResolution = pArg->nRes;
    RssdWork.nUnk012 = pArg->nUnk1A;
    SignalSema(RssdWork.nSemaId);
}

void RssdBusy_19(RSSD_ASYNC_ARG *pArg)
{
    RssdWork.nFlags |= 2;
    RssdWork.nUnk014 = pArg->nUnk1C;
    RssdWork.nUnk008 = pArg->nValue;
    RssdWork.nUnk012 = pArg->nUnk1A;
    RssdWork.nResolution = pArg->nRes;
    SignalSema(RssdWork.nSemaId);
}

void RssdBusy_20(RSSD_ASYNC_ARG *pArg)
{
    RssdWork.nFlags |= 2;
    RssdWork.nUnk014 = pArg->nUnk1C;
    RssdWork.nResolution = pArg->nRes;
    RssdWork.nUnk008 = pArg->nValue;
    RssdWork.nUnk012 = pArg->nUnk1A;
    SignalSema(RssdWork.nSemaId);
}

void RssdBusy_21(RSSD_ASYNC_ARG *pArg)
{
    RssdWork.nFlags |= 2;
    RssdWork.nUnk014 = pArg->nUnk1C;
    RssdWork.nResolution = pArg->nRes;
    RssdWork.nUnk012 = pArg->nUnk1A;
    RssdWork.nUnk008 = pArg->nValue;
    SignalSema(RssdWork.nSemaId);
}

void RssdBusy_22(RSSD_ASYNC_ARG *pArg)
{
    RssdWork.nFlags |= 2;
    RssdWork.nUnk014 = pArg->nUnk1C;
    RssdWork.nUnk012 = pArg->nUnk1A;
    RssdWork.nUnk008 = pArg->nValue;
    RssdWork.nResolution = pArg->nRes;
    SignalSema(RssdWork.nSemaId);
}

void RssdBusy_23(RSSD_ASYNC_ARG *pArg)
{
    RssdWork.nFlags |= 2;
    RssdWork.nUnk014 = pArg->nUnk1C;
    RssdWork.nUnk012 = pArg->nUnk1A;
    RssdWork.nResolution = pArg->nRes;
    RssdWork.nUnk008 = pArg->nValue;
    SignalSema(RssdWork.nSemaId);
}
