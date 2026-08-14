/* MPEG/IPU initialization wrappers */

typedef struct {
    char pad00[0x46];
    short nUnk46;        /* 0x46 */
    char pad48[0x51];
    unsigned char nUnk99;/* 0x99 */
    unsigned char nUnk9A;/* 0x9A */
    char pad9B[0x1D];
    void *pIpuBuf;       /* 0xB8 */
    void *pSpuBuf;       /* 0xBC */
    char padC0[9];
    char nUnkC9;         /* 0xC9 */
    unsigned char nUnkCA;/* 0xCA */
    unsigned char nUnkCB;/* 0xCB */
} XGLMOVIEINFO;

void *xglMpeg2InfoInit2(void *, void *, int);
void sceIpuInit(void);
void xglCdStreamParamInit(void *);
int xglCdStreamClose(void *);
void setD4_CHCR(int nVal);
int sceIpuSync(int nMode, int nTimeout);
int sceMpegReset(void *);
int sceMpegDelete(void *);
int SsdStopVagStream(int);
int SsdDisposeVagStream(void);
int SsdGetResultValue(int *);

void *xglMpeg2InfoInit(void *pInfo, void *pData)
{
    return xglMpeg2InfoInit2(pInfo, pData, 0x100000);
}

void xglMovieInit(void)
{
    sceIpuInit();
}

/* Reset a movie-info block: stream params, scratchpad buffer pointers
 * and the playback bookkeeping fields */
void xglMovieInfoInit(XGLMOVIEINFO *pInfo)
{
    int nInit = -1;

    xglCdStreamParamInit(pInfo);
    pInfo->pIpuBuf = (void *)0x70000040;
    pInfo->pSpuBuf = (void *)0x70001000;
    pInfo->nUnk46 = nInit;
    pInfo->nUnk9A = 0;
    pInfo->nUnkC9 = nInit;
    pInfo->nUnkCA = 0;
    pInfo->nUnkCB = 0;
}

/* Stop the IPU and its DMA channel, waiting for the decoder to idle */
int xglMovieClose(XGLMOVIEINFO *pInfo)
{
    int nRet;
    int nSync;

    if (pInfo->nUnk99 != 0) {
        xglCdStreamClose(pInfo);
    }
    setD4_CHCR(0);
    *(volatile unsigned int *)0x10002010 = 0x40000000;
    if (sceIpuSync(0, 64) < 0) {
        return -1;
    }
    *(volatile unsigned int *)0x10002000 = 0;
    nSync = sceIpuSync(0, 64);
    nRet = -1;
    if (!(nSync < 0)) {
        nRet = 0;
    }
    return nRet;
}

/* Tear down the MPEG decoder and VAG audio stream for a movie */
int xglMpeg2Close(XGLMOVIEINFO *pInfo)
{
    int nResult;

    sceMpegReset((char *)pInfo + 48);
    sceMpegDelete((char *)pInfo + 48);
    SsdStopVagStream(0);
    while (SsdGetResultValue(&nResult) < 0) {
    }
    SsdDisposeVagStream();
    while (SsdGetResultValue(&nResult) < 0) {
    }
    xglCdStreamClose(pInfo);
    return 0;
}
