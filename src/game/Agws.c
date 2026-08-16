/* AGWS menu recovery setup. */

typedef struct {
    short field_00;
    short field_02;
    char pad_04[0x30];
    short field_34;
    short field_36;
} AGWS_RECOVERY;

extern AGWS_RECOVERY *func_A191C0(int id);
extern AGWS_RECOVERY *func_A11108(int id, int *work0, int *work1);

/* func_A11108 also returns an AGWS_RECOVERY *: the two shorts copied into
 * func_A191C0's record come from ITS return value, not from the record
 * itself (the original reads lhu 0/2 off the second call's v0 and writes
 * sh 0x34/0x36 through the first call's, saved in s0). */
void AgwsAllRecovery(void)
{
    int work0[4];
    int work1[4];
    int i;

    for (i = 17; i < 33; i++) {
        AGWS_RECOVERY *recovery = func_A191C0(i);
        AGWS_RECOVERY *source = func_A11108(i, work0, work1);

        recovery->field_34 = source->field_00;
        recovery->field_36 = source->field_02;
    }
}

/* ================= AGWS sub-window screens ================= */

#include "common.h"

/* WindowDX object: the sliding frame every menu screen hangs on
 * (layout as modelled in Menu.c). */
typedef struct WINDOWDX {
    short nX;                       /* 0x000 */
    short nY;                       /* 0x002 */
    int   nColor;                   /* 0x004 */
    short nW;                       /* 0x008 */
    short nH;                       /* 0x00A */
    char *pTitle;                   /* 0x00C */
    signed char nState;             /* 0x010 */
    char  pad_011[3];
    void (*pFunc)(void);            /* 0x014 */
    void *pMsg;                     /* 0x018 */
    char  pad_01C[0x178];
} WINDOWDX;                         /* 0x194 */

typedef struct ESPRITE {
    char  pad_00[4];
    short nX;                       /* 0x04 */
    short nY;                       /* 0x06 */
    int   nColor;                   /* 0x08 */
    char  pad_0C[0x1C];
} ESPRITE;                          /* 0x28 */

typedef struct AGWS_MENUWORK {
    char  pad_00[3];
    unsigned char nScreen;          /* 0x03 */
    char  pad_04[0x2C];
    unsigned char nFlags;           /* 0x30 */
    char  pad_31[0x21];
    signed char nMember;            /* 0x52 */
    char  pad_53[0x2D];
} AGWS_MENUWORK;                    /* 0x80 */

extern AGWS_MENUWORK MenuWork;

extern void WindowDXSet(WINDOWDX *pWin);
extern void WindowDXMain(WINDOWDX *pWin);
extern void MoveSlide(short *pCur, short *pTarget, float fRate);
extern void eSpriteSet(ESPRITE *pSpr, short nId);
extern void eSpriteMain(ESPRITE *pSpr);
extern int MenuSortGet(int nRow, int nCol);
extern int MenuFaceEpidGet(int nType, int nFlag);

/* The portrait panel that slides in beside the AGWS list. */
typedef struct AGWS_FACE {
    unsigned char nState;           /* 0x000 */
    unsigned char nOpen;            /* 0x001 */
    char     pad_002[2];
    int      nColor;                /* 0x004 */
    WINDOWDX win;                   /* 0x008 */
    ESPRITE  spr;                   /* 0x19C */
} AGWS_FACE;

extern AGWS_FACE *AgwsFace;

void AgwsFaceMain(void)
{
    AGWS_FACE *p = AgwsFace;
    short nTarget;

    if (p->nState != 0) {
        if (p->nState != 2) {
            return;
        }
    } else {
        p->nColor = 0x00FFFFF0;
        WindowDXSet(&p->win);
        p->win.nX = -103;
        p->win.nColor = p->nColor;
        p->win.nY = 128;
        p->win.nW = 71;
        p->win.nH = 100;
        p->win.nState = 1;
        WindowDXMain(&p->win);
        p->win.nState = 3;
        eSpriteSet(&p->spr, 0);
        p->nOpen = 0;
        p->nState = 2;
    }
    nTarget = -103;
    if (MenuWork.nScreen == 194) {
        if ((MenuWork.nFlags & 1) != 0) {
            p->nOpen = 0;
        } else {
            p->nOpen = 1;
        }
        nTarget = 208;
    }
    if (p->nOpen != 0) {
        MoveSlide(&p->win.nX, &nTarget, 3.0f);
        if (p->win.nX == -103) {
            p->nOpen = 0;
        }
        WindowDXMain(&p->win);
        if (MenuWork.nMember >= 0) {
            eSpriteSet(&p->spr,
                       (short)MenuFaceEpidGet((short)MenuSortGet(0, MenuWork.nMember), 0));
            p->spr.nX = p->win.nX + 3;
            p->spr.nY = p->win.nY + 3;
            p->spr.nColor = p->win.nColor + 2;
            eSpriteMain(&p->spr);
        }
    }
}

/* The two L1/R1 tab arrows either side of the AGWS window. */
typedef struct AGWS_SWITCH {
    unsigned char nState;           /* 0x00 */
    char     pad_01;
    signed char aSlide[2];          /* 0x02 */
    int      nColor;                /* 0x04 */
    ESPRITE  spr[2];                /* 0x08 */
} AGWS_SWITCH;

extern AGWS_SWITCH *AgwsSwitch;
extern signed char D_004DAFE8[];

typedef struct AGWS_PADDATA {
    char           pad_00[0x2A];
    unsigned short nPress;          /* 0x2A */
    char           pad_2C[0x3C];
} AGWS_PADDATA;

extern AGWS_PADDATA PadData[2];

/* TODO near-miss (11/126 words, all in the two-byte step copy).  The
 * original loads D_004DAFE8[0..1] with SIGN-extending `lb` before
 * storing them to the stack pair; MIPS gcc's LOAD_EXTEND_OP makes a
 * plain QImode move a zero-extending `lbu`, and no spelling swept
 * (int temporaries, explicit casts, pointer copies, a 2-iteration
 * loop, char vs signed char on either side) makes the sign extension
 * survive to the store.  Everything else, including the sp+16 stack
 * placement of the pair, matches. */
void AgwsSwitchMain(void)
{
    AGWS_SWITCH *p = AgwsSwitch;
    short aTarget[2];
    signed char aStep[2];
    int i;

    if (p->nState != 0) {
        if (p->nState != 2) {
            return;
        }
    } else {
        p->nColor = 0x00FFFFFF;
        eSpriteSet(&p->spr[0], 274);
        eSpriteSet(&p->spr[1], 272);
        p->spr[0].nX = -45;
        p->spr[1].nX = 557;
        for (i = 0; i < 2; i++) {
            p->spr[i].nColor = p->nColor;
            p->spr[i].nY = 224;
        }
        p->aSlide[1] = 0;
        p->nState = 2;
        p->aSlide[0] = 0;
    }
    aTarget[0] = -45;
    aTarget[1] = 557;
    if (MenuWork.nScreen == 34) {
        aTarget[0] = 16;
        aTarget[1] = 467;
        if ((PadData[0].nPress & 4) != 0) {
            p->aSlide[0] = -6;
        } else if ((PadData[0].nPress & 8) != 0) {
            p->aSlide[1] = 6;
        }
    }
    aStep[0] = D_004DAFE8[0];
    aStep[1] = D_004DAFE8[1];
    for (i = 0; i < 2; i++) {
        if (p->aSlide[i] != 0) {
            p->aSlide[i] += aStep[i];
        }
    }
    for (i = 0; i < 2; i++) {
        MoveSlide(&p->spr[i].nX, &aTarget[i], 3.0f);
        p->spr[i].nX += p->aSlide[i];
        eSpriteMain(&p->spr[i]);
    }
}
