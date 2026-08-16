/* tskSeisan* - the settlement (Seisan) result screen tasks.
 *
 * Each entry point is a task handler called with the task node (whose
 * nState at +0x10 drives the two-phase set-up/step split) and the
 * screen's own work block. */

/* A 0x28-byte eSprite object; eSpriteMain does the drawing. */
typedef struct {
    char pad00[4];
    short nX;                  /* 0x04 */
    short nY;                  /* 0x06 */
    int nColor;                /* 0x08 */
    char pad0C[0x28 - 0x0C];
} ESPRITE;

/* A 0x44-byte eMessage object. */
typedef struct {
    char pad00;
    unsigned char nFont;       /* 0x01 */
    char pad02[2];
    short nX;                  /* 0x04 */
    short nY;                  /* 0x06 */
    int nColor;                /* 0x08 */
    char pad0C[0x18 - 0x0C];
    char *pText;               /* 0x18 */
    char pad1C[0x44 - 0x1C];
} EMESSAGE;

typedef struct TSK_NODE {
    char pad000[0x10];
    unsigned char nState;      /* 0x10 */
} TSK_NODE;

typedef struct {
    char pad000[1];
    unsigned char nAbort;      /* 0x01 */
    unsigned char nFlags;      /* 0x02 */
} SEISAN_WORK;

extern SEISAN_WORK *SeisanWork;

extern void eSpriteSet(ESPRITE *pSpr, short nId);
extern void eSpriteMain(ESPRITE *pSpr);
extern void eMessageSet(EMESSAGE *pMsg, char *pText);
extern void eMessageMain(EMESSAGE *pMsg);
extern void MoveSlide(short *pPos, short *pTarget, float fSpeed);

/* --- the "press O / press X" button hint at the bottom of the tally --- */

typedef struct {
    char pad000[4];
    int nColor;                /* 0x04 */
    EMESSAGE msg;              /* 0x08 */
    ESPRITE spr;               /* 0x4C */
} SEISAN_BUTTON_WORK;

/* Slides the button-prompt sprite and its caption in from the right; the
 * caption switches to the second string once SeisanWork's flag bit 2 is
 * set, and the whole pair parks off-screen again when nAbort hits 0xF0. */
void tskSeisanButton(TSK_NODE *node, SEISAN_BUTTON_WORK *w)
{
    static char *msg00[] = {
        " : Next",
        " : End",
    };
    SEISAN_WORK *s;
    short nTarget;

    switch (node->nState) {
    case 0:
        w->nColor = 0x00FFFFF0;
        eSpriteSet(&w->spr, 512);
        w->spr.nX = -112;
        w->spr.nY = 420;
        w->spr.nColor = w->nColor;
        eMessageSet(&w->msg, msg00[0]);
        w->msg.nFont = 32;
        w->msg.nX = w->spr.nX + 20;
        w->msg.nY = w->spr.nY;
        w->msg.nColor = w->nColor;
        break;
    case 2:
        s = SeisanWork;
        nTarget = 384;
        if (s->nFlags & 4) {
            w->msg.pText = msg00[1];
        }
        if (s->nAbort == 240) {
            nTarget = 544;
        }
        MoveSlide(&w->spr.nX, &nTarget, 3.0f);
        eSpriteMain(&w->spr);
        w->msg.nX = w->spr.nX + 20;
        w->msg.nY = w->spr.nY;
        eMessageMain(&w->msg);
        break;
    }
}
