/* Lightweight print-command wrappers. */

extern void endPrintInfoSet(void *info, void *type, int flags);
extern void subPrintLine(void *info, void *line);
extern void subPrintRibbon(void *info, void *ribbon);
extern void subPrintSprite(void *info, void *sprite);
extern void PrintCircleCore(int texture, void *data, int type);
extern void xglFontReloadTexture(void *context, int mode);
extern void endPrintInit(void);
extern void eMessageSpriteReset(void);
extern void sceVif1PkAddDirectDataN(void *packet, void *data,
                                    unsigned int count);
extern unsigned short D_004A90F4[];
extern unsigned char D_004C37C0[];

typedef unsigned long PRINT_U64;
typedef unsigned int PRINT_U32;

typedef struct {
    short x;
    short y;
    PRINT_U32 texture;
    unsigned char r;
    unsigned char g;
    unsigned char b;
    unsigned char a;
    short width;
    short height;
    short u;
    short v;
} PRINT_BACK_SPRITE;
typedef struct {
    void *value;
    int field_04;
} PRINT_TYPE;

extern PRINT_TYPE D_0036CD1C[];

typedef struct {
    void (*func)(void *context, void *arg);
    void *arg;
} PRINT_FUNC;

extern PRINT_FUNC PrintFunc[];

int ePrintWHGet(void)
{
    return 0;
}

typedef struct {
    char pad_00[0x11];
    unsigned char type;
} PRINT_SPRITE;

void PrintLine(void *info, void *line)
{
    endPrintInfoSet(info, 0, 0);
    subPrintLine(info, line);
}

void PrintRibbon(void *info, void *ribbon)
{
    endPrintInfoSet(info, 0, 0);
    subPrintRibbon(info, ribbon);
}

void PrintCircle(void *circle, int type)
{
    PrintCircleCore(*(int *)circle, (char *)circle + 0x30, type);
}

void PrintSprite00(void *info, PRINT_SPRITE *sprite)
{
    endPrintInfoSet(info, D_0036CD1C[sprite->type].value, 1);
    subPrintSprite(info, sprite);
}

/* Run every registered print callback, then reset for the next frame.
 *
 * The loop is written with an explicit goto, and that is load-bearing:
 * a `while`/`for` here carries a NOTE_INSN_LOOP_BEG, and gcc 2.9x's
 * jump.c duplicate_loop_exit_test() then copies the exit test into the
 * preheader and inverts the loop (bnel at the bottom, one extra register
 * copy). The original build has the test at the TOP of the loop with an
 * unconditional branch back to it -- the shape you only get when there
 * is no loop note at all. Every `while (1) { ... break; ... }` and
 * `for (;;)` spelling still inverts; only the goto form does not. */
void PrintFlush(void *context)
{
    PRINT_FUNC *func;
    void (*f)(void *, void *);

    xglFontReloadTexture(context, 2);
    func = PrintFunc;
    f = func->func;
loop:
    if (f != 0) {
        f(context, func->arg);
        func++;
        f = func->func;
        goto loop;
    }
    endPrintInit();
    xglFontReloadTexture(context, 1);
    eMessageSpriteReset();
}

void PrintBackSprite2(void **packet, PRINT_BACK_SPRITE *sprite)
{
    PRINT_U64 *direct;
    PRINT_U32 *entry;
    PRINT_U64 frame;
    short x;
    short y;

    frame = 0x24020000 | (D_004A90F4[0] << 5) |
            (((PRINT_U64)0x20000006 << 32) | 0x40000000);
    direct = (PRINT_U64 *)((char *)packet + 0x30);
    direct[0] = ((PRINT_U64)0x10000000 << 32) | 5;
    direct[1] = 0xE;
    direct[2] = 0x44;
    direct[3] = 0x42;
    direct[4] = 0x7FDFF0;
    direct[5] = 8;
    direct[6] = 0x60;
    direct[7] = 0x14;
    direct[8] = frame;
    direct[9] = 6;
    direct[10] = 0x3000D;
    direct[11] = 0x47;
    sceVif1PkAddDirectDataN(*packet, direct, 6);

    entry = (PRINT_U32 *)direct;
    entry[0] = 0x8001;
    entry[1] = 0x50AB4000;
    entry[2] = 0x53531;
    entry[3] = 0;

    x = sprite->x;
    y = sprite->y;
    entry = (PRINT_U32 *)((char *)packet + 0x40);
    entry[0] = sprite->r;
    entry[1] = sprite->g;
    entry[2] = sprite->b;
    entry[3] = sprite->a;

    entry = (PRINT_U32 *)((char *)packet + 0x50);
    entry[0] = 0;
    entry[1] = 0;
    entry[3] = entry[2] = 0;

    entry = (PRINT_U32 *)((char *)packet + 0x60);
    entry[0] = x * 16 + 0x6FF8;
    entry[1] = y * 16 + 0x71F8;
    entry[2] = sprite->texture;
    entry[3] = 0;

    entry = (PRINT_U32 *)((char *)packet + 0x70);
    entry[0] = sprite->u * 16;
    entry[1] = sprite->v * 16;
    entry[3] = entry[2] = 0;

    entry = (PRINT_U32 *)((char *)packet + 0x80);
    entry[0] = (x + sprite->width) * 16 + 0x6FF8;
    entry[1] = (y + sprite->height) * 16 + 0x71F8;
    entry[2] = sprite->texture;
    entry[3] = 0;
    sceVif1PkAddDirectDataN(*packet, direct, 6);
}

void PrintBackSprite(void **packet, void *data)
{
    PRINT_U64 *direct;
    PRINT_U32 *entry;
    PRINT_U64 frame;
    short indices[8];
    short *index;
    PRINT_U32 *vertex;
    PRINT_U32 color;
    float one;
    int i;

    frame = 0x24020000 | (D_004A90F4[0] << 5) |
            (((PRINT_U64)0x20000006 << 32) | 0x40000000);
    direct = (PRINT_U64 *)((char *)packet + 0x30);
    direct[4] = 0x7FDFF0;
    direct[5] = 8;
    direct[8] = frame;
    direct[0] = ((PRINT_U64)0x10000000 << 32) | 5;
    direct[3] = 0x42;
    direct[1] = 0xE;
    direct[2] = 0x44;
    direct[6] = 0x60;
    direct[7] = 0x14;
    direct[9] = 6;
    direct[10] = 0x30000;
    direct[11] = 0x47;
    sceVif1PkAddDirectDataN(*packet, direct, 6);

    one = 1.0f;
    __builtin_memcpy(indices, D_004C37C0, sizeof(indices));
    entry = (PRINT_U32 *)direct;
    entry[0] = 0x8004;
    entry[1] = 0x302A4000;
    entry[2] = 0x521;
    entry[3] = 0;

    entry = (PRINT_U32 *)((char *)packet + 0x40);
    color = 0x80;
    vertex = (PRINT_U32 *)data;
    index = indices + 1;
    i = 0;
loop:
    {
        short index0;
        short index1;

        index0 = index[-1];
        i++;
        index1 = index[0];
        index += 2;
        entry[3] = color;
        entry[2] = color;
        entry[1] = color;
        entry[0] = color;
        entry += 4;
        ((float *)entry)[0] = *(float *)((char *)data + index0 * 4 + 0x40);
        entry[3] = 0;
        ((float *)entry)[1] = *(float *)((char *)data + index1 * 4 + 0x40);
        ((float *)entry)[2] = one;
        entry += 4;
        entry[0] = vertex[0];
        entry[1] = vertex[1];
        entry[2] = vertex[2];
        entry[3] = 0;
        entry += 4;
        vertex += 4;
    }
    if (i < 4) {
        goto loop;
    }
    sceVif1PkAddDirectDataN(*packet, direct, 0xD);
}

typedef struct {
    short x;                /* 0x00 */
    short y;                /* 0x02 */
    PRINT_U32 color;        /* 0x04 */
    unsigned char r;        /* 0x08 */
    unsigned char g;        /* 0x09 */
    unsigned char b;        /* 0x0A */
    unsigned char a;        /* 0x0B */
} PRINT_POINT;

/* A single GS POINT primitive: three quadwords of GIFtag + RGBAQ + XYZ2. */
void PrintPoint(void **packet, PRINT_POINT *point)
{
    PRINT_U32 *direct;
    /* Initialised on the declaration, not after the call: as a statement
       the address computation schedules ahead of the first store's
       constant instead of behind it. */
    PRINT_U32 *entry = (PRINT_U32 *)((char *)packet + 0x30);

    endPrintInfoSet(packet, 0, 0);
    entry[0] = 0x8001;
    entry[1] = 0x20204000;
    entry[2] = 0x51;
    entry[3] = 0;
    direct = entry;

    entry = (PRINT_U32 *)((char *)packet + 0x40);
    entry[0] = point->r;
    entry[1] = point->g;
    entry[2] = point->b;
    entry[3] = point->a;

    entry = (PRINT_U32 *)((char *)packet + 0x50);
    entry[0] = (point->x << 4) + 0x6FF8;
    entry[1] = (point->y << 4) + 0x71F8;
    entry[2] = point->color & 0xFFFFFF;
    entry[3] = 0;

    sceVif1PkAddDirectDataN(*packet, direct, 3);
}
