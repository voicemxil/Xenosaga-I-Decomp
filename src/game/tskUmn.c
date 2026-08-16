#include "matching.h"

/* tskUmn - ov02 (Umn overlay) menu screen tasks: mail / database /
   simulation / plugin windows.

   Shared shapes recovered from the overlay disassembly:
   - every screen handler is void f(TSK_TASK *pTask, WORK *pWork) driven by
     tskUmnObjectTaskMain (state 0 = init, 2 = per-frame, -1 = teardown);
     each first checks UmnWork.nScene against its own scene id and arms
     state -1 when the scene has moved on.
   - the WORK objects embed a 0x194-byte WindowDX object whose draw
     callback (+0x14) is one of the main-ELF window renderers
     (MenuInfoWindow / MenuPasWindow).
   - gcc 2.96's jump-equivalence CSE explains the register reuse quirks:
     after `beq state,2' the state register is known == 2 and is reused as
     the constant 2 (and the loaded scene byte as 3/4) downstream. */

typedef struct TSK_TASK {
    char pad000[0x10];
    int nState;                                   /* 0x10 */
    void (*pFunc)(struct TSK_TASK *, void *);      /* 0x14 */
    void *pParam;                                  /* 0x18 */
} TSK_TASK;

/* Scene ids in UmnWork.nScene (0xFF = global abort, see tsk.c) */
/* 1? = mail, 2 = database, 3 = simulation, 4 = plugin */

typedef struct {
    char pad00[1];
    unsigned char nScene;               /* 0x01 */
    char pad02[1];
    unsigned char nPage;                /* 0x03 */
    char pad04[0x11 - 0x04];
    signed char nUiLock;                /* 0x11: cleared while the
                                         * database menu owns the cursor */
    char pad12[0x46 - 0x12];
    signed char nDataBaseSel;           /* 0x46 */
    signed char nMailMenuSel;           /* 0x47 */
    char pad48[0x50 - 0x48];
    union {
        int nSimulationScript;          /* 0x50 */
        struct {
            char pad[2];
            unsigned char nSel;         /* 0x52 */
        } plugin;
        struct {
            char pad[2];
            signed char nSel;           /* 0x52: analysed monster id */
        } db;
        struct {
            signed char nLo;            /* 0x50 */
            signed char nHi;            /* 0x51 */
        } ex;
        struct {
            char pad[3];
            signed char nSel;           /* 0x53: mail cursor row */
        } mail;
    } u;
    union {
        int nListNum;                   /* 0x54: rows in the current list */
        struct {
            signed char nNum;           /* 0x54: mail rows on screen */
            char pad55[1];
            signed char aId[4];         /* 0x56: their header ids */
            signed char bReplyOpen;     /* 0x5A */
            signed char bReplyReady;    /* 0x5B */
        } mail;
    } u54;
    signed char nMailSel;               /* 0x5C: mail hint-slot cursor */
    signed char nDataBaseHit;           /* 0x5D: keyword-search result */
    signed char nDataBaseOpen;          /* 0x5E: pop-up may be opened */
    signed char nDataBaseBusy;          /* 0x5F: pop-up must stay open */
    char pad60[0x80 - 0x60];
} UMN_WORK;

extern UMN_WORK UmnWork;

/* --- WindowDX (0x194 bytes) --- */
typedef struct {
    short nX;                           /* 0x00 */
    short nY;                           /* 0x02 */
    int nColor;                         /* 0x04 */
    short nW;                           /* 0x08 */
    short nH;                           /* 0x0A */
    char pad0C[4];
    char nState;                        /* 0x10 */
    char pad11[3];
    void (*pFunc)(void);                /* 0x14: draw callback */
    void *pMsg;                         /* 0x18 */
    char pad1C[0x194 - 0x1C];
} WINDOWDX;

/* --- message object hung off a WindowDX (0x44 bytes) --- */
typedef struct {
    short nX;                           /* 0x00 */
    short nY;                           /* 0x02 */
    char pad04[8];
    char *pText;                        /* 0x0C */
    char pad10[0x44 - 0x10];
} MSGDX;

extern void WindowDXSet(WINDOWDX *pWin);
extern void WindowDXMain(WINDOWDX *pWin);
extern void MoveSlide(void *pPos, void *pTarget, float fSpeed);
extern void MenuInfoWindow(void);

/* --- eMessage work hung off a Pas window (header at +0x0C) --- */
typedef struct {
    char pad00[0xC];
    struct {
        char pad00;
        unsigned char nFont;            /* 0x01 */
        char pad02[2];
        short nX;                       /* 0x04 */
        short nY;                       /* 0x06 */
        int nColor;                     /* 0x08 */
    } emsg;                             /* 0x0C: eMessageSet target */
} EMSGWORK;

/* --- endPrint frame box (same header as a WindowDX) --- */
typedef struct {
    short nX;                           /* 0x00 */
    short nY;                           /* 0x02 */
    int nColor;                         /* 0x04 */
    short nW;                           /* 0x08 */
    short nH;                           /* 0x0A */
} PRINTBOX;

extern void eMessageSet(void *pMsg, char *pText);
extern void eMessageMain(void *pMsg);
extern void endPrintExtFunc(int nColor, int nMode, void *pBox);

extern char *UmnPluginTextGet(int nNo);
extern char *ListText[];
extern char *strcpy(char *pDst, const char *pSrc);
extern void *memcpy(void *pDst, const void *pSrc, unsigned int nSize);
extern char *strcat(char *pDst, const char *pSrc);

/* --- info-window work object (bottom help window of each screen) --- */
typedef struct {
    unsigned char nMode;                /* 0x00 */
    unsigned char bReady;               /* 0x01 */
    char pad02[6];
    int nColor;                         /* 0x08 */
    WINDOWDX win;                       /* 0x0C */
    MSGDX msg;                          /* 0x1A0 */
    char pad1E4[0x350 - 0x1E4];
    union {
        char szText[0x10];              /* 0x350 (simulation) */
        long long nAlign;
    } u;
} UMN_INFO;

/* Database screen: bottom info window; the caption follows the database
 * cursor slot */
void tskUmnDataBaseInfo(TSK_TASK *pTask, UMN_INFO *w)
{
    static char *msg00[] = { 0, 0, 0, 0 };
    short nTarget;

    if (UmnWork.nScene != 2) {
        pTask->nState = -1;
        return;
    }
    switch (pTask->nState) {
    case 0:
        w->nColor = 0xFF0000;
        w->bReady = 0;
        w->nMode = 0;
        WindowDXSet(&w->win);
        w->win.nX = -16;
        *(volatile char *)&w->win.nState = 0;
        w->win.nY = 480;
        w->win.nW = 544;
        w->win.nH = 54;
        w->win.pFunc = MenuInfoWindow;
        w->win.pMsg = &w->msg;
        w->msg.nX = 0;
        w->win.nColor = w->nColor;
        w->win.nState = 1;
        w->msg.nY = 0;
        w->msg.pText = 0;
        WindowDXMain(&w->win);
        w->win.nState = 3;
        break;
    case 2: {
        int nPage = UmnWork.nPage;

        nTarget = 386;
        if (nPage < 18) {
            if (nPage < 16) {
                nTarget = 480;
            }
        } else {
            nTarget = 480;
        }
        w->msg.pText = msg00[UmnWork.nDataBaseSel];
        MoveSlide(&w->win.nY, &nTarget, 3.0f);
        WindowDXMain(&w->win);
        break;
    }
    }
}

/* Plugin screen: bottom info window; caption comes from the plugin data
 * of the selected slot */
void tskUmnPluginInfo(TSK_TASK *pTask, UMN_INFO *w)
{
    short nTarget;

    if (UmnWork.nScene != 4) {
        pTask->nState = -1;
        return;
    }
    switch (pTask->nState) {
    case 0:
        w->nColor = 0xFF0000;
        w->bReady = 0;
        w->nMode = 0;
        WindowDXSet(&w->win);
        w->win.nX = -16;
        *(volatile char *)&w->win.nState = 0;
        w->win.nY = 480;
        w->win.nW = 544;
        w->win.nH = 54;
        w->win.pFunc = MenuInfoWindow;
        w->win.pMsg = &w->msg;
        w->msg.nX = 0;
        w->win.nColor = w->nColor;
        w->win.nState = 1;
        w->msg.nY = 0;
        w->msg.pText = 0;
        WindowDXMain(&w->win);
        w->win.nState = 3;
        break;
    case 2: {
        int nPage = UmnWork.nPage;

        nTarget = 480;
        if (nPage < 18) {
            if (nPage >= 16) {
                nTarget = 386;
            }
        }
        if (UmnWork.u.plugin.nSel < 6) {
            w->msg.pText =
                UmnPluginTextGet((signed char)UmnWork.u.plugin.nSel) + 32;
        } else {
            w->msg.pText = "\xa5\xd7\xa5\xe9\xa5\xb0\xa5\xa4\xa5\xf3";
        }
        MoveSlide(&w->win.nY, &nTarget, 3.0f);
        WindowDXMain(&w->win);
        break;
    }
    }
}

/* --- Pas (top title-bar) window work objects --- */
typedef struct {
    unsigned short nX;                  /* 0x00 */
    unsigned short nY;                  /* 0x02 */
    int nColor;                         /* 0x04 */
    unsigned short nW;                  /* 0x08 */
    unsigned short nH;                  /* 0x0A */
    char pad0C[4];
    char nState;                        /* 0x10 */
    char pad11[3];
    void (*pFunc)(void);                /* 0x14 */
    void *pMsg;                         /* 0x18 */
    char pad1C[0x188 - 0x1C];
} PASWIN;

typedef struct {
    char pad00[1];
    unsigned char bReady;               /* 0x01 */
    char pad02[2];
    int nColor;                         /* 0x04 */
    PASWIN win;                         /* 0x08 */
    EMSGWORK msgwork;                   /* 0x190 */
    char pad1A8[0x1E0 - 0x1A8];
    PRINTBOX box;                       /* 0x1E0 */
} UMN_PAS_S;

typedef struct {
    char pad00[1];
    unsigned char bReady;               /* 0x01 */
    char pad02[2];
    int nColor;                         /* 0x04 */
    PASWIN win;                         /* 0x08 */
    EMSGWORK msgwork;                   /* 0x190 */
    char pad1A8[0x3BC - 0x1A8];
    PRINTBOX box;                       /* 0x3BC */
} UMN_PAS_L;

/* Simulation screen: sliding title bar window */
void tskUmnSimulationPas(TSK_TASK *pTask, UMN_PAS_S *w)
{
    static char *msg00[] = { 0, 0 };
    short nTarget[2];

    if (UmnWork.nScene != 3) {
        pTask->nState = -1;
        return;
    }
    switch (pTask->nState) {
    case 0:
        w->nColor = 0xFFFFF0;
        WindowDXSet((WINDOWDX *)&w->win);
        w->win.nX = -272;
        w->win.nY = 8;
        w->win.nW = 312;
        w->win.nH = 32;
        w->win.nColor = w->nColor;
        w->win.nState = 1;
        WindowDXMain((WINDOWDX *)&w->win);
        w->win.nState = 3;
        {
            EMSGWORK *m;
            short nMsgX = 288;
            short nMsgY = 11;

            eMessageSet(&w->msgwork.emsg, msg00[0]);
            m = &w->msgwork;
            m->emsg.nFont = 32;
            m->emsg.nX = nMsgX;
            m->emsg.nY = nMsgY;
            w->msgwork.emsg.nColor = w->nColor + 2;
        }
        w->bReady = 1;
        break;
    case 2: {
        int nPage = UmnWork.nPage;

        short *pTargetY;

        nTarget[0] = -16;
        pTargetY = &nTarget[1];
        *pTargetY = 288;
        if (nPage < 18) {
            if (nPage >= 16) {
                nTarget[1] = 16;
            } else {
                nTarget[0] = -312;
            }
        } else {
            nTarget[0] = -312;
        }
        if (w->bReady != 0) {
            int nBoxW;
            int nBoxH;

            MoveSlide((short *)&w->win.nX, &nTarget[0], 3.0f);
            WindowDXMain((WINDOWDX *)&w->win);
            w->box.nX = w->win.nX + 3;
            w->box.nY = w->win.nY + 3;
            nBoxW = w->win.nW - 6;
            nBoxH = w->win.nH - 6;
            w->box.nW = nBoxW;
            w->box.nH = nBoxH;
            w->box.nColor = w->nColor;
            endPrintExtFunc(w->nColor, 101, &w->box);
            MoveSlide(&w->msgwork.emsg.nX, pTargetY, 3.0f);
            if (w->msgwork.emsg.nX < 256) {
                eMessageMain(&w->msgwork.emsg);
            }
            endPrintExtFunc(w->nColor, 102, 0);
        }
        break;
    }
    }
}

/* Plugin screen: sliding title bar window */
void tskUmnPluginPas(TSK_TASK *pTask, UMN_PAS_L *w)
{
    static char *msg00[] = { 0, 0 };
    short nTarget[2];

    if (UmnWork.nScene != 4) {
        pTask->nState = -1;
        return;
    }
    switch (pTask->nState) {
    case 0:
        w->nColor = 0xFFFFF0;
        WindowDXSet((WINDOWDX *)&w->win);
        w->win.nX = -272;
        w->win.nY = 8;
        w->win.nW = 272;
        w->win.nH = 32;
        w->win.nColor = w->nColor;
        w->win.nState = 1;
        WindowDXMain((WINDOWDX *)&w->win);
        w->win.nState = 3;
        {
            EMSGWORK *m;
            short nMsgX = 288;
            short nMsgY = 11;

            eMessageSet(&w->msgwork.emsg, msg00[0]);
            m = &w->msgwork;
            m->emsg.nFont = 32;
            m->emsg.nX = nMsgX;
            m->emsg.nY = nMsgY;
            w->msgwork.emsg.nColor = w->nColor + 2;
        }
        w->bReady = 1;
        break;
    case 2: {
        int nPage = UmnWork.nPage;

        short *pTargetY;

        nTarget[0] = -16;
        pTargetY = &nTarget[1];
        *pTargetY = 288;
        if (nPage < 18) {
            if (nPage >= 16) {
                nTarget[1] = 16;
            } else {
                nTarget[0] = -272;
            }
        } else {
            nTarget[0] = -272;
        }
        if (w->bReady != 0) {
            int nBoxW;
            int nBoxH;

            MoveSlide((short *)&w->win.nX, &nTarget[0], 3.0f);
            WindowDXMain((WINDOWDX *)&w->win);
            w->box.nX = w->win.nX + 3;
            w->box.nY = w->win.nY + 3;
            nBoxW = w->win.nW - 6;
            nBoxH = w->win.nH - 6;
            w->box.nW = nBoxW;
            w->box.nH = nBoxH;
            w->box.nColor = w->nColor;
            endPrintExtFunc(w->nColor, 101, &w->box);
            MoveSlide(&w->msgwork.emsg.nX, pTargetY, 3.0f);
            if (w->msgwork.emsg.nX < 256) {
                eMessageMain(&w->msgwork.emsg);
            }
            endPrintExtFunc(w->nColor, 102, 0);
        }
        break;
    }
    }
}

/* Database screen: sliding title bar window; the message target snaps on
 * the folder-index pages */
void tskUmnDataBasePas(TSK_TASK *pTask, UMN_PAS_L *w)
{
    static char *msg00[] = { 0, 0 };
    short nTarget[4];
    short nTargetX;
    int i;

    if (UmnWork.nScene != 2) {
        pTask->nState = -1;
        return;
    }
    switch (pTask->nState) {
    case 0:
        w->nColor = 0xFFFFF0;
        WindowDXSet((WINDOWDX *)&w->win);
        w->win.nX = -272;
        w->win.nY = 8;
        w->win.nColor = w->nColor;
        w->win.nW = 272;
        w->win.nH = 32;
        w->win.nState = 1;
        WindowDXMain((WINDOWDX *)&w->win);
        w->win.nState = 3;
        {
            EMSGWORK *m;
            PIN(unsigned char nOne, "$4");
            short nMsgX = 288;
            short nMsgY = 11;

            eMessageSet(&w->msgwork.emsg, msg00[0]);
            m = &w->msgwork;
            m->emsg.nFont = 32;
            nOne = 1;
            m->emsg.nX = nMsgX;
            m->emsg.nY = nMsgY;
            w->msgwork.emsg.nColor = w->nColor + 2;
            LAUNDER(nOne);
            w->bReady = nOne;
        }
        break;
    case 2:
        nTargetX = -16;
        {
            PIN(short nFill, "$3");

            nFill = 288;
            for (i = 3; i >= 0; i--) {
                nTarget[i] = nFill;
            }
        }
        switch (UmnWork.nPage) {
        case 16:
        case 17:
        case 32:
        case 33:
        case 48:
        case 49:
        case 80:
        case 81:
            nTarget[0] = 16;
            break;
        default:
            nTargetX = -272;
            break;
        }
        if (w->bReady != 0) {
            int nBoxW;
            int nBoxH;

            MoveSlide((short *)&w->win.nX, &nTargetX, 3.0f);
            WindowDXMain((WINDOWDX *)&w->win);
            w->box.nX = w->win.nX + 3;
            w->box.nY = w->win.nY + 3;
            nBoxW = w->win.nW - 6;
            nBoxH = w->win.nH - 6;
            w->box.nW = nBoxW;
            w->box.nH = nBoxH;
            w->box.nColor = w->nColor;
            endPrintExtFunc(w->nColor, 101, &w->box);
            MoveSlide(&w->msgwork.emsg.nX, nTarget, 3.0f);
            if (w->msgwork.emsg.nX < 256) {
                eMessageMain(&w->msgwork.emsg);
            }
            endPrintExtFunc(w->nColor, 102, 0);
        }
        break;
    }
}

/* TODO: near-miss (SCHEDULING, 3) -- the slide-tail rotation: original
   emits addiu a0,s1,14 BEFORE the 3.0f lui/mtc1 pair, ours after. Every
   creation-order lever tried (float local, pinned $f12/$4 pair, laundered
   pY pointer) leaves the pair glued first. One 3-insn rotation; a
   site-indexed swap pass or the permuter would finish it. Not registered. */
/* Simulation screen: bottom info window; on the list pages the caption is
 * built from the selected script name */
void tskUmnSimulationInfo(TSK_TASK *pTask, UMN_INFO *w)
{
    short nTarget;

    if (UmnWork.nScene != 3) {
        pTask->nState = -1;
        return;
    }
    switch (pTask->nState) {
    case 0:
        w->nColor = 0xFF0000;
        strcpy(w->u.szText, "");
        w->bReady = 0;
        w->nMode = 0;
        WindowDXSet(&w->win);
        w->win.nX = -16;
        *(volatile char *)&w->win.nState = 0;
        w->win.nY = 480;
        w->win.nW = 544;
        w->win.nH = 54;
        w->win.nColor = w->nColor;
        w->win.pFunc = MenuInfoWindow;
        w->win.pMsg = &w->msg;
        w->msg.nX = 0;
        w->win.nState = 1;
        w->msg.nY = 0;
        w->msg.pText = 0;
        WindowDXMain(&w->win);
        w->win.nState = 3;
        break;
    case 2: {
        int nPage;
        char *pText;

        nTarget = 386;
        if (w->nMode != 0) {
            if (w->nMode != 2) {
                goto textonly;
            }
        } else {
            w->nMode = 2;
            w->bReady = 1;
        }
        nPage = UmnWork.nPage;
        if (nPage < 18) {
            if (nPage < 16) {
                goto park16;
            }
            if (UmnWork.u.nSimulationScript >= 0) {
                {
                    char *pLit = "\xa5\xb7\xa5\xca\xa5\xea\xa5\xaa\x20";
                    PIN(long long nHead, "$5");
                    PIN(unsigned short nTail, "$3");

                    nTail = *(unsigned short *)(pLit + 8);
                    nHead = *(long long *)pLit;
                    *(long long *)w->u.szText = nHead;
                    *(short *)(w->u.szText + 8) = nTail;
                }
                pText = w->u.szText;
                strcat(pText, ListText[UmnWork.u.nSimulationScript]);
                strcat(pText, "\xa4\xce\xb7\xeb\xb2\xcc");
                w->msg.pText = pText;
                goto slide;
            }
            goto textonly;
        }
park16:
        nTarget = 512;
        pText = w->u.szText;
        w->msg.pText = pText;
        goto slide;
        if (0) {
textonly:
            pText = w->u.szText;
            w->msg.pText = pText;
        }
slide:
        MoveSlide(&w->win.nY, &nTarget, 3.0f);
        WindowDXMain(&w->win);
        break;
    }
    }
}

/* A help-bar hint slot as the multi-hint bars address it: the eMessage sits
   twelve bytes into a 68-byte entry, which is the stride the setup loop
   walks. */
typedef struct {
    char pad00[0xC];
    struct {
        char pad00;
        unsigned char nFont;            /* 0x01 */
        char pad02[2];
        short nX;                       /* 0x04 */
        short nY;                       /* 0x06 */
        int nColor;                     /* 0x08 */
    } emsg;                             /* 0x0C */
    char pad18[0x44 - 0x18];
} PASMSG;

typedef struct {
    char pad00[1];
    unsigned char bReady;               /* 0x001 */
    char pad02[2];
    int nColor;                         /* 0x004 */
    PASWIN win;                         /* 0x008 */
    PASMSG msg[5];                      /* 0x190 */
    char pad2E4[0x3BC - 0x2E4];
    PRINTBOX box;                       /* 0x3BC */
} UMN_PAS_M;

extern int MenuPasLengthGet(char *pMsg);

/* Mail screen help bar: five hints, the widest of the Umn bars. The first
   page family puts its second hint behind a three-slot-per-entry cursor. */
void tskUmnMailPas(TSK_TASK *pTask, UMN_PAS_M *w)
{
    static char *msg00[] = { 0, 0, 0, 0, 0 };
    int aLen[5];
    short aTarget[5];
    short nSlide;
    int i;

    for (i = 0; i < 5; i++) {
        aLen[i] = MenuPasLengthGet(msg00[i]);
    }
    if (UmnWork.nScene != 1) {
        pTask->nState = -1;
        return;
    }
    switch (pTask->nState) {
    case 0:
        w->nColor = 0xFFFFF0;
        WindowDXSet((WINDOWDX *)&w->win);
        w->win.nX = -272;
        w->win.nY = 8;
        w->win.nColor = w->nColor;
        w->win.nW = 272;
        w->win.nH = 32;
        w->win.nState = 1;
        WindowDXMain((WINDOWDX *)&w->win);
        w->win.nState = 3;
        for (i = 0; i < 5; i++) {
            eMessageSet(&w->msg[i].emsg, msg00[i]);
            w->msg[i].emsg.nFont = 32;
            w->msg[i].emsg.nX = 288;
            w->msg[i].emsg.nY = 11;
            w->msg[i].emsg.nColor = w->nColor + 2;
        }
        w->bReady = 1;
        break;
    case 2:
        nSlide = -16;
        for (i = 0; i < 5; i++) {
            aTarget[i] = 288;
        }
        switch (UmnWork.nPage) {
        case 16:
        case 17:
        case 32:
        case 33:
        case 48:
        case 49:
        case 53:
        case 96:
        case 97:
            aTarget[0] = 16;
            aTarget[UmnWork.nMailSel * 3 + 1] = aLen[0] + 16;
            break;
        case 64:
        case 65:
            aTarget[0] = 16;
            aTarget[2] = aLen[0] + 16;
            break;
        case 80:
        case 81:
        case 82:
        case 83:
        case 84:
        case 85:
            aTarget[0] = 16;
            aTarget[3] = aLen[0] + 16;
            break;
        default:
            nSlide = -272;
            break;
        }
        if (w->bReady != 0) {
            int nBoxW;
            int nBoxH;

            MoveSlide((short *)&w->win.nX, &nSlide, 3.0f);
            WindowDXMain((WINDOWDX *)&w->win);
            w->box.nX = w->win.nX + 3;
            w->box.nY = w->win.nY + 3;
            nBoxW = w->win.nW - 6;
            nBoxH = w->win.nH - 6;
            w->box.nW = nBoxW;
            w->box.nH = nBoxH;
            w->box.nColor = w->nColor;
            endPrintExtFunc(w->nColor, 101, &w->box);
            for (i = 0; i < 5; i++) {
                MoveSlide(&w->msg[i].emsg.nX, &aTarget[i], 3.0f);
                if (w->msg[i].emsg.nX < 256) {
                    eMessageMain(&w->msg[i].emsg);
                }
            }
            endPrintExtFunc(w->nColor, 102, 0);
        }
        break;
    }
}

/* --- Gnosis-database "analysis" pop-up (eBattleWin3) --- */

/* The pad's trigger doubleword. Bit 21 (0x20 in the halfword at +0x2A)
 * is the confirm button; the case-12 test masks three buttons at once
 * and reads the whole doubleword to do it. */
typedef struct {
    char pad00[0x28];
    union {
        long long q;                    /* 0x28 */
        struct {
            char pad[2];
            unsigned short h;           /* 0x2A */
        } b;
    } trig;
    char pad30[0x34 - 0x30];            /* 0x30 */
    unsigned short nRepeat;             /* 0x34: auto-repeat trigger */
} PADDATA;

extern PADDATA PadData;
/* Declared as an unsized array so the overlay addresses it with a
 * %hi/%lo pair: a plain `extern int` is small enough for -G8 to route
 * through $gp, which the main ELF's data is not reachable from here. */
extern int BW3BattleOrDataBase[];

/* One Gnosis-database record. */
typedef struct {
    char pad00[0x20];
    unsigned short h20;                 /* 0x20 */
    unsigned short h22;                 /* 0x22 */
    unsigned short h24;                 /* 0x24 */
    char pad26[2];
    int n28;                            /* 0x28 */
    int n2C;                            /* 0x2C */
    int n30;                            /* 0x30 */
    int n34;                            /* 0x34 */
    long long d38;                      /* 0x38 */
    char szStat[0x52 - 0x40];           /* 0x40 */
    char szName[0x68 - 0x52];           /* 0x52 */
} GUNO_DATA;

/* The parameter block eBattleWinOpen3 copies its window contents from. */
typedef struct {
    int nX;                             /* 0x00 */
    int nY;                             /* 0x04 */
    int nColor;                         /* 0x08 */
    GUNO_DATA *pData;                   /* 0x0C */
    unsigned short h10;                 /* 0x10 */
    unsigned short h12;                 /* 0x12 */
    unsigned short h14;                 /* 0x14 */
    char pad16[2];
    int n18;                            /* 0x18 */
    int n1C;                            /* 0x1C */
    int n20;                            /* 0x20 */
    int n24;                            /* 0x24 */
    long long d28;                      /* 0x28 */
    char pad30[4];
    char *pStat;                        /* 0x34 */
    char *pName;                        /* 0x38 */
} BW3PARAM;

/* This screen keeps its own two-byte state machine inline in the task
 * node, right after the standard header. */
typedef struct {
    unsigned char nStep;                /* 0x00 */
    unsigned char bOpen;                /* 0x01 */
} ANALISIS_WORK;

typedef struct {
    char pad000[0x10];
    int nState;                         /* 0x10 */
    void (*pFunc)(void);                /* 0x14 */
    void *pParam;                       /* 0x18 */
    ANALISIS_WORK work;                 /* 0x1C */
} TSK_ANALISIS;

extern GUNO_DATA *UmnGunoDataBaseGet(int nNo);
extern void eBattleWinOpen3(BW3PARAM *pParam);
extern void eBattleWinMain3(void);
extern void eBattleWinClose3(void);
extern void xglSoundEffectNormalID(int nCode, int nRand);

/* Database screen: the analysis pop-up over the Gnosis list. Confirm on
 * page 49 opens the eBattleWin3 window filled from the selected record,
 * and any of the three cancel/page buttons (or the list moving on)
 * closes it again. */
void tskUmnDataBaseAnalisis(TSK_ANALISIS *pTask, void *pParam)
{
    BW3PARAM a;
    GUNO_DATA *p;
    ANALISIS_WORK *w;

    w = &pTask->work;
    if (UmnWork.nScene != 2) {
        pTask->nState = -1;
        return;
    }
    switch (w->nStep) {
    case 0:
        w->bOpen = 0;
        w->nStep = 1;
        /* fall through */
    case 1:
        if (PadData.trig.b.h & 0x20) {
            if (UmnWork.nPage == 49) {
                if (UmnWork.nDataBaseOpen != 0) {
                    w->nStep = 10;
                    xglSoundEffectNormalID(1, 0);
                } else {
                    xglSoundEffectNormalID(5, 0);
                }
            }
        }
        break;
    case 10:
        p = UmnGunoDataBaseGet(UmnWork.u.db.nSel);
        a.nX = 112;
        a.nY = 144;
        a.nColor = 0x00FFFFF0;
        a.pData = p;
        a.h10 = p->h20;
        a.h12 = p->h22;
        a.h14 = p->h24;
        a.n18 = p->n28;
        a.n1C = p->n2C;
        a.n20 = p->n30;
        a.n24 = p->n34;
        a.d28 = p->d38;
        a.pStat = p->szStat;
        a.pName = p->szName;
        BW3BattleOrDataBase[0] = 1;
        eBattleWinOpen3(&a);
        w->bOpen = 1;
        w->nStep = 12;
        /* fall through */
    case 12:
        if (UmnWork.nPage != 49 || UmnWork.nDataBaseBusy != 0 ||
            (PadData.trig.q & 0x2C0000)) {
            w->nStep = 20;
            if (PadData.trig.b.h & 0x20) {
                xglSoundEffectNormalID(2, 0);
            }
        }
        break;
    case 20:
        eBattleWinClose3();
        w->nStep = 1;
        break;
    }
    if (w->bOpen != 0) {
        eBattleWinMain3();
    }
}

/* --- plugin screen: the "Num" counter window over the plugin list --- */

typedef struct {
    char pad00[4];
    unsigned short nX;                  /* 0x04 */
    unsigned short nY;                  /* 0x06 */
    int nColor;                         /* 0x08 */
    char pad0C[0x20 - 0x0C];
} EXTAGFONT;

typedef struct {
    unsigned short nX;                  /* 0x00 */
    unsigned short nY;                  /* 0x02 */
    int nColor;                         /* 0x04 */
    short nW;                           /* 0x08 */
    short nH;                           /* 0x0A */
    char pad0C[0x70 - 0x0C];
} EXRIBBON;

typedef struct {
    short nX;                           /* 0x00 */
    short nY;                           /* 0x02 */
    int nColor;                         /* 0x04 */
    char pad08[6];
    unsigned char nDigits;              /* 0x0E */
    unsigned char nAlign;               /* 0x0F */
    char pad10[4];
    int nValue;                         /* 0x14 */
    char pad18[0x84 - 0x18];
} EXNUMBER;

typedef struct {
    unsigned char nMode;                /* 0x000 */
    unsigned char bReady;               /* 0x001 */
    char pad002[2];
    short nW;                           /* 0x004 */
    short nH;                           /* 0x006 */
    int nColor;                         /* 0x008 */
    unsigned short nX;                  /* 0x00C */
    unsigned short nY;                  /* 0x00E */
    EXTAGFONT tag;                      /* 0x010 */
    EXRIBBON rib;                       /* 0x030 */
    EXNUMBER num;                       /* 0x0A0 */
} UMN_EXWIN;

/* Unprototyped: the keyword screen passes two record pointers where the
   plugin screen passes coordinates. */
extern void eRibbonSet();
extern void eRibbonMain(EXRIBBON *pRib, int nColor);
extern void eTagFontSet(EXTAGFONT *pTag, char *pText);
extern void eTagFontMain(EXTAGFONT *pTag, int nColor);
extern void eNumberSet(EXNUMBER *pNum, int nValue);
/* Unprototyped on purpose: the real eNumberMain takes four arguments
   (object, y, colour, digits) and the plugin screen only ever passes
   three, exactly as retail's object does. */
extern void eNumberMain();

/* TODO: PARKED at 107/134 words (136 orig), all inside the per-frame
 * loop. Behaviour, constants, struct offsets and the loop SHAPE are
 * recovered: `for (i = 0; i < 1; i++)` reproduces the reversed counter
 * (s6 from 0, `bgez` at the latch) alongside a separate byte-offset biv,
 * and computing the element pointer as `(T *)((char *)w + n)` rather
 * than `w[i]` is what turns the latch into `addu s0,s4,s5` -- that edit
 * alone moved the built length from 520 to 536 bytes.
 *
 * What is left is giv GROUPING. The original keeps FOUR address bases
 * live across the loop (s0 = w+n, s1 = w+24+n, s2 = w+144+n,
 * s3 = w+52+n) and, because of the resulting pressure, does NOT hoist
 * the loop-invariant constants 3 and 2 into callee-saved registers. Our
 * build's combine_givs merges all four into one base plus offsets and
 * then has registers spare for the two constants, so it saves s0..s7
 * where the original saves s0..s8. The groups do not line up with the
 * sub-objects (s2 covers every short in all three, s3 covers three ints
 * spread across two), so they are not reachable by introducing
 * sub-object pointers -- swept: `w[i]` indexing, a walking `p++`,
 * per-sub-object pointers, and hoisting the UmnWork reads.
 *
 * Slides the plugin-count ribbon in while the plugin list is on pages
 * 16/17 and parks it off the left edge otherwise; the counter itself is
 * the packed lo/hi pair UmnWork keeps for the screen, plus one. */
void tskUmnPluginExWin(TSK_TASK *pTask, UMN_EXWIN *w)
{
    /* The retail overlay reserves 24 bytes here; a <=8-byte static
     * would be routed through $gp by -G8, which an overlay cannot use
     * to reach its own data. */
    static char msg00[24] = "\001Num";
    short nTarget;

    if (UmnWork.nScene != 4) {
        pTask->nState = -1;
        return;
    }
    switch (pTask->nState) {
    case 0:
        w->nW = 528;
        w->nH = 32;
        w->nColor = 0x00FFFFF0;
        w->nX = -272;
        w->nY = 48;
        eRibbonSet(&w->rib, 3, -272, 48);
        w->rib.nW = 256;
        w->rib.nH = 24;
        eTagFontSet(&w->tag, msg00);
        eNumberSet(&w->num, 0);
        w->nMode = 0;
        w->bReady = 0;
        break;
    case 2: {
        int i;
        int n;
        int nPage = UmnWork.nPage;

        nTarget = -272;
        if (nPage < 18) {
            if (nPage >= 16) {
                nTarget = 0;
            }
        }
        n = 0;
        for (i = 0; i < 1; i++) {
            UMN_EXWIN *p = (UMN_EXWIN *)((char *)w + n);
            int nColor;
            int nColor2;
            unsigned short nY;

            MoveSlide(&p->nX, &nTarget, 3.0f);
            p->rib.nX = p->nX;
            p->rib.nY = p->nY;
            nColor = w->nColor;
            p->rib.nColor = nColor;
            eRibbonMain(&p->rib, nColor);
            nColor2 = p->rib.nColor + 1;
            p->tag.nX = p->nX + 188;
            p->tag.nColor = nColor2;
            p->tag.nY = p->nY + 4;
            eTagFontMain(&p->tag, nColor2);
            nY = p->tag.nY;
            p->num.nX = p->tag.nX + 30;
            p->num.nColor = p->tag.nColor;
            p->num.nDigits = 3;
            p->num.nY = nY;
            p->num.nValue = UmnWork.u.ex.nLo + (UmnWork.u.ex.nHi << 16) + 1;
            p->num.nAlign = 2;
            eNumberMain(&p->num, nY, 2);
            n += sizeof(UMN_EXWIN);
        }
        break;
    }
    }
}

/* --- Simulation screen: the scrolling script list (WindowSP) --- */

/* One selectable row as WindowSP walks them: twelve bytes, terminated by
   a null text pointer. */
typedef struct {
    char *pText;                        /* 0x00 */
    int nParam;                         /* 0x04 */
    char bGray;                         /* 0x08 */
    char pad09[3];                      /* 0x09 */
} SPROW;

/* The WindowSP object itself (same header the equip menu builds in
   Char.c: 0x0C/0x0E size, 0x10 title, 0x14/0x15 style, 0x1C rows). */
typedef struct {
    unsigned char nState;               /* 0x00 */
    unsigned char nRows;                /* 0x01 */
    char pad02[2];                      /* 0x02 */
    short nX;                           /* 0x04 */
    short nY;                           /* 0x06 */
    int nColor;                         /* 0x08 */
    short nW;                           /* 0x0C */
    short nH;                           /* 0x0E */
    char *pTitle;                       /* 0x10 */
    char pad14_0[0];
    unsigned char nStyle;               /* 0x14 */
    unsigned char nFont;                /* 0x15 */
    char pad16[0x1C - 0x16];            /* 0x16 */
    SPROW *pRows;                       /* 0x1C */
} SPWIN;

typedef struct {
    unsigned char nState;               /* 0x00 */
    unsigned char bReady;               /* 0x01 */
    char pad02[2];                      /* 0x02 */
    short nX;                           /* 0x04 */
    short nY;                           /* 0x06 */
    int nColor;                         /* 0x08 */
    SPWIN sp;                           /* 0x0C */
    char pad2C[0x1744 - 0x2C];          /* 0x2C */
    SPROW row[1];                       /* 0x1744 */
} UMN_LIST;

typedef struct {
    char pad00[0x10];                   /* 0x00 */
    int nFlags;                         /* 0x10 */
} UMN_GLS;

extern UMN_GLS GameLoopState;
extern char *ListText[];
extern void WindowSPSet(SPWIN *pWin);
extern void WindowSPMain(SPWIN *pWin);
extern int WindowSPSelect(SPWIN *pWin, int nRepeat);

/* TODO: near-miss (183 of 184 words; the one real divergence is a
 * register-allocation tie-break).  gcc parks the literal 20 of the
 * `w->nState != 20' test in a CALLEE-SAVED register ($s2) across the
 * MoveSlide call and reuses it for the second `w->nState == 20' test
 * below; the original rematerialises the constant with a second `li'
 * and never touches $s2 at all.  That one extra saved register costs
 * `sd $s2'/`ld $s2', saves one `li', and moves every jump-table target
 * by a word, which is where the missing `.p2align' nops go.  Its knock-on
 * is the only other diff: $v0/$v1 swap on the 528/0xFFFF00 constants in
 * the init block, which reschedules the three header stores.
 *
 * Swept without moving it: all six source orders of the nX/nY/nColor
 * stores (the scheduler normalises them); the second test written as
 * `!= 20' with the arms swapped, as `nTarget == 32', as `w->nState - 20
 * == 0', and as an early-exit `if (w->nX != nTarget) break;'; nTarget
 * block-scoped, function-scoped and as a one-element array; the first
 * test as an if/else that assigns nTarget on both arms (182 words);
 * a local copy of the state byte for the second test; and LAUNDER on
 * both the state byte and on a `n20' temporary -- laundering the SECOND
 * use gets the length to 185 with 25 diffs but only turns the reuse into
 * `move $v0,$s2' instead of `li $v0,20', and laundering the FIRST drops
 * to 182.  A permuter run on the case-20/40 body is the next thing to
 * try.
 *
 * Simulation screen: the script list the player scrolls.
 *
 * pTask state 0 fills the row table from ListText[] -- greying every row
 * out unless the "environmental simulator unlocked" bit is set in
 * GameLoopState -- appends a blank row and the null terminator, then
 * hands the table to WindowSPSet.
 *
 * The window's own nState then runs 0 -> 10 (wait for page 17) -> 20
 * (slide in) -> 30 (accept input) -> 40 (slide back out), with 20 and 40
 * sharing one body that picks its slide target from the state. */
void tskUmnSimulationList(TSK_TASK *pTask, UMN_LIST *w)
{
    short nTarget;
    int i;

    if (UmnWork.nScene != 3) {
        pTask->nState = -1;
        return;
    }
    switch (pTask->nState) {
    case 0:
        w->nX = 528;
        w->nY = 112;
        w->nColor = 0xFFFF00;
        for (i = 0; i < UmnWork.u54.nListNum; i++) {
            w->row[i].pText = ListText[i];
            if (GameLoopState.nFlags & 0x400000) {
                w->row[i].bGray = 0;
            } else {
                w->row[i].bGray = 1;
            }
            w->row[i].nParam = 0;
        }
        w->row[i].pText = "";
        w->row[i].nParam = 0;
        w->row[i].bGray = 0;
        w->row[i + 1].pText = 0;
        w->sp.nX = w->nX;
        w->sp.nY = w->nY;
        w->sp.nColor = w->nColor;
        w->sp.nRows = 3;
        w->sp.pTitle = "Information";
        w->sp.nStyle = 1;
        w->sp.nFont = 7;
        w->sp.nW = 422;
        w->sp.nH = 174;
        w->sp.pRows = w->row;
        WindowSPSet(&w->sp);
        w->sp.nState = 17;
        w->bReady = 0;
        w->nState = 0;
        break;
    case 2:
        switch (w->nState) {
        case 0:
            w->bReady = 0;
            w->nState = 10;
            /* fallthrough */
        case 10:
            if (UmnWork.nPage == 17) {
                w->bReady = 1;
                w->nState = 20;
            }
            break;
        case 20:
        case 40:
            nTarget = 32;
            if (w->nState != 20) {
                nTarget = 528;
            }
            MoveSlide(&w->nX, &nTarget, 3.0f);
            if (w->nX == nTarget) {
                if (w->nState == 20) {
                    w->nState = 30;
                } else {
                    w->nState = 0;
                }
            }
            break;
        case 30:
            UmnWork.u.nSimulationScript =
                WindowSPSelect(&w->sp, PadData.nRepeat);
            if (UmnWork.nPage != 17) {
                w->nState = 40;
            }
            break;
        }
        break;
    }
    if (w->bReady) {
        w->sp.nX = w->nX;
        w->sp.nY = w->nY;
        WindowSPMain(&w->sp);
    }
}

/* --- Database screen: the top-level "Menu" select window --- */

/* MenuSelectWindow's own header: same first bytes as a WindowDX, but the
   +0x0C slot is the caption drawn in the title bar rather than padding. */
typedef struct {
    short nX;                           /* 0x00 */
    short nY;                           /* 0x02 */
    int nColor;                         /* 0x04 */
    short nW;                           /* 0x08 */
    short nH;                           /* 0x0A */
    char *pTitle;                       /* 0x0C */
    char nState;                        /* 0x10 */
    char pad11[3];
    void (*pFunc)(void);                /* 0x14 */
    void *pMsg;                         /* 0x18 */
    char pad1C[0x194 - 0x1C];
} SELWIN;

/* The parameter block MenuSelectWindow reads through pMsg. */
typedef struct {
    char pad00[2];
    short nColumns;                     /* 0x02 */
    int nSel;                           /* 0x04 */
    char *pChoices;                     /* 0x08 */
    char *pPrompt;                      /* 0x0C */
    SPROW *pRows;                       /* 0x10 */
} SELPARAM;

typedef struct {
    unsigned char nState;               /* 0x000 */
    unsigned char bReady;               /* 0x001 */
    char pad002[2];
    short nW;                           /* 0x004 */
    short nH;                           /* 0x006 */
    int nColor;                         /* 0x008 */
    int bVisible;                       /* 0x00C */
    SELWIN win;                         /* 0x010 */
    char pad1A4[0x1A4 - 0x1A4];
    SELPARAM sel;                       /* 0x1A4 */
    char pad1B8[0x79C - 0x1B8];
    SPROW row[4];                       /* 0x79C */
} UMN_MENU;

extern void MenuSelectWindow(void);

/* TODO: PARKED at 25 diffs of 190 words -- right length, right opcodes
 * bar two, right control flow.  What is left is ONE register rotation:
 * the row loop's three address givs are a 3-cycle away from the original
 * (orig $a0 = the stride-1 giv at +1956, $a2 = the stride-12 giv at
 * +1956, $a1 = the stride-12 giv at +1948; we get $a1/$a0/$a2), and the
 * same one-slot shift swaps $t0/$t2 between the UmnWork base and the
 * `w+12' base -- which is also why our `w->bVisible = 1' stores as
 * `sw $s0,12($s1)' where the original reuses the loop base, `sw $s0,0($t0)'.
 *
 * Confirmed correct from the jump table at 0x00A13030: entry[0] is three
 * instructions before entry[10] (bReady, nState, fall through), and
 * state 20 falls through into state 21 (its `b' lands one instruction
 * past entry[21], skipping the `addiu $s2,$s1,16' that the table edge
 * needs).
 *
 * Swept without closing it: all 24 orders of the four constant-store
 * groups in state 20's tail plus 11 of the 120 five-group orders (the
 * order kept is the best found); both polarities of the nHi test and of
 * the `i == 0' test; every 2-of-3 split of pText/bGray between an
 * indexed reference and a block-local pointer (all through one SPROW
 * pointer collapses to 185 words); bVisible stored by name and through
 * an `int *' aimed at &w->bVisible; a callee-saved `SELWIN *' shared by
 * states 20 and 21 (buys $s2 but costs the win.nState store its base);
 * and `int i' declared before and after the other locals.  The permuter
 * cannot be used here -- permute_setup.py cannot assemble overlay
 * functions.
 *
 * Database screen: the three-row "Menu" window (Gnosis / Keywords /
 * Cancel) that slides in from the right once the screen reaches page 17.
 * The Gnosis row is greyed out until the player has actually met one.
 *
 * The window's own nState runs 0 -> 10 (wait for the page) -> 20 (build
 * the row table and prime the window) -> 21 (slide in) -> 30 (accept
 * input) -> 40 (slide out). */
void tskUmnDataBaseMenu(TSK_TASK *pTask, UMN_MENU *w)
{
    static char *menu00[] = { "Gnosis", "Keywords", "Cancel", 0 };
    short nTarget;
    short nTargetOut;
    int i;

    if (UmnWork.nScene != 2) {
        pTask->nState = -1;
        return;
    }
    switch (pTask->nState) {
    case 0:
        w->nW = 528;
        w->nH = 48;
        w->nColor = 0x00FFFFF0;
        w->bVisible = 0;
        WindowDXSet((WINDOWDX *)&w->win);
        w->win.nColor = w->nColor;
        w->win.nW = 125;
        w->win.nH = 78;
        w->sel.nColumns = 2;
        w->win.pTitle = "Menu";
        w->win.pFunc = MenuSelectWindow;
        w->win.pMsg = &w->sel;
        w->sel.pRows = 0;
        w->nState = 0;
        w->bReady = 0;
        break;
    case 2:
        switch (w->nState) {
        case 0:
            w->bReady = 0;
            w->nState = 10;
            /* fallthrough: the page test runs on the same frame */
        case 10:
            if (UmnWork.nPage == 17) {
                w->nState = 20;
            }
            break;
        case 20:
            for (i = 0; i < 3; i++) {
                char *pGray = &w->row[i].bGray;

                w->row[i].pText = menu00[i];
                if (i == 0) {
                    if (UmnWork.u.ex.nHi == 0) {
                        *pGray = 1;
                    } else {
                        w->row[i].bGray = 0;
                    }
                } else {
                    w->row[i].bGray = 0;
                }
            }
            w->bVisible = 1;
            w->row[i].pText = 0;
            w->win.nX = 528;
            w->win.nY = 176;
            w->sel.nSel = UmnWork.nDataBaseSel;
            w->sel.pRows = w->row;
            w->win.nState = 1;
            WindowDXMain((WINDOWDX *)&w->win);
            w->win.nState = 3;
            w->bReady = 1;
            w->nState = 21;
            /* fallthrough: state 21 runs on the same frame */
        case 21:
            nTarget = 192;
            MoveSlide(&w->win.nX, &nTarget, 3.0f);
            if (w->win.nX == nTarget) {
                w->nState = 30;
            }
            break;
        case 30:
            w->sel.nSel = UmnWork.nDataBaseSel;
            if (UmnWork.nPage != 17) {
                w->nState = 40;
            } else {
                UmnWork.nUiLock = 0;
            }
            break;
        case 40:
            nTargetOut = -125;
            if ((UmnWork.nPage >> 4) == 5) {
                w->bReady = 0;
                w->win.nX = -125;
            } else {
                MoveSlide(&w->win.nX, &nTargetOut, 3.0f);
                if (w->win.nX == nTargetOut) {
                    w->nState = 0;
                    w->bVisible = 0;
                }
            }
            break;
        }
        break;
    }
    if (w->bReady) {
        if (w->bVisible) {
            WindowDXMain((WINDOWDX *)&w->win);
        }
    }
}

/* --- Plugin screen: the installed-plugin list (WindowSP) --- */

extern unsigned char plugin_folder[];
extern void WindowSPItemChange(SPWIN *pWin);
extern void xglFontDebugPrintf(int nX, int nY, char *pFmt, ...);

typedef struct {
    unsigned char nState;               /* 0x0000 */
    unsigned char bReady;               /* 0x0001 */
    char pad0002[2];
    short nX;                           /* 0x0004 */
    short nY;                           /* 0x0006 */
    int nColor;                         /* 0x0008 */
    SPWIN sp;                           /* 0x000C */
    char pad002C[0x1744 - 0x2C];
    SPROW row[8];                       /* 0x1744 */
} UMN_PLIST;

/* Plugin screen: the list of plugins the player has installed.
 *
 * State 20 rebuilds the row table from plugin_folder[] (each slot's id is
 * turned into its caption by UmnPluginTextGet), appends one blank row and
 * the null terminator, and hands the change to WindowSPItemChange; 30
 * slides in, 40 accepts input and publishes the selection into UmnWork,
 * 50 slides back out. */
void tskUmnPluginList(TSK_TASK *pTask, UMN_PLIST *w)
{
    short nTarget;
    short nTargetOut;
    int i;

    if (UmnWork.nScene != 4) {
        pTask->nState = -1;
        return;
    }
    switch (pTask->nState) {
    case 0:
        w->nState = 0;
        w->bReady = 0;
        w->nX = 528;
        w->nY = 112;
        w->nColor = 0x00FF0000;
        w->sp.nX = 528;
        w->sp.nY = 112;
        w->sp.nColor = 0x00FF0000;
        w->sp.nRows = 3;
        w->sp.pTitle = "List";
        w->sp.nFont = 10;
        w->sp.nStyle = 1;
        w->sp.nW = 400;
        w->sp.nH = 246;
        w->sp.pRows = 0;
        WindowSPSet(&w->sp);
        break;
    case 2:
        switch (w->nState) {
        case 0:
            w->bReady = 0;
            w->nState = 10;
            /* fallthrough: the page test runs on the same frame */
        case 10:
            if (UmnWork.nPage == 17) {
                w->bReady = 1;
                w->nState = 20;
            }
            break;
        case 20:
            for (i = 0; i < UmnWork.u.ex.nHi; i++) {
                w->row[i].pText = UmnPluginTextGet(plugin_folder[i]);
                w->row[i].nParam = 0;
                w->row[i].bGray = 0;
            }
            w->row[i].pText = "";
            w->row[i + 1].pText = 0;
            w->sp.pRows = w->row;
            WindowSPItemChange(&w->sp);
            w->sp.nState = 17;
            WindowSPMain(&w->sp);
            w->nState = 30;
            /* fallthrough: state 30 runs on the same frame */
        case 30:
            nTarget = 56;
            MoveSlide(&w->nX, &nTarget, 3.0f);
            if (w->nX == nTarget) {
                w->nState = 40;
            }
            break;
        case 40: {
            int nSel = WindowSPSelect(&w->sp, PadData.nRepeat);

            if (nSel >= 0) {
                UmnWork.u.ex.nLo = nSel;
                UmnWork.u.plugin.nSel = plugin_folder[UmnWork.u.ex.nLo];
            }
            if (UmnWork.nPage != 17) {
                w->nState = 50;
            }
            break;
        }
        case 50:
            nTargetOut = 528;
            MoveSlide(&w->nX, &nTargetOut, 3.0f);
            if (w->nX == nTargetOut) {
                w->nState = 0;
            }
            break;
        }
        break;
    }
    if (w->bReady) {
        w->sp.nX = w->nX;
        w->sp.nY = w->nY;
        w->sp.nColor = w->nColor;
        WindowSPMain(&w->sp);
    }
    xglFontDebugPrintf(0, 16, "list : %2d", w->nState);
}

/* --- Mail screen: the reply ("hensin") window --- */

/* eMessage object as this screen lays it out: the header the mail list
   shares, 0x44 bytes, position at +4 rather than +0. */
typedef struct {
    char pad00[1];
    unsigned char nFont;                /* 0x01 */
    char pad02[2];
    short nX;                           /* 0x04 */
    short nY;                           /* 0x06 */
    int nColor;                         /* 0x08 */
    char pad0C[0x44 - 0x0C];
} EMSG;

/* eCursol object: same +4 position header. */
typedef struct {
    char pad00[4];
    short nX;                           /* 0x04 */
    short nY;                           /* 0x06 */
    int nColor;                         /* 0x08 */
    char pad0C[0x50 - 0x0C];
} ECURSOL;

typedef struct {
    unsigned char nState;               /* 0x000 */
    unsigned char bReady;               /* 0x001 */
    char pad002[2];
    short nX;                           /* 0x004 */
    short nY;                           /* 0x006 */
    int nColor;                         /* 0x008 */
    WINDOWDX win;                       /* 0x00C */
    EMSG msg;                           /* 0x1A0 */
    EMSG list[3];                       /* 0x1E4 */
    ECURSOL cur;                        /* 0x2B0 */
} UMN_HENSIN;

extern void eCursolSet(ECURSOL *pCur, int nMode);
extern void eCursolModeChange(ECURSOL *pCur, int nMode);
extern void eCursolMain(ECURSOL *pCur);
extern void eMessageModeChange(void *pMsg, int nMode);
extern char *UmnMailHeaderGet(signed char nNo);

/* Mail screen: the three-choice reply window.
 *
 * The window's own nState runs 0 -> 10 (cursor parked, wait for page 81)
 * -> 20 (slide in) -> 30 (accept input) -> 40 (slide out).  The trailer
 * redraws unconditionally once bReady is set: the "1:\n2:\n3:" numbering
 * column, one eMessage per available reply, and the cursor parked on the
 * row UmnWork selected. */
void tskUmnMailHensin(TSK_TASK *pTask, UMN_HENSIN *w)
{
    short nTarget;
    short nTargetOut;
    int i;

    if (UmnWork.nScene != 1) {
        pTask->nState = -1;
        return;
    }
    switch (pTask->nState) {
    case 0:
        w->nX = 528;
        w->nY = 48;
        w->nColor = 0x00FF0000;
        WindowDXSet(&w->win);
        w->win.nX = w->nX;
        w->win.nW = 302;
        w->win.nH = 78;
        w->win.nY = w->nY;
        w->win.nState = 1;
        WindowDXMain(&w->win);
        w->win.nState = 3;
        eCursolSet(&w->cur, 0);
        w->bReady = 0;
        w->nState = 0;
        break;
    case 2:
        switch (w->nState) {
        case 0:
            w->bReady = 0;
            w->nState = 10;
            eCursolModeChange(&w->cur, 112);
            /* fallthrough: the page test runs on the same frame.  The jump
             * table's state-10 entry points PAST this call, which is how
             * the split between the two arms was recovered. */
        case 10:
            if (UmnWork.nPage == 81) {
                w->nState = 20;
                w->bReady = 1;
                eCursolModeChange(&w->cur, 32);
            }
            break;
        case 20:
            nTarget = 16;
            MoveSlide(&w->nX, &nTarget, 3.0f);
            if (w->nX == nTarget) {
                w->nState = 30;
            }
            break;
        case 30:
            if ((UmnWork.nPage >> 4) != 5) {
                w->nState = 40;
            } else {
                UmnWork.nUiLock = 0;
            }
            break;
        case 40:
            nTargetOut = 528;
            MoveSlide(&w->nX, &nTargetOut, 3.0f);
            if (w->nX == nTargetOut) {
                w->nState = 0;
            }
            break;
        }
        break;
    }
    if (w->bReady) {
        w->win.nX = w->nX;
        w->win.nY = w->nY;
        w->win.nColor = w->nColor;
        WindowDXMain(&w->win);
        eMessageSet(&w->msg, "1:\n2:\n3:");
        w->msg.nX = w->win.nX + 19;
        w->msg.nY = w->win.nY + 3;
        w->msg.nColor = w->win.nColor + 2;
        eMessageModeChange(&w->msg, 32);
        eMessageMain(&w->msg);
        for (i = 0; i < UmnWork.u54.mail.nNum; i++) {
            eMessageSet(&w->list[i],
                        UmnMailHeaderGet(UmnWork.u54.mail.aId[i]) + 64);
            w->list[i].nX = w->win.nX + 59;
            w->list[i].nColor = w->win.nColor + 2;
            w->list[i].nY = w->win.nY + i * 24 + 3;
            eMessageModeChange(&w->list[i], 32);
            eMessageMain(&w->list[i]);
        }
        w->cur.nX = w->win.nX + 3;
        w->cur.nY = w->list[UmnWork.u.mail.nSel].nY + 4;
        w->cur.nColor = w->win.nColor + 2;
        eCursolMain(&w->cur);
    }
}

/* --- Mail screen: the "Menu" and "Select" windows --- */

typedef struct {
    unsigned char nState;               /* 0x000 */
    unsigned char bReady;               /* 0x001 */
    char pad002[2];
    short nW;                           /* 0x004 */
    short nH;                           /* 0x006 */
    int nColor;                         /* 0x008 */
    int bVisible;                       /* 0x00C */
    SELWIN win;                         /* 0x010: the yes/no confirmation */
    SELPARAM sel;                       /* 0x1A4 */
    char pad1B8[0x79C - 0x1B8];
    int bVisible2;                      /* 0x79C */
    SELWIN win2;                        /* 0x7A0: the four-row menu */
    SELPARAM sel2;                      /* 0x934 */
    char pad948[0xF2C - 0x948];
    SPROW row[4];                       /* 0xF2C */
} UMN_MAILMENU;

void tskUmnMailMenu(TSK_TASK *pTask, UMN_MAILMENU *w)
{
    static char *msg00[] = { "Is this okay?", "Yes\nNo" };
    static char *menu00[] = { "Read", "History", "Download", "Cancel" };
    short nTarget;
    short nTargetOut;
    short nTarget2;
    short nTargetOut2;
    int i;
    int nOn;
    int nCol;

    if (UmnWork.nScene != 1) {
        pTask->nState = -1;
        return;
    }
    switch (pTask->nState) {
    case 0:
        w->nW = 528;
        w->nH = 48;
        w->nColor = 0x00FFFFF0;
        w->bVisible = 0;
        WindowDXSet((WINDOWDX *)&w->win);
        nCol = w->nColor;
        w->win.pTitle = "Select";
        w->win.nH = 86;
        w->sel.pChoices = msg00[1];
        w->win.pMsg = &w->sel;
        w->sel.nColumns = 3;
        w->sel.pPrompt = msg00[0];
        w->win.nColor = nCol;
        w->win.nW = 169;
        w->win.pFunc = MenuSelectWindow;
        w->bVisible2 = 0;
        WindowDXSet((WINDOWDX *)&w->win2);
        w->win2.nColor = w->nColor;
        w->win2.nW = 125;
        w->win2.nH = 102;
        w->sel2.nColumns = 2;
        w->win2.pTitle = "Menu";
        w->win2.pFunc = MenuSelectWindow;
        w->win2.pMsg = &w->sel2;
        w->nState = 0;
        w->bReady = 0;
        break;
    case 2:
        switch (w->nState) {
        case 0:
            w->bReady = 1;
            w->nState = 10;
            /* fallthrough: the page test runs on the same frame */
        case 10:
            switch (UmnWork.nPage) {
            case 49:
                for (i = 0; i < 4; i++) {
                    w->row[i].pText = menu00[i];
                    w->row[i].bGray = 0;
                }
                if (UmnWork.nMailSel != 0) {
                    w->row[1].bGray = 1;
                }
                if (UmnWork.u54.mail.bReplyReady && UmnWork.u54.mail.bReplyOpen) {
                    w->row[2].bGray = 0;
                } else {
                    w->row[2].bGray = 1;
                }
                w->win2.nX = -125;
                w->win2.nY = 176;
                w->sel2.pRows = w->row;
                nOn = 1;
                w->bVisible2 = nOn;
                w->win2.nState = nOn;
                w->sel2.nSel = 0;
                WindowDXMain((WINDOWDX *)&w->win2);
                w->win2.nState = 3;
                w->nState = 20;
                break;
            case 83:
                w->win.nX = 528;
                w->win.nY = 40;
                w->bVisible = 1;
                w->win.nState = 1;
                w->sel.nSel = 0;
                WindowDXMain((WINDOWDX *)&w->win);
                w->win.nState = 3;
                w->nState = 50;
                break;
            }
            break;
        case 20:
            nTarget = 16;
            MoveSlide(&w->win2.nX, &nTarget, 3.0f);
            if (w->win2.nX == nTarget) {
                w->nState = 30;
            }
            break;
        case 30:
            w->sel2.nSel = UmnWork.nDataBaseSel;
            if (UmnWork.nPage != 49) {
                w->nState = 40;
            } else {
                UmnWork.nUiLock = 0;
            }
            break;
        case 40:
            nTargetOut = -125;
            MoveSlide(&w->win2.nX, &nTargetOut, 3.0f);
            if (w->win2.nX == nTargetOut) {
                w->nState = 0;
                w->bVisible2 = 0;
            }
            break;
        case 50:
            nTarget2 = 496 - w->win.nW;
            MoveSlide(&w->win.nX, &nTarget2, 3.0f);
            if (w->win.nX == nTarget2) {
                w->nState = 60;
            }
            break;
        case 60:
            w->sel.nSel = UmnWork.nMailMenuSel;
            if (UmnWork.nPage != 83) {
                w->nState = 70;
            } else {
                UmnWork.nUiLock = 0;
            }
            break;
        case 70:
            nTargetOut2 = 528;
            MoveSlide(&w->win.nX, &nTargetOut2, 3.0f);
            if (w->win.nX == nTargetOut2) {
                w->nState = 0;
                w->bVisible = 0;
            }
            break;
        }
        break;
    }
    if (w->bReady) {
        if (w->bVisible) {
            WindowDXMain((WINDOWDX *)&w->win);
        }
        if (w->bVisible2) {
            WindowDXMain((WINDOWDX *)&w->win2);
        }
    }
    xglFontDebugPrintf(0, 40, "yn : %2d", w->nState);
}

/* --- Database screen: the button-guide strip (five captions + two
   L1/R1 sprites) --- */

/* One caption row: its own slide position followed by the eMessage
   object that draws it.  Five of them tile the work object end to end,
   so row 0's header doubles as the work object's own header. */
/* The drawn half of an eMessage: everything past its four-byte header.
   Retail keeps the address of this sub-object live in a register while
   it recolours row 4, which is why it is spelled out as its own type. */
typedef struct {
    short nX;                           /* 0x04 */
    short nY;                           /* 0x06 */
    int nColor;                         /* 0x08 */
    char pad0C[4];                      /* 0x0C */
    signed char nRgb[3];                /* 0x10 */
    char pad0F[0x38 - 0x0F];            /* 0x0F */
} DBEXPOS;

typedef struct {
    char pad00[4];                      /* 0x00 */
    DBEXPOS p;                          /* 0x04 */
} DBEXMSG;

typedef struct {
    unsigned char nMode;                /* 0x00 */
    unsigned char bReady;               /* 0x01 */
    char pad02[1];                      /* 0x02 */
    unsigned char bShow;                /* 0x03 */
    int nColor;                         /* 0x04 */
    short nX;                           /* 0x08 */
    short nY;                           /* 0x0A */
    DBEXMSG emsg;                       /* 0x0C */
} DBEXROW;

/* The L1/R1 shoulder-button sprites. */
typedef struct {
    char pad00[4];                      /* 0x00 */
    short nX;                           /* 0x04 */
    short nY;                           /* 0x06 */
    int nColor;                         /* 0x08 */
    char pad0C[0x28 - 0x0C];            /* 0x0C */
} DBEXSPR;

typedef struct {
    DBEXROW row[5];                     /* 0x000 */
    char pad168[8];                     /* 0x168 */
    int nSlide[2];                      /* 0x170 */
    DBEXSPR spr[2];                     /* 0x178 */
} UMN_DBEX;

extern void eSpriteSet(DBEXSPR *pSpr, int nId);
extern void eSpriteMain(DBEXSPR *pSpr);

/* TODO: PARKED at 17 differing words of 249.  Length, frame layout and
 * every branch offset are right.  Two clusters remain:
 *
 *  - the sprite loop hoists its invariants in the order -45, &spr,
 *    214, &nSlide; retail emits -45 LAST, after the other three.
 *  - the page-49 arm loads PadData's trigger halfword three times
 *    where retail loads it twice and widens each with `andi 0xffff'.
 *    The andi is a zero-extend that survives only when the loaded
 *    HImode value has more than one user, i.e. retail holds the
 *    widened value in one register across both equality tests while
 *    gcc reloads before the second.
 *
 * Fixed since: the row loop's -196 and the sprite loop's -45 shared one
 * `nX' local.  One C local is one pseudo, so the shared register made
 * gcc materialise 288 before -196; separate locals, with the row x
 * assigned first, put `li -196' ahead of `li 288' as retail has it
 * (19 -> 17).
 *
 * Swept: all six orders of the three sprite-header stores crossed with
 * both orders of the caption x/y stores (twelve builds); the trigger
 * halfword as `unsigned short' + `int', as one `int', read once before
 * the test, once after the bShow xor, re-read inside the xor arm, and
 * at every use (every cached form costs six words); `nSprX = -45'
 * before the nId block, inside it, in the for-init, and block-scoped
 * with the array (no effect on the hoist order). */
void tskUmnDataBaseExWin(TSK_TASK *pTask, UMN_DBEX *w)
{
    static char *text[] = {
        "\036\066\241\247Zoom out",
        "\036\065\241\247Zoom in",
        "\036\064\241\247Rotate",
        "\036\000\241\247Turn menu off",
        "\036\001\241\247Analysis info",
        0
    };
    int i;

    if (UmnWork.nScene != 2) {
        pTask->nState = -1;
        return;
    }
    switch (pTask->nState) {
    case 0: {
        int nY;
        int nX;
        int nSprX;

        w->row[0].nColor = 0x00FFFFF0;
        nX = -196;
        nY = 288;
        for (i = 0; i < 5; i++) {
            w->row[i].nY = nY;
            nY += 24;
            w->row[i].nX = nX;
            eMessageSet(&w->row[i].emsg, text[i]);
            eMessageModeChange(&w->row[i].emsg, 32);
        }
        {
            short nId[2] = { 274, 272 };

            nSprX = -45;
            for (i = 0; i < 2; i++) {
                eSpriteSet(&w->spr[i], nId[i]);
                w->spr[i].nX = nSprX;
                nSprX += 573;
                w->spr[i].nColor = w->row[0].nColor;
                w->spr[i].nY = 214;
                w->nSlide[i] = 0;
            }
        }
        w->row[0].bReady = 0;
        w->row[0].bShow = 1;
        w->row[0].nMode = 0;
        break;
    }
    case 2: {
        short nTarget[5];
        /* Five entries, not two: retail's slot for this array is a full
           16 bytes, which is what keeps it from reusing the sprite-id
           array's freed four-byte temp slot in the init arm. */
        short nSprTarget[5];
        DBEXPOS *e;

        for (i = 0; i < 5; i++) {
            nTarget[i] = -196;
        }
        nSprTarget[0] = -45;
        nSprTarget[1] = 528;
        if (UmnWork.nPage == 49) {
            for (i = 0; i < 5; i++) {
                nTarget[i] = 16;
            }
            /* Read the field at each use rather than caching it in a
               local: CSE folds the three reads into one load, but the
               two equality tests still need it widened to int, which is
               the `andi 0xffff' pair retail emits.  The store to bShow
               may alias, so the second half reloads. */
            if (PadData.trig.b.h & 0x80) {
                w->row[0].bShow ^= 1;
            }
            nSprTarget[0] = 8;
            nSprTarget[1] = 475;
            if (PadData.trig.b.h == 4) {
                w->nSlide[0] = -6;
            }
            if (PadData.trig.b.h == 8) {
                w->nSlide[1] = 6;
            }
        } else {
            w->row[0].bShow = 1;
        }
        for (i = 0, e = &w->row[4].emsg.p; i < 5; i++) {
            MoveSlide(&w->row[i].nX, &nTarget[i], 3.0f);
            w->row[i].emsg.p.nX = w->row[i].nX;
            w->row[i].emsg.p.nY = w->row[i].nY;
            w->row[i].emsg.p.nColor = w->row[0].nColor;
            if (i == 4) {
                if (UmnWork.nDataBaseOpen) {
                    e->nRgb[2] = -128;
                    e->nRgb[1] = -128;
                    e->nRgb[0] = -128;
                } else {
                    e->nRgb[2] = 64;
                    e->nRgb[1] = 64;
                    e->nRgb[0] = 64;
                }
            }
            if (w->row[0].bShow) {
                eMessageMain(&w->row[i].emsg);
            }
        }
        for (i = 0; i < 2; i++) {
            if (w->nSlide[i]) {
                int nAdd[2] = { 1, -1 };

                w->nSlide[i] += nAdd[i];
            }
            MoveSlide(&w->spr[i].nX, &nSprTarget[i], 3.0f);
            w->spr[i].nX = w->spr[i].nX + w->nSlide[i];
            eSpriteMain(&w->spr[i]);
        }
        break;
    }
    }
}

/* --- Database screen: the keyword ("Index Search") viewer --- */

/* The keyword viewer's work area.  The tyaUml display object at 0x7E0
   is spelled out inline rather than as its own struct: it ends with a
   four-byte field at 0x848 but contains an eight-byte one at 0x7F0, so
   a nested struct would be rounded up to 0x70 and push the ribbon and
   tag objects four bytes past where retail puts them. */
typedef struct {
    unsigned char nMode;                /* 0x000 */
    unsigned char bReady;               /* 0x001 */
    unsigned char bSlide;               /* 0x002 */
    unsigned char bInfo;                /* 0x003 */
    int nColor;                         /* 0x004 */
    char aList[0x19C - 0x008];          /* 0x008 */
    char aRec[0x794 - 0x19C];           /* 0x19C */
    char aName[0x7B8 - 0x794];          /* 0x794 */
    char aText[0x7E0 - 0x7B8];          /* 0x7B8 */
    char aUml[8];                       /* 0x7E0 */
    short nUmlW;                        /* 0x7E8 */
    short nUmlW2;                       /* 0x7EA */
    short nUmlH;                        /* 0x7EC */
    short nUmlH2;                       /* 0x7EE */
    long long nUmlColor;                /* 0x7F0 */
    char pad7F8[0x820 - 0x7F8];         /* 0x7F8 */
    void *pUmlList;                     /* 0x820 */
    void *pUmlRec;                      /* 0x824 */
    void *pUmlName;                     /* 0x828 */
    void *pUmlText;                     /* 0x82C */
    short nUmlStep;                     /* 0x830 */
    char pad832[0x838 - 0x832];         /* 0x832 */
    short nUmlTotal;                    /* 0x838 */
    short nUmlSel;                      /* 0x83A */
    char pad83C[0x840 - 0x83C];         /* 0x83C */
    short nUmlX;                        /* 0x840 */
    unsigned short nUmlY;               /* 0x842 */
    int nUmlSelI;                       /* 0x844 */
    int nUmlTotalI;                     /* 0x848 */
    EXTAGFONT tag;                      /* 0x84C */
    EXRIBBON rib;                       /* 0x86C */
    EXNUMBER num;                       /* 0x8DC */
    char pad960[0x96C - 0x960];         /* 0x960 */
    EMSG msg;                           /* 0x96C */
} UMN_KEYWORD;

extern void tyaUmlDispParamReset(void *pUml, int nMode);
extern signed char tyaUmlDatabaseMain(void *pUml);

/* TODO: PARKED at 216 built words against 210 original (185 differing
 * words by tools/scratch_diff.py; checkfile.py still prints 191,
 * because for a length-mismatched function its count is dominated by
 * the tail shift rather than by the real divergences).
 * Behaviour, every constant, every struct offset and both dispatch
 * shapes are recovered; the six extra words are all register
 * allocation:
 *
 *  - the init arm computes the two record pointers into $t4/$t5 and
 *    then `move's them into $a2/$a3 for eRibbonSet, where retail builds
 *    them in $a2/$a3 once and stores from there (2 words).
 *  - the trailer materialised the constants 48 and 3 twice each and
 *    loaded tag.nY/tag.nColor once per use.  Naming each of them once
 *    (n48, n3, nNumY, nNumColor) is worth six differing words and is
 *    kept below; it did not shorten the function, so the extra length
 *    is entirely in the init arm.
 *
 * Measured this run: for THIS block the emitted store order is a
 * left-rotation of the source order by SIX groups, and it is stable --
 * writing the eleven header stores in retail's own emission order
 * produced exactly that order rotated by six.  The two pointer stores
 * are the one exception: whatever the rotation, gcc pushes them three
 * to four slots later than the pure rotation predicts, which is the
 * same fact as the two `move's.  So the next thing to try is not
 * another statement order -- it is whatever makes the pointer that is
 * ALSO a call argument keep one register for both roles.
 *
 * Swept: source order = retail emission order, = that rotated by one,
 * and the original order; the pointers as `void *' locals, as casts,
 * and read back out of the struct; eRibbonSet prototyped and
 * unprototyped. */
void tskUmnDataBaseKeyWord(TSK_TASK *pTask, UMN_KEYWORD *w)
{
    if (UmnWork.nScene != 2) {
        pTask->nState = -1;
        return;
    }
    switch (pTask->nState) {
    case 0: {
        static char msg00[16] = "\036\000Index Search:";

        w->nColor = 0x00F00000;
        tyaUmlDispParamReset(w->aUml, 0);
        w->nUmlH2 = 314;
        w->nUmlX = -272;
        w->nUmlY = 48;
        w->pUmlList = w->aList;
        w->pUmlRec = w->aRec;
        w->pUmlName = w->aName;
        w->pUmlText = w->aText;
        w->nUmlW = 1808;
        w->nUmlH = 480;
        w->nUmlW2 = 1920;
        w->nUmlColor = (unsigned int)w->nColor;
        eRibbonSet(&w->rib, 3, w->aName, w->aText);
        w->rib.nW = 256;
        w->rib.nH = 24;
        eTagFontSet(&w->tag, "\001Num");
        eNumberSet(&w->num, 0);
        eMessageSet(&w->msg, msg00);
        w->msg.nFont = 32;
        w->nMode = 0;
        w->bReady = 0;
        break;
    }
    case 2:
        switch (w->nMode) {
        case 0:
            w->bReady = 0;
            w->nUmlStep = 0;
            /* fallthrough */
        case 1:
            if (UmnWork.nPage == 81) {
                w->nMode = 10;
            }
            break;
        case 10:
            w->bReady = 1;
            w->nMode = 11;
            w->bSlide = 1;
            w->bInfo = 1;
            /* fallthrough */
        case 11:
            if (UmnWork.nPage == 17) {
                if (UmnWork.nDataBaseHit == 1) {
                    w->nMode = 0;
                }
            }
            break;
        }
        if (w->bReady) {
            if (w->nUmlSel >= 0) {
                if (PadData.trig.b.h & 0x20) {
                    w->bInfo = 0;
                }
                if (PadData.trig.b.h & 0x40) {
                    w->bInfo = 1;
                }
            }
            UmnWork.nDataBaseHit = tyaUmlDatabaseMain(w->aUml);
            w->nUmlSelI = w->nUmlSel;
            w->nUmlTotalI = w->nUmlTotal;
            if (w->bInfo) {
                w->msg.nX = 48;
                w->msg.nY = 416;
                w->msg.nColor = w->nColor + 15;
                eMessageMain(&w->msg);
            }
        }
        if (w->bSlide) {
            short nTarget;
            int nTagY;
            /* One local per value the original keeps in ONE register:
               the 48 is stored twice and passed to eRibbonMain, the 3
               is stored twice and passed to eNumberMain, and the two
               tag fields are stored into `num' AND passed.  Written as
               repeated literals/field reads gcc materialises each of
               them a second time for the argument. */
            int n48;
            int n3;
            unsigned short nNumY;
            int nNumColor;

            nTarget = -272;
            if ((UmnWork.nPage >> 4) == 5) {
                nTarget = 0;
            }
            MoveSlide(&w->nUmlX, &nTarget, 3.0f);
            if (w->nUmlX == -272) {
                w->bSlide = 0;
            }
            n48 = 48;
            w->nUmlY = n48;
            w->rib.nColor = w->nColor;
            w->rib.nY = n48;
            w->rib.nX = w->nUmlX;
            eRibbonMain(&w->rib, n48);
            w->tag.nX = w->nUmlX + 166;
            nTagY = w->nUmlY + 4;
            w->tag.nY = nTagY;
            w->tag.nColor = w->rib.nColor + 1;
            eTagFontMain(&w->tag, nTagY);
            nNumY = w->tag.nY;
            nNumColor = w->tag.nColor;
            n3 = 3;
            w->num.nX = w->tag.nX + 41;
            w->num.nY = nNumY;
            w->num.nColor = nNumColor;
            w->num.nValue = (w->nUmlTotalI << 16) + (unsigned short)w->nUmlSelI;
            w->num.nDigits = n3;
            w->num.nAlign = n3;
            eNumberMain(&w->num, nNumY, nNumColor, n3);
        }
        break;
    }
}
