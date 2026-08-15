#include "matching.h"

/* Party roster management - bitmask/status accessors around a single global PARTY_DATA record */

typedef struct {
    char pad00[0x24];               /* 0x00 */
    unsigned short nLockParty;      /* 0x24 */
    unsigned short nOutFriend;      /* 0x26 */
    unsigned short nFriend;         /* 0x28 */
    unsigned short nTakeAgws;       /* 0x2A */
    unsigned short nLeader;         /* 0x2C */
    unsigned char nRadarDisp;       /* 0x2E */
} PARTY_DATA;

typedef struct {
    int nA;                         /* 0x00 */
    int nB;                         /* 0x04 */
} XGL_CLOCK;

typedef struct {
    unsigned char nUnk0;            /* 0x00 */
    unsigned char nUnk1;            /* 0x01 */
    unsigned char nUnk2;            /* 0x02 */
    unsigned char nDay;             /* 0x03 */
    unsigned char nMonth;           /* 0x04 */
} PARTY_TIME_DISP;

extern PARTY_DATA D_004A1828;
extern int D_00491818[];
extern XGL_CLOCK _CountTime;

int func_A1A2E8(int);
void xglClockRead(XGL_CLOCK *);
void xglClockUInt2DayTime(void *, int);
int xglClockDayTime2UInt(void *);
void *PartyTimeUpDate(void);
int PartyTimeLimitCheck(void *);

/* Fetch the single global party status record */
PARTY_DATA *PartyDataGet(void)
{
    return &D_004A1828;
}

/* Mark a character as an active friend/companion */
void PartyFriendOn(int nNo)
{
    PARTY_DATA *pData;
    unsigned int nShift;

    pData = PartyDataGet();
    nShift = nNo - 1;
    if (nShift < 7 && nNo != 4) {
        pData->nFriend |= 1 << nShift;
    }
}

/* Clear a character's active-friend flag */
void PartyFriendOff(int nNo)
{
    PARTY_DATA *pData;
    unsigned int nShift;

    pData = PartyDataGet();
    nShift = nNo - 1;
    if (nShift < 7 && nNo != 4) {
        pData->nFriend &= ~(1 << nShift);
    }
}

/* Ask whether a character is currently an active friend */
int PartyFriendCheck(int nNo)
{
    PIN(PARTY_DATA *pData, "$6");
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

/* Lock a character into the active party */
void PartyLockPartyOn(int nNo)
{
    PARTY_DATA *pData;
    unsigned int nShift;

    pData = PartyDataGet();
    nShift = nNo - 1;
    if (nShift < 0xC && nNo != 4) {
        pData->nLockParty |= 1 << nShift;
    }
}

/* Release a character's party lock */
void PartyLockPartyOff(int nNo)
{
    PARTY_DATA *pData;
    unsigned int nShift;

    pData = PartyDataGet();
    nShift = nNo - 1;
    if (nShift < 0xC && nNo != 4) {
        pData->nLockParty &= ~(1 << nShift);
    }
}

/* Ask whether a character's party lock is set */
int PartyLockPartyCheck(int nNo)
{
    PIN(PARTY_DATA *pData, "$6");
    unsigned int nShift;
    unsigned int nMask;

    pData = PartyDataGet();
    nShift = nNo - 1;
    if (nShift < 0xC && nNo != 4) {
        nMask = 1 << nShift;
        return (pData->nLockParty & nMask) != 0;
    }
    return 0;
}

/* Mark a character as a friend belonging to the reserve (out-of-party) pool */
void PartyOutFriendOn(int nNo)
{
    PARTY_DATA *pData;
    unsigned int nShift;

    pData = PartyDataGet();
    nShift = nNo - 1;
    if (nShift < 0xC && nNo != 4) {
        pData->nOutFriend |= 1 << nShift;
    }
}

/* Clear a character's out-of-party friend flag */
void PartyOutFriendOff(int nNo)
{
    PARTY_DATA *pData;
    unsigned int nShift;

    pData = PartyDataGet();
    nShift = nNo - 1;
    if (nShift < 0xC && nNo != 4) {
        pData->nOutFriend &= ~(1 << nShift);
    }
}

/* Ask whether a character's out-of-party friend flag is set */
int PartyOutFriendCheck(int nNo)
{
    PIN(PARTY_DATA *pData, "$6");
    unsigned int nShift;
    unsigned int nMask;

    pData = PartyDataGet();
    nShift = nNo - 1;
    if (nShift < 0xC && nNo != 4) {
        nMask = 1 << nShift;
        return (pData->nOutFriend & nMask) != 0;
    }
    return 0;
}

/* Mark an AGWS slot as taken */
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

/* Clear an AGWS slot's taken flag */
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

/* TODO: Matches except an irreducible pointer/mask register tie-break (a0 vs a1). */
/* Ask whether an AGWS slot is currently taken */
int PartyTakeAgwsCheck(int nNo)
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

/* Ask whether a character is the current party leader */
int PartyLeaderCheck(int nNo)
{
    PARTY_DATA *pData;
    int nCheck;

    pData = PartyDataGet();
    nCheck = func_A1A2E8(nNo);
    return (pData->nLeader ^ nCheck) == 0;
}

/* Toggle whether the radar is displayed */
void PartyRadarDispSet(int nDisp)
{
    PartyDataGet()->nRadarDisp = nDisp;
}

/* Seed the elapsed-play-time counters from the current system clock */
void PartyTimeInit(void)
{
    xglClockUInt2DayTime(D_00491818, 0);
    xglClockRead(&_CountTime);
}

/* Freeze the running play-time counter (delegates to the update routine) */
void PartyTimePauseStart(void)
{
    PartyTimeUpDate();
}

/* Resume the running play-time counter from the current system clock */
void PartyTimePauseEnd(void)
{
    xglClockRead(&_CountTime);
}

/* Advance the play-time counters and re-check the elapsed-time limit */
void *PartyTimeUpDate(void)
{
    XGL_CLOCK sTemp;
    int nBaseUint;
    int nCountUint;
    int nTempUint;

    xglClockRead(&sTemp);
    nBaseUint = xglClockDayTime2UInt(D_00491818);
    nCountUint = xglClockDayTime2UInt(&_CountTime);
    nTempUint = xglClockDayTime2UInt(&sTemp);
    xglClockUInt2DayTime(D_00491818, nBaseUint + (nTempUint - nCountUint));
    _CountTime = sTemp;
    PartyTimeLimitCheck(D_00491818);
    return D_00491818;
}

/* TODO: Matches except an irreducible a0/a1 alias-copy register tie-break. */
/* Roll a display day/month pair forward by one in-game "tick" */
void PartyTimeDispChange(PARTY_TIME_DISP *pDisp)
{
    unsigned char nMonth;

    nMonth = pDisp->nMonth;
    if (nMonth != 0) {
        pDisp->nDay = pDisp->nDay + nMonth * 24 - 24;
    }
}

extern void PartyAttackerSet(int, int, int);

/* Reset the starting friend/attacker roster and clear the leader slot */
void PartyDataInit2(void)
{
    PartyFriendOn(3);
    PartyFriendOn(1);
    PartyFriendOn(2);
    PartyAttackerSet(0, 3, 1);
    PartyAttackerSet(1, 1, 2);
    PartyAttackerSet(2, 2, 3);
    PartyDataGet()->nLeader = 0;
}

/* Count the set bits (0-15) in the party-membership byte pair at the front of the record */
int PartyCharNumGet(void)
{
    unsigned char *p;
    int i;
    int cnt;
    int byteIdx;
    int bitIdx;
    unsigned char b;

    p = (unsigned char *)PartyDataGet();
    for (i = 0, cnt = 0; i < 16; i++) {
        byteIdx = i / 8;
        bitIdx = i - byteIdx * 8;
        b = p[byteIdx];
        cnt += (b >> bitIdx) & 1;
    }
    return cnt;
}

/* Count the set bits (0-15) in the AGWS-slot nibble-per-byte record at the front of the record */
int PartyAgwsNumGet(void)
{
    unsigned char *p;
    int i;
    int cnt;
    int byteIdx;
    int bitIdx;
    unsigned char b;

    p = (unsigned char *)PartyDataGet();
    for (i = 0, cnt = 0; i < 16; i++) {
        byteIdx = i / 4;
        bitIdx = i - byteIdx * 4;
        b = p[byteIdx];
        cnt += (b >> bitIdx) & 1;
    }
    return cnt;
}

typedef struct {
    unsigned short nPos;
    signed char nId;
    char pad3;
} ATTACKPOS;

/* Search the 3-slot attack-position table by character id, return its assigned position */
int PartyAttackPosCheck(int nId)
{
    ATTACKPOS *p;
    int i;

    p = (ATTACKPOS *)((char *)PartyDataGet() + 0x30);
    for (i = 0; i < 3; i++, p++) {
        if (p->nId == nId) {
            return p->nPos;
        }
    }
    return 0;
}

/* Search the 3-slot attack-position table by position, return its assigned character id */
int PartyAttackPosGet(int nPos)
{
    ATTACKPOS *p;
    int i;

    p = (ATTACKPOS *)((char *)PartyDataGet() + 0x30);
    for (i = 0; i < 3; i++, p++) {
        if (p->nPos == nPos) {
            return p->nId;
        }
    }
    return 0;
}

/* Assign a character id to the attack-position table slot matching nPos */
void PartyAttackPosSet(int nPos, int nId)
{
    ATTACKPOS *p;
    int i;

    p = (ATTACKPOS *)((char *)PartyDataGet() + 0x30);
    for (i = 0; i < 3; i++, p++) {
        if (p->nPos == nPos) {
            p->nId = nId;
            return;
        }
    }
}

extern int MenuMaryIdChange(int);

/* TODO: near-miss - loop structure/scheduling diff (32 orig vs 35 built words); parked after 2 attempts. */
/* Fill out[] with converted character ids for each nonzero attack-position slot, return the count */
int PartyAttackerGet(int *out)
{
    ATTACKPOS *p;
    int i;
    int cnt;
    int id;

    p = (ATTACKPOS *)((char *)PartyDataGet() + 0x30);
    cnt = 0;
    for (i = 0; i < 3; i++, out++) {
        if (p->nPos == 0) {
            break;
        }
        id = MenuMaryIdChange(p->nPos);
        p++;
        cnt++;
        *out = id;
    }
    return cnt;
}
