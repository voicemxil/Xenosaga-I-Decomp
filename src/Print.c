/* Lightweight print-command wrappers. */

extern void endPrintInfoSet(void *info, void *type, int flags);
extern void subPrintLine(void *info, void *line);
extern void subPrintRibbon(void *info, void *ribbon);
extern void subPrintSprite(void *info, void *sprite);
extern void PrintCircleCore(int texture, void *data, int type);
extern void xglFontReloadTexture(void *context, int mode);
extern void endPrintInit(void);
extern void eMessageSpriteReset(void *context);
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

/* TODO: near-match (LENGTH) - callback iteration is recovered, but the
 * compiler emits a different loop/save schedule (28 original vs 30 built
 * instructions). Find the original iterator/source-control-flow shape. */
void PrintFlush(void *context)
{
    PRINT_FUNC *func;

    xglFontReloadTexture(context, 2);
    for (func = PrintFunc; func->func != 0; func++) {
        func->func(context, func->arg);
    }
    endPrintInit();
    xglFontReloadTexture(context, 1);
    eMessageSpriteReset(context);
}
