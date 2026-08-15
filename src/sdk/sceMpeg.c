/* PS2 SDK sceMpeg (libmpeg) handle accessors.
 *
 * A "handle" passed to these calls is an opaque struct whose real
 * decode context lives via a pointer stored at offset 0x40; every
 * accessor here dereferences that pointer first, then reads/writes a
 * fixed field inside the inner context. Field names are offset-based
 * (house style, see mpeg.c's MPEGSTREAM) since the real SDK field
 * names are not recoverable from the object code alone.
 */

typedef struct {
    int nUnk000;                 /* 0x000 */
    int nUnk004;                 /* 0x004 */
    char pad008[0x70 - 0x08];
    int nUnk070;                 /* 0x070 */
    char pad074[0x78 - 0x74];
    long long nUnk078;           /* 0x078 */
    char pad080[0x94 - 0x80];
    int nUnk094;                 /* 0x094 */
    int nUnk098;                 /* 0x098 */
    int nUnk09C;                 /* 0x09C */
    char padA0[0xB4 - 0xA0];
    int nUnk0B4;                 /* 0x0B4 */
    char padB8[0xCC - 0xB8];
    int nUnk0CC;                 /* 0x0CC */
    int nUnk0D0;                 /* 0x0D0 */
    char padD4[0xD8 - 0xD4];
    int nUnk0D8;                 /* 0x0D8 */
    char padDC[0xE8 - 0xDC];
    int nUnk0E8;                 /* 0x0E8 */
    char padEC[0xF0 - 0xEC];
    long long nUnk0F0;           /* 0x0F0 */
    int nUnk0F8;                 /* 0x0F8 */
} SCEMPEGCTX;

typedef struct {
    char pad[0x40];
    SCEMPEGCTX *pCtx;             /* 0x040 */
} SCEMPEGHANDLE;

int sceMpegDelete(void *mp)
{
    return 1;
}

int sceMpegIsEnd(SCEMPEGHANDLE *mp)
{
    return mp->pCtx->nUnk000;
}

void *sceMpegDispCenterOffX(SCEMPEGHANDLE *mp)
{
    return (char *)mp->pCtx + 0xB4;
}

void *sceMpegDispCenterOffY(SCEMPEGHANDLE *mp)
{
    return (char *)mp->pCtx + 0xB4;
}

int sceMpegDispHeight(SCEMPEGHANDLE *mp)
{
    return mp->pCtx->nUnk0D0;
}

int sceMpegDispWidth(SCEMPEGHANDLE *mp)
{
    return mp->pCtx->nUnk0CC;
}

void sceMpegSetImageBuff(SCEMPEGHANDLE *mp, int buff)
{
    mp->pCtx->nUnk0D8 = buff;
}

int sceSetBrokenLink(SCEMPEGHANDLE *mp, int flag)
{
    int old;

    old = mp->pCtx->nUnk0E8;
    mp->pCtx->nUnk0E8 = flag;
    return old;
}

int sceMpegIsRefBuffEmpty(SCEMPEGHANDLE *mp)
{
    return mp->pCtx->nUnk004 == 0;
}

void sceMpegResetDefaultPtsGap(SCEMPEGHANDLE *mp)
{
    mp->pCtx->nUnk078 = 0;
    mp->pCtx->nUnk070 = 0;
}

void sceMpegSetDefaultPtsGap(SCEMPEGHANDLE *mp, long long gap)
{
    int one;

    one = 1;
    mp->pCtx->nUnk078 = gap;
    mp->pCtx->nUnk070 = one;
}

void sceMpegSetDecodeMode(SCEMPEGHANDLE *mp, int a1, int a2, int a3)
{
    mp->pCtx->nUnk09C = a3;
    mp->pCtx->nUnk094 = a1;
    mp->pCtx->nUnk098 = a2;
}

void sceMpegGetDecodeMode(SCEMPEGHANDLE *mp, int *pA1, int *pA2, int *pA3)
{
    SCEMPEGCTX *ctx;

    ctx = mp->pCtx;
    *pA1 = ctx->nUnk094;
    *pA2 = ctx->nUnk098;
    *pA3 = ctx->nUnk09C;
}

int sceSetPtm(SCEMPEGHANDLE *mp, long long ptm)
{
    mp->pCtx->nUnk0F8 = 1;
    mp->pCtx->nUnk0F0 = ptm;
    return 1;
}
