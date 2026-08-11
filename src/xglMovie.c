/* MPEG/IPU initialization wrappers */

void *xglMpeg2InfoInit2(void *, void *, int);
void sceIpuInit(void);

void *xglMpeg2InfoInit(void *pInfo, void *pData)
{
    return xglMpeg2InfoInit2(pInfo, pData, 0x100000);
}

void xglMovieInit(void)
{
    sceIpuInit();
}
