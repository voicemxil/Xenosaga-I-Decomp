/* Menu (Umn) subsystem - mail box, encyclopedia flags and the mail screen driver */

#include "matching.h"

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

typedef struct {
    unsigned char nFlags;           /* 0x00 */
    unsigned char nFlags2;          /* 0x01 */
} MAIL_DATA;

short UmnKosmosSpecialBox[4];

extern XGLRENDER sRender;
extern GAMELOOP GameLoopState;
extern UMN_DATABASE D_004A1A0C;
extern MAIL_DATA D_004A1AB4[];

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

typedef struct {
    char pad00[1];
    unsigned char nScene;           /* 0x01 */
    char pad02[1];
    unsigned char nPage;            /* 0x03 */
    char pad04[0x80 - 0x04];
} UMN_WORK;

typedef struct { char pad[0x68]; } GUNO_DATA;

extern UMN_WORK UmnWork;
extern GUNO_DATA *UmnGunoDataBaseTop[];
extern signed char compulsion_down_load_tbl[];
extern unsigned char event_tbl[];
extern signed char event_end_mail[];

extern int xglCdReadFile(char *pName, unsigned int nAddr, int nOfs, int nSize);
extern void UmnProcuratorSet(void);

extern MAIL_ATTACH umn_attach_tbl[];
extern MAIL_HEADER *UmnMailHeaderBuf[];
extern EVENT_TEXT_BUF *uet_text_buf[];
extern int uet_flag[];
#define uet_flag (uet_flag[0])
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

/* Align the raw event-text buffer, install it, and return the entry table start */
void *UmnEventTextInit(void *pRaw)
{
    EVENT_TEXT_BUF *pAligned = (EVENT_TEXT_BUF *)(((unsigned int)pRaw + 15) & ~15);
    uet_text_buf[0] = pAligned;
    uet_flag = 0;
    return (char *)pAligned + 0x2000;
}

/* Look up a history-tree slot by index; NULL if out of range */
int *UmnHistoryTreeGet(int nNo)
{
    int *pBase = UmnHistoryTreeBuf[0];
    int *pRet = 0;
    if ((unsigned int)nNo < 128) {
        pRet = pBase + nNo;
    }
    return pRet;
}

char *UmnPluginTextGet(int nNo)
{
    char *pBase = umn_text[0];
    if ((unsigned int)nNo >= 6) {
        return pBase;
    }
    return pBase + nNo * 128;
}

/* Advance the event-text cursor by nInc entries and return the next entry slot */
void *UmnEventTextNextGet(int nInc)
{
    EVENT_TEXT_BUF *pBuf = uet_text_buf[0];
    int nNewPos = pBuf->nPos + nInc;
    short nPos;
    PIN(void *result, "$2") = 0;

    pBuf->nPos = (unsigned short)nNewPos;
    nPos = (short)nNewPos;
    if (nPos < pBuf->nLimit) {
        result = (char *)pBuf + nPos * 8 + 4;
    }
    return result;
}

/* Skip the text cursor past a run of newline characters */
void UmnEventTextGyouJump(char **ppText)
{
    unsigned char *p = (unsigned char *)*ppText;
    unsigned char *q;
    if (*p == '\n') {
        do {
            q = p + 1;
            *ppText = (char *)q;
            p = q;
        } while (*p == '\n');
    }
}

/* TODO: near-miss (LOGIC, 25 diffs, was 33) -- not registered. Algorithm is
   confirmed correct (decimal vs octal digit run parser, byte-truncated
   accumulator matches the andi 0xff in the original). PIN(char **p, "$8")
   forces the original's bare "move t0,a0" staging instruction (a plain
   local no longer got optimized back to a0) and took this from 33 to 25
   diffs. Remaining diffs: the original's *entry* range check (c<'0' ||
   c>nMax, before the loop) uses sltiu+sltu (unsigned) while gcc emits
   sltiu+slt (signed) for the same C comparison against `int nMax` here;
   rewriting as a single rotated `while` loop (no separate precheck) matches
   the original's control-flow shape (one shared test block, entered via an
   initial jump and re-entered on the back edge skipping the pointer
   reload) but regressed to 27 diffs -- the do-while+precheck form controls
   better despite looking structurally different. Declaring nMax as
   `unsigned int` to coax sltu out of the comparison did not change the
   opcode choice; explicit `(unsigned char)nMax` casts on both upper-bound
   tests regressed further (29 diffs, LENGTH). Leave at 25. */
/* Parse a run of digits (decimal when nMode == 1, otherwise octal) starting
   at *ppText, advancing *ppText past the digits consumed.

   Three source facts carry most of this: nMax is unsigned (the original
   compares the digit with sltu, not slt); the function has a SINGLE exit
   returning nVal, which is what lets the "no digits" path share the
   epilogue instead of const-folding to `move v0,zero`; and the running
   value is truncated TWICE per digit -- once after the multiply and once
   after the add -- so the accumulator is an unsigned char written by two
   separate assignments, not one cast expression.

   The product needs its own variable: with `nVal *= nBase` gcc gives the
   multiply the same register as nVal, and then it cannot be hoisted into
   the guard branch's delay slot (nVal is the return value on the taken
   path), which costs both that slot and the loop-head alignment. */
int UmnEventTextNumberGet(char **ppText, int nMode)
{
    /* $8/$5/$3: allocator tie-breaks. p is a plain copy of the argument
       and gcc otherwise leaves it in $a0; nProd and the digit both land
       one register off, and every dependent name follows them. */
    PIN(char **p, "$8");
    PIN(int nProd, "$5");
    PIN(unsigned char c, "$3");
    int nBase;
    unsigned int nMax;
    unsigned char nVal;

    p = ppText;
    nVal = 0;
    if (nMode == 1) {
        nMax = '9';
        nBase = 10;
    } else {
        nMax = '7';
        nBase = 8;
    }

    c = **p;
    if (c >= '0' && c <= nMax) {
        do {
            nProd = nVal * nBase;
            nVal = nProd;
            nVal += **p - '0';
            (*p)++;
            c = **p;
        } while (c >= '0' && c <= nMax);
    }
    return nVal;
}

/* Look up one mail table entry by index */
/* TODO: Find the natural source shape for this matched return-delay scaffold. */
MAIL_DATA *UmnMailDataGet(int nNo)
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


/* ------------------------------------------------------------------ */

/* Look up a Gnosis-database record by monster id; ids outside the
   34..62 range fall back to the table's first entry. */
GUNO_DATA *UmnGunoDataBaseGet(int nNo)
{
    GUNO_DATA *pTop = UmnGunoDataBaseTop[0];

    if ((unsigned int)(nNo - 34) < 29) {
        return &pTop[nNo - 34];
    }
    return pTop;
}

/* Load the conversation-history tree into a 2K-aligned slice of the
   caller's scratch block and return the next free byte. */
void *UmnHistoryTreeLoad(void *pRaw)
{
    char *pAligned;
    char *pNext;

    pAligned = (char *)(((unsigned int)pRaw + 2047) & ~2047);
    pNext = pAligned + 2048;
    UmnHistoryTreeBuf[0] = (int *)pAligned;
    xglCdReadFile("data\\endou\\umn\\histree.bin", (unsigned int)pAligned, 0, 0);
    return pNext;
}

/* Switch the menu to a top-level scene: wipe the per-scene scratch half
   of UmnWork, reset the page and rebuild the 3D procurator model. */
void UmnChangeTopLevel(int nScene)
{
    typedef int T128 __attribute__((mode(TI)));
    UMN_WORK *w;
    T128 *p;
    int i;

    w = &UmnWork;
    p = (T128 *)((char *)w + 0x10);
    for (i = 0; i < 7; i++) {
        *p = 0;
        p++;
    }
    UmnWork.nScene = nScene;
    UmnWork.nPage = 0;
    UmnProcuratorSet();
}

/* True when this mail is one of the story mails whose download the game
   forces, and it has not been pulled down yet. */
int UmnMailCompulsionDownLoadCheck(int nNo)
{
    int i;

    for (i = 0; i < 13; i++) {
        if (compulsion_down_load_tbl[i] == nNo) {
            if ((UmnMailDataGet(nNo)->nFlags2 & 0x80) == 0) {
                return 1;
            }
            break;
        }
    }
    return 0;
}

/* Index+1 of this mail in the event-mail table if its event has not yet
   fired, else 0. */
int UmnMailEventCheck(int nNo)
{
    int i;

    for (i = 0; i < 18; i++) {
        if (event_tbl[i] == nNo) {
            if ((UmnMailDataGet(nNo)->nFlags & 4) == 0) {
                return i + 1;
            }
            break;
        }
    }
    return 0;
}

/* When the just-finished event is the one that unlocks the end-of-event
   mail, mark that mail seen and drop it in the inbox. */
int UmnEventEndMailCheck(int nNo)
{
    MAIL_DATA *pData;
    signed char *pMail;
    unsigned char nFlags;
    unsigned char nSeen;

    if (event_end_mail[0] == nNo) {
        pMail = &event_end_mail[1];
        pData = UmnMailDataGet(*pMail);
        nFlags = pData->nFlags;
        nSeen = nFlags & 1;
        if (nSeen == 0) {
            pData->nFlags = nFlags | 1;
            UmnMailBoxSet(*pMail);
            return 1;
        }
    }
    return 0;
}
