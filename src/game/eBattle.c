/* eBattle - the battle-result / battle-window overlay screens */

extern void *MenuWorkEndGet(void);
extern void *eBattleWinInit2(unsigned char *);
extern void endPrintInit(void);

extern unsigned char *g_pEBattleWork;
extern void *g_pEBattleUnk0;
extern void *g_pEBattleUnk2;
extern void *g_pEBattleUnk3;
extern void *g_pEBattleUnk4;
extern void *g_pEBattleUnk5;

/* Reset the win-screen work area's status/flag bytes */
void eBattleWinClose(void)
{
    g_pEBattleWork[8] = 4;
    g_pEBattleWork[5968] = 4;
    g_pEBattleWork[6357] = 0;
}

/* Grab the shared menu work buffer and hand it to the win-screen setup */
void eBattleWinInit(void)
{
    eBattleWinInit2(MenuWorkEndGet());
}

/* Clear the sub-window's status byte (BW2) */
void eBattleWinClose2(void)
{
    *((char *)g_pEBattleUnk3 + 24) = 0;
}

/* Set two status bytes on the BW3 sub-window */
void eBattleWinClose3(void)
{
    *((char *)g_pEBattleUnk4 + 20) = 4;
    *((char *)g_pEBattleUnk4 + 804) = 4;
}

/* Set the BW4 sub-window's status byte */
void eBattleWinClose4(void)
{
    *((char *)g_pEBattleUnk5 + 24) = 4;
}

/* Read the BW4 sub-window's page/check word */
int eBattleWinPageCheck4(void)
{
    return *(int *)((char *)g_pEBattleUnk5 + 4);
}

/* Carve up the win-screen work buffer into its sub-areas and init the end-print module */
void *eBattleWinInit2(unsigned char *pWork)
{
    void *p;

    g_pEBattleWork = pWork;
    pWork[1] = 0;
    p = pWork + 6424;
    g_pEBattleUnk2 = p;
    p = pWork + 9496;
    g_pEBattleUnk3 = p;
    *(int *)p = 0;
    p = pWork + 9976;
    g_pEBattleUnk4 = p;
    *(int *)p = 0;
    g_pEBattleUnk0 = 0;
    p = pWork + 12320;
    g_pEBattleUnk5 = p;
    *(int *)p = 0;
    p = pWork + 12800;
    endPrintInit();
    return p;
}

/* Every win sub-window shares this layout: a status word at 0, a page
 * word at 4, a WindowDX block at 8, a state byte at 0x18, and its
 * message context at 0x19C. */
typedef struct {
    int nStatus;               /* 0x000 */
    int nPage;                 /* 0x004 */
    unsigned short nX;         /* 0x008 WindowDX block */
    unsigned short nY;         /* 0x00A */
    int nDepth;                /* 0x00C */
    short nW;                  /* 0x010 */
    short nH;                  /* 0x012 */
    int nUnk14;                /* 0x014 */
    char nState;               /* 0x018 */
    char nOpen;                /* 0x019 */
    char pad1A[0x19C - 0x1A];
    char msg[1];               /* 0x19C */
    char nMsgAttr;             /* 0x19D */
    char pad19E[2];
    short nMsgX;               /* 0x1A0 */
    short nMsgY;               /* 0x1A2 */
    int nMsgDepth;             /* 0x1A4 */
    char pad1A8[0x1B8 - 0x1A8];
    char nMsgFlag;             /* 0x1B8 */
} EBWIN;

/* Caller-supplied window description passed to the *Open entry points */
typedef struct {
    unsigned short nX;         /* 0x00 */
    char pad02[2];
    unsigned short nY;         /* 0x04 */
    char pad06[2];
    int nDepth;                /* 0x08 */
    int nUnk0C;                /* 0x0C */
    int nMessage;              /* 0x10 */
} EBOPEN;

typedef struct {
    char pad0[0x2A];
    unsigned short hHeld;      /* 0x2A */
} EBPADWORK;
extern EBPADWORK PadData;

extern void endPrintExtFunc(int nColor, int a, int b);
extern void WindowDXMain(void *p);
extern void eMessageMain(void *p);
extern int eMessageNextPage(void *p, int a);

/* BW3: the plain message sub-window. State 0 closes it, state 3 keeps
 * pumping the message, anything else in range just stays open. */
int eBattleWinMain2(void)
{
    EBWIN *w = (EBWIN *)g_pEBattleUnk3;
    int ret;
    if (w->nStatus == 0) {
        return 0;
    }
    endPrintExtFunc(0xFFFFFF, 100, 0);
    WindowDXMain((char *)g_pEBattleUnk3 + 8);
    w = (EBWIN *)g_pEBattleUnk3;
    switch (w->nState) {
    case 0:
        w->nStatus = 0;
        goto closed;
    case 3:
        eMessageMain(w->msg);
        ret = 2;
        break;
    case 1:
    case 2:
    case 4:
    case 5:
        ret = 1;
        break;
    default:
    closed:
        ret = 0;
        break;
    }
    return ret;
}

/* BW4: same shape, but its message advances a page on the confirm button
 * and the state numbering starts at 1. */
/* TODO: near-miss (2 words short of 48). Everything up to the switch and
   every arm body matches. The original materialises the default arm's
   `ret = 0` as its own block after the ret=1 block, so the ret=1 arm has
   to branch over it; 2.96 instead annuls the move into the dispatch
   test's delay slot (`beqzl`) and drops the block, which is the same
   beqz/beqzl wall as scDispatchScript. Swept: hoisting `p = w` above the
   switch (gcc sinks it back), `default: return 0`, initialising ret
   before the switch with an empty default, and routing the early exit
   through a shared `closed:` label on the default arm (24 diffs, worse).
   The same shared-label trick is what closed eBattleWinMain2. */
int eBattleWinMain4(void)
{
    EBWIN *w = (EBWIN *)g_pEBattleUnk5;
    EBWIN *p;
    int ret;
    if (w->nStatus == 0) {
        return 0;
    }
    endPrintExtFunc(0xFFFFFF, 100, 0);
    WindowDXMain((char *)g_pEBattleUnk5 + 8);
    w = (EBWIN *)g_pEBattleUnk5;
    /* The window pointer is re-read once after eMessageNextPage and that
       single reload feeds both the page store and the eMessageMain
       argument; the initial copy is made before the switch, which is what
       fills the dispatch branch's delay slot. */
    p = w;
    switch (w->nState) {
    case 3:
        {
            if (PadData.hHeld & 0x20) {
                int last = eMessageNextPage(w->msg, 0);
                p = (EBWIN *)g_pEBattleUnk5;
                if (last == 0) {
                    p->nPage = 1;
                }
            }
            eMessageMain(p->msg);
        }
        ret = 2;
        break;
    case 1:
    case 2:
    case 4:
    case 5:
        ret = 1;
        break;
    default:
        ret = 0;
        break;
    }
    return ret;
}

extern void WindowDXSet(void *p);
extern void eMessageSet(void *p, int nMsg);
extern void eMessageTextChange(void *p, int nMsg);

/* BW4: open the message window from a caller-supplied description, then
 * place its shadow copy three pixels down-right and one step nearer. */
void eBattleWinOpen4(EBOPEN *src)
{
    EBWIN *w;
    int nUnk14;
    int nX;
    int nY;
    int nDepth;
    WindowDXSet((char *)g_pEBattleUnk5 + 8);
    w = (EBWIN *)g_pEBattleUnk5;
    /* Read-all then store-all: the original loads all four description
       fields before it stores any of them. */
    nUnk14 = src->nUnk0C;
    nY = src->nY;
    nX = src->nX;
    nDepth = src->nDepth;
    w->nOpen = 1;
    w->nUnk14 = nUnk14;
    w->nY = nY;
    w->nDepth = nDepth;
    w->nW = 468;
    w->nH = 112;
    w->nX = nX;
    eMessageSet((char *)g_pEBattleUnk5 + 412, src->nMessage);
    ((EBWIN *)g_pEBattleUnk5)->nMsgFlag = 1;
    eMessageTextChange((char *)g_pEBattleUnk5 + 412, src->nMessage);
    w = (EBWIN *)g_pEBattleUnk5;
    w->nMsgAttr = 34;
    w->nMsgDepth = w->nDepth + 2;
    w->nMsgY = w->nY + 3;
    w->nMsgX = w->nX + 3;
    ((EBWIN *)g_pEBattleUnk5)->nState = 1;
    ((EBWIN *)g_pEBattleUnk5)->nStatus = 1;
    ((EBWIN *)g_pEBattleUnk5)->nPage = 0;
}
