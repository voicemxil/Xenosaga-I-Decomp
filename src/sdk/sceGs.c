#include "matching.h"

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
    short omode;          /*  2  video output mode (NTSC/PAL/VESA...) */
    short ffmd;           /*  4  1 = FIELD mode (vs FRAME) */
    short field;          /*  6  GS revision/ID, read out of CSR bits 16..23 */
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
extern int GsPutIMR(unsigned int imr);
extern void SetGsCrt(int interlace, int omode, int ffmd);

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
/* sceGszbufaddr: how many 2048-byte GS pages a Z buffer of w x h pixels
   needs, in the units the FRAME/ZBUF registers use.  `psm` bit 1 selects a
   32-bit Z format (page height 64) over a 16-bit one (page height 32); the
   width always rounds up to a 64-pixel page.

   The mode test really is ONE 64-bit load: the parameter block's interlace
   and FFMD shorts sit at byte offsets 0 and 4, so masking the first eight
   bytes with 0x0000FFFF0000FFFF and comparing against 1 asks "interlaced AND
   frame mode" in a single compare.  Written as two short compares gcc emits
   two lh/bne pairs and four extra words.

   TODO: near-miss (27 of 50 words).  The instruction multiset is EXACT --
   `tools/scratch_diff.py --all` shows only ordering and two register roles.
   Two things are left:
     - the prologue: the four callee-saved `sd`s and the three parameter
       sign-extensions are the same instructions in a different sched2 order,
       and the original puts `sra s2,a2,16` in the sceGsGetGParam delay slot
       where gcc puts `sll s0,a1,16`;
     - the tail: the original loads the parameter block into $v0 and the
       0x0000FFFF0000FFFF mask into $v1 (so the literal 1 is forced out to
       $a0); gcc does it the other way round and reuses $v0 for the 1.
   Naming the masked value in its own local `m` BEFORE the multiply is what
   recovers the original's `b` and the mult-in-the-delay-slot (34 -> 27
   words); without it reorg fills the branch slot from the target instead.
   Ruled out: PIN/PASSTHRU on m and on the product (both drop instructions
   or add a bnel), hoisting the mask or the literal 1 into a laundered
   long-lived temp (the constant-range lever -- costs a stack frame here),
   and all five tail phrasings of the final truncation. */
short sceGszbufaddr(short psm, short w, short h)
{
    sceGsGParam *g;
    int pw, ph;
    int pages;
    long long m;

    g = sceGsGetGParam();

    pw = (w + 63) / 64;
    if (psm & 2)
        ph = (h + 63) / 64;
    else
        ph = (h + 31) / 32;

    m = *(long long *)g & 0x0000FFFF0000FFFFLL;
    pages = pw * ph;
    if (m == 1)
        return (short)pages;
    return (short)(pages * 2);
}

/* sceGsResetGraph: bring the GS back to a known state.
 *
 *   mode 0  full reset -- CSR RESET, re-latch the GS id out of CSR bits
 *           16..23, mask every GS interrupt, tear down any installed VSync
 *           handler, then re-CRT;
 *   mode 1  CSR FLUSH only, nothing else touched;
 *   mode 5  re-CRT with new video parameters, leaving the CSR and any
 *           installed VSync handler alone;
 *   anything else is ignored.
 *
 * The function is void: the original tail-jumps into SetGsCrt from both the
 * mode-0 and mode-5 arms and never materialises a return value.
 *
 * TWO SHAPE FACTS, both reusable:
 *   1. gcc 2.9 turns `f(...); return;` at the end of a VOID function into a
 *      tail `j f` after the epilogue -- but only for a void call in a void
 *      function.  Making this return int (and SetGsCrt return int) costs the
 *      tail call in BOTH arms: gcc emits jal + a branch to a shared epilogue
 *      and the function is 9 words short.
 *   2. The trailing `default: return;` is load-bearing.  Without it the
 *      mode-5 arm is the function's LAST exit, gcc merges it with the
 *      implicit end-of-function return before the tail-call conversion runs,
 *      and that arm alone loses its `j`.  Giving the merge a different
 *      target restores it.  (Reordering the cases to 0/5/1 also works but
 *      then the emitted block order is wrong -- gcc lays cases out in source
 *      order and the original is 0/1/5.)
 *
 * TODO: near-miss (23 of 100 words).  The instruction multiset is EXACT.
 * What is left is the mode-0 and mode-5 blocks' scheduling -- the original
 * interleaves the four `sh` field stores with the CSR read and the argument
 * masks differently, and in mode 1 it puts the CSR address in $v0 and the
 * 0x100 value in $v1 where gcc does the reverse.  Ruled out: pinning the
 * mode-1 value (no effect), and returning int.  The $a2 pin below IS
 * load-bearing: the original computes `ffmd != 0` into $a2, the register
 * SetGsCrt's third argument later occupies, and gcc picks $v0 (27 -> 23). */
void sceGsResetGraph(short mode, short interlace, short omode, short ffmd)
{
    sceGsGParam *g;
    PIN(int fm, "$6");

    switch (mode) {
    case 0:
        g = sceGsGetGParam();
        GS_CSR = 0x200;
        g->interlace = interlace;
        g->omode = omode;
        g->field = (short)((GS_CSR >> 16) & 0xFF);
        GsPutIMR(0xFF00);
        g->ffmd = (ffmd != 0);
        if (g->vsyncCb != 0) {
            DisableIntc(2);
            RemoveIntcHandler(2, g->vsyncHid);
            g->vsyncHid = 0;
            g->vsyncCb = 0;
        }
        SetGsCrt(interlace & 1, omode & 0xFF, ffmd & 1);
        return;
    case 1:
        GS_CSR = 0x100;
        return;
    case 5:
        g = sceGsGetGParam();
        PASSTHRU(fm, ffmd != 0);
        g->ffmd = fm;
        g->interlace = interlace;
        g->omode = omode;
        g->field = (short)((GS_CSR >> 16) & 0xFF);
        SetGsCrt(interlace & 1, omode & 0xFF, ffmd & 1);
        return;
    default:
        return;
    }
}
