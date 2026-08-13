/* Defocus (screen blur) post-effect - per-slot VIF1 packet builders */

typedef unsigned char u8;
typedef unsigned long u_long;
typedef unsigned short u_short;

typedef struct {
    u_long nPMode;          /* 0x00 */
    u_long aUnk08[15];      /* 0x08 */
} XGLDISPENV;

extern XGLDISPENV DispEnv;

typedef struct {
    u_short nUnk00;
    short nPsm;
    short nWidth;
    u_short nUnk06;
    u_short nUnk08;
    u_short nUnk0A;
    u_short nUnk0C;
    u_short nUnk0E;
    u_short nDispFbp;
    u_short nUnk12;
    u_short nDrawFbp;
    u_short nUnk16;
    u_short nUnk18;
    u_short nTexBase;
    u_short nUnk1C;
    u_short nUnk1E;
    u_short nFrontFbp;
    u_short nUnk22;
    unsigned int aUnk24[8];
    int nFade;
    int nUnk48;
    int nDrop;
    int nUnk50;
    int nUnk54;
} XGLRENDER;

extern XGLRENDER sRender;

typedef struct {
    u8 nType;               /* 0x00 */
    u8 nUnk01;              /* 0x01 */
    u8 nId;                 /* 0x02 */
    u8 nUnk03;              /* 0x03 */
    unsigned int nParam;    /* 0x04 */
    unsigned int nFlags;    /* 0x08 */
    int nUnk0C;             /* 0x0C */
    int nUnk10;             /* 0x10 */
    int nUnk14;             /* 0x14 */
    int nUnk18;             /* 0x18 */
    int nUnk1C;             /* 0x1C */
    int nUnk20;             /* 0x20 */
    u8 pad24[0x20];
} GAME_DEFOCUS;

extern u_long GetTex0(int nMode, int nUnk);
extern void sceVif1PkCnt(void *pkt, unsigned int nCode);
extern void sceVif1PkAddDataN(void *pkt, void *pData, int nQw);

extern void DefocusMainType01(GAME_DEFOCUS *p, void *pkt);
extern void DefocusMainType02(GAME_DEFOCUS *p, void *pkt);
extern void DefocusMainType03(GAME_DEFOCUS *p, void *pkt);
extern void DefocusMainType05(GAME_DEFOCUS *p, void *pkt);
extern void DefocusMainType06(GAME_DEFOCUS *p, void *pkt);
extern void DefocusMainType07(GAME_DEFOCUS *p, void *pkt);
extern void DefocusMainType08(GAME_DEFOCUS *p, void *pkt);
extern void DefocusMainType09(GAME_DEFOCUS *p, void *pkt);
extern void DefocusMainType10(GAME_DEFOCUS *p, void *pkt);
extern void DefocusMainType11(GAME_DEFOCUS *p, void *pkt);

GAME_DEFOCUS GameDefocusParam[16];

/* Head.13 @ 0x00367480 - GIF/VIF preamble for the defocus pass */
static unsigned int Head_13[20] __asm__("Head.13") = {
    0x00000000, 0x00000000, 0x00000000, 0x51000004,
    0x00008001, 0x30000000, 0x00000EEE, 0x00000000,
    0x00000000, 0x00000000, 0x0000003F, 0x00000000,
    0x31000000, 0x00000001, 0x0000004E, 0x00000000,
    0x007FC005, 0x000006FC, 0x00000008, 0x00000000,
};

/* func.14 @ 0x003674D0 - dispatch table, indexed by nType - 1 */
static void (*func_14[12])(GAME_DEFOCUS *, void *) __asm__("func.14") = {
    DefocusMainType01,
    DefocusMainType02,
    DefocusMainType03,
    DefocusMainType01,
    DefocusMainType05,
    DefocusMainType06,
    DefocusMainType07,
    DefocusMainType08,
    DefocusMainType09,
    DefocusMainType10,
    DefocusMainType11,
    0,
};

/* Tail.15 @ 0x00367500 - GIF/VIF epilogue for the defocus pass */
static unsigned int Tail_15[16] __asm__("Tail.15") = {
    0x00000000, 0x00000000, 0x00000000, 0x51000003,
    0x00008001, 0x20000000, 0x000000EE, 0x00000000,
    0x00000000, 0x00000000, 0x0000003F, 0x00000000,
    0x31000000, 0x00000000, 0x0000004E, 0x00000000,
};

/* Build the whole defocus pass: dispatch every active slot */
void DefocusMain(void *pkt)
{
    GAME_DEFOCUS *p;
    int i;
    int nType;

    p = GameDefocusParam;
    sceVif1PkCnt(pkt, 0);
    sceVif1PkAddDataN(pkt, Head_13, 0x14);
    for (i = 15; i >= 0; i--) {
        nType = p->nType;
        if (nType != 0) {
            if (nType >= 0) {
                if (nType < 12) {
                    func_14[nType - 1](p, pkt);
                }
            }
        }
        p++;
    }
    sceVif1PkCnt(pkt, 0);
    sceVif1PkAddDataN(pkt, Tail_15, 0x10);
}

/* Head.6 @ 0x00367240 - GIF preamble for the type-6 sprite blit */
static unsigned int Head_6[16] __asm__("Head.6_00367240") = {
    0x00000000, 0x00000000, 0x00000000, 0x51000003,
    0x00008001, 0x20000000, 0x000000EE, 0x00000000,
    0x00071001, 0x00000000, 0x00000047, 0x00000000,
    0x00000084, 0x00000000, 0x00000042, 0x00000000,
};

/* Type 6: build a single flat-shaded sprite in scratchpad and send it */
void DefocusMainType06(GAME_DEFOCUS *p, void *pkt)
{
    unsigned int *q = (unsigned int *)0x70000000;
    u8 *col = (u8 *)&p->nFlags;
    int a;
    int b;
    unsigned int c;

    q[3] = 0x51000004;
    q[4] = 0x8001;
    q[5] = 0x30234000;
    q[6] = 0x551;
    q[0] = 0;
    q[1] = 0;
    q[2] = 0;
    q[7] = 0;
    q[8] = col[0];
    q[9] = col[1];
    q[10] = col[2];
    q[11] = col[3];
    a = (p->nUnk0C << 4) + 0x6FF8;
    q[12] = a;
    q[16] = a + (p->nUnk14 << 4);
    b = (p->nUnk10 << 4) + 0x71F7;
    q[13] = b;
    q[17] = b + (p->nUnk18 << 4);
    c = p->nParam;
    q[14] = c;
    q[15] = 0;
    q[18] = c;
    q[19] = 0;
    sceVif1PkCnt(pkt, 0);
    sceVif1PkAddDataN(pkt, Head_6, 0x10);
    sceVif1PkAddDataN(pkt, (void *)0x70000000, 0x14);
}

int ScanLineInterpolate;

/* TODO near-match (10 diffs): the two 64-bit DispEnv stores come out in
   the opposite order and the two `and`s with -2 swap registers; no source
   ordering of the three statements reaches it (permuter: 6/6 tried). */
/* Type 9 teardown: drop the interlace bit and reset the scanline filter */
void DefocusMainType09Final(GAME_DEFOCUS *p)
{
    u_long f;

    f = p->nFlags;
    ScanLineInterpolate = p->nParam;
    DispEnv.aUnk08[8] &= ~1L;
    DispEnv.nPMode = (DispEnv.nPMode & ~1L) | (f & 1);
}

/* TestPrim.1 @ 0x00366FC0 - full-screen blend pass */
static unsigned int TestPrim_1[60] __asm__("TestPrim.1") = {
    0x00000000, 0x00000000, 0x00000000, 0x5100000E,
    0x00008001, 0xD08B4000, 0x31EEEEEE, 0x000EE535,
    0x31000000, 0x00000001, 0x0000004E, 0x00000000,
    0x00071001, 0x00000000, 0x00000047, 0x00000000,
    0x00000000, 0x00000000, 0x00000014, 0x00000000,
    0x00000001, 0x00000000, 0x0000004A, 0x00000000,
    0x00000000, 0x00000000, 0x0000004C, 0x00000000,
    0x00000000, 0x00000000, 0x00000006, 0x00000000,
    0x00000080, 0x00000080, 0x00000080, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00006FF8, 0x000071F7, 0xFFFFFFFF, 0x00000000,
    0x00002000, 0x00001C00, 0x00000000, 0x00000000,
    0x00008FF8, 0x00008DF7, 0xFFFFFFFF, 0x00000000,
    0x00000000, 0x00000000, 0x0000003F, 0x00000000,
    0x00000000, 0x00000000, 0x0000004A, 0x00000000,
};

/* TestPrim.2 @ 0x003670B0 - alpha-blended copy back to the front buffer */
static unsigned int TestPrim_2[36] __asm__("TestPrim.2") = {
    0x00000000, 0x00000000, 0x00000000, 0x51000008,
    0x00008001, 0x70034000, 0x0EE551EE, 0x00000000,
    0x00070000, 0x00000000, 0x00000047, 0x00000000,
    0x00000000, 0x00000000, 0x0000004C, 0x00000000,
    0x00000000, 0x00000000, 0x00000000, 0x00000000,
    0x00006FF8, 0x000071F7, 0x0000C000, 0x00000000,
    0x00008FF8, 0x00008DF7, 0x0000C000, 0x00000000,
    0x00000000, 0x00000000, 0x0000003F, 0x00000000,
    0x00000000, 0x00000000, 0x0000004C, 0x00000000,
};

typedef struct {
    unsigned int aHead[8];  /* 0x00 */
    u_long nFrame;          /* 0x20 */
    unsigned int aTail[2];  /* 0x28 */
} DEFOCUS_PRIM3;

/* TestPrim.3 @ 0x00367140 - bare frame-buffer select */
static DEFOCUS_PRIM3 TestPrim_3 __asm__("TestPrim.3") = {
    { 0x00000000, 0x00000000, 0x00000000, 0x51000002,
      0x00008001, 0x10000000, 0x0000000E, 0x00000000 },
    0,
    { 0x0000004C, 0x00000000 },
};

/* Type 2: blend the blurred buffer over the frame, then resolve */
void DefocusMainType02(GAME_DEFOCUS *p, void *pkt)
{
    unsigned int *q;
    unsigned int n;

    q = TestPrim_1;
    *(u_long *)&q[0x60 / 4] = sRender.nDrawFbp | 0x80000L;
    *(u_long *)&q[0x70 / 4] = (sRender.nFrontFbp << 5) | 0x24020000 | 0x640000000;
    sceVif1PkCnt(pkt, 0);
    sceVif1PkAddDataN(pkt, q, 0x3C);
    n = p->nFlags;
    if (n != 0) {
        TestPrim_2[0x58 / 4] = n;
        *(u_long *)&TestPrim_2[0x30 / 4] = sRender.nDrawFbp | 0x80000L;
        TestPrim_2[0x68 / 4] = p->nFlags;
        *(u_long *)&TestPrim_2[0x80 / 4] = sRender.nFrontFbp | 0x80000L;
        sceVif1PkCnt(pkt, 0);
        sceVif1PkAddDataN(pkt, TestPrim_2, 0x24);
    } else {
        TestPrim_3.nFrame = sRender.nFrontFbp | 0x80000L;
        sceVif1PkCnt(pkt, 0);
        sceVif1PkAddDataN(pkt, &TestPrim_3, 0xC);
    }
}

typedef struct {
    unsigned int aHead[20]; /* 0x00 */
    u_long nTex0;           /* 0x50 */
    unsigned int aTail[2];  /* 0x58 */
} DEFOCUS_HEAD0;

/* Head.0 @ 0x00366F60 - GIF preamble for the type-1 textured blit */
static DEFOCUS_HEAD0 Head_0 __asm__("Head.0") = {
    { 0x00000000, 0x00000000, 0x00000000, 0x51000005,
      0x00008001, 0x40000000, 0x0000EEEE, 0x00000000,
      0x00000060, 0x00000000, 0x00000014, 0x00000000,
      0x00070000, 0x00000000, 0x00000047, 0x00000000,
      0x00000044, 0x00000000, 0x00000042, 0x00000000 },
    0,
    { 0x00000006, 0x00000000 },
};

/* Type 1: textured sprite, repeated nFlags times */
void DefocusMainType01(GAME_DEFOCUS *p, void *pkt)
{
    unsigned int *q = (unsigned int *)0x70000000;
    unsigned int i;
    u8 *col;
    int a;
    int b;
    int c;

    i = 0;
    Head_0.nTex0 = GetTex0(p->nParam, 0);
    q[5] = 0x50AB4000;
    q[6] = 0x53531;
    q[3] = 0x51000006;
    col = (u8 *)&p->nUnk10;
    q[4] = 0x8001;
    q[0] = 0;
    q[1] = 0;
    q[2] = 0;
    q[7] = 0;
    q[8] = col[0];
    q[9] = col[1];
    q[10] = col[2];
    q[20] = 0x2000;
    q[11] = col[3];
    q[12] = 0;
    a = (p->nUnk14 << 4) + 0x6FF8;
    q[16] = a;
    q[21] = 0x1C00;
    q[13] = 0;
    q[24] = ((p->nUnk14 + p->nUnk1C) << 4) + 0x8FF8;
    b = (p->nUnk18 << 4) + 0x71F7;
    q[17] = b;
    q[25] = ((p->nUnk18 + p->nUnk20) << 4) + 0x8DF7;
    c = p->nUnk0C;
    q[26] = c;
    q[18] = c;
    q[19] = 0;
    q[27] = 0;
    sceVif1PkCnt(pkt, 0);
    sceVif1PkAddDataN(pkt, &Head_0, 0x18);
    if (p->nFlags != 0) {
        do {
            sceVif1PkAddDataN(pkt, (void *)0x70000000, 0x1C);
            i++;
        } while (i < p->nFlags);
    }
}
