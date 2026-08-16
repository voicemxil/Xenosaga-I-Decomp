#include "matching.h"

/* Font rendering / packet-flush subsystem */

typedef unsigned char u_char;
typedef unsigned short u_short;
typedef unsigned int u_int;

typedef struct
{
    u_short  nUnk00;
    u_short  nTail;
} FSOT;

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
    FSOT     aOT[16];     /* 0x20 */
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
/* TODO: near-miss (39/57 words, 228 vs 232 bytes). Control flow, the
 * lb-then-lbu double load and every loop body now line up; the whole
 * residue is HOW the FS.pStream address is materialised. The original
 * builds it three times as a *folded* symbol address -- `addiu sN,
 * %hi-reg, %lo(FS+28)` then `lw 0(sN)` -- once hoisted for the outer
 * store, once again for the inner loop (off a second %hi copy in $s2),
 * and a third time at the tail as `&FS` + a separate `addiu 28`. Ours
 * hoists plain `&FS` and leaves the 28 in the load/store offset
 * (`lw 28($s1)`), which is one instruction shorter and costs the two
 * extra materialisations.
 * Swept (24 body variants x entry/inner/outer/tail shapes, sweep.py):
 * writing the loops through a local `u_char **pp = &FS.pStream` DOES
 * fold the offset exactly as the original does, but gcc's GCSE then
 * collapses all three sites onto one pseudo (196 bytes, 48 diffs) even
 * when pp is reassigned at each site or scoped per block; three
 * LAUNDER(pp)s reproduce the three materialisations and land at 228
 * bytes / 44 diffs, i.e. no better than the plain-C shape below, so the
 * steering is not carrying its weight. What is missing is whatever made
 * the original compiler treat `&FS.pStream` as a symbol+offset value in
 * the loops and as `&FS`-plus-28 at the tail in the SAME function --
 * the sibling matched functions (xglFontDebugHex, xglFontPrintExtFunc)
 * all use the plain `lw 28($base)` form, so this shape is specific to
 * this TU position. Not re-swept: entry/loop-rotation shapes. */
void xglFontPrintDirectCore(char *pStr)
{
    u_char *pSrc;
    u_char *pDst;
    u_char c;
    int n;

    pSrc = (u_char *)pStr;
    c = *pSrc;
    if (*(char *)pSrc != 0) {
        do {
            if (*pSrc < 32) {
                n = xglFontGetSPcodeSize(*pSrc, pSrc);
                while (n > 0) {
                    n--;
                    pDst = FS.pStream;
                    *pDst = *pSrc;
                    pSrc++;
                    FS.pStream = pDst + 1;
                }
                c = *pSrc;
            }
            pSrc++;
            pDst = FS.pStream;
            *pDst = c;
            FS.pStream = pDst + 1;
            c = *pSrc;
        } while (c != 0);
    }
    pDst = FS.pStream;
    *pDst = 0;
    FS.pStream = pDst + 1;
}

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

/* TODO: near-miss (4 words, OPERANDS). Diagnosed exactly: the pass is
 * gcc 2.96's CSE running with -fcse-skip-blocks (on at -O2). CSE records
 * the byte pair's materialized &FS pseudo as equivalent to (symbol FS),
 * then extends its path into BOTH arms of the `if (nBank == 0)` and
 * rewrites each `lw %lo(FS)($hi)` to `lw 0($&FS)`, which kills the %hi
 * register and collapses lui/addiu into one register.
 *   PROOF: compiling this file with -fno-cse-skip-blocks emits the
 *   original's exact shape -- lui $3,%hi(FS) / addiu $2,$3,%lo(FS) /
 *   lbu 10($2) / lw $5,%lo(FS)($3) twice -- and xglFontLoad matches.
 *   It is not usable as a per-file flag: it costs xglFontDebugHex and
 *   xglFontGetProportionalSize, which match with the flag off.
 *   -fno-gcse/-fno-rerun-cse-after-loop/-fno-thread-jumps/-fno-regmove/
 *   -fno-force-mem/-fno-caller-saves/-fno-expensive-optimizations/
 *   -fno-strict-aliasing/-fno-cse-follow-jumps all leave it unchanged;
 *   only cse-skip-blocks moves it.
 * C shapes swept and rejected (all still emit lw 0($v0)): named field
 * vs *(int *)&FS vs ((int *)&FS)[0]; volatile read of the word;
 * volatile struct pointer; LAUNDER/LAUNDER_V of the byte-base pointer;
 * hoisting the word read into a temp (collapses to one lw); early-return
 * arms; inverted test; switch; ternary (splits, but leaves one call);
 * goto arms; do/while(0); two separate ifs; extra work in each arm;
 * memory-clobber barrier before or inside the arms; byte base as
 * (u_char*)&FS+10 (moves both to %hi(FS+10)).
 * CLOSEST NON-MATCH: `extern int FSw[] __asm__("FS");` used for the word
 * read does produce lw %lo(FS)($2) in both arms -- but as a second
 * SYMBOL_REF it also emits a second lui, so 29 words instead of 28.
 * What is still needed is one symbol whose (high FS) is shared while the
 * (lo_sum) is NOT substituted -- i.e. the word read must reach CSE with
 * an empty table while the %hi stays available. */
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

    PASSTHRU(nSize,
             (*(u_short *)(FS.nLoadAddr + 0x78040 + nCode * 2) + 255) & 0xFFFF);
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
typedef __builtin_va_list va_list;
#define va_start(ap, last) __builtin_stdarg_start(ap, last)
#define va_arg(ap, type) __builtin_va_arg(ap, type)

/* Queue a printf-style debug command (11) into the font stream buffer.
 * The whole caller argument list is snapshotted into a 32-doubleword
 * scratch -- element 0 is the format word, the rest is the raw varargs
 * area -- because xglFontPrintSub consumes it after this frame is gone.
 * Going through `pp = &FS.pStream` rather than naming FS.pStream twice
 * is what makes gcc keep the member address live in a register across
 * the load/store pair, as the original does. */
void xglFontDebugPrintf(int nX, int nY, u_int nFmt, ...)
{
    va_list ap;
    long long aBuf[32];
    long long *pDst;
    u_char *p;
    u_char **pp;
    int i;

    if ((FS.nFlags & 3) != 3) {
        return;
    }
    aBuf[0] = nFmt;
    va_start(ap, nFmt);
    pDst = &aBuf[1];
    for (i = 30; i >= 0; i--) {
        *pDst = va_arg(ap, long long);
        pDst++;
    }
    if (FS.nDebugMode == 0) {
        nX <<= 1;
        nY <<= 1;
    }
    set_xyz(nX, nY, -1);
    pp = &FS.pStream;
    p = *pp;
    *p = 11;
    *pp = p + 1;
    xglFontPrintSub((char *)aBuf);
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
    /* set_xyz is called either way -- only the coordinate doubling is
     * conditional (the original branches past the two slls, not past
     * the call). */
    if (FS.nDebugMode == 0) {
        nX <<= 1;
        nY <<= 1;
    }
    set_xyz(nX, nY, -1);
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

/* Reset the 16 ordering-table buckets to empty (each bucket's tail link
 * points at itself) and rewind the packet stream to just past them */
void buffer_reset(void)
{
    FSOT *p;
    int i;

    p = FS.aOT;
    for (i = 0; i < 16; i++) {
        p->nTail = (u_short)((u_char *)p - (u_char *)FS.aOT);
        p->nUnk00 = 0;
        p++;
    }
    FS.pStream = (u_char *)p;
    FS.pad11 = 0;
}

/* Append the current stream position to the ordering-table bucket picked
 * by the top nibble of nZ, then reserve a two-byte link cell */
void set_ot(int nZ)
{
    FSOT *p;
    u_short nOfs;
    u_char *q;

    p = (FSOT *)((u_char *)FS.aOT + ((nZ >> 22) & 0x3C));
    nOfs = (u_short)((u_int)FS.pStream - (u_int)FS.aOT);
    q = (u_char *)FS.aOT + p->nTail;
    q[0] = (u_char)nOfs;
    q[1] = (u_char)(nOfs >> 8);
    p->nTail = nOfs;
    FS.pStream[0] = 0;
    FS.pStream[1] = 0;
    FS.pStream += 2;
}

/* Emit the position/colour command for the next string.  Declared K&R so
 * the one-argument call sites in xglFontPrint/xglFontPrintf still compile
 * -- the original has no prototype for this function either. */
void set_xyz(nX, nY, nZ)
int nX;
int nY;
int nZ;
{
    set_ot(nZ);
    if (nX < 0 || nX >= 768 || nY < 0 || nY >= 512) {
        u_char *p = FS.pStream;
        p[0] = 8;
        p[1] = 7;
        p[2] = nX >> 8;
        p[3] = nX;
        p[4] = nY >> 8;
        p[5] = nY;
        p[6] = nZ >> 16;
        p[7] = nZ >> 8;
        p[8] = nZ;
        FS.pStream += 9;
    } else {
        u_char *p = FS.pStream;
        p[0] = (nX >> 8) + 1;
        p[1] = nX;
        p[2] = (nY >> 8) + 4;
        p[3] = nY;
        p[4] = 6;
        p[5] = nZ >> 16;
        p[6] = nZ >> 8;
        p[7] = nZ;
        FS.pStream += 8;
    }
}

/* One hex digit to its value; anything else is 0 */
int hex2val(int nChar)
{
    char c = nChar;
    u_char n;

    n = c - '0';
    if (n < 10) {
        return n;
    }
    if (c >= 'a' && c <= 'f') {
        return (u_char)(c - 'a' + 10);
    }
    if (c >= 'A' && c <= 'F') {
        return (u_char)(c - 'A' + 10);
    }
    return 0;
}

/* TODO: near-miss (46 words vs 44). All the arithmetic is right; the
 * residue is that sched2 sinks the `pClut != 0` branch to the very end
 * and computes both the CLUT value and the return value above it, where
 * the original branches first and computes the v half in the merge
 * block. Consequences: an extra `move a3,a1` (pClut spilled out of $a1
 * because $a1 becomes an arithmetic temp) and one extra word.
 * Levers applied to get here: nIdx built with a separate `nIdx &= 0xFFFF`
 * statement per step (a `(u_short)` cast on the whole expression turns
 * `addiu -161` into `li 0xff5f` + `addu`), and `u_char nRow` -- with
 * `u_int nRow = (nIdx >> 5) & 0xFF`, combine folds the later `& 0x1F`
 * into the same andi and the 0xff mask disappears, because in the
 * original those two masks sit in different basic blocks.
 * Swept: nU inlined into the return, nU accumulated, LAUNDER(nRow). */
/* Map a Shift-JIS style kanji code to its (u,v) position in the font
 * texture, and optionally to the CLUT address that goes with its row */
/* TODO: near-miss.  As written it is 46 words against retail's 44: gcc
 * spills pClut into $a3 (`move $a3,$a1`) because the two `sltiu` range
 * flags take $a1/$a2, and the extra word drags an alignment nop in too.
 * Wave-5 sweep: a PIN'd `$a1` copy of pClut removes the spill and gets
 * the length right (44 words, 34 diffs); adding PIN(nLo,"$6") takes it
 * to 28; splitting the column term into its own statement so it is
 * finished before the pClut test takes it to 21.  What is left after
 * that is (a) the whole nIdx chain naming $a0 where retail names $v0 --
 * PIN(nIdx,"$2") costs four words back -- and (b) retail testing pClut
 * BEFORE the `(nRow & 0x1F) * 3` term while gcc sinks the branch past
 * it however the source is ordered.  Not registered, and the PINs are
 * left out: three of them for 176 bytes is worse steering than the
 * miss.  Also swept with no effect: u_char nLo, hi-before-lo, a local
 * copy of nCode, nested ifs, and the reversed >= comparison chain. */
u_int xglFontGetKanjiClutUV(u_short nCode, u_int *pClut)
{
    u_int nLo;
    u_int nHi;
    u_int nIdx;
    u_int nCol;
    u_char nRow;

    nLo = nCode & 0xFF;
    nHi = nCode >> 8;
    nIdx = nHi + nLo * 94 - 161;
    nIdx &= 0xFFFF;
    if (nLo < 0xAD) {
        nIdx = 0xC4E2 + nIdx;
        nIdx &= 0xFFFF;
    } else if (nLo < 0xB0) {
        nIdx = 0xC30C + nIdx;
        nIdx &= 0xFFFF;
    } else {
        nIdx = 0xC250 + nIdx;
        nIdx &= 0xFFFF;
    }
    nCol = nIdx & 0x1F;
    nRow = nIdx >> 5;
    if (pClut != 0) {
        *pClut = ((nRow >> 5) << 12) + 4100;
    }
    return (((nRow & 0x1F) * 3) << 23) + ((nCol * 5) << 6);
}

typedef unsigned long u_long;

void xglFontFlushCore(void *pPk, u_char *pData);
void xglFontReloadTexture(void *pPk, u_int nArg);
void *xglPacketGetCurrent(void);
void sceVif1PkAddDirectDataN(void *pPk, u_int nAddr, int nQwc);
void sceVif1PkCloseDirectHLCode(void *pPk);

/* TODO: near-miss (34 words, 123 built vs 125). Levers that got here:
 * the scratchpad draw-state fields really are PLAIN literal casts
 * (`*(char *)0x70000021 = 1;`) -- the `lui at` absolute-MEM expansion in
 * the second block IS what the original has, unlike the volatile
 * pointer form the DMA packet builders need -- but the FIRST four
 * stores go through a `char *pDraw` so they share one base register the
 * way the original does; the three 127 stores in DESCENDING offset
 * order (0x16, 0x15, 0x14).
 * Residue, both worth ~2 words each: (a) gcc CSEs the constant
 * 0x70000000 of the three `*(void **)0x70000000` packet-pointer reads
 * into a callee-saved register, where the original re-materialises
 * `lui a0,0x7000` at each of the three sites -- that alone is the whole
 * length difference; (b) the two function-static tables get addressed
 * as one symbol + 96 instead of two %hi/%lo pairs (the same folding
 * xglCameraTravelInit hit).
 * Swept: volatile-qualified packet-pointer reads (35), both byte orders
 * for the link `(pBase[1] << 8) + pBase[0]` (no change), ascending
 * order for the three 127 stores (36). */
/* Flush every ordering-table bucket into the current VIF packet: reset
 * the scratchpad draw state, reload the font texture, walk the 16
 * buckets' link chains, and in debug mode append the z-buffer and test
 * register blocks before closing the packet */
void xglFontFlush(void)
{
    static u_long zbuf[12] = {
        0x1000000000008005UL, 0xEUL,
        0x0000720000007000UL, 0x18UL,
        0x01BF000001FF0000UL, 0x40UL,
        0x0000000031000000UL, 0x4EUL,
        0x0000000000070000UL, 0x47UL,
        0x0000000000080070UL, 0x4CUL,
    };
    static u_long test[4] = {
        0x1000000000008001UL, 0xEUL,
        0x0UL, 0x3FUL,
    };
    char *pDraw;
    u_short *pOT;
    u_char *pBase;
    u_int nOfs;
    int i;

    pDraw = (char *)0x70000000;
    *FS.pStream = 0;
    *(int *)(pDraw + 0x0C) = -1;
    pDraw[0x16] = 127;
    pDraw[0x15] = 127;
    pDraw[0x14] = 127;
    if (FS.nDebugMode != 0) {
        *(short *)0x70000010 = 0;
        *(short *)0x70000012 = 0;
    } else {
        *(short *)0x70000010 = 1792;
        *(short *)0x70000012 = 1824;
    }
    *(char *)0x70000018 = 0;
    *(char *)0x70000019 = 1;
    *(char *)0x7000001A = -1;
    *(char *)0x7000001B = 0;
    *(char *)0x70000020 = 1;
    *(char *)0x70000021 = 1;
    *(char *)0x70000024 = 0;
    *(char *)0x70000025 = 0;
    *(char *)0x70000026 = 0;
    *(char *)0x70000028 = 68;
    *(char *)0x70000029 = 0;
    *(char *)0x7000002A = -128;
    *(short *)0x7000002C = 16;
    *(short *)0x7000002E = 16;
    *(void **)0x70000000 = xglPacketGetCurrent();
    xglFontReloadTexture((void *)0x70000000, 0x80000001);
    pOT = (u_short *)FS.aOT;
    for (i = 15; i >= 0; i--) {
        nOfs = *pOT;
        while (nOfs != 0) {
            pBase = (u_char *)FS.aOT + nOfs;
            xglFontFlushCore((void *)0x70000000, pBase + 2);
            nOfs = (pBase[0] + (pBase[1] << 8)) & 0xFFFF;
        }
        pOT += 2;
    }
    if (FS.nDebugMode != 0) {
        sceVif1PkAddDirectDataN(*(void **)0x70000000, (u_int)zbuf, 6);
        sceVif1PkAddDirectDataN(*(void **)0x70000000, (u_int)test, 2);
    }
    sceVif1PkCloseDirectHLCode(*(void **)0x70000000);
    buffer_reset();
}
