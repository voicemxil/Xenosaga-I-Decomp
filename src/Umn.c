/* Menu (Umn) subsystem - mail box, encyclopedia flags and the mail screen driver */

typedef unsigned short u_short;

typedef struct {
    u_short pad000[8];              /* 0x00 */
    u_short nDispFbp;               /* 0x10 */
    u_short nUnk12;                 /* 0x12 */
    u_short pad014[6];              /* 0x14 */
} XGLRENDER;

typedef struct {
    int pad000[4];                  /* 0x00 */
    int nFlags;                     /* 0x10 */
} GAMELOOP;

typedef struct {
    char nMailBox[0x80];            /* 0x00 */
    char pad080[0x8];               /* 0x80 */
    unsigned char nMonster[0x10];   /* 0x88 */
    unsigned char nAnalisis[0x10];  /* 0x98 */
} UMN_DATABASE;

short UmnKosmosSpecialBox[4];

extern XGLRENDER sRender;
extern GAMELOOP GameLoopState;
extern UMN_DATABASE D_004A1A0C;
extern short D_004A1AB4[];

extern void *memset(void *pDst, int nVal, unsigned int nSize);
extern void func_A19750(int nType, int nId);
extern void *xglPacketGetCurrent(void);
extern void endPrintDirectFrameCopy(void *pPacket, int nSrc, int nDst);
extern void xglSleep(void);
extern void xglCdLoadOverlay(int nOverlay);
extern void UmnMain2(int nMode);

typedef struct { char b[3]; } MAIL_ATTACH;
typedef struct { char b[0x90]; } MAIL_HEADER;
typedef struct {
    short nLimit;
    unsigned short nPos;
    char entries[1][8];
} EVENT_TEXT_BUF;

extern MAIL_ATTACH umn_attach_tbl[];
extern MAIL_HEADER *UmnMailHeaderBuf[];
extern EVENT_TEXT_BUF *uet_text_buf[];
extern int uet_flag;
extern int *UmnHistoryTreeBuf[];
extern char *umn_text[];

/* Debug hook for the menu print test, stubbed out in the retail build */
void UmnPrintTest(void)
{
}

/* Clear the pending KOS-MOS special-attack unlock queue */
void UmnkosmosSpecialInit(void)
{
    memset(UmnKosmosSpecialBox, 0, 8);
}

/* Flush the queued KOS-MOS special-attack unlocks back into the party data */
void UmnkosmosSpecialSet(void)
{
    int i;
    short *pBox;
    short nId;

    i = 0;
    pBox = UmnKosmosSpecialBox;
    while (1) {
        if (i >= 4) {
            return;
        }
        i++;
        nId = *pBox++;
        if (nId == 0) {
            return;
        }
        func_A19750(2, nId);
    }
}

/* Run the mail viewer: swap in the menu overlay, drive it, then restore */
void UmnMailMain(int nMode)
{
    UmnkosmosSpecialInit();
    endPrintDirectFrameCopy(xglPacketGetCurrent(), sRender.nDispFbp << 5, sRender.nUnk12 << 5);
    GameLoopState.nFlags |= 0x4000000;
    xglSleep();
    xglCdLoadOverlay(2);
    xglSleep();
    UmnMain2(nMode);
    xglCdLoadOverlay(1);
    xglSleep();
    UmnkosmosSpecialSet();
}

/* Append a mail id to the first free mail box slot */
void UmnMailBoxSet(int nNo)
{
    char *pBox;
    int i;

    pBox = (char *)&D_004A1A0C;
    for (i = 0; i < 0x80; i++) {
        if (pBox[i] == -1) {
            pBox[i] = nNo;
            break;
        }
    }
}

/* Look up one mail attachment table entry by index */
MAIL_ATTACH *UmnMailAttachGet(int nNo)
{
    return &umn_attach_tbl[nNo];
}

/* Look up one mail header table entry by index */
MAIL_HEADER *UmnMailHeaderGet(int nNo)
{
    return &UmnMailHeaderBuf[0][nNo];
}

/* TODO: near-miss (LOGIC, register-alloc/scheduling) -- not registered. */
/* Align the raw event-text buffer, install it, and return the entry table start */
void *UmnEventTextInit(void *pRaw)
{
    EVENT_TEXT_BUF *pAligned = (EVENT_TEXT_BUF *)(((unsigned int)pRaw + 15) & ~15);
    uet_text_buf[0] = pAligned;
    uet_flag = 0;
    return (char *)pAligned + 0x2000;
}

/* TODO: near-miss (LOGIC, register-alloc/scheduling) -- not registered. */
/* Look up a history-tree slot by index; NULL if out of range */
int *UmnHistoryTreeGet(int nNo)
{
    int *pRet = 0;
    if ((unsigned int)nNo < 128) {
        pRet = &UmnHistoryTreeBuf[0][nNo];
    }
    return pRet;
}

/* TODO: near-miss (LOGIC, register-alloc/scheduling) -- not registered. */
/* Look up a plugin text slot by index; clamps to the base slot when out of range */
char *UmnPluginTextGet(int nNo)
{
    char *pBase = umn_text[0];
    char *pRet = pBase;
    if ((unsigned int)nNo < 6) {
        pRet = pBase + nNo * 128;
    }
    return pRet;
}

/* TODO: near-miss (LOGIC, register-alloc/scheduling) -- not registered. */
/* Advance the event-text cursor by nInc entries and return the next entry slot */
void *UmnEventTextNextGet(int nInc)
{
    EVENT_TEXT_BUF *pBuf = uet_text_buf[0];
    unsigned short nNewPos = pBuf->nPos + nInc;
    short nPos;

    pBuf->nPos = nNewPos;
    nPos = (short)nNewPos;
    if (nPos < pBuf->nLimit) {
        return (char *)pBuf + nPos * 8 + 4;
    }
    return 0;
}

/* TODO: near-miss (LOGIC, register-alloc/scheduling on the loop) -- not registered. */
/* Skip the text cursor past a run of newline characters */
void UmnEventTextGyouJump(char **ppText)
{
    unsigned char *p = (unsigned char *)*ppText;
    if (*p == '\n') {
        do {
            p++;
            *ppText = (char *)p;
        } while (*p == '\n');
    }
}

/* Look up one mail table entry by index */
/* TODO: Find the natural source shape for this matched return-delay scaffold. */
short *UmnMailDataGet(int nNo)
{
    return &D_004A1AB4[nNo];
}

/* Mark a monster as registered in the encyclopedia */
void UmnDataBaseMonsterSet(int nNo)
{
    UMN_DATABASE *pData;
    int n;

    n = nNo - 0x22;
    pData = &D_004A1A0C;
    if ((unsigned int)n < 0x1D) {
        pData->nMonster[n / 8] |= 1 << (n % 8);
    }
}

/* Test whether a monster is registered in the encyclopedia */
int UmnDataBaseMonsterCheck(int nNo)
{
    UMN_DATABASE *pData;
    int n;
    int nRet;

    n = nNo - 0x22;
    pData = &D_004A1A0C;
    nRet = 0;
    if ((unsigned int)n < 0x1D) {
        nRet = ((unsigned int)pData->nMonster[n / 8] >> (n % 8)) & 1;
    }
    return nRet;
}

/* Mark a monster as fully analysed in the encyclopedia */
void UmnDataBaseAnalisisSet(int nNo)
{
    UMN_DATABASE *pData;
    int n;

    n = nNo - 0x22;
    pData = &D_004A1A0C;
    if ((unsigned int)n < 0x1D) {
        pData->nAnalisis[n / 8] |= 1 << (n % 8);
    }
}

/* Test whether a monster has been fully analysed */
int UmnDataBaseAnalisisCheck(int nNo)
{
    UMN_DATABASE *pData;
    int n;
    int nRet;

    n = nNo - 0x22;
    pData = &D_004A1A0C;
    nRet = 0;
    if ((unsigned int)n < 0x1D) {
        nRet = ((unsigned int)pData->nAnalisis[n / 8] >> (n % 8)) & 1;
    }
    return nRet;
}
