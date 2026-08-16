/* VIF1 packet construction helpers for the normal-map model renderer */

#include "matching.h"

typedef unsigned char u_char;
typedef unsigned short u_short;
typedef unsigned int u_int;
typedef unsigned long u_long;
typedef int TI __attribute__((mode(TI)));

typedef struct {
    u_short nUnk00;
    short nPsm;
    short nWidth;
    u_short nUnk06;
    u_short nZbp;
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
    u_int aUnk24[8];
    int nFade;
    int nUnk48;
    int nDrop;
    int nUnk50;
    int nUnk54;
} XGLRENDER;

typedef struct {
    char pad0[0x1C0];
    u_int aFogColor[3];     /* 0x1C0 */
    int nFogAlpha;          /* 0x1CC */
    u_int aFogParam[4];     /* 0x1D0 */
    char pad1E0[0x110];
    float afFogDensity[4];  /* 0x2F0 */
} NMLSCREEN;

/* GS A+D register buffer: entry 0 is the giftag, entries 1.. are
 * (data, address) qword pairs, viewed as 64-bit or 32-bit words */
typedef union {
    u_long l[202];
    u_int w[404];
} GSTAGBUF;

u_int *xglPacketGetCurrent(void);
void sceVif1PkCnt(u_int *pPk, u_int nFlags);
void sceVif1PkAddCode(u_int *pPk, u_int nCode);
void sceVif1PkOpenUpkCode(u_int *pPk, u_int nAddr, u_int nCmd, u_int nNum, u_int nFlag);
void sceVif1PkCloseUpkCode(u_int *pPk);
void sceVif1PkAddUpkData128N(u_int *pPk, u_int *pData, u_int nNum);
void *memcpy(void *, const void *, u_int);
void nmlPacketAddGsFlushWide(void);

extern XGLRENDER sRender;
extern int g_aSubWindow[4];

int g_nGsEntry;
GSTAGBUF g_aGsTag;

typedef struct {
    u_long nData;
    u_long nReg;
} GSPACKETENTRY;

static u_int *s_pPacket;
static void *s_pCacheTexture;
static void *s_pMatrixCache;
static void *s_pModelCache;
static void *s_pModelLayout;
static void *s_pLightLayout;

/* Add a VIF1 flush code waiting for the microprogram to finish */
void nmlPacketAddWaitMicrocode(void)
{
    s_pPacket = xglPacketGetCurrent();
    sceVif1PkCnt(s_pPacket, 0);
    sceVif1PkAddCode(s_pPacket, 0x11000000);
}

/* Invalidate the cached texture pointer */
void nmlPacketClrTextureCache(void)
{
    s_pCacheTexture = 0;
}

/* Invalidate all cached model state pointers */
void nmlPacketClrModelCache(void)
{
    s_pMatrixCache = 0;
    s_pModelCache = 0;
    s_pModelLayout = 0;
    s_pLightLayout = 0;
}

/* Copy attribute data into the top-of-packet attribute area */
u_int nmlPacketSetAttributeData(void *pData, int nSize)
{
    s_pPacket = xglPacketGetCurrent();
    s_pPacket[9] -= nSize;
    memcpy((void *)s_pPacket[9], pData, nSize);
    return s_pPacket[9];
}

/* Reserve quadwords in the attribute area and return the new top */
u_int nmlPacketSetAttributeAlloc16N(int nNum)
{
    s_pPacket = xglPacketGetCurrent();
    s_pPacket[9] -= nNum << 4;
    return s_pPacket[9];
}

/* Copy one quadword-aligned 64-byte block into the top of the attribute
 * area */
/* Copy one quadword-aligned 64-byte block into the top of the attribute
 * area. Real MMI quadword work, and the original was assembly: the four
 * lq/sq pairs are strictly interleaved through $v0 with no scheduling,
 * the same block nmlPacketSetAttributeData64N runs in a loop. */
u_int nmlPacketSetAttributeData64(void *pData)
{
    void *p;

    s_pPacket = xglPacketGetCurrent();
    s_pPacket[9] -= 64;
    p = (void *)s_pPacket[9];
    PS2_ASM(".set noreorder\n"
        "lq $2, 0x0(%1)\n"
        "sq $2, 0x0(%0)\n"
        "lq $2, 0x10(%1)\n"
        "sq $2, 0x10(%0)\n"
        "lq $2, 0x20(%1)\n"
        "sq $2, 0x20(%0)\n"
        "lq $2, 0x30(%1)\n"
        "sq $2, 0x30(%0)\n"
        ".set reorder"
        : : "r"(p), "r"(pData) : "$2", "memory");
    return s_pPacket[9];
}

/* Copy nNum 16-byte quadwords into the top of the attribute area */
/* Copy nNum 16-byte quadwords into the top of the attribute area. The
 * original's loop is assembly, not compiled C: it carries its own local
 * label (_$psa16_loop, still in the ELF symbol table), uses the trapping
 * `addi` rather than `addiu` for all three induction registers, and tests
 * with `bne $zero,$s0` -- an operand order gcc never emits. */
u_int nmlPacketSetAttributeData16N(void *pData, int nNum)
{
    void *p;

    s_pPacket = xglPacketGetCurrent();
    s_pPacket[9] -= nNum << 4;
    p = (void *)s_pPacket[9];
    PS2_ASM(".set noreorder\n"
        "_$psa16_loop:\n"
        "lq $2, 0x0(%1)\n"
        "sq $2, 0x0(%0)\n"
        "addi %2, %2, -1\n"
        "addi %0, %0, 16\n"
        "addi %1, %1, 16\n"
        "bne $0, %2, _$psa16_loop\n"
        "nop\n"
        ".set reorder"
        : "+r"(p), "+r"(pData), "+r"(nNum) : : "$2", "memory");
    return s_pPacket[9];
}

/* Copy nNum 64-byte (4-quadword) blocks into the top of the attribute
 * area */
/* Copy nNum 64-byte (4-quadword) blocks into the top of the attribute
 * area. Same hand-written SDK loop as nmlPacketSetAttributeData16N, four
 * quadwords per iteration; its label _$psa_loop is in the ELF too. */
u_int nmlPacketSetAttributeData64N(void *pData, int nNum)
{
    void *p;

    s_pPacket = xglPacketGetCurrent();
    s_pPacket[9] -= nNum << 6;
    p = (void *)s_pPacket[9];
    PS2_ASM(".set noreorder\n"
        "_$psa_loop:\n"
        "lq $2, 0x0(%1)\n"
        "sq $2, 0x0(%0)\n"
        "lq $2, 0x10(%1)\n"
        "sq $2, 0x10(%0)\n"
        "lq $2, 0x20(%1)\n"
        "sq $2, 0x20(%0)\n"
        "lq $2, 0x30(%1)\n"
        "sq $2, 0x30(%0)\n"
        "addi %2, %2, -1\n"
        "addi %0, %0, 64\n"
        "addi %1, %1, 64\n"
        "bne $0, %2, _$psa_loop\n"
        "nop\n"
        ".set reorder"
        : "+r"(p), "+r"(pData), "+r"(nNum) : : "$2", "memory");
    return s_pPacket[9];
}

/* Upload fog color and fog parameters to VU memory */
void nmlPacketAddFog(NMLSCREEN *pScreen, int nWindow)
{
    s_pPacket = xglPacketGetCurrent();
    sceVif1PkCnt(s_pPacket, 0);
    if (g_aSubWindow[nWindow] != 0) {
        pScreen->nFogAlpha = (int)(pScreen->afFogDensity[nWindow] * 255.0f) << 4;
    }
    sceVif1PkOpenUpkCode(s_pPacket, 0x3E6, 0x6C, 1, 1);
    sceVif1PkAddUpkData128N(s_pPacket, pScreen->aFogColor, 1);
    sceVif1PkCloseUpkCode(s_pPacket);
    sceVif1PkOpenUpkCode(s_pPacket, 0x3EF, 0x6C, 1, 1);
    sceVif1PkAddUpkData128N(s_pPacket, pScreen->aFogParam, 1);
    sceVif1PkCloseUpkCode(s_pPacket);
}

/* Latch the current double-buffered packet builder */
void nmlPacketSetCurrent(void)
{
    s_pPacket = xglPacketGetCurrent();
}

/* Reset the GS register entry buffer */
void nmlPacketGsInit(void)
{
    *(volatile int *)&g_nGsEntry = 0;
}

/* Queue a CLAMP_1 register write */
void nmlPacketAddGsClamp(u_long nClamp)
{
    g_aGsTag.l[g_nGsEntry * 2 + 2] = nClamp;
    g_aGsTag.l[g_nGsEntry * 2 + 3] = 0x08;
    g_nGsEntry++;
}

/* Queue a TEST_1 register write */
void nmlPacketAddGsPixeltest(u_int nTest)
{
    g_aGsTag.w[g_nGsEntry * 4 + 4] = nTest;
    g_aGsTag.w[g_nGsEntry * 4 + 5] = 0;
    g_aGsTag.l[g_nGsEntry * 2 + 3] = 0x47;
    g_nGsEntry++;
}

/* Queue a TEST_2 register write */
void nmlPacketAddGsPixeltest1(u_int nTest)
{
    g_aGsTag.w[g_nGsEntry * 4 + 4] = nTest;
    g_aGsTag.w[g_nGsEntry * 4 + 5] = 0;
    g_aGsTag.l[g_nGsEntry * 2 + 3] = 0x48;
    g_nGsEntry++;
}

/* Queue a ZBUF_1 register write with the current Z buffer base */
void nmlPacketAddGsZbuf(u_int nZmsk)
{
    g_aGsTag.l[g_nGsEntry * 2 + 2] = (sRender.nZbp | ((u_long)nZmsk << 32)) | 0x1000000;
    g_aGsTag.l[g_nGsEntry * 2 + 3] = 0x4E;
    g_nGsEntry++;
}

/* Queue a ZBUF_2 register write with the current Z buffer base */
void nmlPacketAddGsZbuf1(u_int nZmsk)
{
    g_aGsTag.l[g_nGsEntry * 2 + 2] = (sRender.nZbp | ((u_long)nZmsk << 32)) | 0x1000000;
    g_aGsTag.l[g_nGsEntry * 2 + 3] = 0x4F;
    g_nGsEntry++;
}

/* Queue a TEXFLUSH register write */
void nmlPacketAddGsTexture(void)
{
    g_aGsTag.w[g_nGsEntry * 4 + 4] = 0;
    g_aGsTag.w[g_nGsEntry * 4 + 5] = 0;
    g_aGsTag.l[g_nGsEntry * 2 + 3] = 0x3F;
    g_nGsEntry++;
}

/* Queue an ALPHA_1 register write */
void nmlPacketAddGsAlpha(u_long nAlpha)
{
    g_aGsTag.l[g_nGsEntry * 2 + 2] = nAlpha;
    g_aGsTag.l[g_nGsEntry * 2 + 3] = 0x42;
    g_nGsEntry++;
}

/* Queue an ALPHA_2 register write */
void nmlPacketAddGsAlpha1(u_long nAlpha)
{
    g_aGsTag.l[g_nGsEntry * 2 + 2] = nAlpha;
    g_aGsTag.l[g_nGsEntry * 2 + 3] = 0x43;
    g_nGsEntry++;
}

/* Queue a SCISSOR_1 register write */
void nmlPacketAddGsScissor(u_long nScissor)
{
    g_aGsTag.l[g_nGsEntry * 2 + 2] = nScissor;
    g_aGsTag.l[g_nGsEntry * 2 + 3] = 0x40;
    g_nGsEntry++;
}

/* Queue a SCISSOR_2 register write */
void nmlPacketAddGsScissor1(u_long nScissor)
{
    g_aGsTag.l[g_nGsEntry * 2 + 2] = nScissor;
    g_aGsTag.l[g_nGsEntry * 2 + 3] = 0x41;
    g_nGsEntry++;
}

/* Queue an FBA_1 register write from a boolean flag */
void nmlPacketAddGsFBA(u_int nFba)
{
    g_aGsTag.w[g_nGsEntry * 4 + 4] = nFba != 0;
    g_aGsTag.w[g_nGsEntry * 4 + 5] = 0;
    g_aGsTag.l[g_nGsEntry * 2 + 3] = 0x4A;
    g_nGsEntry++;
}

/* Queue an FBA_2 register write from a boolean flag */
void nmlPacketAddGsFBA1(u_int nFba)
{
    g_aGsTag.w[g_nGsEntry * 4 + 4] = nFba != 0;
    g_aGsTag.w[g_nGsEntry * 4 + 5] = 0;
    g_aGsTag.l[g_nGsEntry * 2 + 3] = 0x4B;
    g_nGsEntry++;
}

/* Queue a FOGCOL register write from a float RGB triple */
void nmlPacketAddGsFogCol(float *pfCol)
{
    u_long nCol;

    nCol = (u_long)(u_int)pfCol[0] | ((u_long)(u_int)pfCol[1] << 8) | ((u_long)(u_int)pfCol[2] << 16);
    g_aGsTag.w[g_nGsEntry * 4 + 4] = nCol;
    g_aGsTag.w[g_nGsEntry * 4 + 5] = 0;
    g_aGsTag.l[g_nGsEntry * 2 + 3] = 0x3D;
    g_nGsEntry++;
}

/* Queue a FRAME_1 register write for the current render target */
void nmlPacketAddGsFrame(u_int nFbp, u_int nFbmsk)
{
    u_long nData;
    u_long nEnv;

    nEnv = ((u_long)sRender.nPsm << 24) | ((u_long)(sRender.nWidth / 64) << 16);
    nData = (nFbp & 0xFFFF) | ((u_long)nFbmsk << 32);
    g_aGsTag.l[g_nGsEntry * 2 + 2] = nData | nEnv;
    g_aGsTag.l[g_nGsEntry * 2 + 3] = 0x4C;
    g_nGsEntry++;
}

/* Queue a FRAME_2 register write for the current render target */
void nmlPacketAddGsFrame1(u_int nFbp, u_int nFbmsk)
{
    u_long nData;
    u_long nEnv;

    nEnv = ((u_long)sRender.nPsm << 24) | ((u_long)(sRender.nWidth / 64) << 16);
    nData = (nFbp & 0xFFFF) | ((u_long)nFbmsk << 32);
    g_aGsTag.l[g_nGsEntry * 2 + 2] = nData | nEnv;
    g_aGsTag.l[g_nGsEntry * 2 + 3] = 0x4D;
    g_nGsEntry++;
}

/* Queue a PABE register write */
void nmlPacketAddGsPAbe(u_long nPabe)
{
    g_aGsTag.l[g_nGsEntry * 2 + 2] = nPabe;
    g_aGsTag.l[g_nGsEntry * 2 + 3] = 0x49;
    g_nGsEntry++;
}

/* TODO: near-miss, 17/18 words. ROOT CAUSE of the missing word found:
 * the original's `xori v1,v1,0` is an unfolded compare-against-zero from
 * the conditional-move expander. MIPS gen_conditional_move rewrites
 * `a != b` into `(a ^ b) != 0` before selecting movn; gcc 2.96 guards that
 * XOR on `op1 != const0_rtx`, so comparing against 0 skips it, while the
 * compiler that built the original emitted it unconditionally. No C shape
 * can produce a 16-bit-immediate xor by zero out of our compiler: swept
 * !=0 / ==0 / ternary / bare-truth-value / int / unsigned / u_long forms
 * and a LAUNDER on the flag, all 17 words.
 * Forcing it with `__asm__("xori %0,%0,0" : "+r"(flag))` does restore the
 * 18th word and the instruction multiset, but leaves 16 words differing on
 * a register permutation (the original holds 88 in $a0 and 72 in $a2,
 * gcc uses $a1/$a0) plus a header reschedule, so the steering buys
 * nothing and is not shipped. */
/* Queue a PRMODE register write; the value is 88 or 72 depending on bit 0
 * of the model's flags word at +0xC0 */
void nmlPacketAddGsPrmode(void *pModel)
{
    int flag = (*(int *)((char *)pModel + 0xC0) & 1) != 0;
    int size = 72;

    if (flag) {
        size = 88;
    }
    g_aGsTag.l[g_nGsEntry * 2 + 2] = size;
    g_aGsTag.l[g_nGsEntry * 2 + 3] = 0x1B;
    g_nGsEntry++;
}

/* Queue a PRMODECONT register write */
void nmlPacketAddGsPrmodecont(u_long nCont)
{
    g_aGsTag.l[g_nGsEntry * 2 + 2] = nCont;
    g_aGsTag.l[g_nGsEntry * 2 + 3] = 0x1A;
    g_nGsEntry++;
}

/* Queue an FBA_1 register write with a raw value */
void nmlPacketAddGsFba(u_long nFba)
{
    g_aGsTag.l[g_nGsEntry * 2 + 2] = nFba;
    g_aGsTag.l[g_nGsEntry * 2 + 3] = 0x4A;
    g_nGsEntry++;
}

void sceVif1PkAddUpkData128(u_int *pPk, TI data);
unsigned int sceVif1PkSize(u_int *pPk);
void sceVif1PkRef(u_int *pPk, u_int a1, u_int a2, u_int a3, u_int t0, u_int t1);

/* Reference pData directly from the packet (DMA-ref, no copy) if doing
 * so wouldn't push the packet's total span past the 2MB DMA window;
 * returns 1 if it was too big to reference, 0 on success */
int nmlPacketDirectData(void *pData, int nSize)
{
    u_int *pk;
    u_int nSizeQw;

    s_pPacket = xglPacketGetCurrent();
    nSizeQw = sceVif1PkSize(s_pPacket);
    pk = s_pPacket;
    if (0x200000 < (nSizeQw << 4) + (pk[8] - pk[9]) + 0x10000) {
        return 1;
    }
    sceVif1PkRef(pk, (u_int)pData, nSize, 0, 0, 0);
    return 0;
}

/* Reference the model's transform data block (offset/size stored at
 * +0x24/+0x28) directly from the packet and flush the microprogram */
void nmlPacketAddTransData(void *pModel)
{
    s_pPacket = xglPacketGetCurrent();
    sceVif1PkRef(s_pPacket, *(int *)((char *)pModel + 0x24) + (u_int)pModel,
                 *(int *)((char *)pModel + 0x28) / 16, 0, 0, 0);
    nmlPacketAddWaitMicrocode();
}

void xglMatrixUnit(void *pDst);
void xglMatrixStackUnit(void);
void xglMatrixStackRotY(float);
void xglMatrixStackRotX(float);
void xglMatrixStackSave(void *pMtx);
int s_nReflRotType;
float s_inReflRotY;
float s_inReflRotX;

/* Queue the reflection rotation matrix. A switch, not an if/else-if
 * chain: gcc sorts the case tests (1 then 2) while emitting the bodies in
 * source order, which is how the nType==2 body ends up inline and the
 * nType==1 body out of line the way the original lays them out. The
 * default arm returns rather than falling into the packet tail -- the
 * original's `bnel` on the second test goes straight to the epilogue. */
void nmlPacketAddReflRot(void *pModel)
{
    u_int aMtx[16];
    int nType;

    nType = 1;
    if ((*(int *)((char *)pModel + 0xC0) & 0x2000) != 0) {
        nType = 2;
    }
    s_pPacket = xglPacketGetCurrent();
    switch (nType) {
    case 2:
        if (s_nReflRotType == nType) {
            return;
        }
        xglMatrixStackUnit();
        xglMatrixStackRotY(s_inReflRotY);
        xglMatrixStackRotX(s_inReflRotX);
        xglMatrixStackSave(aMtx);
        break;
    case 1:
        if (s_nReflRotType == nType) {
            return;
        }
        xglMatrixUnit(aMtx);
        break;
    default:
        return;
    }
    sceVif1PkCnt(s_pPacket, 0);
    sceVif1PkOpenUpkCode(s_pPacket, 0x3EC, 0x6C, 1, 1);
    sceVif1PkAddUpkData128N(s_pPacket, aMtx, 3);
    sceVif1PkCloseUpkCode(s_pPacket);
    s_nReflRotType = nType;
}

/* Add a standard (non-textured) GIF tag with the given PRIM/NREG-ish
 * fields packed into word 1 */
void nmlPacketAddGifTagStandard(int nA, int nB)
{
    u_int aTag[4];

    s_pPacket = xglPacketGetCurrent();
    sceVif1PkCnt(s_pPacket, 0);
    aTag[0] = 0x8000;
    aTag[1] = (nA << 19) | (nB << 21) | 0x30064000;
    aTag[2] = 0x412;
    aTag[3] = 0;
    sceVif1PkOpenUpkCode(s_pPacket, 0x3F3, 0x6C, 1, 1);
    sceVif1PkAddUpkData128(s_pPacket, *(TI *)aTag);
    sceVif1PkCloseUpkCode(s_pPacket);
}


void nmlPacketAddScreen(void *pModel)
{
    u_int aBuf[4];
    char *pSrc;

    s_pPacket = xglPacketGetCurrent();
    sceVif1PkCnt(s_pPacket, 0);
    pSrc = (char *)pModel + 0x70;
    /* Real MMI quadword copy, same hand-written idiom as the attribute
     * builders above: the source address is materialised in its own
     * register at offset 0 rather than folded into the lq. */
    PS2_ASM("lq $2, 0x0(%1)\n sq $2, 0x0(%0)"
        : : "r"(aBuf), "r"(pSrc) : "$2", "memory");
    /* Unconditional -- it sits in the delay slot of the nTexBase compare,
     * so it runs on both arms. */
    aBuf[3] = 0;
    if ((sRender.nTexBase << 5) != 0x3800) {
        ((float *)aBuf)[3] = 3.141592741f;
    }
    sceVif1PkOpenUpkCode(s_pPacket, 0x3C7, 0x6C, 1, 1);
    sceVif1PkAddUpkData128(s_pPacket, *(TI *)aBuf);
    sceVif1PkCloseUpkCode(s_pPacket);
    sceVif1PkOpenUpkCode(s_pPacket, 0x3C6, 0x6C, 1, 1);
    sceVif1PkAddUpkData128(s_pPacket, *(TI *)((char *)pModel + 0x80));
    sceVif1PkCloseUpkCode(s_pPacket);
}

/* Queue a raw 64-bit (data, register) pair from a scratchpad entry */
/* TODO: near-miss, 14/17 words. Semantics verified: the dsll32/dsrl32
 * pair (the u_int -> u_long zero-extension of nReg) is present in both.
 * The three missing words are all register COPIES the original never
 * coalesced -- `move a3,a2` before incrementing n, `move v1,a2` before
 * adding the table base, and `move a2,v1` before the first sd, so the two
 * stores use two different registers holding the same address -- plus the
 * original materialising &g_nGsEntry into $t1 for the write while reading
 * the same variable gp-relative in one word. That combination looks like
 * a build with register coalescing off for this function, not a source
 * shape: swept direct-read/direct-write, read-direct/write-through-pointer,
 * split n/nNext temporaries, both store orders, array-index vs pointer
 * form, and a LAUNDER on the entry pointer; every one of them lands on
 * 13 or 14 words, never 17. */
void packet_gs_entry64(u_int nReg, u_long *pData)
{
    int *pn;
    u_long *q;
    int n;

    pn = &g_nGsEntry;
    n = *pn;
    q = &g_aGsTag.l[n * 2];
    q[2] = pData[0];
    q[3] = nReg;
    *pn = n + 1;
}

/* Clear the screen by building an eight-register GS packet in scratchpad
 * memory, copying it to the normal GS packet buffer, then flushing it. */
void nmlPacketAddScreenClear(void)
{
    volatile GSPACKETENTRY *pScratch;
    GSPACKETENTRY *pDst;
    int nWidth;
    int nHeight;
    int n;
    u_long nZbuf;

    nZbuf = sRender.nZbp | 0x01000000;
    pScratch = (volatile GSPACKETENTRY *)0x70000000;
    pScratch->nReg = 0x4A;
    pScratch->nData = 0;
    pScratch = (volatile GSPACKETENTRY *)0x70000010;
    pScratch->nReg = 0x4E;
    pScratch->nData = nZbuf;
    pScratch = (volatile GSPACKETENTRY *)0x70000020;
    pScratch->nReg = 0x47;
    pScratch->nData = 0x30000;
    nWidth = sRender.nWidth;
    pScratch = (volatile GSPACKETENTRY *)0x70000030;
    pScratch->nReg = 0x4C;
    pScratch->nData = sRender.nFrontFbp |
        ((u_long)(nWidth / 64) << 16) |
        ((u_long)sRender.nPsm << 24);
    pScratch = (volatile GSPACKETENTRY *)0x70000040;
    pScratch->nReg = 1;
    pScratch->nData = (u_long)0xFE00 << 46;
    pScratch = (volatile GSPACKETENTRY *)0x70000050;
    pScratch->nData = 6;
    pScratch->nReg = 0;

    nHeight = (short)sRender.nUnk06;
    pScratch = (volatile GSPACKETENTRY *)0x70000060;
    pScratch->nReg = 4;
    pScratch->nData = ((u_long)(0x800 - nWidth) << 4) |
        ((u_long)(0x800 - nHeight) << 20);
    pScratch = (volatile GSPACKETENTRY *)0x70000070;
    pScratch->nReg = 4;
    pScratch->nData = ((u_long)(nWidth + 0x800) << 4) |
        ((u_long)(nHeight + 0x800) << 20);

    g_nGsEntry = 0;
    pScratch = (volatile GSPACKETENTRY *)0x70000000;
    n = 7;
    do {
        --n;
        pDst = (GSPACKETENTRY *)&g_aGsTag.l[*(volatile int *)&g_nGsEntry * 2 + 2];
        ++*(volatile int *)&g_nGsEntry;
        pDst->nData = pScratch->nData;
        pDst->nReg = *(volatile u_int *)((char *)pScratch + 8);
        ++pScratch;
    } while (n >= 0);

    nmlPacketAddGsFlushWide();
}

/* --- Transform-microcode init --- */

typedef union {
    float f[4];
    TI q;
} VEC4P;

void xglMatrixUnit(void *pDst);
float xglSin(float fAngle);

extern int D_004AE5A0[];
extern char D_004AE5B0[];
extern int D_00338690[];
extern float s_fTransAngleStep;

int s_nTransUnk38;
int s_nTransUnk3C;
int s_nTransUnk1C;
int s_nTransUnk20;
int s_nTransUnk24;
int s_nTransUnk2C;
int s_nTransUnk30;
int s_nTransUnk40;
float s_fTransAngle;
float s_fTransSin;
VEC4P s_inTransMtx[4];

/* Reference the transform microprogram, reset the transform state,
 * advance the water angle and upload a unit matrix */
/* TODO: near-miss (3 diffs, 70 orig vs 69 built) -- everything matches
 * except the tail: the original stalls one nop after mtc1 (ld ra
 * scheduled earlier) before the four 1.0f stores; ours fills the slot
 * with the last store. Same hazard-nop scheduling class as
 * nmlModelSetFadeIn / nmlModelFogPara; asm barriers cannot reproduce
 * the stall. */
void nmlPacketAddTransMicrocodeInit(void)
{
    VEC4P aMtx[4];
    VEC4P *p;
    float fOne;

    s_pPacket = xglPacketGetCurrent();
    sceVif1PkRef(s_pPacket, (u_int)D_004AE5B0, D_004AE5A0[0], 0, 0, 0);
    s_nTransUnk38 = 2;
    s_nTransUnk3C = -1;
    s_nTransUnk1C = 0;
    s_nTransUnk20 = 0;
    s_nTransUnk24 = 0;
    s_nTransUnk2C = 0;
    s_nTransUnk30 = 0;
    s_fTransSin = xglSin(s_fTransAngle);
    if ((D_00338690[0] & 1) == 0) {
        s_fTransAngle += s_fTransAngleStep;
    }
    xglMatrixUnit(aMtx);
    sceVif1PkCnt(s_pPacket, 0);
    sceVif1PkOpenUpkCode(s_pPacket, 0x3EC, 0x6C, 1, 1);
    sceVif1PkAddUpkData128N(s_pPacket, (u_int *)aMtx, 3);
    sceVif1PkCloseUpkCode(s_pPacket);
    s_nTransUnk40 = 1;
    p = s_inTransMtx;
    __asm__ __volatile__(
        "sq $0, 0x0(%0)\n sq $0, 0x10(%0)\n"
        "sq $0, 0x20(%0)\n sq $0, 0x30(%0)"
        : : "r"(p) : "memory");
    fOne = 1.0f;
    /* Statement order is load-bearing: sched2 hoists the LAST of these
     * stores to the front, and the original binary stores 48,60,56,52
     * -- only this f[3],f[2],f[1],f[0] source order reproduces it
     * (permutation sweep 2026-08-14). */
    p[3].f[3] = fOne;
    p[3].f[2] = fOne;
    p[3].f[1] = fOne;
    p[3].f[0] = fOne;
}
