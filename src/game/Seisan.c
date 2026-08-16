/* Seisan ("settlement") - the post-battle tally screen: the rolling
 * number counters, the result window and its fade. */

#include "common.h"

typedef struct SEISAN_WORK {
    char pad_00;
    unsigned char nState;           /* 0x01 */
    char pad_02[3];
    unsigned char field_05;         /* 0x05 */
    char pad_06[0x0A];
} SEISAN_WORK;

extern SEISAN_WORK *SeisanWork;
extern void *SeisanBgTask;
extern void *SeisanTask;

extern int subSeisanMain(void);

/* One tally frame; returns 0 once the work block has run out of states. */
int SeisanMain(void)
{
    subSeisanMain();
    return SeisanWork->nState != 0xFF;
}

/* Step nCur towards nTarget by nStep, clamping on overshoot. *pBusy is
 * raised only while the counter is still short of its target. */
int SeisanNumberCount(int nCur, int nTarget, int nStep, int *pBusy)
{
    if (nCur != nTarget) {
        if (nCur < nTarget) {
            nCur += nStep;
            if (nTarget < nCur) {
                nCur = nTarget;
            } else {
                *pBusy = 1;
            }
        } else {
            nCur -= nStep;
            if (nCur < nTarget) {
                nCur = nTarget;
            } else {
                *pBusy = 1;
            }
        }
    }
    return nCur;
}

typedef struct SEISAN_ITEM {
    int field_00;
    int nPrice;                     /* 0x04 */
} SEISAN_ITEM;

typedef struct SEISAN_CN {
    char pad_00[0x70];
    int  aPrice[12];                /* 0x70 */
    char pad_A0[0x10];
} SEISAN_CN;

extern SEISAN_CN SeisanCN;
extern SEISAN_ITEM *func_A19210(int nId);
extern void *memset(void *pDst, int nVal, unsigned int nSize);

/* Clear the counter block and prime it with the twelve item prices. */
void SeisanCountInit1(void)
{
    int *p;
    int i;

    memset(&SeisanCN, 0, sizeof(SeisanCN));
    p = SeisanCN.aPrice;
    for (i = 1; i < 13; i++) {
        *p = func_A19210(i)->nPrice;
        p++;
    }
}

typedef struct SEISAN_PADDATA {
    char pad_00[0x28];
    long nState;                    /* 0x28 */
    char pad_30[0x38];
} SEISAN_PADDATA;

extern SEISAN_PADDATA PadData[2];

extern void xglCdLoadOverlay(int nOverlay);
extern void xglRenderClearFrame(void);
extern void xglSleep(void);
extern void SeisanInit(void);
extern void SeisanDisp(void);

/* Standalone debug driver: run the tally screen until L1+R1 (or whatever
 * the 0x8000100 pair is) is held. */
void SeisanTest(void)
{
    xglCdLoadOverlay(1);
    SeisanInit();
    xglRenderClearFrame();
    while ((PadData[0].nState & 0x8000100) != 0x8000100) {
        SeisanMain();
        SeisanDisp();
        xglSleep();
    }
}

/* A 60-byte 4-aligned blob.  The alignment is load-bearing: at 4 gcc
 * copies the seven 8-byte chunks with unaligned ldl/ldr + sdl/sdr (8 > 4)
 * but the 4-byte tail with a plain lw/sw, which is exactly what the
 * original emits.  At alignment 1 the tail becomes lwl/lwr too. */
typedef struct SEISAN_RIBBON {
    int data[15];
} SEISAN_RIBBON;

extern SEISAN_RIBBON D_004C7350;

extern void endPrintDirectRibbon(void *pRibbon);
extern void endPrintExtFunc(int a, int b, int c);
extern int MenuLoadSync(void);
extern void MenuBgTaskMain(void);
extern void xglTaskExecute(void *pTask);
extern void SeisanFadeMain(void);

/* One tally-screen draw pass. */
void SeisanDisp(void)
{
    SEISAN_RIBBON ribbon;

    ribbon = D_004C7350;
    endPrintDirectRibbon(&ribbon);
    endPrintExtFunc(0, 100, 0);
    if (MenuLoadSync() != 0) {
        xglTaskExecute(SeisanBgTask);
    } else {
        MenuBgTaskMain();
        xglTaskExecute(SeisanBgTask);
        xglTaskExecute(SeisanTask);
        SeisanFadeMain();
    }
}

typedef struct SEISAN_FADEELEM {
    int  nColor;                    /* 0x00 */
    unsigned char c[4];             /* 0x04 */
    short x;                        /* 0x08 */
    short y;                        /* 0x0A */
} SEISAN_FADEELEM;

typedef struct SEISAN_FADE {
    unsigned char nState;           /* 0x00 */
    unsigned char field_01;         /* 0x01 */
    char pad_02[2];
    short field_04;                 /* 0x04 */
    short field_06;                 /* 0x06 */
    SEISAN_FADEELEM aElem[4];       /* 0x08 */
    int  field_38;                  /* 0x38 */
} SEISAN_FADE;

extern SEISAN_FADE *SeisanFade;

/* Drive the four-corner fade quad; while the work block sits in state
 * 0xF0 the corner alphas track its countdown.
 *
 * TODO near-miss (1 extra instruction, everything after the init loop
 * shifts).  The original's init loop keeps exactly two induction
 * variables -- a down counter and a byte offset added to BOTH `p` and a
 * precomputed `p + 8` -- while gcc here also strength-reduces the four
 * byte stores into a third pointer giv at `p + 13`.  Swept body orders,
 * ascending/descending loops, an element pointer, and a separate
 * pointer for the colour store; gcc reduces every spelling the same
 * way. */
void SeisanFadeMain(void)
{
    SEISAN_FADE *p = SeisanFade;
    int i;

    if (p->nState != 0) {
        if (p->nState != 2) {
            return;
        }
    } else {
        p->field_01 = 0;
        for (i = 0; i < 4; i++) {
            p->aElem[i].nColor = 0xFFFFFF;
            p->aElem[i].c[0] = 0;
            p->aElem[i].c[3] = 0;
            p->aElem[i].c[2] = 0;
            p->aElem[i].c[1] = 0;
        }
        p->nState = 2;
        p->field_04 = 0;
        p->field_06 = 0;
        p->aElem[0].x = 0;
        p->aElem[0].y = 448;
        p->aElem[1].x = 512;
        p->aElem[1].y = 0;
        p->aElem[2].x = 512;
        p->aElem[2].y = 448;
        p->field_38 = 0;
    }
    if (SeisanWork->nState == 240) {
        int nAlpha = (16 - SeisanWork->field_05) * 8;

        p->aElem[0].c[3] = nAlpha;
        p->aElem[3].c[3] = nAlpha;
        p->aElem[2].c[3] = nAlpha;
        p->aElem[1].c[3] = nAlpha;
    }
    endPrintExtFunc(0x02FFFFFF, 3, (int)&p->field_04);
}
