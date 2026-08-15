#include "matching.h"

/* tskMenuPausePage - debug pause-menu pages (resource / actor dumps) */

typedef struct PADDATA {
    char pad00[0x2A];
    unsigned short hTrig;          /* 0x2A: trigger bits, 0x40 = SELECT? */
} PADDATA;

/* One actor slot as seen by the debug list: id short at +0x86, slots
 * packed every 0xA70 bytes starting 0x80 into the actor array */
typedef struct PAUSEACT {
    char pad00[6];
    short h06;
    char pad08[0xA70 - 8];
} PAUSEACT;

extern PADDATA PadData;
extern char actor[];
extern char *PauseWorkEnd;

extern void GameResourceDump(int mode);
extern void xglFontDebugPrintf(int x, int y, char *fmt, ...);

/* Debug page 0: dump the resource list and print the work-end pointer
 * plus the first 16 actor ids while the trigger is held.
 * PARKED 5-word diff (was 8): i=$16/p=$17 pins + post-call volatile barrier
 * + second volatile launder of base (conflicts base out of s1, keeps the
 * 3-op addiu s1,v0,128) fix the allocation. Remaining pure scheduling:
 * printf-arg lui-a2/lw-a3 swap, and the loop-invariant "loadene" lui s3
 * hoists to preheader end (after p/i inits) where retail has it between
 * the base addiu pair and the p init. Fixer-flag candidate (two swaps). */
void PauseMenuPage0(void)
{
    PIN(PAUSEACT *p, "$17");
    int y;
    PIN(int i, "$16");
    char *base;

    if (PadData.hTrig & 0x40) {
        GameResourceDump(0);
    }
    xglFontDebugPrintf(136, 52, "WrkEnd:%8x", PauseWorkEnd);
    y = 64;
    __asm__ volatile("");
    base = actor;
    LAUNDER(base);
    p = (PAUSEACT *)(base + 0x80);
    LAUNDER_V(base);
    i = 15;
    do {
        i--;
        xglFontDebugPrintf(8, y, "loadene", p->h06);
        p++;
        y += 8;
    } while (i >= 0);
    GameResourceDump(1);
}
