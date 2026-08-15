/* Debug widget layer - lightweight on-screen component pool, container plumbing and draw passes */

typedef unsigned short u16;

typedef struct EWCOMP_t {
    u16 nFlags;                     /* 0x00 */
    u16 nType;                      /* 0x02 */
    u16 pad004[6];                  /* 0x04 */
    short nNum;                     /* 0x10 */
    short pad012;                   /* 0x12 */
    struct EWCOMP_t **ppItems;      /* 0x14 */
    u16 pad018[8];                 /* 0x18 */
} EWCOMP;

typedef struct {
    short nNum;                     /* 0x00 */
    short pad002;                   /* 0x02 */
    EWCOMP **ppItems;               /* 0x04 */
} EWCONTAINER;

EWCOMP ewComponent[64];
int ew_send_mode;

void frame_init(void *pFrame)
{
    *(short *)pFrame = 0;
    *((unsigned char *)pFrame + 2) = 0x60;
}

void sprt_init(void *pSprite)
{
    *(short *)pSprite = 0;
    *((unsigned char *)pSprite + 2) = 0x80;
}

extern void EW_drawComoponent(void *pPacket, EWCOMP *pComp);
extern void EW_setDrawEnv(void *pPacket);
extern void EW_sendPacket(void *pPacket);
extern void xglFontReloadTexture(void *pPacket, int nMode);
extern void xglFontPrintExtFunc(int nAddr, void (*pFunc)(void *), int nArg);

/* Free a component, clearing every child slot of a container first */
void EW_dispose(EWCOMP *pComp)
{
    EWCOMP **pp;
    int i;

    if (pComp->nType == 4) {
        if (pComp->ppItems != 0) {
            if (pComp->nNum > 0) {
                pp = pComp->ppItems;
                i = pComp->nNum;
                do {
                    if (*pp != 0) {
                        (*pp)->nFlags = 0;
                    }
                    pp++;
                    i--;
                } while (i != 0);
            }
        }
    }
    pComp->nFlags = 0;
}

/* Attach a component to a container slot, -1 picks the first free one */
int EW_addComponent(EWCOMP *pCont, int nIndex, EWCOMP *pComp)
{
    EWCOMP **pp;
    int nNum;
    int i;

    if (pCont->nType != 4 || (pp = pCont->ppItems) == 0) {
        return -1;
    }
    if (pComp == 0) {
        return -1;
    }
    nNum = pCont->nNum;
    if (nIndex < 0) {
        if (nIndex == -1) {
            for (i = 0; i < nNum; i++) {
                if (pp[i] == 0) {
                    nIndex = i;
                    break;
                }
            }
        } else {
            for (i = nNum - 1; i >= 0; i--) {
                if (pp[i] == 0) {
                    nIndex = i;
                    break;
                }
            }
        }
    }
    if (nIndex < 0) {
        nIndex = -1;
    } else if (nIndex < nNum) {
        pp[nIndex] = pComp;
        pComp->nFlags |= 0x2000;
    } else {
        nIndex = -1;
    }
    return nIndex;
}

/* Draw every marked child of a container */
void EW_drawContainer(void *pPacket, EWCONTAINER *pCont)
{
    EWCOMP **pp;
    int i;
    EWCOMP *p;

    if (pCont->ppItems != 0) {
        if (pCont->nNum > 0) {
            pp = pCont->ppItems;
            i = pCont->nNum;
            do {
                p = *pp;
                pp++;
                if (p != 0) {
                    if ((p->nFlags & 0x4000) != 0) {
                        EW_drawComoponent(pPacket, p);
                    }
                }
                i--;
            } while (i != 0);
        }
    }
}

/* Clear the whole component pool */
void EW_init(void)
{
    int i;

    for (i = 63; i >= 0; i--) {
        ewComponent[i].nFlags = 0;
    }
}

/* Emit every live component into the supplied packet */
void EW_sendPacket(void *pPacket)
{
    EWCOMP *p;
    int i;

    xglFontReloadTexture(pPacket, 2);
    ew_send_mode = 1;
    EW_setDrawEnv(pPacket);
    p = ewComponent;
    for (i = 63; i >= 0; i--) {
        if ((p->nFlags & 0xE000) == 0xC000) {
            EW_drawComoponent(pPacket, p);
        }
        p++;
    }
    xglFontReloadTexture(pPacket, 1);
    ew_send_mode = 0;
}

/* Immediate-mode draw of every live component */
void EW_draw(void)
{
    EWCOMP *p;
    int i;

    xglFontPrintExtFunc(0xFFFFF0, EW_sendPacket, 0);
    ew_send_mode = 2;
    p = ewComponent;
    for (i = 63; i >= 0; i--) {
        if ((p->nFlags & 0xE000) == 0xC000) {
            EW_drawComoponent(0, p);
        }
        p++;
    }
}
