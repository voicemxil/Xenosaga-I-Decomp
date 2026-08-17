/*
 * Font: the debug font-test screens (FontTest*) and the runtime font
 * texture uploader (FontTex*).
 *
 * The FontTest* screens are the developer's glyph browser: they reload a
 * font bank, walk the EUC code space and draw every glyph, with the DEBUG
 * pad (PadData[0]) selecting between three pages.  Nothing here runs in
 * the shipped game, but the packets they build document the font
 * subsystem's GS transfer layout, which is why they are worth keeping.
 */

#include "common.h"

/* --- sceVif1Pk packet builder (src/sdk/sceVif1Pk.c) --- */
extern void sceVif1PkAddDirectDataN(void *pPk, void *pData, u_int nQwc);
extern void sceVif1PkOpenDirectHLCode(void *pPk, u_int nFlags);
extern void sceVif1PkCloseDirectHLCode(void *pPk);
extern void sceVif1PkCnt(void *pPk, u_int nFlags);
extern void sceVif1PkAddDataN(void *pPk, void *pData, u_int nWords);
extern void sceVif1PkRef(void *pPk, void *pData, u_int nQwc,
                         int nUnk3, int nUnk4, int nUnk5);

/* --- xgl font/render entry points --- */
extern void xglFontReloadTexture(void *pTex, int nBank);
extern void nmlModelDirectSend(int nOT, void *pPacket, int nQwc);

/*
 * FontTex: the per-bank font texture uploader.  pPk is the VIF1 packet
 * builder the caller is currently filling; aFlush is a scratch GIF packet
 * embedded in the object so the uploader never has to allocate.
 */
typedef struct {
    void *pPk;              /* 0x00 */
    char pad04[0x2C];
    u_long aFlush[4];       /* 0x30: GIFtag + A+D write of TEXFLUSH */
} FONTTEX;

/* One GS-side glyph page: where it lives in GS memory and where its
 * texels live in the source image.  Stride 20 bytes. */
typedef struct {
    u_int nSize;            /* 0x00: low half = TRXREG width, high half = BITBLTBUF DBW */
    u_int nHeight;          /* 0x04: TRXREG height */
    u_int nGsAddr;          /* 0x08: GS byte address, biased by 0xF0000 */
    u_int nQwc;             /* 0x0C: quadwords of texel data */
    u_int nDataOfs;         /* 0x10: texel data, relative to the image base */
} FONTTEXPAGE;

typedef struct {
    char pad00[8];
    int nPageCount;         /* 0x08 */
    int nPageOfs;           /* 0x0C: FONTTEXPAGE[] relative to this header */
} FONTTEXIMAGE;

/* GIFtag (NLOOP=0, EOP, NREG=1, A+D) followed by one A+D register write. */
#define FONT_GIFTAG_AD_EOP 0x1000000000008000
#define GS_A_D             14
#define GS_TEXFLUSH        0x3F

/* Flush the GS texture cache through the caller's packet, then re-upload
 * the currently selected font bank. */
void FontTexReload(FONTTEX *pTex)
{
    u_long *pGif = pTex->aFlush;

    pGif[0] = FONT_GIFTAG_AD_EOP;
    pGif[1] = GS_A_D;
    pGif[2] = 0;
    pGif[3] = GS_TEXFLUSH;
    sceVif1PkAddDirectDataN(pTex->pPk, pGif, 1);
    xglFontReloadTexture(pTex, 2);
}

/* GS local-to-local transfer environment: four VIF NOPs then a DIRECTHL
 * of five quadwords -- a GIFtag plus A+D writes of BITBLTBUF, TRXPOS,
 * TRXREG and TRXDIR.  Only the two data words the page varies get
 * rewritten per page. */
static u_long Env[12] = {
    0x5100000500000000, 0x1000000000000004, GS_A_D,
    0x50, 0,                /* BITBLTBUF: rewritten per page */
    0x51, 0,                /* TRXPOS */
    0x52, 0,                /* TRXREG: rewritten per page */
    0x53, 0, 0
};

/* Upload every page of a font image to its GS address.  The caller's
 * direct-HL code block is closed around the transfer because the pages go
 * out as PATH2 DMA references rather than inline data. */
void FontTexChange(FONTTEX *pTex, FONTTEXIMAGE **ppImage)
{
    u_long *pGif;
    FONTTEXIMAGE *pImage;
    FONTTEXPAGE *pPage;
    int i;

    pGif = pTex->aFlush;
    pGif[0] = FONT_GIFTAG_AD_EOP;
    pGif[1] = GS_A_D;
    pGif[2] = 0;
    pGif[3] = GS_TEXFLUSH;
    sceVif1PkAddDirectDataN(pTex->pPk, pGif, 1);
    sceVif1PkCloseDirectHLCode(pTex->pPk);

    pImage = *ppImage;
    pPage = (FONTTEXPAGE *)((char *)pImage + pImage->nPageOfs);
    for (i = 0; i < pImage->nPageCount; i++) {
        u_int nSize = pPage->nSize;
        u_int nBufWidth = nSize >> 16;
        u_int nWidth = nSize & 0xFFFF;

        /* Both halves of nSize must reach the shifts through one register:
         * given the expression inline, 2.96's combine narrows the high half
         * to its own `lhu` at +2 and emits two loads. */
        Env[4] = ((u_long)((pPage->nGsAddr + 0xF0000) >> 6) << 32)
               | ((u_long)nBufWidth << 48);
        Env[8] = (u_long)nWidth | ((u_long)pPage->nHeight << 32);
        sceVif1PkCnt(pTex->pPk, 0);
        sceVif1PkAddDataN(pTex->pPk, Env, 24);
        sceVif1PkRef(pTex->pPk, (char *)pImage + pPage->nDataOfs,
                     pPage->nQwc, 0, 0, 0);
        pPage++;
    }
    sceVif1PkCnt(pTex->pPk, 0);
    sceVif1PkOpenDirectHLCode(pTex->pPk, 0);
}

/* GS packet drawing one flat line: VIF DIRECTHL of a GIFtag (PRIM=LINE,
 * A+D/RGBAQ/XYZ2/XYZ2), the TEST_1 setup, the colour and the two
 * endpoints.  FontTestLine only rewrites the two XY pairs. */
static u_int TestEnv[24] = {
    0, 0, 0, 0x51000005,
    0x00008001, 0x40014000, 0x0000551E, 0,
    0x00070000, 0, 0x47, 0,        /* A+D: TEST_1 */
    0xC0, 0x80, 0x80, 0x80,        /* RGBAQ */
    0, 0, 0xFFFFFFFF, 0,           /* XYZ2 (start) */
    0, 0, 0xFFFFFFFF, 0            /* XYZ2 (end) */
};

/* Draw a horizontal rule nW characters wide at screen cell (nX, nY). */
void FontTestLine(int nX, int nY, int nW)
{
    int nSx = (nX << 4) + 28664;
    int nSy = (nY << 4) + 29175;

    TestEnv[16] = nSx;
    TestEnv[17] = nSy;
    TestEnv[20] = nSx + (nW << 4);
    TestEnv[21] = nSy;
    nmlModelDirectSend(1, TestEnv, 6);
}

/* --- Debug font browser --------------------------------------------- */

/* The pad view this TU uses.  The exit check reads the held/trigger words
 * as one 64-bit quantity, which is why nUnk28 exists as a named anchor. */
typedef struct {
    char pad00[0x28];
    union {
        u_long nHoldPair;   /* 0x28: FontTest's exit check reads all four
                             * button words as one doubleword */
        struct {
            u_short nUnk28; /* 0x28 */
            u_short nHold;  /* 0x2A */
            u_short nTrig;  /* 0x2C */
            u_short nUnk2E; /* 0x2E */
        } b;
    } btn;
    char pad30[0x68 - 0x30];
} FONTPAD;

extern FONTPAD PadData[2];

extern int xglFontLoad(int nBank, int nNow);
extern void xglFontDebugPrintf(int nX, int nY, char *pFmt, ...);
extern void xglFontDebugHex(int nX, int nY, u_int nValue, int nDigits);
extern void xglFontPrint(int nX, int nY, int nColor, char *pStr);
extern void xglFontPrintDirect(char *pStr);
extern int xglFontGetStringWidth(char *pStr);
extern int xglFontGetProportionalSize(int nCode);
extern void FontTestSub(void);
extern void FontTestP0(void);
extern void FontTestP2(void);
extern void FlushCache(int nMode);
extern int xglCdReadFile(char *pName, void *pAddr, int nOfs, void *pCb);
extern int xglCdGetFileSize(char *pName);
extern int sceOpen(char *pName, int nFlags);
extern int sceWrite(int fd, void *pBuf, int nLen);
extern int sceClose(int fd);
extern void GameSnapShotNumber(int nNumber);
extern void GameSnapShotCheck(void);
extern void xglSleep(void);
extern int printf(const char *pFmt, ...);

/* SELECT reloads the kanji bank, START the ASCII bank, and L2 dumps the
 * EUC conversion table to host0:. Shared by every page. */

/* Page 1: the raw 8-bit code page, sixteen codes to a row. */
void FontTestP1(void)
{
    static char test00[] = "\x19\x02#00#01#02#03#04#05#06#07#08#09#0a#0b#0c#0d#0e#0f";
    static char test01[] = "\x19\x02#10#11#12#13#14#15#16#17#18#19#1a#1b#1c#1d#1e#1f";
    static char test02[] = "\x19\x02#20#21#22#23#24#25#26#27#28#29#2a#2b#2c#2d#2e#2f";
    static char test03[] = "\x19\x02#30#31#32#33#34#35#36#37#38#39#3a#3b#3c#3d#3e#3f";
    static char test04[] = "\x19\x02#40#41#42#43#44#45#46#47#48#49#4a#4b#4c#4d#4e#4f";
    static char test05[] = "\x19\x02#50#51#52#53#54#55#56#57#58#59#5a#5b#5c#5d#5e#5f";
    static char test06[] = "\x19\x02#60#61#62#63#64#65#66#67#68#69#6a#6b#6c#6d#6e#6f";
    static char test07[] = "\x19\x02#70#71#72#73#74#75#76#77#78#79#7a#7b#7c#7d#7e#7f###00";

    if (PadData[0].btn.b.nHold & 0x20) {
        xglFontLoad(0, 0);
    }
    if (PadData[0].btn.b.nHold & 0x40) {
        xglFontLoad(1, 0);
    }
    if (PadData[0].btn.b.nHold & 0x10) {
        FontTestSub();
    }
    xglFontDebugPrintf(16, 16, test00);
    xglFontDebugPrintf(16, 28, test01);
    xglFontDebugPrintf(16, 40, test02);
    xglFontDebugPrintf(16, 52, test03);
    xglFontDebugPrintf(16, 64, test04);
    xglFontDebugPrintf(16, 76, test05);
    xglFontDebugPrintf(16, 88, test06);
    xglFontDebugPrintf(16, 100, test07);
}

/* Debug font browser main loop. Up/down on pad 1 wrap through the three
 * pages; L1+? on pad 0 quits. */
void FontTest(void)
{
    int nPage;

    nPage = 0;
    GameSnapShotNumber(1);
    while ((PadData[0].btn.nHoldPair & 0x08000100) != 0x08000100) {
        u_short nTrig = PadData[0].btn.b.nHold;

        if (nTrig & 0x2000) {
            nPage++;
        }
        if (nTrig & 0x8000) {
            nPage--;
        }
        switch (nPage) {
        default:
            nPage = 0;
        case 0:
            FontTestP0();
            break;
        case 1:
            FontTestP1();
            break;
        case -1:
            nPage = 2;
        case 2:
            FontTestP2();
            break;
        }
        GameSnapShotCheck();
        xglSleep();
    }
}

/* TODO: near-miss (74 diffs, 111 built vs 121 orig words). The control
 * flow, the 0xD1 lead-byte threshold, the append shape and the closing
 * length walk are all verified against the original. Two things are still
 * missing and they are the same thing twice: the original keeps the
 * table's first byte zero-extended in a QImode register and re-extends it
 * with `andi $x,$y,0xff` at each use, which we cannot reproduce -- 2.96's
 * PROMOTE_MODE puts the byte straight into an SImode pseudo via lbu, so no
 * andi is ever emitted. Every u_char/char/cast shaping tried gives the
 * SImode form. Without the andi the exit branch's delay slot is filled
 * with it rather than with the append's first store, and the gas r5900
 * short-loop nop padding then differs by a few nops. */
/* The EUC glyph-usage collector: read the reference EUC table into the
 * scratch area, walk the scenario text, and append every two-byte code
 * that is not already listed.  The result is written back to the host so
 * the font ROM can be rebuilt with only the glyphs the script uses. */
#define FONT_TEST_TABLE ((u_char *)0x01000000)
#define FONT_TEST_TEXT  ((u_char *)0x01000100)

void FontTestSub(void)
{
    u_char *pSrc;
    u_char *pEnt;
    u_char nTop;
    int nSize;
    int nFd;

    if (xglCdReadFile("data\\yajima\\nisui.euc", FONT_TEST_TABLE, 0, 0) <= 0) {
        return;
    }
    pSrc = FONT_TEST_TEXT;
    nSize = xglCdGetFileSize("data\\yajima\\scenario.txt");
    xglCdReadFile("data\\yajima\\scenario.txt", pSrc, 0, 0);
    FONT_TEST_TEXT[nSize] = 0;
    FONT_TEST_TEXT[nSize + 1] = 0;

    /* nTop mirrors pTbl[0].  The table's first code can only change when a
     * new code is appended to an *empty* table, so caching it lets both the
     * scan and the closing length walk skip re-reading it.  This is the
     * hoist the original build has; 2.96 will not derive it (its gcse does
     * no load motion), so it is written out. */
    nTop = *FONT_TEST_TABLE;
    while (*pSrc != 0) {
        u_int nHi;
        u_int nLo;
        u_char nEnt;

        nHi = *pSrc++;
        if (nHi == '\n') {
            continue;
        }
        nLo = *pSrc++;
        if (nHi < 0xD1) {
            continue;
        }
        pEnt = FONT_TEST_TABLE;
        nEnt = nTop;
        while (nEnt != 0) {
            if (nEnt == nHi && pEnt[1] == nLo) {
                break;
            }
            pEnt += 2;
            nEnt = *pEnt;
        }
        if (nEnt == 0) {
            pEnt[0] = nHi;
            pEnt[1] = nLo;
            pEnt[3] = 0;
            pEnt[2] = 0;
            nTop = *FONT_TEST_TABLE;
        }
    }
    pEnt = FONT_TEST_TABLE;
    if (nTop != 0) {
        do {
            pEnt++;
        } while (*pEnt != 0);
    }
    nFd = sceOpen("host0:nisui.txt", 1538);
    sceWrite(nFd, FONT_TEST_TABLE, pEnt - FONT_TEST_TABLE);
    sceClose(nFd);
}

/* Page 0: the kanji/proportional sample sheet.  The two rate/dir pairs
 * are triangle-wave animators -- one drives the colour ramp of the header
 * bar, the other the plotted dot pattern. */
void FontTestP0(void)
{
    static char test[] = "\017\000\000\000\015\002\016\004\004\000\000\000\245\242\245\353\245\325\245\241\013";
    static int rate = 128;
    static int dir = -1;

    if (PadData[0].btn.b.nHold & 0x20) {
        xglFontLoad(0, 0);
    }
    if (PadData[0].btn.b.nHold & 0x40) {
        xglFontLoad(1, 0);
    }
    if (PadData[0].btn.b.nHold & 0x10) {
        FontTestSub();
    }
    xglFontDebugPrintf(16, 16, "\015\003\022\014\014\275\314\276\256\312\270\273\372\244\316\245\306\245\271\245\310\244\303\244\271\022  \263\310\302\347\022\014\014\312\356\273\247\313\342\313\241\315\362\306\256\022\012\012\312\356\273\247\313\342\313\241\315\362\306\256\012\262\376\271\324\245\306\245\271\245\310");
    xglFontDebugPrintf(16, 32, "\015\002\016\004\004");
    xglFontDebugPrintf(16, 48, "\015\001\245\306\245\271\245\310\241\312\243\261\241\313");

    rate += dir;
    if (rate <= 0) {
        dir = 1;
    }
    if (rate >= 128) {
        dir = -1;
    }
    test[1] = 68;
    test[2] = 0;
    test[3] = rate;
    FlushCache(0);
    xglFontDebugPrintf(16, 64, test);
    xglFontDebugHex(0, 64, rate, 2);

    {
        static char test[] = "\015\003\016\002\002\000\000\000\261\357\244\311\244\352\013";

        xglFontDebugPrintf(48, 80, test);
    }
    {
        static char test[] = "\015\003\016\001\001\000\000\000\261\357\244\311\244\352\013";
        static char test1[] = "\015\003\030\001\245\353\245\323\244\316\245\306\245\271\245\310\245\364\243\301\243\302\243\303\243\260\243\261\243\262\030";
        static char test2[] = "\015\003\264\301\273\372\244\310\244\316\302\320\310\346";

        xglFontDebugPrintf(16, 80, test);
        xglFontDebugPrintf(20, 96, test1);
        xglFontDebugPrintf(16, 99, test2);
    }
    {
        static int rate = 128;
        static int dir = -1;
        static char test[] = "\015\000\016\000\000\000\000\000\017D\000\200";
        static char txt[] = "\001\200\005\000\312\270\273\372\245\306\245\271\245\310";
        static u_char xy3[] = {
            5, 0,
            1, 0, 0, 1, 1, 1, 2, 1, 1, 2
        };
        static u_char xy5[] = {
            13, 0,
            2, 0, 1, 1, 2, 1, 3, 1, 0, 2, 1, 2,
            2, 2, 3, 2, 4, 2, 1, 3, 2, 3, 3, 3,
            2, 4
        };
        static u_char xy7[] = {
            25, 0,
            3, 0, 2, 1, 3, 1, 4, 1, 1, 2, 2, 2,
            3, 2, 4, 2, 5, 2, 0, 3, 1, 3, 2, 3,
            3, 3, 4, 3, 5, 3, 6, 3, 1, 4, 2, 4,
            3, 4, 4, 4, 5, 4, 2, 5, 3, 5, 4, 5,
            3, 6
        };
        static u_char xy9[] = {
            41, 0,
            4, 0, 3, 1, 4, 1, 5, 1, 2, 2, 3, 2,
            4, 2, 5, 2, 6, 2, 1, 3, 2, 3, 3, 3,
            4, 3, 5, 3, 6, 3, 7, 3, 0, 4, 1, 4,
            2, 4, 3, 4, 4, 4, 5, 4, 6, 4, 7, 4,
            8, 4, 1, 5, 2, 5, 3, 5, 4, 5, 5, 5,
            6, 5, 7, 5, 2, 6, 3, 6, 4, 6, 5, 6,
            6, 6, 3, 7, 4, 7, 5, 7, 4, 8
        };
        u_char *pXy;
        int i;

        xglFontPrint(0, 0, 0xFFFFFF, test);
        txt[1] = 32;
        txt[3] = 0;
        xglFontPrint(0, 0, 0xFFFFFF, txt);
        pXy = xy3;
        for (i = 0; i < pXy[0]; i++) {
            u_char *pPair = pXy + 2 + i * 2;

            txt[1] = pPair[0] + 32;
            txt[3] = pPair[1] + 32;
            FlushCache(0);
            xglFontPrint(0, 0, 0xFFFFFF, txt);
        }
        pXy = xy5;
        for (i = 0; i < pXy[0]; i++) {
            u_char *pPair = pXy + 2 + i * 2;

            txt[1] = pPair[0] + 32;
            txt[3] = pPair[1] + 64;
            FlushCache(0);
            xglFontPrint(0, 0, 0xFFFFFF, txt);
        }
        pXy = xy7;
        for (i = 0; i < pXy[0]; i++) {
            u_char *pPair = pXy + 2 + i * 2;

            txt[1] = pPair[0] + 32;
            txt[3] = pPair[1] + 96;
            FlushCache(0);
            xglFontPrint(0, 0, 0xFFFFFF, txt);
        }
        pXy = xy9;
        for (i = 0; i < pXy[0]; i++) {
            u_char *pPair = pXy + 2 + i * 2;

            txt[1] = pPair[0] + 32;
            txt[3] = pPair[1] + 128;
            FlushCache(0);
            xglFontPrint(0, 0, 0xFFFFFF, txt);
        }
        rate += dir;
        if (rate < 2) {
            dir = 1;
        }
        if (rate >= 127) {
            dir = -1;
        }
        xglFontDebugHex(0, 128, rate, 2);
    }
    {
        static char test[] = "\031\000\243\316\243\357\243\362\243\355\243\341\243\354\241\245\243\260\243\261\243\262\243\263";

        xglFontDebugPrintf(96, 32, test);
    }
    {
        static char test[] = "\031\001\243\320\243\362\243\357\243\360\243\357\243\362\243\364\243\351\243\357\243\356\243\341\243\354\241\245\243\260\243\261\243\262\243\263\247\241\247\243\247\246";

        xglFontDebugPrintf(96, 48, test);
    }
    {
        static char test[] = "\015\000\245\306\245\271\245\310\015\004\245\306\245\271\245\310\264\301\273\372\015\003\016\001\001\200\200\200\245\306\245\271\245\310\264\301\273\372\013";

        xglFontDebugPrintf(96, 64, test);
    }
    {
        static char test[] = "\246\330\247\241\247\242\247\243\247\301\247\321\247\322\247\323\247\361\250\241\250\242\250\243\250\300\255\241\255\242";

        xglFontDebugPrintf(96, 80, test);
    }
    {
        static char init[] = "\015\003\016\002\002\030\030\030\014\200\200\200";

        xglFontPrint(0, 0, 0x10000, init);
    }
    xglFontPrint(192, 192, 0x10000, "\244\250\244\250\244\310\250\241\250\241\241\241\244\242\244\242\241\241\245\336\241\274\245\257\255\266\255\302");
    {
        static char test[] = "\025\002\004\000\031\003\025\002\002\010Half-width iiiiii test";

        xglFontDebugPrintf(96, 112, test);
    }
    {
        static char test[] = "\301\264\263\321\312\270\273\372\316\363\244\310\260\354\275\357\244\313\241\304";

        xglFontDebugPrintf(96, 128, test);
    }
    {
        static char test[] = "\025\002\004\001\031\003\025\002\002\010Half-width iiiiii test";

        xglFontDebugPrintf(96, 144, test);
    }
    {
        static char test[] = "\031\003\025\002\002\010\025\002\005\006Itaric Text TEST\025\002\005";

        xglFontDebugPrintf(96, 160, test);
    }
    {
        static char test[] = "\031\003\025\002\002\010\025\002\005\012More Itaric\025\002\005";

        xglFontDebugPrintf(96, 176, test);
    }
}

/* Page 2: the proportional-width sheet.  Pad 1's up/down walks a single
 * EUC code, drawn oversized with a rule under it; below that come five
 * fixed sample strings, each with its measured width ruled off. */
void FontTestP2(void)
{
    static int width = 0x100;
    static char test[] = "\025\003\003\001";
    static char test0[] = "\031\003Proportional font auto linefeed test\n";
    static char test1[] = "\031\003!\"#$%&'()*+,-./\n";
    static char test2[] = "\031\0030123456789:;<=>?\n";
    static char test3[] = "\031\003@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_\n";
    static char test4[] = "\031\003`abcdefghijklmnopqrstuvwxyz{|}~\n";
    u_short nTrig;
    int i;

    xglFontDebugPrintf(8, 8, "%3d", width);
    nTrig = PadData[1].btn.b.nTrig;
    if (nTrig & 0x2000) {
        width++;
    }
    if (nTrig & 0x8000) {
        width--;
    }
    xglFontPrint(64, 96, 0xFFFFFF, "\013\015\002\025\002\001\014\025\002\002\010");
    test[3] = width >> 8;
    test[4] = width;
    xglFontPrintDirect(test);
    FontTestLine(64, 94, width);

    xglFontPrint(64, 96, 0xFFFFFF, test0);
    FontTestLine(64, 96, xglFontGetStringWidth(test0));
    xglFontPrint(64, 256, 0xFFFFFF, test1);
    FontTestLine(64, 256, xglFontGetStringWidth(test1));
    xglFontPrint(64, 288, 0xFFFFFF, test2);
    FontTestLine(64, 288, xglFontGetStringWidth(test2));
    xglFontPrint(64, 320, 0xFFFFFF, test3);
    FontTestLine(64, 320, xglFontGetStringWidth(test3));
    xglFontPrint(64, 352, 0xFFFFFF, test4);
    FontTestLine(64, 352, xglFontGetStringWidth(test4));
    xglFontPrint(64, 96, 0xFFFFFF, "\025\003\003");

    if (PadData[0].btn.b.nHold & 0x20) {
        for (i = 0; i < 64; i++) {
            printf("%3d:%04x\n", i, xglFontGetProportionalSize(i));
        }
    }
}
