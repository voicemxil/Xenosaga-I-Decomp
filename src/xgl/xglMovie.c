/* MPEG/IPU initialization wrappers */

typedef struct {
    char pad00[0x46];
    short nUnk46;        /* 0x46 */
    char pad48[0x52];
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
