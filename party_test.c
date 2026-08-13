typedef struct {
    char pad00[0x24];
    unsigned short nLockParty;
    unsigned short nOutFriend;
    unsigned short nFriend;
    unsigned short nTakeAgws;
    short nLeader;
    unsigned char nRadarDisp;
} PARTY_DATA;

extern PARTY_DATA D_004A1828;

PARTY_DATA *PartyDataGet(void)
{
    return &D_004A1828;
}

void PartyFriendOn(int nNo)
{
    PARTY_DATA *pData;
    int nShift;

    pData = PartyDataGet();
    nShift = nNo - 1;
    if (nShift < 7 && nNo != 4) {
        pData->nFriend |= 1 << nShift;
    }
}

void PartyFriendOff(int nNo)
{
    PARTY_DATA *pData;
    int nShift;

    pData = PartyDataGet();
    nShift = nNo - 1;
    if (nShift < 7 && nNo != 4) {
        pData->nFriend &= ~(1 << nShift);
    }
}

int PartyFriendCheck(int nNo)
{
    PARTY_DATA *pData;
    int nShift;

    pData = PartyDataGet();
    nShift = nNo - 1;
    if (nShift < 7 && nNo != 4) {
        return (pData->nFriend & (1 << nShift)) != 0;
    }
    return 0;
}

void PartyTakeAgwsOn(int nNo)
{
    PARTY_DATA *pData;
    unsigned int nShift;

    pData = PartyDataGet();
    nShift = nNo - 0x11;
    if (nShift < 0x10) {
        pData->nTakeAgws |= 1 << nShift;
    }
}

int PartyFriendCheck2(int nNo)
{
    PARTY_DATA *pData;
    int nShift;
    unsigned int nMask;

    pData = PartyDataGet();
    nShift = nNo - 1;
    if (nShift < 7 && nNo != 4) {
        nMask = 1 << nShift;
        return (pData->nFriend & nMask) != 0;
    }
    return 0;
}

int PartyFriendCheck3(int nNo)
{
    PARTY_DATA *pData;
    int nShift;

    pData = PartyDataGet();
    nShift = nNo - 1;
    if (nShift < 7 && nNo != 4) {
        return ((pData->nFriend >> nShift) & 1) != 0 ? 1 : (pData->nFriend & (1 << nShift)) != 0;
    }
    return 0;
}
