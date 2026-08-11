/* Screenshot / capture-mode draw passes for actors, shadows and map units */

typedef unsigned char u8;
typedef unsigned short u16;

typedef struct {
    float fRate;            /* 0x00 */
    u8 pad04[0x7C];
    int nFlags;             /* 0x80 */
    u8 pad84[0x1C];
    volatile float fAlphaRate; /* 0xA0 */
    u8 padA4[0x14];
    volatile int nAlpha;    /* 0xB8 */
} ACT_WORK;

typedef struct {
    u8 pad80[0x80];
    u8 nLayer;              /* 0x80 */
} ACT_OWNER;

typedef struct {
    int nFlags;             /* 0x000 */
    u8 pad004[0x4];
    void *pFilter;          /* 0x008 */
    u8 pad00C[0x7A];
    short nAlive;           /* 0x086 */
    u8 pad088[0x874];
    ACT_OWNER *pOwner;      /* 0x8FC */
    u8 pad900[0x20];
    ACT_WORK work;          /* 0x920 */
    u8 pad9DC[0x94];
} ACTOR;

typedef struct {
    u8 pad000[0xA4];
    short nState;           /* 0xA4 */
    u8 padA6[0x25A];
} MAP_UNIT;

typedef struct {
    u8 pad0[0x10];
    int nFlags;             /* 0x10 */
} GAME_LOOP_STATE;

extern ACTOR actor[];
extern MAP_UNIT MapUnit[];
extern GAME_LOOP_STATE GameLoopState;
extern void ACT_filterGuno(void);

u16 layer;
int alpha;
int transparency;

extern void ACT_modelDraw(ACTOR *a);
extern void ACT_DrawShadowBegin(void);
extern void ACT_DrawShadow(ACTOR *a);
extern void ACT_DrawShadowEnd(void);
extern void MAP_drawUnitAt(MAP_UNIT *u);
extern int F2I(float f);
extern void tyaCaptureMain(int mode);
extern u16 xglFontGetFlags(void);
extern void xglFontSetFlags(u16 flags);

/* Capture layer callback that draws nothing */
void tyaCaptureNull(void)
{
}

/* Draw every visible actor belonging to the current capture layer */
void tyaCaptureActor(void)
{
    ACTOR *a = actor;
    ACT_WORK *pWork;
    int i;

    for (i = 0; i < 0x40; i++, a++) {
        if (a->nAlive == 0) {
            continue;
        }
        if (a->nFlags & 8) {
            continue;
        }
        if (layer != i + 0x200) {
            if (a->pOwner == 0) {
                continue;
            }
            if (layer != a->pOwner->nLayer + 0x200) {
                continue;
            }
        }
        if (a->pFilter == ACT_filterGuno) {
            alpha = 1;
            transparency = F2I(a->work.fRate * 128.0f);
        } else {
            pWork = &a->work;
            if (pWork->nFlags & 1) {
                alpha = pWork->nAlpha;
                transparency = F2I(pWork->fAlphaRate);
            } else {
                alpha = 0;
            }
        }
        ACT_modelDraw(a);
    }
}

/* Draw the shadows of every actor on the current capture layer */
void tyaCaptureShadow(void)
{
    ACTOR *a = actor;
    int i;

    ACT_DrawShadowBegin();
    for (i = 0; i < 0x40; i++, a++) {
        if (a->nAlive == 0) {
            continue;
        }
        if (a->nFlags & 8) {
            continue;
        }
        if (layer != i + 0x200) {
            continue;
        }
        if (a->nFlags & 0x20) {
            ACT_DrawShadow(a);
        }
    }
    ACT_DrawShadowEnd();
}

/* Draw every map unit belonging to the current capture layer */
void tyaCaptureUnit(void)
{
    MAP_UNIT *u = MapUnit;
    int i;

    for (i = 0; i < 0x40; i++, u++) {
        if (u->nState >= 0) {
            if (layer == i + 0x300) {
                MAP_drawUnitAt(u);
            }
        }
    }
}

/* Finish a capture pass and restore the normal draw state */
void tyaCaptureEnd(void)
{
    tyaCaptureMain(1);
    xglFontSetFlags(xglFontGetFlags() | 3);
    GameLoopState.nFlags &= 0xFAFFFFFF;
}
