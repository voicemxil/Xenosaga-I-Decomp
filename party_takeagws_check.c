typedef struct {
    char pad00[0x24];
    unsigned short nLockParty;
    unsigned short nOutFriend;
    unsigned short nFriend;
    unsigned short nTakeAgws;
} PARTY_DATA;

extern PARTY_DATA D_004A1828;
PARTY_DATA *PartyDataGet(void);

int PartyTakeAgwsCheck_A(int nNo)
{
    PARTY_DATA *pData;
    unsigned int nShift;
    unsigned int nMask;

    pData = PartyDataGet();
    nShift = nNo - 0x11;
    nMask = 1 << nShift;
    if (nShift >= 0x10) {
        return 0;
    }
    return (pData->nTakeAgws & nMask) != 0;
}

int PartyTakeAgwsCheck_B(int nNo)
{
    PARTY_DATA *pData;
    unsigned int nShift;
    unsigned int nMask;
    int nResult;

    pData = PartyDataGet();
    nShift = nNo - 0x11;
    nMask = 1 << nShift;
    nResult = 0;
    if (nShift < 0x10) {
        nResult = (pData->nTakeAgws & nMask) != 0;
    }
    return nResult;
}

int PartyTakeAgwsCheck_C(int nNo)
{
    PARTY_DATA *pData;
    unsigned int nShift;

    pData = PartyDataGet();
    nShift = nNo - 0x11;
    if (nShift >= 0x10) {
        return 0;
    }
    return (pData->nTakeAgws & (1 << nShift)) != 0;
}

int PartyTakeAgwsCheck_D(int nNo)
{
    unsigned int nShift;
    unsigned int nMask;
    PARTY_DATA *pData;

    pData = PartyDataGet();
    nShift = nNo - 0x11;
    nMask = 1 << nShift;
    if (nShift >= 0x10) {
        return 0;
    }
    return (pData->nTakeAgws & nMask) != 0;
}

int PartyTakeAgwsCheck_E(int nNo)
{
    register PARTY_DATA *pData __asm__("$4");
    unsigned int nShift;
    unsigned int nMask;

    pData = PartyDataGet();
    nShift = nNo - 0x11;
    nMask = 1 << nShift;
    if (nShift >= 0x10) {
        return 0;
    }
    return (pData->nTakeAgws & nMask) != 0;
}
