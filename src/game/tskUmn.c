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
    char pad04[0x46 - 0x04];
    signed char nDataBaseSel;           /* 0x46 */
    char pad47[0x50 - 0x47];
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
    } u;
    int nListNum;                       /* 0x54: rows in the current list */
    char pad58[0x5C - 0x58];
    signed char nMailSel;               /* 0x5C: mail hint-slot cursor */
    char pad5D[1];
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

extern char *UmnPluginTextGet(signed char nNo);
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
            w->msg.pText = UmnPluginTextGet(UmnWork.u.plugin.nSel) + 32;
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

extern void eRibbonSet(EXRIBBON *pRib, int nId, int nX, int nY);
extern void eRibbonMain(EXRIBBON *pRib, int nColor);
extern void eTagFontSet(EXTAGFONT *pTag, char *pText);
extern void eTagFontMain(EXTAGFONT *pTag, int nColor);
extern void eNumberSet(EXNUMBER *pNum, int nValue);
extern void eNumberMain(EXNUMBER *pNum, int nY, int nAlign);

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
        for (i = 0; i < UmnWork.nListNum; i++) {
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
