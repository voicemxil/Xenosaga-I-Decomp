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
