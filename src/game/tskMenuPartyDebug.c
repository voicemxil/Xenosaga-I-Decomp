#include "matching.h"

/* tskMenuPartyDebug - pause-menu party debug pages (EUC-JP debug UI) */

typedef struct PADDATA {
    char pad00[0x2A];
    unsigned short hHeld;          /* 0x2A */
    unsigned short hTrig;          /* 0x2C */
} PADDATA;

extern PADDATA PadData;

static int PartyDebugPage;
static int PartyDebugSel;
static int PartyDebugSelWrap;

/* "off" / "on" strings indexed by PartyLockPartyCheck result */
extern char *LockStrs[];

extern void xglFontDebugPrintf(int x, int y, char *fmt, ...);
extern char *MenuCharNameGet(int nChr);
extern int PartyLockPartyCheck(int nChr);
extern void PartyLockPartyOn(int nChr);
extern void PartyLockPartyOff(int nChr);

extern void PauseMenuPagePartyDebugFriend(void);
extern void PauseMenuPagePartyDebugAttacker(void);
extern void PauseMenuPagePartyDebugOutFriend(void);
extern void PauseMenuPagePartyDebugTakeAgws(void);

static int PartyDebugFriendSel;
static int PartyDebugFriendSelWrap;
static int PartyDebugOutFriendSel;
static int PartyDebugOutFriendSelWrap;

extern char *FriendStrs[];
extern char *OutFriendStrs[];

extern int PartyFriendCheck(int nChr);
extern void PartyFriendOn(int nChr);
extern void PartyFriendOff(int nChr);
extern int PartyOutFriendCheck(int nChr);
extern void PartyOutFriendOn(int nChr);
extern void PartyOutFriendOff(int nChr);

static int PartyDebugAgwsSel;
static int PartyDebugAgwsSelWrap;

extern char *AgwsStrs[];

extern int PartyTakeAgwsCheck(int nChr);
extern void PartyTakeAgwsOn(int nChr);
extern void PartyTakeAgwsOff(int nChr);

/* AGWS-take page: toggles whether each of the 12 AGWS slots
 * (char ids 17..28) is taken along */
void PauseMenuPagePartyDebugTakeAgws(void)
{
    int sel;
    int i;
    int j;
    int col;
    int row;
    PIN(int trig, "$5");

    trig = PadData.hTrig;
    switch (trig) {
    case 0x8000:
        sel = PartyDebugAgwsSel;
        sel -= 1;
        break;
    case 0x2000:
        sel = PartyDebugAgwsSel;
        sel += 1;
        break;
    case 0x1000:
        sel = PartyDebugAgwsSel;
        sel -= 2;
        break;
    case 0x4000:
        sel = PartyDebugAgwsSel;
        sel += 2;
        break;
    default:
        sel = PartyDebugAgwsSel;
        break;
    }
    if (sel < 0) {
        sel += 12;
        PartyDebugAgwsSelWrap = sel;
    }
    PartyDebugAgwsSel = sel % 12;
    if (PadData.hHeld & 0x20) {
        if (PartyTakeAgwsCheck(PartyDebugAgwsSel + 17) != 0) {
            PartyTakeAgwsOff(PartyDebugAgwsSel + 17);
        } else {
            PartyTakeAgwsOn(PartyDebugAgwsSel + 17);
        }
    }
    xglFontDebugPrintf(8, 32,
        "\xa1\xbc\xa1\xa1\xa3\xc1\xa3\xc7\xa3\xd7\xa3\xd3\xc0\xdf\xc4\xea"
        "\xa1\xa1\xa1\xbc");
    {
        int cs = PartyDebugAgwsSel;

        xglFontDebugPrintf(cs % 2 * 128 + 8, cs / 2 * 16 + 48, "\xa1\xe4");
    }
    i = 0;
    do {
        j = i + 17;
        col = i % 2 * 128;
        row = i / 2 * 16 + 48;
        i++;
        xglFontDebugPrintf(col + 16, row, MenuCharNameGet(j));
        xglFontDebugPrintf(col + 104, row, AgwsStrs[PartyTakeAgwsCheck(j)]);
    } while (i < 12);
    xglFontDebugPrintf(8, 192,
        "\xbd\xbd\xbb\xfa\xa5\xad\xa1\xbc\xa1\xa7\xa5\xab\xa1\xbc\xa5\xbd"
        "\xa5\xeb\xb0\xdc\xc6\xb0\xa1\xa1\xa1\xa1\xa1\xa1\xa1\xfb\xa1\xa7"
        "\xa4\xa2\xa4\xea\xa1\xbf\xa4\xca\xa4\xb7");
}

/* Party-lock page: d-pad moves the 2x6 cursor, the confirm button
 * toggles the character's party lock, and all 12 slots are listed */
void PauseMenuPagePartyDebugLockParty(void)
{
    int sel;
    int i;
    int j;
    int col;
    int row;
    PIN(int trig, "$5");

    trig = PadData.hTrig;
    switch (trig) {
    case 0x8000:
        sel = PartyDebugSel;
        sel -= 1;
        break;
    case 0x2000:
        sel = PartyDebugSel;
        sel += 1;
        break;
    case 0x1000:
        sel = PartyDebugSel;
        sel -= 2;
        break;
    case 0x4000:
        sel = PartyDebugSel;
        sel += 2;
        break;
    default:
        sel = PartyDebugSel;
        break;
    }
    if (sel < 0) {
        sel += 12;
        PartyDebugSelWrap = sel;
    }
    PartyDebugSel = sel % 12;
    if (PadData.hHeld & 0x20) {
        if (PartyLockPartyCheck(PartyDebugSel + 1) != 0) {
            PartyLockPartyOff(PartyDebugSel + 1);
        } else {
            PartyLockPartyOn(PartyDebugSel + 1);
        }
    }
    xglFontDebugPrintf(8, 32,
        "\xa1\xbc\xa1\xa1\xa5\xed\xa5\xc3\xa5\xaf\xa5\xd1\xa1\xbc\xa5\xc6"
        "\xa5\xa3\xa1\xbc\xc0\xdf\xc4\xea\xa1\xa1\xa1\xbc");
    {
        int cs = PartyDebugSel;

        xglFontDebugPrintf(cs % 2 * 128 + 8, cs / 2 * 16 + 48, "\xa1\xe4");
    }
    i = 0;
    do {
        j = i + 1;
        col = i % 2 * 128;
        row = i / 2 * 16 + 48;
        i = j;
        LAUNDER(i);
        xglFontDebugPrintf(col + 16, row, MenuCharNameGet(j));
        xglFontDebugPrintf(col + 88, row, LockStrs[PartyLockPartyCheck(j)]);
    } while (i < 12);
    xglFontDebugPrintf(8, 192,
        "\xbd\xbd\xbb\xfa\xa5\xad\xa1\xbc\xa1\xa7\xa5\xab\xa1\xbc\xa5\xbd"
        "\xa5\xeb\xb0\xdc\xc6\xb0\xa1\xa1\xa1\xa1\xa1\xa1\xa1\xfb\xa1\xa7"
        "\xa5\xaa\xa5\xf3\xa1\xbf\xa5\xaa\xa5\xd5");
}

/* Out-friend page: toggles the out-of-party friend flag for all
 * 12 slots */
void PauseMenuPagePartyDebugOutFriend(void)
{
    int sel;
    int i;
    int j;
    int col;
    int row;
    PIN(int trig, "$5");

    trig = PadData.hTrig;
    switch (trig) {
    case 0x8000:
        sel = PartyDebugOutFriendSel;
        sel -= 1;
        break;
    case 0x2000:
        sel = PartyDebugOutFriendSel;
        sel += 1;
        break;
    case 0x1000:
        sel = PartyDebugOutFriendSel;
        sel -= 2;
        break;
    case 0x4000:
        sel = PartyDebugOutFriendSel;
        sel += 2;
        break;
    default:
        sel = PartyDebugOutFriendSel;
        break;
    }
    if (sel < 0) {
        sel += 12;
        PartyDebugOutFriendSelWrap = sel;
    }
    PartyDebugOutFriendSel = sel % 12;
    if (PadData.hHeld & 0x20) {
        if (PartyOutFriendCheck(PartyDebugOutFriendSel + 1) != 0) {
            PartyOutFriendOff(PartyDebugOutFriendSel + 1);
        } else {
            PartyOutFriendOn(PartyDebugOutFriendSel + 1);
        }
    }
    xglFontDebugPrintf(8, 32,
        "\xa1\xbc\xa1\xa1\xa5\xa2\xa5\xa6\xa5\xc8\xa5\xd5\xa5\xec\xa5\xf3"
        "\xa5\xc9\xc0\xdf\xc4\xea\xa1\xa1\xa1\xbc");
    {
        int cs = PartyDebugOutFriendSel;

        xglFontDebugPrintf(cs % 2 * 128 + 8, cs / 2 * 16 + 48, "\xa1\xe4");
    }
    i = 0;
    do {
        j = i + 1;
        col = i % 2 * 128;
        row = i / 2 * 16 + 48;
        i = j;
        LAUNDER(i);
        xglFontDebugPrintf(col + 16, row, MenuCharNameGet(j));
        xglFontDebugPrintf(col + 88, row, OutFriendStrs[PartyOutFriendCheck(j)]);
    } while (i < 12);
    xglFontDebugPrintf(8, 192,
        "\xbd\xbd\xbb\xfa\xa5\xad\xa1\xbc\xa1\xa7\xa5\xab\xa1\xbc\xa5\xbd"
        "\xa5\xeb\xb0\xdc\xc6\xb0\xa1\xa1\xa1\xa1\xa1\xa1\xa1\xfb\xa1\xa7"
        "\xa5\xaa\xa5\xf3\xa1\xbf\xa5\xaa\xa5\xd5");
}

/* Friend page: same 2-column cursor, toggles the friend flag for
 * the 7 friend slots */
void PauseMenuPagePartyDebugFriend(void)
{
    int sel;
    int i;
    int j;
    int col;
    int row;
    PIN(int trig, "$5");

    trig = PadData.hTrig;
    switch (trig) {
    case 0x8000:
        sel = PartyDebugFriendSel;
        sel -= 1;
        break;
    case 0x2000:
        sel = PartyDebugFriendSel;
        sel += 1;
        break;
    case 0x1000:
        sel = PartyDebugFriendSel;
        sel -= 2;
        break;
    case 0x4000:
        sel = PartyDebugFriendSel;
        sel += 2;
        break;
    default:
        sel = PartyDebugFriendSel;
        break;
    }
    if (sel < 0) {
        sel += 7;
        PartyDebugFriendSelWrap = sel;
    }
    PartyDebugFriendSel = sel % 7;
    if (PadData.hHeld & 0x20) {
        if (PartyFriendCheck(PartyDebugFriendSel + 1) != 0) {
            PartyFriendOff(PartyDebugFriendSel + 1);
        } else {
            PartyFriendOn(PartyDebugFriendSel + 1);
        }
    }
    xglFontDebugPrintf(8, 32,
        "\xa1\xbc\xa1\xa1\xa5\xd5\xa5\xec\xa5\xf3\xa5\xc9\xc0\xdf\xc4\xea"
        "\xa1\xa1\xa1\xbc");
    {
        int cs = PartyDebugFriendSel;

        xglFontDebugPrintf(cs % 2 * 128 + 8, cs / 2 * 16 + 48, "\xa1\xe4");
    }
    i = 0;
    do {
        j = i + 1;
        col = i % 2 * 128;
        row = i / 2 * 16 + 48;
        i = j;
        LAUNDER(i);
        xglFontDebugPrintf(col + 16, row, MenuCharNameGet(j));
        xglFontDebugPrintf(col + 88, row, FriendStrs[PartyFriendCheck(j)]);
    } while (i < 7);
    xglFontDebugPrintf(8, 192,
        "\xbd\xbd\xbb\xfa\xa5\xad\xa1\xbc\xa1\xa7\xa5\xab\xa1\xbc\xa5\xbd"
        "\xa5\xeb\xb0\xdc\xc6\xb0\xa1\xa1\xa1\xa1\xa1\xa1\xa1\xfb\xa1\xa7"
        "\xa4\xa4\xa4\xeb\xa1\xbf\xa4\xa4\xa4\xca\xa4\xa4");
}


static int PartyDebugAtkSel;

typedef struct ATKVIEW {           /* PartyDataGet() + sel*4 view */
    char pad00[0x26];
    signed char nPos;              /* 0x26 (sel 3..5) */
    char pad27[0x30 - 0x27];
    unsigned short hChr;           /* 0x30 (sel 0..2) */
} ATKVIEW;

typedef struct ATKSLOT {           /* battle-member slot at +0x30 */
    unsigned short hChr;
    signed char nPos;
    char pad03;
} ATKSLOT;

extern char *AtkRowStrs[];         /* "1." "2." "3." */
extern char *AtkColStrs[];         /* chara / position */
extern char *AtkPosStrs[];         /* NULL / P1 / P2 / P3 */

extern void *PartyDataGet(void);

/* Battle-member page: pick one of six rows (3 chars, 3 positions),
 * left/right cycles the value, and both tables are listed */
void PauseMenuPagePartyDebugAttacker(void)
{
    void *p;
    ATKSLOT *sl;
    char **row;
    int i;
    int y;
    int pad;

    if (PadData.hTrig & 0x1000) {
        if (PartyDebugAtkSel != 0) {
            PartyDebugAtkSel--;
        } else {
            PartyDebugAtkSel = 5;
        }
    }
    if (PadData.hTrig & 0x4000) {
        if (PartyDebugAtkSel == 5) {
            PartyDebugAtkSel = 0;
        } else {
            PartyDebugAtkSel++;
        }
    }
    if (PartyDebugAtkSel < 3) {
        int chr;

        chr = ((ATKVIEW *)((char *)PartyDataGet() + PartyDebugAtkSel * 4))->hChr;
        pad = PadData.hTrig;
        if (pad & 0x8000) {
            chr = chr != 0 ? chr - 1 : 29;
        }
        if (pad & 0x2000) {
            chr = chr != 29 ? chr + 1 : 0;
        }
        ((ATKVIEW *)((char *)PartyDataGet() + PartyDebugAtkSel * 4))->hChr = chr;
    } else {
        int pos;

        pos = ((ATKVIEW *)((char *)PartyDataGet() + PartyDebugAtkSel * 4))->nPos;
        pad = PadData.hTrig;
        if (pad & 0x8000) {
            pos = pos != 0 ? pos - 1 : 9;
        }
        if (pad & 0x2000) {
            pos = pos != 9 ? pos + 1 : 0;
        }
        ((ATKVIEW *)((char *)PartyDataGet() + PartyDebugAtkSel * 4))->nPos = pos;
    }
    p = PartyDataGet();
    i = 0;
    xglFontDebugPrintf(8, 32,
        "\xa1\xbc\xa1\xa1\xa5\xd0\xa5\xc8\xa5\xeb\xa5\xe1\xa5\xf3"
        "\xa5\xd0\xa1\xbc\xc0\xdf\xc4\xea\xa1\xa1\xa1\xbc");
    xglFontDebugPrintf(8, PartyDebugAtkSel * 16 + 48, "\xa1\xe4");
    /* PARKED ~60-word diff: the retail object keeps i as an un-reversed
     * counter while y/row/slot walk as strength-reduced givs and the
     * second loop recomputes the slot base from the raw PartyDataGet
     * pointer; every loop form tried here either reverses the counter,
     * CSEs the slot base between the loops, or spills an extra s-reg. */
    row = AtkRowStrs;
    sl = (ATKSLOT *)((char *)p + 0x30);
    do {
        y = i * 16 + 48;
        xglFontDebugPrintf(16, y, *row);
        row++;
        xglFontDebugPrintf(48, y, AtkColStrs[0]);
        i++;
        xglFontDebugPrintf(112, y, MenuCharNameGet(sl->hChr));
        sl++;
    } while (i < 3);
    {
        char **cols = AtkColStrs;

        row = AtkRowStrs;
        sl = (ATKSLOT *)((char *)p + 0x30);
        i = 0;
        do {
            y = i * 16 + 96;
            xglFontDebugPrintf(16, y, *row);
            row++;
            xglFontDebugPrintf(48, y, cols[1]);
            i++;
            xglFontDebugPrintf(112, y, AtkPosStrs[sl->nPos]);
            sl++;
        } while (i < 3);
    }
    xglFontDebugPrintf(8, 192,
        "\xa2\xac\xa2\xad\xa1\xa7\xa5\xab\xa1\xbc\xa5\xbd\xa5\xeb"
        "\xb0\xdc\xc6\xb0\xa1\xa1\xa2\xab\xa2\xaa\xa1\xa7\xa5\xad"
        "\xa5\xe3\xa5\xe9\xa1\xbf\xa5\xdd\xa5\xb8\xa5\xb7\xa5\xe7"
        "\xa5\xf3\xca\xd1\xb9\xb9");
}

/* Page dispatcher: two header lines, page-cycle on the trigger bit,
 * then jump to the selected debug page */
void PauseMenuPagePartyDebug(void)
{
    xglFontDebugPrintf(8, 16,
        "\xa1\xe1\xa1\xe1\xa1\xa1\xa5\xd1\xa1\xbc\xa5\xc6\xa5\xa3\xa1\xbc"
        "\xa5\xc7\xa5\xd0\xa5\xc3\xa5\xaf\xa5\xe1\xa5\xcb\xa5\xe5\xa1\xbc"
        "\xa1\xa1\xa1\xe1\xa1\xe1");
    xglFontDebugPrintf(8, 176,
        "\xa2\xa2\xa1\xa7\xc0\xdf\xc4\xea\xc0\xda\xa4\xea\xc2\xd8\xa4\xa8");
    if (PadData.hTrig & 0x80) {
        if (PartyDebugPage == 4) {
            PartyDebugPage = 0;
        } else {
            PartyDebugPage++;
        }
    }
    switch (PartyDebugPage) {
    case 0:
        PauseMenuPagePartyDebugFriend();
        break;
    case 1:
        PauseMenuPagePartyDebugAttacker();
        break;
    case 2:
        PauseMenuPagePartyDebugOutFriend();
        break;
    case 3:
        PauseMenuPagePartyDebugLockParty();
        break;
    case 4:
        PauseMenuPagePartyDebugTakeAgws();
        __asm__ volatile("");
        break;
    }
}
