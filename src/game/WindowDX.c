/*
 * WindowDX: the shared UI window frame object embedded in the various Umn
 * segment/list windows (Pas title bars, item segment headers, etc). The
 * 0x194-byte layout was derived by tskUmn.c's header from its callers and
 * is NOT owned by this TU -- see src/game/tskUmn.c for the canonical
 * typedef. This TU only implements the WindowDX* entry points themselves
 * and therefore addresses fields by raw byte offset instead of pulling in
 * (and risking diverging from) that struct.
 *
 * WINDOWDX layout (0x194 bytes), as known so far:
 *   0x00 s16 nX
 *   0x02 s16 nY
 *   0x04 s32 nColor
 *   0x08 s16 nW
 *   0x0A s16 nH
 *   0x0C void* pText        -- passed as eTagFontSet's text arg (was "pad0C")
 *   0x10 s8  nState          -- lifecycle 0->1->3->2->5->-1 (see WindowDXMain)
 *   0x11 s8  unk_11          -- zeroed by WindowDXSet alongside nState
 *   0x14 void(*pFunc)(void)  -- draw/open callback
 *   0x18 void* pMsg
 *   0x2C s8  unk_2C = -128
 *   0x2D s8  unk_2D = -128
 *   0x2E s8  unk_2E = -128
 *   0x2F u8  unk_2F = 96
 *   0x80 ...                 -- embedded eTagFont object
 *   0xA0 ...                 -- embedded eRibbon object #1
 *   0x110 ...                -- embedded eRibbon object #2
 *   0x190 s16 unk_190 = 1545 (0x609)
 */

#include "common.h"

/* Store order here is load-bearing: gcc 2.9x reorders the three -128 byte
 * stores relative to each other and to the zeroing stores, so the source
 * order that reproduces the original is not the original's emission order.
 * Found by exhausting the placements of the three -128 stores among the
 * nine (C(9,3) * 3! = 504); only this one lands 0x2C first and 0x2E in the
 * jr-ra delay slot. Do not "tidy" this list back into address order. */
void WindowDXSet(void *pWin)
{
    char *p = (char *)pWin;

    *(u8 *)(p + 0x2F) = 96;
    *(s32 *)(p + 0xC) = 0;
    *(void **)(p + 0x14) = NULL;
    *(void **)(p + 0x18) = NULL;
    *(s8 *)(p + 0x10) = 0;
    *(s8 *)(p + 0x11) = 0;
    *(s8 *)(p + 0x2D) = -128;
    *(s8 *)(p + 0x2E) = -128;
    *(s8 *)(p + 0x2C) = -128;
}
