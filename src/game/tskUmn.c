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
    } u;
    char pad54[0x5C - 0x54];
    signed char nMailSel;               /* 0x5C: mail hint-slot cursor */
    char pad5D[0x80 - 0x5D];
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
