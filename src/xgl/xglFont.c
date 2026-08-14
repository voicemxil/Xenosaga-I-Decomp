/* Font rendering / packet-flush subsystem */

typedef unsigned char u_char;
typedef unsigned short u_short;
typedef unsigned int u_int;

typedef struct
{
    int      nLoadAddr;   /* 0x00 */
    char     pad04[4];
    u_short  nFlags;      /* 0x08 */
    u_char   pad0A;       /* 0x0A */
    u_char   nDebugMode;  /* 0x0B */
    u_short  nLineHeight; /* 0x0C */
} FSDATA;

extern FSDATA FS;

/* Scratchpad-resident font cursor state */
typedef struct
{
    char     pad00[4];    /* 0x00 */
    u_short  nHomeX;      /* 0x04 */
    char     pad06[2];    /* 0x06 */
    u_short  nX;          /* 0x08 */
    u_short  nY;          /* 0x0A */
} FSPWORK;

#define FSP ((FSPWORK *)0x70000000)

extern long long ModeEnv[];

void xglFontFlushSub(int nAddr, int nB, int nC, int nD);

void xglFontPrintDirectOT(int nOT, char *pStr);
int xglFontGetStringWidth2(void *pStr, int nArg);
void set_ot(int nOT);
void set_xyz(int nOT);
void xglFontPrintDirectCore(char *pStr);
void xglFontPrintSub(char *pStr);

/* Return the font microcode load address */
int xglFontGetLoadAddress(void)
{
    return FS.nLoadAddr;
}

/* Return the current font flags word */
int xglFontGetFlags(void)
{
    return FS.nFlags;
}

/* Set the font flags word, forcing bit 1 (0x2) clear */
void xglFontSetFlags(int nFlags)
{
    FS.nFlags = nFlags & 0xFFFD;
}

/* Print a string with no OT registration */
void xglFontPrintDirect(char *pStr)
{
    xglFontPrintDirectOT(0, pStr);
}

/* Get the pixel width of a string (default height/mode) */
int xglFontGetStringWidth(void *pStr)
{
    return xglFontGetStringWidth2(pStr, 0);
}

/* Print a string, optionally registering it with the OT first */
void xglFontPrintDirectOT(int nOT, char *pStr)
{
    if ((FS.nFlags & 1) != 0) {
        set_ot(nOT);
        xglFontPrintDirectCore(pStr);
    }
}

/* Print a string at a screen position/colour (y, nColor unused in this
 * path -- x is forwarded to set_xyz, pStr to xglFontPrintDirectCore) */
void xglFontPrint(int x, int y, int nColor, char *pStr)
{
    if ((FS.nFlags & 1) != 0) {
        set_xyz(x);
        xglFontPrintDirectCore(pStr);
    }
}

/* Formatted-print entry point (y, nColor unused in this path) */
void xglFontPrintf(int x, int y, int nColor, char *pStr)
{
    if ((FS.nFlags & 1) != 0) {
        set_xyz(x);
        xglFontPrintSub(pStr);
    }
}

/* Flush the font packet for the debug-hex overlay layer */
void xglFontFlushSubHex(int nPage)
{
    xglFontFlushSub(nPage << 7, 0x2F00, 0x608, 19);
}

/* Advance the scratchpad cursor to the next line (double-spaced unless
 * the debug mode byte is set) and return X to the home column */
void xglFontFlushSubCRLF(void)
{
    FSP->nY += FS.nLineHeight;
    FSP->nX = FSP->nHomeX;
    if (FS.nDebugMode == 0) {
        FSP->nY += FS.nLineHeight;
    }
}

/* Select the debug font mode and refresh the two mode-dependent GS
 * environment quadwords (negative = leave unchanged) */
/* TODO: near-miss (17 vs 18 insns). The 0x50000LL (DImode) vs 0x50000
 * (SImode) constants correctly refuse to CSE (two luis, like the
 * original) and the asm memory barrier reproduces the original's lbu
 * re-read of the just-stored mode byte; remaining diff is one `move
 * v0,a0` param copy the original carries (plus scheduling shifts from
 * it) that no local-copy variant reproduced. */
void xglFontDebugMode(int nMode)
{
    if (nMode >= 0) {
        FS.nDebugMode = nMode;
        __asm__ __volatile__("" : : : "memory");
        ModeEnv[4] = ((nMode & 0xFF) + 35) | 0x50000;
        ModeEnv[20] = FS.nDebugMode | 0x50000LL;
    }
}
