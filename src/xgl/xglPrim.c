/* Primitive-packet builders */

typedef struct {
    void *pBase;         /* 0x00 */
    char pad04[8];
    void *pOpen;         /* 0x0C */
} XGLPRIM;

typedef struct {
    char pad00[8];
    int nUnk08;          /* 0x08 */
    int nUnk0C;          /* 0x0C */
} XGLPK;

int xglPrimAddGifTagDirect(XGLPK *pPk, void *pData, int nQwc);

/* Add a GIF tag through the packet's base/open pointers */
void xglPrimAddGifTag(XGLPRIM *pPrim, int nData)
{
    xglPrimAddGifTagDirect(pPrim->pBase, pPrim->pOpen, nData);
}

XGLPK *xglPacketGetCurrent(void);
int sceVif1PkCnt(XGLPK *pPk, int nArg);
int sceVif1PkAddCode(XGLPK *pPk, unsigned int nCode);
int sceVif1PkOpenDirectHLCode(XGLPK *pPk, int nArg);
int sceVif1PkAddDirectDataN(XGLPK *pPk, void *pData, int nQwc);
int sceVif1PkCloseDirectHLCode(XGLPK *pPk);

/* Add DIRECT GIF data to a VIF1 packet, opening a fresh DIRECT HL block
 * unless one is already open on the packet */
int xglPrimAddGifTagDirect(XGLPK *pPk, void *pData, int nQwc)
{
    if (pPk == 0) {
        pPk = xglPacketGetCurrent();
    }
    if (pPk->nUnk08 != 0 && pPk->nUnk0C != 0) {
        return sceVif1PkAddDirectDataN(pPk, pData, nQwc);
    }
    sceVif1PkCnt(pPk, 0);
    sceVif1PkAddCode(pPk, 0x11000000);
    sceVif1PkOpenDirectHLCode(pPk, 0);
    sceVif1PkAddDirectDataN(pPk, pData, nQwc);
    return sceVif1PkCloseDirectHLCode(pPk);
}
