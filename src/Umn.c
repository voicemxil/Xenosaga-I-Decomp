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
