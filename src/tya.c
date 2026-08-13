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

typedef struct {
    int state;
    u8 pad04[4];
    short x;
    short y;
    short width;
    short height;
    unsigned long long color;
    short source_x;
    short source_y;
    short source_width;
    short source_height;
    int image;
    u8 pad24[4];
    u8 drawing;
} TYA_UML_DISP;

typedef struct {
    u8 pad00[8];
    int state;
    u8 pad0C[4];
} TYA_UML_IMAGE;

extern TYA_UML_IMAGE image_00A10BC0[];
extern int buffer_00A13660[];

/* Reset one UML display parameter block to its default rectangle and color. */
int tyaUmlDispParamReset(TYA_UML_DISP *disp)
{
    disp->color = 0xFFFF00ULL;
    disp->state = -1;
    disp->x = 0;
    disp->y = 0;
    disp->width = 0x200;
    disp->height = 0x1C0;
    disp->source_width = 0x200;
    disp->source_height = 0x1C0;
    disp->source_x = 0;
    disp->source_y = 0;
    disp->image = -1;
    return 0;
}

/* Initialize the UML image slots and retain their shared output buffer. */
void tyaUmlDispInit2(int buffer)
{
    int i;

    buffer_00A13660[0] = buffer;
    for (i = 7; i >= 0; i--) {
        image_00A10BC0[i].state = 0;
    }
}

/* Initialize UML display output in the default memory region. */
void tyaUmlDispInit(void)
{
    tyaUmlDispInit2(0x1000000);
}

extern void *memset(void *destination, int value, unsigned int size);
extern void tyaUmlDispType3(TYA_UML_DISP *disp);
extern int tyaUmlDispLoad(TYA_UML_DISP *disp);

/* Refresh or load one UML display image when its requested state changes. */
int tyaUmlDispMain(TYA_UML_DISP *disp)
{
    int result = -1;

    if (disp != 0) {
        if (disp->state < 0) {
            memset((void *)buffer_00A13660[0], 0, 0x64);
            memset(image_00A10BC0, 0, 0x80);
            tyaUmlDispType3(disp);
            result = 0;
        } else if (disp->state != disp->image) {
            return tyaUmlDispLoad(disp);
        } else {
            tyaUmlDispType3(disp);
            result = 0;
        }
    }
    return result;
}

typedef unsigned long tya_u64;

typedef struct {
    u8 pad00[4];
    short width;
    short height;
    u8 pad08[0x50];
} TYA_RENDER_SIZE;

typedef struct {
    void *packet;
    u8 pad04[0x2C];
    tya_u64 direct[4];
} TYA_UML_PACKET;

extern TYA_RENDER_SIZE sRender;
extern void sceVif1PkAddDirectDataN(void *packet, void *data, int count);

/* Append the UML display's direct GS-coordinate packet. */
void tyaUmlDispType3Sub1(void *work)
{
    TYA_UML_PACKET *packet = (TYA_UML_PACKET *)work;
    tya_u64 *direct = packet->direct;

    direct[0] = 0x1000000000008001UL;
    direct[1] = 0xE;
    direct[2] = ((tya_u64)(sRender.width - 1) << 16) |
                ((tya_u64)(sRender.height - 1) << 48);
    direct[3] = 0x40;
    sceVif1PkAddDirectDataN(packet->packet, direct, 2);
}

typedef struct {
    u8 pad000[0xA4];
    short state;
    u8 pad0A6[0x56];
    ACTOR *owner;
    u8 pad100[0x200];
} TYA_MAP_UNIT_LINK;

/* Draw map units attached to the actor selected by the current layer. */
void tyaCaptureUnit2(void)
{
    ACTOR *a = actor;
    ACTOR *owner = (ACTOR *)-1;
    int i;

    for (i = 0; i < 0x40; i++, a++) {
        if (a->nAlive != 0 && (a->nFlags & 8) == 0 &&
            layer == i + 0x200) {
            owner = a;
            break;
        }
    }
    {
        TYA_MAP_UNIT_LINK *unit = (TYA_MAP_UNIT_LINK *)MapUnit;

        for (i = 0x3F; i >= 0; i--, unit++) {
            if (unit->state >= 0 && unit->owner == owner) {
                MAP_drawUnitAt((MAP_UNIT *)unit);
            }
        }
    }
}

extern void tyaUmlDispType3Sub0(void *work);
extern void xglFontPrintExtFunc(void *text, void (*callback)(void *), void *work);
extern void xglFontPrint(int x, int y, void *text, void *output);
extern int D_00A13380[];

/* Draw the two text passes and their direct-packet callbacks for UML type 3. */
void tyaUmlDispType3(TYA_UML_DISP *disp)
{
    int x;
    int y;

    disp->drawing = 0;
    x = disp->x - 0x700;
    y = disp->y - 0x720;
    xglFontPrintExtFunc((void *)(disp->color + 1), tyaUmlDispType3Sub0, disp);
    xglFontPrint(0, 0, (void *)(disp->color + 0xF), D_00A13380);
    xglFontPrint(x - disp->source_x, y - disp->source_y,
                 (void *)(disp->color + 0xF),
                 (char *)buffer_00A13660[0] + 0x60);
    xglFontPrintExtFunc((void *)(disp->color + 1), tyaUmlDispType3Sub1, disp);
}
