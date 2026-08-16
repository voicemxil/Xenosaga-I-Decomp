#include "matching.h"

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
    int nUnk008;                 /* 0x008 */
    char pad00C[0x70 - 0x0C];
    int nUnk070;                 /* 0x070 */
    char pad074[0x78 - 0x74];
    long long nUnk078;           /* 0x078 */
    int nUnk080;                 /* 0x080 */
    char pad084[0x94 - 0x84];
    int nUnk094;                 /* 0x094 */
    int nUnk098;                 /* 0x098 */
    int nUnk09C;                 /* 0x09C */
    char padA0[0xAC - 0xA0];
    int nUnk0AC;                 /* 0x0AC */
    int nUnk0B0;                 /* 0x0B0 */
    int nUnk0B4;                 /* 0x0B4 */
    char padB8[0xCC - 0xB8];
    int nUnk0CC;                 /* 0x0CC */
    int nUnk0D0;                 /* 0x0D0 */
    char padD4[0xD8 - 0xD4];
    int nUnk0D8;                 /* 0x0D8 */
    int nUnk0DC;                 /* 0x0DC */
    int nUnk0E0;                 /* 0x0E0 */
    int nUnk0E4;                 /* 0x0E4 */
    int nUnk0E8;                 /* 0x0E8 */
    char padEC[0xF0 - 0xEC];
    long long nUnk0F0;           /* 0x0F0 */
    int nUnk0F8;                 /* 0x0F8 */
    char padFC[0x118 - 0xFC];
    int nUnk118;                 /* 0x118 */
    char pad11C[0x1B8 - 0x11C];
    struct SCEMPEGREF *pRef0;    /* 0x1B8 */
    struct SCEMPEGREF *pRef3;    /* 0x1BC */
    char pad1C0[0x1C8 - 0x1C0];
    struct SCEMPEGREF *pRef1;    /* 0x1C8 */
    struct SCEMPEGREF *pRef4;    /* 0x1CC */
    char pad1D0[0x1D8 - 0x1D0];
    struct SCEMPEGREF *pRef2;    /* 0x1D8 */
    struct SCEMPEGREF *pRef5;    /* 0x1DC */
} SCEMPEGCTX;

struct SCEMPEGREF {
    char pad000[0x28];
    int  nUnk028;                /* 0x028 */
};

typedef struct {
    char pad000[8];
    int  nUnk008;                /* 0x008 */
    char pad00C[0x40 - 0x0C];
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

extern int sceMpegDemuxPssRing(int a0, int a1, int a2, int a3, int a4);

int sceMpegDemuxPss(int a0, int a1, int a2)
{
    return sceMpegDemuxPssRing(a0, a1, a2, 0, -1);
}

int sceMpegAddCallback(SCEMPEGHANDLE *mp, int idx, int a2, int a3)
{
    char *ctx;
    int off;
    char *p1;
    char *p0;
    int old;

    ctx = (char *)mp->pCtx;
    off = idx << 3;
    p1 = ctx + 12;
    p0 = ctx + off;
    p1 = p1 + off;
    *(int *)(p0 + 16) = a3;
    old = *(int *)p1;
    *(int *)p1 = a2;
    return old;
}


/* ------------------------------------------------------------------
 * Bitstream feed and picture retrieval.
 *
 * Note on the void return types below: gcc 2.9 only emits the tail-call
 * form (`j _target` with no frame at all) when the caller returns void
 * and its frame is empty.  Written as `return _sendDataToIPU(...);`
 * these grow a full addiu sp / sd ra / jal / ld ra / jr prologue and
 * epilogue -- 14 words against the original's 9 -- so the void return
 * is load-bearing, and it is also what the original build did.
 * ------------------------------------------------------------------ */

extern int _sendDataToIPU(SCEMPEGCTX *ctx, void *addr, int size);
extern int _getpic(SCEMPEGHANDLE *mp);
extern void _clearEach(SCEMPEGCTX *ctx);
extern int _initSeqAgain(SCEMPEGCTX *ctx);

/* Hand `size` bytes of bitstream to the IPU, rounded to a multiple of
 * 16 after a fixed 19-byte header allowance.  The `/ 16 * 16` really is
 * a SIGNED division: it is what produces the original's
 * slt/movn pair (add 15 when negative before the arithmetic shift). */
void sceMpegAddBs(SCEMPEGHANDLE *mp, void *addr, int size)
{
    _sendDataToIPU(mp->pCtx, addr, ((size + 19) / 16) * 16);
}

/* Decode into `addr` (forced into the uncached-accelerated 0x2000_0000
 * window).  Splitting the mask and the window bits into two statements
 * with the context load between them is load-bearing for the xy variant
 * below: it is what lets gcc reuse v0 for both constants instead of
 * materialising them up front. */
int sceMpegGetPicture(SCEMPEGHANDLE *mp, unsigned int addr, int a2)
{
    SCEMPEGCTX *c;

    c = mp->pCtx;
    addr = (addr & 0x0FFFFFFF) | 0x20000000;
    c->nUnk0B0 = 1;
    c->nUnk0D8 = addr;
    c->nUnk0E4 = a2;
    c->nUnk0DC = 0;
    c->nUnk0E0 = 0;
    return _getpic(mp);
}

int sceMpegGetPictureRAW8(SCEMPEGHANDLE *mp, unsigned int addr, int a2)
{
    SCEMPEGCTX *c;

    c = mp->pCtx;
    addr = (addr & 0x0FFFFFFF) | 0x20000000;
    c->nUnk0E4 = a2;
    c->nUnk0D8 = addr;
    c->nUnk0DC = 0;
    c->nUnk0B0 = 0;
    c->nUnk0E0 = 0;
    return _getpic(mp);
}

int sceMpegGetPictureRAW8xy(SCEMPEGHANDLE *mp, unsigned int addr, int w, int h)
{
    SCEMPEGCTX *c;
    int wh, w16, h16;

    wh = w * h;
    w16 = w << 4;
    addr = addr & 0x0FFFFFFF;
    c = mp->pCtx;
    h16 = h << 4;
    addr = addr | 0x20000000;
    c->nUnk0E0 = h16;
    c->nUnk0D8 = addr;
    c->nUnk0E4 = wh;
    c->nUnk0DC = w16;
    c->nUnk0B0 = 0;
    return _getpic(mp);
}

/* The handle really is copied into $v0 and everything after that reads it
 * through the copy: gcc otherwise leaves the handle in $a0, computes
 * a0 = ctx just before the _clearEach call and is one `move` short.
 * PIN alone does not stick on a plain dereference -- the copy has to be
 * forced with PASSTHRU.  The -1 stored into nUnk080 is likewise named in
 * its own $v1-pinned local so it is materialised in the prologue rather
 * than at the store.  The remaining `li`/`sd $s0` prologue transpose is a
 * pure post-reload reorder (FILE_FIX_FLAGS --swap-adjacent site 3). */
void sceMpegReset(SCEMPEGHANDLE *mp)
{
    SCEMPEGCTX *c;
    PIN(SCEMPEGHANDLE *h, "$2");
    PIN(int m1, "$3");

    PASSTHRU(h, mp);
    m1 = -1;
    c = h->pCtx;
    c->nUnk000 = 0;
    c->nUnk004 = 0;
    c->nUnk008 = 0;
    h->nUnk008 = 0;
    c->nUnk0AC = 0;
    c->nUnk080 = m1;
    _clearEach(c);
    c->nUnk118 = 0;
    _initSeqAgain(c);
}

int sceMpegClearRefBuff(SCEMPEGHANDLE *mp)
{
    SCEMPEGCTX *c;

    c = mp->pCtx;
    if (c->pRef0 != 0)
        c->pRef0->nUnk028 = 0;
    if (c->pRef1 != 0)
        c->pRef1->nUnk028 = 0;
    if (c->pRef2 != 0)
        c->pRef2->nUnk028 = 0;
    if (c->pRef3 != 0)
        c->pRef3->nUnk028 = 0;
    if (c->pRef4 != 0)
        c->pRef4->nUnk028 = 0;
    if (c->pRef5 != 0)
        c->pRef5->nUnk028 = 0;
    return 1;
}
