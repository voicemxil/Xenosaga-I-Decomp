#include "matching.h"
/* System menu drawing for the xgl engine */

/* One selectable line: a printf argument and the submenu it opens */
typedef struct {
    int nArg;                /* 0x00 */
    void *pNext;             /* 0x04 */
} XGLMENUITEM;

/* One level of the menu tree */
typedef struct {
    unsigned char nCount;    /* 0x00 */
    char pad01[3];
    char nSel;               /* 0x04 */
    char pad05[0x13];
    XGLMENUITEM *pItems;     /* 0x18 */
} XGLMENUNODE;

/* The per-level node pointers and their line pitches */
typedef struct {
    XGLMENUNODE *apNode[8];  /* 0x00 */
    short aStep[8];          /* 0x20 */
} XGLMENULIST;

/* The block at +0x20 the drawer keeps a pointer to */
typedef struct {
    int pad00[2];            /* 0x00 */
    unsigned int nMaxDepth;  /* 0x08 */
} XGLMENUSUB;

typedef struct {
    char pad0[4];           /* 0x00 */
    unsigned char nType;    /* 0x04 */
    char pad5[2];           /* 0x05 */
    unsigned char nActive;  /* 0x07 */
    char pad8[0x18];        /* 0x08 */
    XGLMENUSUB sSub;        /* 0x20 */
    XGLMENULIST sList;      /* 0x2C */
    char pad5C[0x24];       /* 0x5C */
} XGLMENU;

/* Cursor state the type 1 drawer walks down the tree with */
typedef struct {
    int pad0;               /* 0x00 */
    int nX;                 /* 0x04 */
    int nY;                 /* 0x08 */
} XGLMENUDRAW;

extern char D_004DC2B8[];
extern char D_004DC2C0[];
extern char D_004DC2C8[];
void xglFontDebugPrintf(int nX, int nY, char *pFmt, int nArg);

/* Print one level of a type 1 menu and recurse into the open submenu */
void xglMenuDrawType1Sub(XGLMENU *pMenu, XGLMENUDRAW *pDraw, int nDepth)
{
    XGLMENUNODE *pNode;
    XGLMENUITEM *pItem;
    XGLMENUSUB *pSub;
    XGLMENULIST *pList;
    int i;

    i = 0;
    pSub = &pMenu->sSub;
    pList = &pMenu->sList;
    pNode = pList->apNode[nDepth];
    if (pNode->nCount == 0) {
        return;
    }
    do {
        pItem = &pNode->pItems[i];
        if (i == pNode->nSel) {
            xglFontDebugPrintf(pDraw->nX, pDraw->nY, D_004DC2B8, pItem->nArg);
        } else if (pItem->pNext == (void *)-1) {
            xglFontDebugPrintf(pDraw->nX, pDraw->nY, D_004DC2C0, pItem->nArg);
        } else {
            xglFontDebugPrintf(pDraw->nX, pDraw->nY, D_004DC2C8, pItem->nArg);
        }
        pDraw->nY += pList->aStep[nDepth];
        if (nDepth < pSub->nMaxDepth
            && (void *)pList->apNode[nDepth + 1] == pItem->pNext) {
            pDraw->nX += 8;
            xglMenuDrawType1Sub(pMenu, pDraw, nDepth + 1);
            pDraw->nX -= 8;
        }
        i++;
    } while (i < pNode->nCount);
}

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
