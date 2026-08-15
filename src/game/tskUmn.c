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
    } u;
    char pad54[0x80 - 0x54];
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
extern void MoveSlide(short *pPos, short *pTarget, float fSpeed);
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

/* --- info-window work object (bottom help window of each screen) --- */
typedef struct {
    unsigned char nMode;                /* 0x00 */
    unsigned char bReady;               /* 0x01 */
    char pad02[6];
    int nColor;                         /* 0x08 */
    WINDOWDX win;                       /* 0x0C */
    MSGDX msg;                          /* 0x1A0 */
    char pad1E4[0x350 - 0x1E4];
    char szText[0x10];                  /* 0x350 (simulation) */
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
    short nX;                           /* 0x00 */
    short nY;                           /* 0x02 */
    int nColor;                         /* 0x04 */
    short nW;                           /* 0x08 */
    short nH;                           /* 0x0A */
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
            m->emsg.nColor = w->nColor + 2;
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
            MoveSlide(&w->win.nX, &nTarget[0], 3.0f);
            WindowDXMain((WINDOWDX *)&w->win);
            w->box.nX = (unsigned short)w->win.nX + 3;
            w->box.nY = (unsigned short)w->win.nY + 3;
            w->box.nW = (unsigned short)w->win.nW - 6;
            w->box.nH = (unsigned short)w->win.nH - 6;
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
            m->emsg.nColor = w->nColor + 2;
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
            MoveSlide(&w->win.nX, &nTarget[0], 3.0f);
            WindowDXMain((WINDOWDX *)&w->win);
            w->box.nX = (unsigned short)w->win.nX + 3;
            w->box.nY = (unsigned short)w->win.nY + 3;
            w->box.nW = (unsigned short)w->win.nW - 6;
            w->box.nH = (unsigned short)w->win.nH - 6;
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
            m->emsg.nColor = w->nColor + 2;
        }
        w->bReady = 1;
        break;
    case 2:
        nTargetX = -16;
        for (i = 3; i >= 0; i--) {
            nTarget[i] = 288;
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
            MoveSlide(&w->win.nX, &nTargetX, 3.0f);
            WindowDXMain((WINDOWDX *)&w->win);
            w->box.nX = (unsigned short)w->win.nX + 3;
            w->box.nY = (unsigned short)w->win.nY + 3;
            w->box.nW = (unsigned short)w->win.nW - 6;
            w->box.nH = (unsigned short)w->win.nH - 6;
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
