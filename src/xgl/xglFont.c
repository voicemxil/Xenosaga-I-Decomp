#include "matching.h"

/* Font rendering / packet-flush subsystem */

typedef unsigned char u_char;
typedef unsigned short u_short;
typedef unsigned int u_int;

typedef struct
{
    int      nLoadAddr;   /* 0x00 */
    int      nTexAddr;    /* 0x04 */
    u_short  nFlags;      /* 0x08 */
    u_char   nBank;       /* 0x0A */
    u_char   nDebugMode;  /* 0x0B */
    u_short  nLineHeight; /* 0x0C */
    u_short  nUnk0E;      /* 0x0E */
    u_char   nPropBase;   /* 0x10 */
    u_char   pad11;       /* 0x11 */
    u_char   nProportional; /* 0x12 */
    u_char   nUnk13;      /* 0x13 */
    char     pad14[0x8];  /* 0x14 */
    u_char  *pStream;     /* 0x1C */
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
extern int xglCdReadFile(char *pName, int nAddr, int nOfs, int nCb);

void xglFontPrintDirectOT(int nOT, char *pStr);
int xglFontGetStringWidth2(void *pStr, int nArg);
void set_ot(int nOT);
void set_xyz();
void xglFontPrintDirectCore(char *pStr);
void xglFontPrintSub(char *pStr);

typedef unsigned int u_int;
typedef struct { u_char a[8]; } SPBITS;
extern u_char D_004DC2A9[];   /* SP-code-8 bit sizes live at -9 from here */

/* TODO: near-miss (44 vs 47, LENGTH). Findings: u_short idx + int
 * intermediate reproduces the addiu -161/andi + gas $at addiu-macro arm
 * shape; $3/$7 passthrough pins put the compare bools in the right regs.
 * Blockers: (1) c must land in a FRESH $v0 while the lo*94 chain also
 * wants $v0 -- pinning c to $2 makes sched1 hoist the chain's sll above
 * the hi srl (chain then allocates $t0), unpinned c folds in-place into
 * $a0; (2) a stray `move $t0,$a1` pClut save appears in every variant;
 * (3) arm 1 splits its 0xC4E2 add (li $a0) whenever idx lands in $a0. */
/* Map a Shift-JIS kanji code to its cache-texture UV origin, optionally
 * returning the CLUT/page word through pClut */
int xglFontGetKanjiClutUV(int nCode, u_int *pClut)
{
    u_int c;
    PIN(u_int lo, "$6");
    PIN(u_int hi, "$4");
    u_short idx;
    int t;
    PIN(int b173, "$3");
    PIN(int b176, "$7");
    u_int u;
    u_int page;

    PASSTHRU(c, nCode & 0xFFFF);
    PASSTHRU(lo, c & 0xFF);
    PASSTHRU(hi, c >> 8);
    PASSTHRU(b173, lo < 173);
    PASSTHRU(b176, lo < 176);
    PASSTHRU(t, hi + lo * 94 - 161);
    idx = t;

    if (b173) {
        idx += 0xC4E2;
    } else if (b176) {
        idx += 0xC30C;
    } else {
        idx += 0xC250;
    }
    u = idx & 0x1F;
    page = (idx >> 5) & 0xFF;
    if (pClut != 0) {
        *pClut = ((page >> 5) << 12) + 4100;
    }
    return ((page & 0x1F) * 3 << 23) + (u * 5 << 6);
}

/* Byte length of a special (control) code sequence: table-driven, with
 * code 8 (bitfield payload) and code 21 (counted payload) computed */
int xglFontGetSPcodeSize(int nCode, u_char *pStr)
{
    static u_char spcode[32] = {
        0, 1, 1, 1, 1, 1, 3, 0, 255, 0, 0, 0, 3, 1, 5, 3,
        5, 6, 2, 2, 0, 255, 0, 7, 1, 1, 0, 1, 0, 0, 0, 0,
    };
    SPBITS bits;
    u_char nSize;
    u_char nBit;
    u_char nBits;
    int i;

    nCode &= 0xFF;
    nSize = spcode[nCode];
    if (nSize == 255) {
        switch (nCode) {
        case 8:
            bits = *(SPBITS *)(D_004DC2A9 - 9);
            nBits = pStr[1];
            nSize = 1;
            for (i = 0; i < 8; i++) {
                nBit = nBits & 1;
                if (nBit != 0) {
                    nSize = nSize + bits.a[i];
                }
                nBits >>= 1;
            }
            break;
        case 21:
            nSize = pStr[1] + 1;
            break;
        }
    }
    return nSize;
}

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

/* Queue an extension-function command (23) with two 24/32-bit payload
 * words into the font stream buffer */
void xglFontPrintExtFunc(int nOT, u_int n1, u_int n2)
{
    u_char *p;

    if ((u_short)(FS.nFlags & 1) != 0) {
        set_ot(nOT);
        p = FS.pStream;
        p[0] = 23;
        p[1] = n1 >> 16;
        p[2] = n1 >> 8;
        p[3] = n1;
        p[4] = n2 >> 24;
        p[5] = n2 >> 16;
        p[6] = n2 >> 8;
        p[7] = n2;
        p[8] = 0;
        FS.pStream += 9;
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
        /* Source order really is [20] then [4]: gcc's scheduler swaps the
         * two sd's back (distinct constant offsets off one base), but the
         * order it materializes the two 0x50000 constants in follows the
         * source, and that is what pins the register assignment. */
        ModeEnv[20] = FS.nDebugMode | 0x50000LL;
        ModeEnv[4] = (FS.nDebugMode + 35) | 0x50000;
    }
}

/* TODO: near-miss (4 words). Structure/args/strings all line up; the
 * residue is addressing-mode CSE: the original loads FS.nLoadAddr via
 * the %hi reg with the %lo folded into the lw (4480($v1)) while ours
 * folds it onto the byte-pair's materialized &FS pseudo (0($v0)).
 * *(int *)&FS and pointer-staging variants did not split them.
 * Wave-3 findings: the built lui is IN-PLACE (lui $v0 / addiu $v0,$v0)
 * because CSE rewrites the lw's address to reuse the materialized &FS,
 * killing the hi reg early; orig keeps hi in $v1 across the calls.
 * Tried and failed: volatile read (address CSE unaffected), asm-renamed
 * alias symbol (extra lui, or sdata path), plain/opaque byte-pointer
 * staging (opaque asm passthrough perturbs the call-setup schedule). */
/* Load one of the two font texture pages, returning the previous bank */
int xglFontLoad(int nBank, int nNow)
{
    int nOld;
    int nOfs;
    int nCb;

    nOld = FS.nBank;
    FS.nBank = nBank;
    nOfs = 1;
    nCb = nNow;
    if (nNow == 0) {
        nOfs = 0;
        nCb = 1;
    }
    if (nBank == 0) {
        xglCdReadFile("data\\font0.tex", *(int *)&FS, nOfs, nCb);
    } else {
        xglCdReadFile("data\\font1.tex", *(int *)&FS, nOfs, nCb);
    }
    return nOld;
}

void WindowTexLoad(int nAddr, int nArg);
void buffer_reset(void);
extern char FontImage[];
extern char WindowImage[];

/* Bring up the font system: set the texture load addresses, force bank 0
 * resident, load the window texture and reset the packet buffer, then
 * install the default metrics */
void xglFontInitial(void)
{
    FS.nBank = -1;
    FS.nLoadAddr = (int)FontImage;
    xglFontLoad(1, 0);
    FS.nTexAddr = (int)WindowImage;
    WindowTexLoad((int)WindowImage, 0);
    buffer_reset();
    FS.nFlags = 9;
    FS.nLineHeight = 8;
    FS.nPropBase = 4;
    FS.nDebugMode = 0;
    FS.nProportional = 0;
    FS.nUnk13 = 0;
    FS.nUnk0E = 0;
}

/* Look up a glyph's packed width pair from the proportional-width table,
 * optionally rebalancing it into the proportional spacing range */
int xglFontGetProportionalSize(int nCode)
{
    u_int nSize;
    PIN(u_int nLow, "$3");
    int n;

    __asm__("" : "=r"(nSize)
        : "0"((*(u_short *)(FS.nLoadAddr + 0x78040 + nCode * 2) + 255) & 0xFFFF));
    PASSTHRU(nLow, nSize & 0xFF);
    if (nLow == 255) {
        nSize = FS.nPropBase << 8;
    }
    if (FS.nProportional != 0) {
        n = (signed char)(((int)((nSize & 0xFF) + (nSize >> 8)) >> 1) - 5);
        if (n < 0) {
            n = 0;
        }
        if (!(n < 11)) {
            n = 10;
        }
        nSize = (((n + 9) << 8) + n) & 0xFFFF;
    }
    return nSize;
}
/* TODO: near-miss (11 words). Logic and the 8-byte command layout are
 * right; ours converts the nDebugMode test to bnezl with the
 * FS.pStream lw hoisted into the annulled slot, the original keeps
 * plain bnez + nop and loads after the join. A memory barrier after
 * the if regresses (blocks the +=8 reload CSE). */
/* Queue a debug hex-print command (11) into the font stream buffer */
void xglFontDebugHex(int nX, int nY, unsigned int nValue, int nDigits)
{
    u_char *p;

    if ((FS.nFlags & 3) != 3) {
        return;
    }
    if (nDigits == 0) {
        return;
    }
    if (FS.nDebugMode == 0) {
        set_xyz(nX << 1, nY << 1, -1);
    }
    p = FS.pStream;
    p[0] = 11;
    p[1] = 16;
    p[2] = nDigits - 1;
    p[3] = nValue >> 24;
    p[4] = nValue >> 16;
    p[5] = nValue >> 8;
    p[6] = nValue;
    p[7] = 0;
    FS.pStream += 8;
}

int hex2val(int nChar);
extern u_char D_004908D6[];

/* TODO: near-miss (16 words). Single-return-variable shape with the
 * ##-block as the == fallthrough and a shared *ppStr/return tail
 * reproduces the whole branch layout (the ## path's b lands on the hex
 * path's cursor-store delay slot, as in the original). Residue: the
 * original materializes 94 twice (li $3 AND a fresh li $2 at the divu,
 * check beqzl on $2) where gcc CSEs one li ($3) -- a $2-pinned
 * passthrough divisor splits the divu into two, unpinned keeps one li
 * plus a stray li $4; downstream mflo/mfhi register roles follow. */
int xglFontAscii2Euc(char nChar, u_char **ppStr)
{
    u_char *pStr;
    int nHigh;
    int nCode;
    char nNext;
    /* nRet is unsigned so the /94 and %94 come out as divu, and it
     * doubles as the scratch the original divided: the 0xFFFF mask, the
     * remainder and the result all live in nRet's register ($a0). */
    u_int nRet;

    nRet = 0xA1A1;
    if (!(nChar < 32)) {
        if (nChar == '#') {
            if (ppStr != 0) {
                pStr = *ppStr;
                nNext = pStr[1];
                if (nNext == nChar) {
                    pStr += 1;
                    nRet = *(u_short *)D_004908D6;
                } else {
                    nHigh = hex2val(nNext);
                    nCode = hex2val((char)pStr[2]);
                    pStr += 2;
                    nRet = (((nHigh << 4) + nCode) + 32) & 0xFFFF;
                    nCode = nRet % 94;
                    nRet = ((((nRet / 94) & 0xFF) << 8) + nCode - 12127) & 0xFFFF;
                }
                *ppStr = pStr;
                return nRet;
            }
        }
        nRet = *(u_short *)(D_004908D6 - 70 + nChar * 2);
    }
    return nRet;
}
