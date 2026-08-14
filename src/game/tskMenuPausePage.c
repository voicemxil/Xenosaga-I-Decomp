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
 * PARKED 8-word diff: retail allocates i=s0/p=s1 (and hoists the format
 * string lui before the pointer init); every loop/decl permutation tried
 * here allocates p=s0/i=s1. Same allocno-priority class of diff as
 * DrawCredit. */
void PauseMenuPage0(void)
{
    PAUSEACT *p;
    int y;
    int i;
    char *base;

    if (PadData.hTrig & 0x40) {
        GameResourceDump(0);
    }
    xglFontDebugPrintf(136, 52, "WrkEnd:%8x", PauseWorkEnd);
    y = 64;
    i = 15;
    base = actor;
    __asm__("" : "+r"(base));
    p = (PAUSEACT *)(base + 0x80);
    do {
        i--;
        xglFontDebugPrintf(8, y, "loadene", p->h06);
        p++;
        y += 8;
    } while (i >= 0);
    GameResourceDump(1);
}
