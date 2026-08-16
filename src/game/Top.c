/*
 * Top: the top-level (pause) menu screen -- the command window, the party
 * status strip, the info line and the face/EXP panel.  Each is a small
 * state machine wrapped around one WindowDX frame that slides in from off
 * screen and stays until the screen is torn down.
 */

#include "common.h"

/* --- The shared UI window frame (src/game/WindowDX.c) --- */
typedef struct {
    short nX;                  /* 0x000 */
    short nY;                  /* 0x002 */
    int nColor;                /* 0x004 */
    short nW;                  /* 0x008 */
    short nH;                  /* 0x00A */
    char *pText;               /* 0x00C */
    signed char nState;        /* 0x010 */
    signed char unk011;        /* 0x011 */
    char pad012[2];
    void (*pFunc)(void);       /* 0x014 */
    void *pMsg;                /* 0x018 */
    char pad01C[0x194 - 0x1C];
} WINDOWDX;

extern void WindowDXSet(WINDOWDX *pWin);
extern void WindowDXMain(WINDOWDX *pWin);

/* Slide a window coordinate towards its target at the given rate. */
extern void MoveSlide(short *pCur, short *pTarget, float fRate);

/* --- Menu-side helpers --- */
extern int MenuTopCommandCheck(int nCommand);
extern void MenuSelectWindow(void);
extern u8 MenuTopCommand[];

typedef struct {
    char pad000[3];
    u8 nPage;                  /* 0x003 */
    char pad004;
    signed char nRow;          /* 0x005 */
    char pad006[0x100 - 6];
} TOPMENUWORK;

extern TOPMENUWORK MenuWork;

/* One selectable row of the command window. */
typedef struct {
    char *pText;               /* 0x00 */
    int nUnk04;                /* 0x04 */
    u8 bLocked;                /* 0x08 */
    char pad09[3];
} TOPCOMMAND;

/* The command window's message object: the renderer walks pCommand to
 * draw the rows and greys out the disabled ones. */
typedef struct {
    short nX;                  /* 0x00 */
    short nY;                  /* 0x02 */
    int nUnk04;                /* 0x04 */
    char pad08[8];
    TOPCOMMAND *pCommand;      /* 0x10 */
} TOPMENUMSG;

typedef struct {
    int nColor;                /* 0x000 */
    u8 nState;                 /* 0x004 */
    u8 bReady;                 /* 0x005 */
    char pad006[2];
    WINDOWDX win;              /* 0x008 */
    TOPMENUMSG msg;            /* 0x19C */
} TOPMENUWIN;

extern TOPMENUWIN *TopMenuWin;

/* The top command window: nine entries whose enabled state is refreshed
 * from the command-lock mask when the window is built, then a permanent
 * slide between the on-screen and off-screen X depending on which page
 * of the menu is up. */
void TopMenuWinMain(void)
{
    static TOPCOMMAND command[10] = {
        { "Items", 0, 0 },
        { "Ether", 0, 0 },
        { "Tech Attacks", 0, 0 },
        { "Skills", 0, 0 },
        { "Characters", 0, 0 },
        { "A.G.W.S.", 0, 0 },
        { "Battle Formation", 0, 0 },
        { "U.M.N.", 0, 0 },
        { "Game Options", 0, 0 },
        { "", 0, 0 }
    };
    TOPMENUWIN *w;
    short nX;
    int nPage;
    int i;

    w = TopMenuWin;
    switch (w->nState) {
    case 0:
        w->nColor = 0x00F000F0;
        for (i = 0; i < 9; i++) {
            if (MenuTopCommandCheck(MenuTopCommand[i]) == 0) {
                command[i].bLocked = 1;
            } else {
                command[i].bLocked = 0;
            }
        }
        WindowDXSet(&w->win);
        w->win.nX = 544;
        w->win.nY = 28;
        w->win.nColor = w->nColor;
        w->win.nW = 194;
        w->win.nH = 222;
        w->win.pFunc = MenuSelectWindow;
        w->win.pMsg = &w->msg;
        w->win.pText = "Menu";
        w->msg.pCommand = command;
        w->msg.nUnk04 = 0;
        w->msg.nY = 2;
        w->win.nState = 1;
        WindowDXMain(&w->win);
        w->win.nState = 3;
        w->bReady = 1;
        w->nState = 2;
        break;
    case 2:
        break;
    default:
        return;
    }
    nX = 316;
    nPage = MenuWork.nPage;
    if (nPage < 34) {
        if (nPage >= 32) {
            w->msg.nUnk04 = MenuWork.nRow;
        } else {
            nX = 544;
        }
    } else {
        nX = 544;
    }
    MoveSlide(&w->win.nX, &nX, 5.0f);
    WindowDXMain(&w->win);
}

/* The bottom info line's message object. */
typedef struct {
    short nX;                  /* 0x00 */
    short nY;                  /* 0x02 */
    signed char nRow;          /* 0x04 */
    char pad05[3];
    int nMode;                 /* 0x08 */
    char *pText;               /* 0x0C */
} TOPMSGDX;

typedef struct {
    u8 nState;                 /* 0x000 */
    u8 bReady;                 /* 0x001 */
    signed char nSel;          /* 0x002 */
    char pad003;
    WINDOWDX win;              /* 0x004 */
    char pad198[8];
    TOPMSGDX msg;              /* 0x1A0 */
} TOPINFO;

extern TOPINFO *TopInfo;
extern void MenuInfoWindow(void);

/* The help text for each command row. */
extern char *text[];

/* The info line along the bottom of the top menu: one wide frame that
 * slides up while a command page is open and back off screen otherwise,
 * carrying the help text for the row the cursor is on. */
void TopInfoMain(void)
{
    TOPINFO *w;
    short nY;
    u8 nPage;

    w = TopInfo;
    switch (w->nState) {
    case 0:
        WindowDXSet(&w->win);
        w->win.nX = -16;
        w->win.nY = 480;
        w->win.nW = 544;
        w->win.nH = 54;
        w->win.nColor = 0x00F000F0;
        w->win.pFunc = MenuInfoWindow;
        w->win.pMsg = &w->msg;
        w->msg.nX = 0;
        w->msg.nY = 0;
        w->win.nState = 1;
        WindowDXMain(&w->win);
        w->win.nState = 3;
        w->nSel = -1;
        w->bReady = 1;
        w->nState = 2;
        w->msg.nRow = 0;
        break;
    case 2:
        break;
    default:
        return;
    }
    nY = 386;
    nPage = MenuWork.nPage;
    switch (nPage) {
    case 32:
        w->nSel = -1;
    case 33:
        w->msg.nRow = 0;
        w->msg.nY = 0;
        break;
    default:
        nY = 480;
        break;
    }
    if (MenuWork.nRow < 9) {
        w->msg.pText = text[MenuWork.nRow];
    } else {
        w->msg.pText = "";
    }
    MoveSlide(&w->win.nY, &nY, 5.0f);
    WindowDXMain(&w->win);
}
