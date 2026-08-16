/*
 * eNumber/eSprite/eRibbon HUD widgets. Started from the SquareMan tree
 * (only the empty mode-change stub is decompiled there so far).
 */

#include "common.h"

void eRibbonModeChange(void *pRibbon, unsigned char nMode)
{
    *(unsigned char *)((char *)pRibbon + 0xC) = nMode;
}

void eNumberNumberChange(void)
{
}

/* eNumber field reset: the -128 group and the zero group. */
typedef struct
{
    u8 unk_0[8];
    s8 unk_8;
    s8 unk_9;
    s8 unk_a;
    s8 unk_b;
    s8 unk_c;
    s8 unk_d;
    s8 unk_e;
    s8 unk_f;
    s8 unk_10;
    s8 unk_11;
    s8 unk_12;
    s8 unk_13;
} eNumber;

void eNumberSet(eNumber *p)
{
    p->unk_8 = p->unk_9 = p->unk_a = p->unk_b = -128;
    p->unk_c = 3;
    p->unk_10 = p->unk_11 = p->unk_12 = p->unk_13 = 0;
}
