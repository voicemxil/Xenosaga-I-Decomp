/* PS2 SDK sceGs: the graphics-parameter block and VSync plumbing.
 *
 * `gp` is a file-static in the original, so splat had to disambiguate
 * it and the data listing calls it `gp.6` -- a name C cannot spell.
 * The declaration below uses gcc's asm-rename to spell it anyway, which
 * is what lets sceGsGetGParam be real C: the ordinary %hi/%lo
 * relocation then produces the original's lui+addiu, where a numeric
 * `(void *)0x004AB940` cast gets lui+ori (CONTRIBUTING.md, "Raw
 * addresses").  This replaces an inline-asm body.
 */

typedef struct sceGsGParam {
    short interlace;      /*  0  1 = interlaced */
    short pad02;
    short pad04;
    short field;          /*  6  1 = FIELD mode (vs FRAME) */
    void *vsyncCb;        /*  8  installed VSync callback, 0 if none */
    int   vsyncHid;       /* 12  its INTC handler id */
} sceGsGParam;

extern sceGsGParam gp __asm__("gp.6");

extern void VSync(void);
extern long long VSync2(void);
extern int DisableIntc(int cause);
extern int EnableIntc(int cause);
extern int AddIntcHandler(int cause, void *handler, int next);
extern int RemoveIntcHandler(int cause, int hid);

/* GS privileged CSR; bit 13 is the current output FIELD. */
#define GS_CSR (*(volatile unsigned long long *)0x12001000)

sceGsGParam *sceGsGetGParam(void)
{
    return &gp;
}

/* sceGsSyncV: wait for the next vertical blank and report which field
 * is now being displayed. With a VSync callback installed the wait goes
 * through VSync2 (which returns the CSR it observed); without one it
 * calls VSync and reads the CSR directly -- hence the two nearly
 * identical shift/mask sequences, one logical (dsrl, the CSR is
 * unsigned) and one arithmetic (dsra, VSync2 returns a signed value).
 *
 * Two deliberate oddities, both required to match and both faithful:
 *
 *   - There is no return in the non-interlaced case. The original
 *     really does fall out with whatever happens to be in v0, which is
 *     why the two arms disagree about what that is (1 in one, the
 *     unmasked shift result in the other). Adding a `return 0;` costs a
 *     word and changes the branch shape.
 *   - `bit` is long long, so the 64->32 truncation (dsll32/dsra32)
 *     happens at the return rather than at the assignment. Declared
 *     int, gcc truncates eagerly and the function is a word long.
 */
int sceGsSyncV(void)
{
    sceGsGParam *g;
    long long v;

    g = sceGsGetGParam();
    if (g->vsyncCb == 0) {
        VSync();
        if (g->interlace == 1)
            return (GS_CSR >> 13) & 1;
    } else {
        long long bit;

        v = VSync2();
        bit = (v >> 13) & 1;
        if (g->interlace == 1)
            return bit;
    }
}

/* sceGsSyncVCallback: install (or with cb == 0, remove) the VSync INTC
 * handler, returning the previous one.
 *
 * NEAR-MISS, not registered: 2 words, an argument-setup order.  The
 * original emits `move a1,s1` before `li a0,2` for the AddIntcHandler
 * call; gcc emits them the other way round.  Identical instruction
 * multiset, adjacent positions, no logic difference.  Four phrasings
 * were tried (call result through a temp, handler through a temp,
 * re-reading g->vsyncCb as the argument, and the plain form) and all
 * four give the same order, so this is not reachable from the C.
 *
 * FIX-FLAG REQUEST (verified): FILE_FIX_FLAGS["sceGs.c"] =
 * "--swap-adjacent sceGsSyncVCallback:26" -- the swap site is the
 * 0-based instruction index of `li a0,2` in the emitted function.  This
 * is the same tool and the same kind of site as the existing
 * TMENU.c/xgl entries in configure.py.  Registering it needs that flag;
 * the C below is otherwise byte-exact at 40 words.
 *
 * Note the structure: both arms fall through to a single `return old`.
 * Returning early from the removal arm makes gcc materialise the return
 * value twice and costs a word. */
void *sceGsSyncVCallback(void *cb)
{
    sceGsGParam *g;
    void *old;

    g = sceGsGetGParam();
    old = g->vsyncCb;
    if (cb == 0) {
        DisableIntc(2);
        RemoveIntcHandler(2, g->vsyncHid);
        g->vsyncCb = 0;
        g->vsyncHid = 0;
    } else {
        if (old != 0) {
            DisableIntc(2);
            RemoveIntcHandler(2, g->vsyncHid);
        }
        g->vsyncCb = cb;
        g->vsyncHid = AddIntcHandler(2, cb, -1);
        EnableIntc(2);
    }
    return old;
}
