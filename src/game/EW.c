/* Debug widget layer - lightweight on-screen component pool, container plumbing and draw passes */

typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;

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

void mask_init(int *pMask)
{
    pMask[0] = 0x60;
    pMask[1] = 0x8000;
}

void container_init(EWCONTAINER *pContainer)
{
    pContainer->ppItems = 0;
    pContainer->nNum = 0;
    *(int *)((char *)pContainer + 8) = 0;
    *(int *)((char *)pContainer + 12) = 0;
}

extern void EW_drawComoponent(void *pPacket, EWCOMP *pComp);
extern void EW_setDrawEnv(void *pPacket);
extern void EW_sendPacket(void *pPacket);
extern void xglFontReloadTexture(void *pPacket, int nMode);
extern void xglFontPrintExtFunc(int nAddr, void (*pFunc)(void *), int nArg);
extern int xglPrimAddGifTagDirect(void *pPacket, void *pData, int nQwc);

typedef union {
    u32 word[4];
    u64 half[2];
} EWQWORD;

typedef struct {
    char pad00[4];
    short x;
    short y;
    u32 z;
    short width;
    short height;
    u32 flags;
    unsigned char color1;
    unsigned char color2;
    unsigned char color3;
    unsigned char pad17;
} EWMASK;

#define MASK_COLOR(q, color) do { \
    (q)->word[0] = (color); \
    (q)->word[1] = (color); \
    (q)->word[2] = (color); \
    (q)->word[3] = (color); \
} while (0)

#define MASK_XYZ(q, x, y, z) do { \
    (q)->word[0] = (x); \
    (q)->word[1] = (y); \
    (q)->word[2] = (z); \
    (q)->word[3] = 0; \
} while (0)

void mask_put(void **pPacketWork, EWMASK *pMask)
{
    EWQWORD *pBase;
    EWQWORD *p;
    int x0;
    int y0;
    int x1;
    int y1;
    u32 z;
    u32 *pData;
    unsigned char color;

    pBase = (EWQWORD *)(((u32)pPacketWork + 0x3F) & ~0xF);
    pData = (u32 *)((char *)pMask + 0x10);
    p = pBase;

    p->word[0] = 1;
    p->word[1] = 0x10000000;
    p->word[2] = -2;
    p->word[3] = 0;
    p++;
    p->word[0] = pData[0];
    p->word[1] = pData[1];
    p->word[2] = 0x42;
    p++;
    p->word[0] = 0x8004;
    p->word[1] = 0x20264000;
    p->word[2] = 0xFFFFFF51;
    p->word[3] = 0;
    p++;

    x0 = (pMask->x + 0x700) << 4;
    y0 = (pMask->y + 0x720) << 4;
    x1 = x0 + (pMask->width << 4);
    y1 = y0 + (pMask->height << 4);
    z = pMask->z;

    if (pData[0] & 0x100) {
        color = *((unsigned char *)pData + 6);
        MASK_COLOR(p, color); p++;
        MASK_XYZ(p, x0, y0, z); p++;
        MASK_COLOR(p, color); p++;
        MASK_XYZ(p, x1, y0, z); p++;
        color = *((unsigned char *)pData + 5);
        MASK_COLOR(p, color); p++;
        MASK_XYZ(p, x0, y1, z); p++;
    } else {
        color = *((unsigned char *)pData + 6);
        MASK_COLOR(p, color); p++;
        MASK_XYZ(p, x0, y0, z); p++;
        MASK_COLOR(p, color); p++;
        MASK_XYZ(p, x0, y1, z); p++;
        color = *((unsigned char *)pData + 5);
        MASK_COLOR(p, color); p++;
        MASK_XYZ(p, x1, y0, z); p++;
    }
    MASK_COLOR(p, color); p++;
    MASK_XYZ(p, x1, y1, z); p++;

    xglPrimAddGifTagDirect(*pPacketWork, pBase, p - pBase);
}

void sprt_put(void **pPacketWork, EWMASK *pSprite)
{
    EWQWORD *pBase;
    EWQWORD *p;
    unsigned short *pUV;
    int u0;
    int v0;
    int u1;
    int v1;
    int x0;
    int y0;
    int x1;
    int y1;
    u32 z;

    pBase = (EWQWORD *)(((u32)pPacketWork + 0x3F) & ~0xF);
    pUV = (unsigned short *)((char *)pSprite + 0x10);

    p = pBase + 1;
    p->half[0] = 0x7F1FC200;
    p->half[1] = 8;
    p = pBase + 2;
    p->half[0] = 0x44;
    p->half[1] = 0x42;
    p = pBase;
    p->word[0] = 1;
    p->word[1] = 0x20000000;
    p->word[2] = -0x12;
    p->word[3] = 0;
    p = pBase + 3;
    p->word[0] = 0x8001;
    p->word[1] = 0x50AB4000;
    p->word[2] = 0xFFF53531;
    p->word[3] = 0;

    u0 = pUV[2] << 4;
    v0 = pUV[3] << 4;
    x0 = (pSprite->x + 0x700) << 4;
    y0 = (pSprite->y + 0x720) << 4;
    u1 = u0 + (pSprite->width << 4);
    v1 = v0 + (pSprite->height << 4);
    x1 = x0 + (pSprite->width << 4);
    y1 = y0 + (pSprite->height << 4);
    z = pSprite->z;

    p = pBase + 4;
    p->word[0] = 0x80;
    p->word[1] = 0x80;
    p->word[2] = 0x80;
    p->word[3] = 0x80;
    p = pBase + 5;
    p->word[0] = u0;
    p->word[1] = v0;
    p = pBase + 6;
    MASK_XYZ(p, x0, y0, z);
    p = pBase + 7;
    p->word[0] = u1;
    p->word[1] = v1;
    p = pBase + 8;
    MASK_XYZ(p, x1, y1, z);

    xglPrimAddGifTagDirect(*pPacketWork, pBase, 9);
}

void set_clip(void **pPacketWork, EWMASK *pRect)
{
    EWQWORD *pBase;
    int x0;
    int y0;
    int x1;
    int y1;
    u64 packed;

    x0 = pRect->x;
    y0 = pRect->y;
    x1 = x0 + pRect->width - 1;
    y1 = y0 + pRect->height - 1;
    if (x0 < 0) {
        x0 = 0;
    }
    if (y0 < 0) {
        y0 = 0;
    }
    if (x1 >= 0x200) {
        x1 = 0x1FF;
    }
    if (y1 >= 0x1C0) {
        y1 = 0x1BF;
    }

    pBase = (EWQWORD *)(((u32)pPacketWork + 0x3F) & ~0xF);
    packed = (u64)x0 |
             ((u64)x1 << 16) |
             ((u64)y0 << 32) |
             ((u64)(y1 - 2) << 48);
    pBase[1].half[1] = 0x40;
    pBase[0].word[0] = 0x8001;
    pBase[0].word[1] = 0x10000000;
    pBase[0].word[2] = -2;
    pBase[0].word[3] = 0;
    pBase[1].half[0] = packed;
    xglPrimAddGifTagDirect(*pPacketWork, pBase, 2);
}

int checkN2(int value)
{
    int bit;
    int i;
    int result;

    i = 0;
    do {
        result = i;
        bit = 1 << i;
        if (value == bit) {
            return result;
        }
        i++;
    } while (i < 32);
    return 0;
}

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
