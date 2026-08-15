/* VIF1 packet double-buffer management for the xgl engine */

typedef unsigned int u_int;

typedef struct XGLPACKET {
    u_int *pCur;
    u_int *pBase;
    u_int nState;
    u_int *pOpenDirect;
    u_int nUnk10;
    u_int *pOpenGif;
    u_int nUnk18;
    u_int nUnk1C;
    u_int *pEnd;
    u_int *pLimit;
} XGLPACKET;

void sceVif1PkInit(XGLPACKET *pPk, u_int *pBuff);
void sceVif1PkEnd(XGLPACKET *pPk, u_int nFlags);
u_int sceVif1PkTerminate(XGLPACKET *pPk);
void sceVif1PkReset(XGLPACKET *pPk);
void FlushCache(int nMode);
void xglDmaDirectSrcChain(u_int nCh, u_int nAddr);

XGLPACKET asPacketSource[2];
XGLPACKET *pCurrentPacket;
XGLPACKET *pSendPacket;

/* Return the packet currently being built */
XGLPACKET *xglPacketGetCurrent(void)
{
    return pCurrentPacket;
}

/* Set up the double-buffered packet builders */
void xglPacketInit(void)
{
    sceVif1PkInit(&asPacketSource[0], (u_int *)0xC00000);
    sceVif1PkInit(&asPacketSource[1], (u_int *)0xE00000);
    asPacketSource[0].pEnd = (u_int *)0xE00000;
    asPacketSource[0].pLimit = (u_int *)0xE00000;
    asPacketSource[1].pEnd = (u_int *)0x1000000;
    asPacketSource[1].pLimit = (u_int *)0x1000000;
    pCurrentPacket = &asPacketSource[0];
    pSendPacket = 0;
}

/* Finish the current packet and switch to the other build buffer */
void xglPacketMove(void)
{
    sceVif1PkEnd(pCurrentPacket, 0);
    sceVif1PkTerminate(pCurrentPacket);
    FlushCache(0);
    ((u_int *)pCurrentPacket)[9] = ((u_int *)pCurrentPacket)[8];
    pSendPacket = pCurrentPacket;
    if (pCurrentPacket == &asPacketSource[1]) {
        pCurrentPacket = &asPacketSource[0];
    } else {
        pCurrentPacket = &asPacketSource[1];
    }
    sceVif1PkReset(pCurrentPacket);
}

/* Kick the finished packet to VIF1 via source-chain DMA */
void xglPacketSend(void)
{
    if (pSendPacket != 0) {
        xglDmaDirectSrcChain(1, (u_int)pSendPacket->pBase);
    }
}

typedef unsigned long u_long;

int sceVif1PkCnt(XGLPACKET *pPk, int nArg);
int sceVif1PkAddCode(XGLPACKET *pPk, unsigned int nCode);
int sceVif1PkOpenDirectHLCode(XGLPACKET *pPk, int nArg);
int sceVif1PkAddDirectDataN(XGLPACKET *pPk, void *pData, int nQwc);
int sceVif1PkCloseDirectCode(XGLPACKET *pPk);

extern unsigned int TestPrim_0_004A8D00[];
extern unsigned short D_004A9100[];

/* TODO: near-miss (13 diffs of 45 words, pure SCHEDULING -- same
 * instruction multiset, no logic difference). Register width/pinning is
 * fully solved: `volatile u_long *p` for the 0x70000000 indexed stores
 * defeats CSE against the literal-address call argument; `t0`/`v1` must
 * be u_long (not unsigned int) so `t0 = 0xC800; t0 <<= 19;` emits a
 * plain `dsll` instead of the `dsll32+dsra32` 32-bit-truncation
 * round-trip; the 0xC800 and 6 constants need tied empty-asm
 * passthroughs (`"=r"(x):"0"(...)`) or 2.96 constant-folds the whole
 * shift into a single `lui`; pinning pPk into its own $4-tied copy
 * (`pPkA0`) right after the D_004A9100 lhu reproduces the original's
 * early, otherwise-inexplicable `move a0,s0` (freeing $a0 the moment
 * D_004A9100's base is no longer needed). What's left: gcc 2.96's
 * scheduler places the `sd` of the literal 6 and the two `or`s in a
 * different relative order than ours no matter how the equivalent C
 * statements are reordered (confirmed non-source-order-sensitive here,
 * unlike most of this file) -- the memory barriers above pin some of it
 * but not all. Likely needs a --swap-adjacent/--branch-likely-style
 * site-indexed fixer flag rather than more C-level massaging. */
/* Queue the interpolation-matrix DIRECT block into the current packet:
 * fixed template quadwords plus a VU MSCAL row built from D_004A9100 */
int xglPacketInterpolate(void)
{
    XGLPACKET *pPk;
    register volatile u_long *p __asm__("$9");
    register u_long v1 __asm__("$3");
    register unsigned int v0 __asm__("$2");
    register u_long t0 __asm__("$8");
    register u_long nSix __asm__("$7");
    register XGLPACKET *pPkA0 __asm__("$4");

    pPk = pCurrentPacket;
    sceVif1PkCnt(pPk, 0);
    sceVif1PkAddCode(pPk, 0x11000000);
    sceVif1PkOpenDirectHLCode(pPk, 0);
    sceVif1PkAddDirectDataN(pPk, TestPrim_0_004A8D00, 4);

    p = (volatile u_long *)0x70000000;
    v0 = D_004A9100[0];
    __asm__("" : "=r"(pPkA0) : "0"(pPk));
    __asm__("" : "=r"(t0) : "0"((u_long)0xC800));
    t0 <<= 19;
    __asm__("" : "=r"(nSix) : "0"((u_long)6));
    v0 <<= 5;
    p[1] = nSix;
    __asm__ __volatile__("" : : : "memory");
    v1 = 0x24020000;
    v1 |= v0;
    __asm__ __volatile__("" : : : "memory");
    v1 |= t0;
    p[0] = v1;
    sceVif1PkAddDirectDataN(pPkA0, (void *)0x70000000, 1);
    sceVif1PkAddDirectDataN(pPk, (char *)TestPrim_0_004A8D00 + 0x50, 7);
    return sceVif1PkCloseDirectCode(pPk);
}
