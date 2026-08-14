/* VIF1 packet construction helpers for the normal-map model renderer */

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

extern XGLRENDER sRender;
extern int g_aSubWindow[4];

int g_nGsEntry;
GSTAGBUF g_aGsTag;

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
/* TODO: near-miss (permute.py swept all 24 store orderings, best 17/25
 * words differ, LOGIC not scheduling -- reload/interleave shape differs
 * from source, not just store order). Parked per budget rule. */
u_int nmlPacketSetAttributeData64(void *pData)
{
    void *p;

    s_pPacket = xglPacketGetCurrent();
    s_pPacket[9] -= 64;
    p = (void *)s_pPacket[9];
    ((TI *)p)[0] = ((TI *)pData)[0];
    ((TI *)p)[1] = ((TI *)pData)[1];
    ((TI *)p)[2] = ((TI *)pData)[2];
    ((TI *)p)[3] = ((TI *)pData)[3];
    return s_pPacket[9];
}

/* TODO: near-miss (10/28 words differ; original loop body uses `addi`
 * not `addiu` for the induction registers and a `bne $zero,$sN` operand
 * order our compiler never emits, and the asm carries a local label
 * `_$psa16_loop` -- looks like a hand-written SDK packet-loop macro, not
 * plain compiled C. Parked per budget rule after 1 attempt. */
/* Copy nNum 16-byte quadwords into the top of the attribute area */
u_int nmlPacketSetAttributeData16N(void *pData, int nNum)
{
    void *p;

    s_pPacket = xglPacketGetCurrent();
    s_pPacket[9] -= nNum << 4;
    p = (void *)s_pPacket[9];
    do {
        *(TI *)p = *(TI *)pData;
        p = (char *)p + 16;
        pData = (char *)pData + 16;
    } while (--nNum);
    return s_pPacket[9];
}

/* TODO: near-miss (same hand-written `_$psa_loop`-style SDK macro shape
 * as nmlPacketSetAttributeData16N -- addi vs addiu, bne operand order.
 * Parked per budget rule after 1 attempt. */
/* Copy nNum 64-byte (4-quadword) blocks into the top of the attribute
 * area */
u_int nmlPacketSetAttributeData64N(void *pData, int nNum)
{
    void *p;

    s_pPacket = xglPacketGetCurrent();
    s_pPacket[9] -= nNum << 6;
    p = (void *)s_pPacket[9];
    do {
        ((TI *)p)[0] = ((TI *)pData)[0];
        ((TI *)p)[1] = ((TI *)pData)[1];
        ((TI *)p)[2] = ((TI *)pData)[2];
        ((TI *)p)[3] = ((TI *)pData)[3];
        p = (char *)p + 64;
        pData = (char *)pData + 64;
    } while (--nNum);
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
    g_nGsEntry = 0;
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

/* TODO: near-miss (17/18 words, missing one xori-0 boolean-normalize
 * instruction vs orig's movn-based ternary; logic/values verified correct).
 * Parked per budget rule after triage + one extra source-shape attempt. */
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

/* TODO: near-miss (18/54 words, LOGIC/register). Logic, field offsets,
 * and call sequence verified against asm. Original's `li s0,1; li s1,2;
 * movn s0,s1,cond` keeps s0=1 as the kept/default value and s1=2 as the
 * override, with the SECOND textual branch (nType==1) jumping forward
 * to a block placed AFTER the nType==2 code -- i.e. the else-if order is
 * inverted from what a plain top-down `if/else if` naturally lays out.
 * Writing the source with nType==2 checked first reproduces that branch
 * layout but flips which of s0/s1 gets the default vs override value;
 * every natural pairing of (check order) x (default-value order) tried
 * fixes one half and breaks the other. */
void nmlPacketAddReflRot(void *pModel)
{
    u_int aMtx[16];
    int nType;

    nType = 1;
    if ((*(int *)((char *)pModel + 0xC0) & 0x2000) != 0) {
        nType = 2;
    }
    s_pPacket = xglPacketGetCurrent();
    if (nType == 2) {
        if (s_nReflRotType == nType) {
            return;
        }
        xglMatrixStackUnit();
        xglMatrixStackRotY(s_inReflRotY);
        xglMatrixStackRotX(s_inReflRotX);
        xglMatrixStackSave(aMtx);
    } else if (nType == 1) {
        if (s_nReflRotType == nType) {
            return;
        }
        xglMatrixUnit(aMtx);
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

extern unsigned short D_004A90FA;

/* TODO: near-miss (33/46 words, LENGTH). Logic, field offsets and the
 * pi-override condition are verified against asm. Original schedules
 * the independent D_004A90FA load+shift LATE (right before its compare,
 * after the pModel+0x70 quadword copy); gcc here always hoists that
 * independent load earlier regardless of C statement order or pointer
 * staging -- looks like a pure scheduler tie-break. */
void nmlPacketAddScreen(void *pModel)
{
    u_int aBuf[4];
    char *pSrc;

    s_pPacket = xglPacketGetCurrent();
    sceVif1PkCnt(s_pPacket, 0);
    pSrc = (char *)pModel + 0x70;
    *(TI *)aBuf = *(TI *)pSrc;
    if ((D_004A90FA << 5) != 0x3800) {
        ((float *)aBuf)[3] = 3.141592741f;
    }
    sceVif1PkOpenUpkCode(s_pPacket, 0x3C7, 0x6C, 1, 1);
    sceVif1PkAddUpkData128(s_pPacket, *(TI *)aBuf);
    sceVif1PkCloseUpkCode(s_pPacket);
    sceVif1PkOpenUpkCode(s_pPacket, 0x3C6, 0x6C, 1, 1);
    sceVif1PkAddUpkData128(s_pPacket, *(TI *)((char *)pModel + 0x80));
    sceVif1PkCloseUpkCode(s_pPacket);
}
