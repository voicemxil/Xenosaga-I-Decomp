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

/* TODO: near-miss, 15 of 24 words (was 23 of 24).
 *
 * Recovered so far: the retail build keeps pWork in a0 for the whole
 * body and computes every sub-area address INTO s0, and it computes the
 * return value BEFORE the call (it sits in the jal's delay slot). Both
 * fall out of using ONE local reassigned all the way through, including
 * for the result -- gcc 2.9x gives a C local exactly one pseudo, and
 * because that pseudo is live across endPrintInit() it lands in the
 * callee-saved s0, so every `p = pWork + K` becomes `addiu s0,a0,K`.
 * Writing `return pWork + 12800;` instead makes pWork itself live across
 * the call, and then the whole prologue shifts (`move s0,a0` + gp stores
 * off s0). Words 0-7 and 10 now match.
 *
 * What is left is three REDUNDANT register copies the retail build has
 * and 2.96 does not: `move v0,s0` / `move v1,s0` before each zero-store,
 * where s0 still holds the same value at the store (the last one is
 * provably dead -- s0 is not rewritten between the copy and the use).
 * That is a reload artifact of the original compiler; 2.96 coalesces the
 * copies away. Transcribing them as explicit `q = p;` locals does
 * reproduce two of the three and gets to 14 of 26 words, but that is
 * encoding a compiler artifact in the source and it still cannot make
 * the last, dead copy appear -- it needs PINs. Left as plain C. Swept:
 * inline-address form, separate p2/p3/p4 locals, stores through the
 * globals themselves, and both return spellings.
 */
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
    char win[0x18 - 8];        /* 0x008 WindowDX block */
    char nState;               /* 0x018 */
    char pad19[0x19C - 0x19];
    char msg[4];               /* 0x19C */
} EBWIN;

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
    WindowDXMain(((EBWIN *)g_pEBattleUnk3)->win);
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
int eBattleWinMain4(void)
{
    EBWIN *w = (EBWIN *)g_pEBattleUnk5;
    int ret;
    if (w->nStatus == 0) {
        return 0;
    }
    endPrintExtFunc(0xFFFFFF, 100, 0);
    WindowDXMain(((EBWIN *)g_pEBattleUnk5)->win);
    w = (EBWIN *)g_pEBattleUnk5;
    switch (w->nState) {
    case 3:
        if (PadData.hHeld & 0x20) {
            if (eMessageNextPage(w->msg, 0) == 0) {
                ((EBWIN *)g_pEBattleUnk5)->nPage = 1;
            }
        }
        eMessageMain(((EBWIN *)g_pEBattleUnk5)->msg);
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
