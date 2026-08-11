/* VIF1 packet construction helpers for the normal-map model renderer */

typedef unsigned char u_char;
typedef unsigned short u_short;
typedef unsigned int u_int;
typedef unsigned long u_long;

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
