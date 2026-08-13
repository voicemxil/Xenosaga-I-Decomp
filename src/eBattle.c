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

/* TODO: not decompiled -- LENGTH mismatch (27 orig vs 24 built words). 2.96
 * batches/reorders the gp-rel stores to g_pEBattleUnk* near the end instead
 * of interleaving them with the byte/word stores as the original does;
 * logic is believed correct, register/scheduling tie-break not found in one
 * pass. */
/* Carve up the win-screen work buffer into its sub-areas and init the end-print module */
void *eBattleWinInit2(unsigned char *pWork)
{
    g_pEBattleWork = pWork;
    pWork[1] = 0;
    g_pEBattleUnk2 = pWork + 6424;
    g_pEBattleUnk3 = pWork + 9496;
    *(int *)(pWork + 9496) = 0;
    g_pEBattleUnk4 = pWork + 9976;
    *(int *)(pWork + 9976) = 0;
    g_pEBattleUnk0 = 0;
    *(int *)(pWork + 12320) = 0;
    g_pEBattleUnk5 = pWork + 12320;
    endPrintInit();
    return pWork + 12800;
}
