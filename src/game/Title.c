/*
 * Title: the title screen and its HDD install/uninstall sub-menus.
 *
 * These live in the boot overlay: the title screen owns the whole machine
 * before the field engine is up, so it does its own resource bring-up
 * (flags, party table, font bank, window texture) rather than going
 * through Game's loop state.
 */

#include "common.h"

/* The whole game-loop state block, cleared wholesale on a new game. */
extern char GameLoopState[0x2A030];

extern void *memset(void *pDst, int nVal, u_int nSize);
extern void xglFlagsInitial(void);
extern void PartyDataInit(void);
extern int xglFontLoad(int nBank, int nNow);
extern void WindowTexLoad(int nAddr, int nArg);

/* Retail boot-display hook; intentionally empty in this build. */
void BootDisplay(void)
{
}

/* Bring the game state back to its power-on values before a new game. */
void TitleGameInit(void)
{
    memset(GameLoopState, 0, 0x2A030);
    xglFlagsInitial();
    PartyDataInit();
    xglFontLoad(1, 0);
    WindowTexLoad(0, 0);
}
