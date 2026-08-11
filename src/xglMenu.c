/* System menu drawing for the xgl engine */

typedef struct {
    char pad0[4];           /* 0x00 */
    unsigned char nType;    /* 0x04 */
    char pad5[2];           /* 0x05 */
    unsigned char nActive;  /* 0x07 */
    char pad8[0x78];        /* 0x08 */
} XGLMENU;

XGLMENU menutbl[16];

void xglMenuDrawType1(XGLMENU *pMenu, void *pDest);

/* Draw a type 0 menu entry: nothing to show, so retire it */
void xglMenuDrawType0(XGLMENU *pMenu, void *pDest)
{
    pMenu->nActive = 0;
}

/* Draw every active menu entry according to its type */
void xglMenuDraw(void)
{
    XGLMENU *pMenu;
    int i;

    pMenu = menutbl;
    for (i = 15; i >= 0; i--) {
        if (pMenu->nActive != 0) {
            switch (pMenu->nType) {
            case 0:
                xglMenuDrawType0(pMenu, (void *)0x70000000);
                break;
            case 1:
                xglMenuDrawType1(pMenu, (void *)0x70000000);
                break;
            }
        }
        pMenu++;
    }
}

/* Retire every menu entry */
void xglMenuInitial(void)
{
    int i;

    for (i = 15; i >= 0; i--) {
        menutbl[i].nActive = 0;
    }
}
