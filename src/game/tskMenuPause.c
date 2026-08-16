/* tskMenuPause - in-game PAUSE overlay: shadow/credit packets, the pause
   state machine and its centered-text renderers */

typedef struct VIFPK {
    unsigned int *pPk;             /* 0x00: current packet write pointer */
} VIFPK;

/* 7-quadword direct DMA blob drawn behind the pause text */
static unsigned int PauseBgPacket[28] = {
    0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x51000006, 0x00000180, 0x50234000, 0x000551EE,
    0x00000000, 0x00071001, 0x00000000, 0x00000047,
    0x00000000, 0x00000044, 0x00000000, 0x00000042,
    0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00000040, 0x00006FF8, 0x000071F7, 0x40000000,
    0x00000000, 0x00008FF8, 0x00008DF7, 0x40000000,
};

char PauseStr[] = "\x0b\x0d\x03\x19\x03PAUSE\x19\x02";
char PauseSkipStr[] =
    "\x0b\x0d\x03\x0c \x80 \xa2\xa4\x0c\x80\x80\x80\x19\x03"
    " Button : Skip and proceed\x19\x02";
char PauseCancelStr[] =
    "\x0b\x0d\x03\x19\x03START Button : Cancel PAUSE\x19\x02";

extern void sceVif1PkAddDirectDataN(unsigned int *pPk, void *pData, int n);
extern void sceVif1PkRef(unsigned int *pPk, void *pData, int n,
                         int a3, int t0, int t1);
extern unsigned int *xglPacketGetCurrent(void);
extern int xglFontGetStringWidth(char *pStr);
extern void xglFontPrint(int x, int y, int nColor, char *pStr);
extern void xglFontPrintExtFunc(int nColor, void (*pFunc)(VIFPK *), int nArg);
extern void xglFontSetFlags(int nFlags);

/* Queue the pause background quad behind the currently printing string.
   The definition lives in Draw.c (where it is registered and matches);
   a second, static copy here made the built image carry two DrawShadows
   and PauseMenu reference the wrong one. */
extern void DrawShadow(VIFPK *pk);

/* Build and queue an 8-quadword sprite packet for the credit screen.
 * PARKED 6-word diff: the five constants that get callee-saved registers
 * are the right five, but the roles of $s2/$s3/$s4 are rotated -- retail
 * puts the twice-used -1 in $s2 and the two single-use constants after it,
 * we put -1 last. This is a coloring-priority effect of that one 2-use
 * pseudo, and it is NOT reachable by store order: an exhaustive sweep of
 * all 720 orderings of the six constant stores and a hill climb over all
 * 23 statements in the packet both bottom out at 6 (from 7), which is the
 * 0x50/0x54 pair swapped as written here. Whoever picks this up should
 * look for what shortens the -1 pseudo's live range or lengthens the other
 * two, not for another permutation. */
static void DrawCredit(VIFPK *pk)
{
    unsigned int *q = (unsigned int *)((char *)pk + 0x30);

    q[0x00 / 4] = 0x8001;
    q[0x04 / 4] = 0x70AB4000;
    q[0x08 / 4] = 0x0535316E;
    q[0x0C / 4] = 0;
    *(long long *)(q + 0x10 / 4) = 0x0007000DLL;
    *(long long *)(q + 0x18 / 4) = 71;
    *(long long *)(q + 0x20 / 4) = 0x2007ED8629343C00LL;
    q[0x30 / 4] = 128;
    q[0x34 / 4] = 128;
    q[0x38 / 4] = 128;
    q[0x3C / 4] = 128;
    q[0x5C / 4] = 0;
    q[0x60 / 4] = 10752;
    q[0x64 / 4] = 752;
    q[0x70 / 4] = 0x8F78;
    q[0x74 / 4] = 0x8D77;
    q[0x7C / 4] = 0;
    q[0x40 / 4] = 4096;
    q[0x44 / 4] = 432;
    q[0x58 / 4] = -1;
    q[0x54 / 4] = 0x8C37;
    q[0x50 / 4] = 30072;
    q[0x78 / 4] = -1;
    sceVif1PkAddDirectDataN(pk->pPk, q, 8);
}

typedef struct PAUSEWORK {
    char pad000[0xD0];
    signed char nSnapMode;         /* 0x0D0 */
    char pad0D1[0x29F50 - 0xD1];
    unsigned char nState;          /* 0x29F50: 0 draw, 1..99 wait,
                                    * 100..101 fade, 102 snapshot */
    char pad29F51[0x29F70 - 0x29F51];
    unsigned short nFontFlags;     /* 0x29F70 */
    unsigned short nProgType;      /* 0x29F72 */
} PAUSEWORK;

extern PAUSEWORK GameLoopState;
extern unsigned char PauseCancelFlag;
extern char *PauseWorkEnd;
extern int s_nProgType;

extern int GameSnapShotSave(int mode, char *buf);
extern void GameSnapShotSaveFile(int n, char *buf, int size);
extern void GamePauseDispCf(void);

/* Pause state machine: draw the PAUSE banner on entry, run the fade
 * counter, and on state 102 snapshot the screen and restore the font */
void PauseMenu(void)
{
    PAUSEWORK *g = &GameLoopState;
    int st = g->nState;

    switch (st) {
    case 0:
        xglFontPrintExtFunc(0xFFFFFF, DrawShadow, 0);
        GamePauseDispCf();
        break;
    case 100:
    case 101:
        if (PauseCancelFlag != 0) {
            xglFontPrintExtFunc(-1, DrawCredit, 0);
        }
        g->nState++;
        break;
    case 102: {
        char *buf = (char *)(((int)PauseWorkEnd + 63) & ~63);

        /* PARKED 2-word scheduling diff: the retail object loads the
         * SaveFile args as li a0 / move a1,s0 / jal / [slot] move a2,v0,
         * while every source form tried here emits the coalesced
         * move a2,v0 right after the GameSnapShotSave return instead.
         * Needs a fixer pass that can swap two adjacent independent
         * moves at one site (see report). */
        int n = GameSnapShotSave(g->nSnapMode, buf);

        GameSnapShotSaveFile(-1, buf, n);
        xglFontSetFlags(g->nFontFlags);
        s_nProgType = g->nProgType;
        g->nState = 0;
        break;
    }
    }
}

/* GamePauseDispBG / GamePauseDispCf / GamePauseDispEvent used to be
   duplicated here; they are registered against (and match in) Game.c,
   and two definitions of one symbol let the linker pick the copy no
   per-function check was looking at.  Removed. */
