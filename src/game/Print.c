/* Lightweight print-command wrappers. */

extern void endPrintInfoSet(void *info, void *type, int flags);
extern void subPrintLine(void *info, void *line);
extern void subPrintRibbon(void *info, void *ribbon);
extern void subPrintSprite(void *info, void *sprite);
extern void PrintCircleCore(int texture, void *data, int type);
extern void xglFontReloadTexture(void *context, int mode);
extern void endPrintInit(void);
extern void eMessageSpriteReset(void);
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
