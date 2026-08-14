/* Primitive-packet builders */

typedef struct {
    void *pBase;         /* 0x00 */
    char pad04[8];
    void *pOpen;         /* 0x0C */
} XGLPRIM;

void xglPrimAddGifTagDirect(void *pBase, void *pOpen, int nData);

/* Add a GIF tag through the packet's base/open pointers */
void xglPrimAddGifTag(XGLPRIM *pPrim, int nData)
{
    xglPrimAddGifTagDirect(pPrim->pBase, pPrim->pOpen, nData);
}
