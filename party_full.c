typedef struct {
    char pad00[0x24];               /* 0x00 */
    unsigned short nLockParty;      /* 0x24 */
    unsigned short nOutFriend;      /* 0x26 */
    unsigned short nFriend;         /* 0x28 */
    unsigned short nTakeAgws;       /* 0x2A */
    short nLeader;                  /* 0x2C */
    unsigned char nRadarDisp;       /* 0x2E */
} PARTY_DATA;

extern PARTY_DATA D_004A1828;

int func_A1A2E8(int);
void xglClockRead(void *);
void xglClockUInt2DayTime(void *, int);
int xglClockDayTime2UInt(void *);
extern int D_00491818[];
extern int _CountTime[];
int PartyTimeUpDate(void);
int PartyTimeLimitCheck(void *);

typedef struct {
    unsigned char nUnk0;
    unsigned char nUnk1;
    unsigned char nDay;
    unsigned char nMonth;
} PARTY_TIME_DISP;

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
    unsigned int nMask;

    pData = PartyDataGet();
    nShift = nNo - 1;
    if (nShift < 7 && nNo != 4) {
        nMask = 1 << nShift;
        return (pData->nFriend & nMask) != 0;
    }
    return 0;
}

void PartyLockPartyOn(int nNo)
{
    PARTY_DATA *pData;
    int nShift;

    pData = PartyDataGet();
    nShift = nNo - 1;
    if (nShift < 0xC && nNo != 4) {
        pData->nLockParty |= 1 << nShift;
    }
}

void PartyLockPartyOff(int nNo)
{
    PARTY_DATA *pData;
    int nShift;

    pData = PartyDataGet();
    nShift = nNo - 1;
    if (nShift < 0xC && nNo != 4) {
        pData->nLockParty &= ~(1 << nShift);
    }
}

int PartyLockPartyCheck(int nNo)
{
    PARTY_DATA *pData;
    int nShift;
    unsigned int nMask;

    pData = PartyDataGet();
    nShift = nNo - 1;
    if (nShift < 0xC && nNo != 4) {
        nMask = 1 << nShift;
        return (pData->nLockParty & nMask) != 0;
    }
    return 0;
}

void PartyOutFriendOn(int nNo)
{
    PARTY_DATA *pData;
    int nShift;

    pData = PartyDataGet();
    nShift = nNo - 1;
    if (nShift < 0xC && nNo != 4) {
        pData->nOutFriend |= 1 << nShift;
    }
}

void PartyOutFriendOff(int nNo)
{
    PARTY_DATA *pData;
    int nShift;

    pData = PartyDataGet();
    nShift = nNo - 1;
    if (nShift < 0xC && nNo != 4) {
        pData->nOutFriend &= ~(1 << nShift);
    }
}

int PartyOutFriendCheck(int nNo)
{
    PARTY_DATA *pData;
    int nShift;
    unsigned int nMask;

    pData = PartyDataGet();
    nShift = nNo - 1;
    if (nShift < 0xC && nNo != 4) {
        nMask = 1 << nShift;
        return (pData->nOutFriend & nMask) != 0;
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

void PartyTakeAgwsOff(int nNo)
{
    PARTY_DATA *pData;
    unsigned int nShift;

    pData = PartyDataGet();
    nShift = nNo - 0x11;
    if (nShift < 0x10) {
        pData->nTakeAgws &= ~(1 << nShift);
    }
}

int PartyTakeAgwsCheck(int nNo)
{
    PARTY_DATA *pData;
    unsigned int nShift;
    unsigned int nMask;

    pData = PartyDataGet();
    nShift = nNo - 0x11;
    if (nShift < 0x10) {
        nMask = 1 << nShift;
        return (pData->nTakeAgws & nMask) != 0;
    }
    return 0;
}

int PartyLeaderCheck(int nNo)
{
    PARTY_DATA *pData;
    int nCheck;

    pData = PartyDataGet();
    nCheck = func_A1A2E8(nNo);
    return (pData->nLeader ^ nCheck) == 0;
}

void PartyRadarDispSet(int nDisp)
{
    PartyDataGet()->nRadarDisp = nDisp;
}

void PartyTimeInit(void)
{
    xglClockUInt2DayTime(D_00491818, 0);
    xglClockRead(_CountTime);
}

void PartyTimePauseStart(void)
{
    PartyTimeUpDate();
}

void PartyTimePauseEnd(void)
{
    xglClockRead(_CountTime);
}

void PartyTimeDispChange(PARTY_TIME_DISP *pDisp)
{
    unsigned char nMonth;

    nMonth = pDisp->nMonth;
    if (nMonth != 0) {
        pDisp->nDay = pDisp->nDay + nMonth * 24 - 24;
    }
}
