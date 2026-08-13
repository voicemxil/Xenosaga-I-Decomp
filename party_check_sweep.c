typedef struct {
    char pad00[0x24];
    unsigned short nLockParty;
    unsigned short nOutFriend;
    unsigned short nFriend;
} PARTY_DATA;

PARTY_DATA *PartyDataGet(void);

int PartyFriendCheck_A(int nNo)
{
    PARTY_DATA *pData;
    unsigned int nShift;
    unsigned int nMask;

    pData = PartyDataGet();
    nShift = nNo - 1;
    if (nNo != 4 && nShift < 7) {
        nMask = 1 << nShift;
        return (pData->nFriend & nMask) != 0;
    }
    return 0;
}

int PartyFriendCheck_B(int nNo)
{
    unsigned int nShift;
    unsigned int nMask;
    PARTY_DATA *pData;

    pData = PartyDataGet();
    nShift = nNo - 1;
    if (nShift < 7 && nNo != 4) {
        nMask = 1 << nShift;
        return (pData->nFriend & nMask) != 0;
    }
    return 0;
}

int PartyFriendCheck_C(int nNo)
{
    PARTY_DATA *pData;
    unsigned int nShift;

    pData = PartyDataGet();
    nShift = nNo - 1;
    if (nShift < 7 && nNo != 4) {
        return (pData->nFriend & (1 << nShift)) != 0;
    }
    return 0;
}

int PartyFriendCheck_D(int nNo)
{
    register PARTY_DATA *pData __asm__("$6");
    unsigned int nShift;
    unsigned int nMask;

    pData = PartyDataGet();
    nShift = nNo - 1;
    if (nShift < 7 && nNo != 4) {
        nMask = 1 << nShift;
        return (pData->nFriend & nMask) != 0;
    }
    return 0;
}
