#include "matching.h"

/* VIF1 packet builder structure (matches PS2 assembly layout) */
typedef struct Vif1Packet
{
    unsigned int *current;   /* 0x00 - current write pointer */
    unsigned int *base;      /* 0x04 - base/start pointer */
    unsigned int  reserved;  /* 0x08 - state / unused */
    unsigned int *openDirect;  /* 0x0C */
    unsigned int  unk1;      /* 0x10 */
    unsigned int *openGif;   /* 0x14 - pointer to open GIF tag */
} Vif1Packet;

/* 128-bit quadword value (passed in a single VU/EE 128-bit register) */
typedef int u128 __attribute__((mode(TI)));
typedef union { u128 q; long d[2]; } U128;


/* --- TU 52 ---------------------------------------------------------------
   tools/tu_boundaries.py shows sceVif1Pk is 26 separate __gnu_compiled_c
   translation units, one per function: it is a library archive.
   sceVif1PkRefLoadImage is TU 52 entirely on its own, so it may legitimately
   have been built with different flags from its siblings. Split out here so
   its flags can be swept independently. */

extern const long D_004D53C0[2];

void sceVif1PkCnt(Vif1Packet *pkt, unsigned int flag);
void sceVif1PkOpenDirectHLCode(Vif1Packet *pkt, unsigned int size);
void sceVif1PkOpenGifTag(Vif1Packet *pkt, u128 tag);
void sceVif1PkAddGsAD(Vif1Packet *pkt, unsigned int addr, unsigned long long data);
void sceVif1PkCloseGifTag(Vif1Packet *pkt);
void sceVif1PkCloseDirectHLCode(Vif1Packet *pkt);
void sceVif1PkAlign(Vif1Packet *pkt, unsigned int a, unsigned int b);
void sceVif1PkAddCode(Vif1Packet *pkt, unsigned int value);
unsigned int *sceVif1PkReserve(Vif1Packet *pkt, unsigned int count);
void sceVif1PkRef(Vif1Packet *pkt, unsigned int addr, unsigned int n,
                  unsigned int c, unsigned int code, unsigned int d);

/* Build a VIF1 packet that uploads an image to the GS: a DIRECTHL block sets up
   BITBLTBUF/TRXPOS/TRXREG/TRXDIR through a GIF A+D tag, then the pixel data is
   chained in as REF transfers of at most 32767 quadwords each. */
/* TODO: near-miss, 4 of 122 words, REGISTER class only (was 20).

   THE BIG FINDING: tools/tu_boundaries.py shows sceVif1Pk is 26 separate
   __gnu_compiled_c translation units, one per function -- a library archive.
   sceVif1PkRefLoadImage is TU 52 all by itself, so it could legitimately be
   built with different flags from its 25 siblings, and a single .c file
   cannot express that.  Split out here, it wants `-O2 -G0`: the SDK default
   `-fno-schedule-insns` is WRONG for this one TU.  That alone took it
   20 -> 14 diffs and, crucially, 19 wrong opcodes -> 4, which then let the
   remaining register levers land.  Same shape as the mpeg.c / sceMpegDec.c
   split.  Sibling files that resist should be checked against
   tu_boundaries.py before anyone spends more time on source levers.

   The levers, in the order they mattered:
     1. TU split + `-O2 -G0` (above).                        20 -> 14
     2. dbw and rrw each need their own PIN.  With the first scheduler on,
        gcc's global-alloc ranks them the other way round from the original
        ($s1 <-> $s2).  `bw` on $17 and `rw` on $18 -- and `rw` has to be the
        WIDE (unsigned long) type so the zero-extending dsll32/dsrl32 pair
        lands in place in $s2 rather than through a scratch. 14 -> 6
     3. Make `gt` the explicit in-place accumulator:
              gt  = (unsigned long)n;
              eop = gt | EOP_BIT;
              gt |= NONEOP_BIT;
        Writing the two ORs as independent expressions off `n` lets gcc
        coalesce the zero-extended temp with `eop` (which is PIN'd to $a0)
        and fold EOP in place; the original folds the NON-eop value in place
        into $v1.  Naming the accumulator decides which one is in place.
        This is a general lever: to choose WHICH of two `x|c` values reuses
        the shared temp's register, seed that value's variable with the temp
        and use |= for it.                                   6 -> 6 (fixed 4 words)
     4. `LAUNDER_V(sx)` before the TRXPOS shifts.  Without it the second
        AddGsAD's `dsll32 $s6` is hoisted into the FIRST AddGsAD's delay slot
        and its own `li $a1,0x50` is pushed above the jal.   6 -> 4
     Also still needed: `LAUNDER_V(dbp)` after sceVif1PkOpenGifTag (removing
     it costs 25 words even at the new flags).

   WHAT IS LEFT (4 words, registers only, $a1<->$a2 / $s0<->$s4):
     the scratch register holding `n ^ qwc` for the movz.  The original picks
     $a1 and therefore sets up the `n` argument ($a2) early and the `addr`
     argument ($a1) after the movz; we pick $a2 and do it the other way.
     Both are the same four instructions in the same places -- only the
     scratch choice differs.

   SWEPT AND RULED OUT for those 4 words:
     - flags: -fno-regmove, -fno-caller-saves, -fno-expensive-optimizations,
       -fno-cse-follow-jumps, -fno-rerun-cse-after-loop, -fno-force-mem,
       -fno-thread-jumps, -fomit-frame-pointer, -funroll-loops, -O1, -O3,
       -fno-schedule-insns2 (85 diffs), -fno-delayed-branch. None moves it.
     - `PIN(unsigned int ne, "$5")` alone: IGNORED.  The pseudo is dead after
       combine merges the xor into the movz, so the register var evaporates.
     - PIN + LAUNDER / LAUNDER_V / PASSTHRU / PASSTHRU_V on it: these DO fix
       the register (triage flips to SCHEDULING, i.e. the multiset becomes
       exact) but every asm form is a hard scheduling barrier for this gcc
       and costs 9-14 words of order.  NEW LEVER, recorded: an empty asm is
       the only way to keep a combine-folded compare temp alive, and it buys
       the register at the price of the schedule -- worth it only if nothing
       downstream needs to move.
     - swapping the eop/gt statements (9), pinning gt to $3 (28, LENGTH),
       double-pinned $3 alias (30, LENGTH), pinning n to $16 (54, LENGTH),
       pinning addr to $20 (84, LENGTH), naming the `n | 0x51000000` code arm
       in a local, `0x51000000 | n`, `addr + 0`, `qwc > 0x7FFF` vs
       `0x7FFF < qwc`, `qwc == n` vs `n == qwc`, ?: instead of if,
       swapping `qwc -= n` / `addr += n << 4`, hoisting the block-scope
       declarations to function scope, reordering `n` and `p`,
       LAUNDER_V/LAUNDER2 on n / addr / qwc / gt, swapping p[0]/p[1] stores.
       All 4 or worse. */
void sceVif1PkRefLoadImage(Vif1Packet *pkt, unsigned int dbp, unsigned int dpsm,
                           unsigned int dbw, unsigned int addr, unsigned int qwc,
                           unsigned int dsax, unsigned int dsay,
                           unsigned int rrw, unsigned int rrh)
{
    U128 tag;
    long g0;
    PIN(long g1, "$12");
    PIN(unsigned int bw, "$17");
    PIN(unsigned long rw, "$18");
    PIN(unsigned long psm, "$19");
    PIN(unsigned int sx, "$22");
    PIN(unsigned long sxq, "$22");   /* alias: lets the shift land in $s6 */
    PIN(unsigned int sy, "$21");
    PIN(unsigned long syq, "$21");   /* alias: lets the shift land in $s5 */

    g0 = D_004D53C0[0];
    g1 = D_004D53C0[1];
    tag.d[0] = g0;
    tag.d[1] = g1;
    psm  = dpsm & 0xFF;
    bw   = dbw & 0xFFFF;
    dbp  = dbp & 0xFFFF;
    sx = dsax;
    sy = dsay;

    sceVif1PkCnt(pkt, 0);
    sceVif1PkOpenDirectHLCode(pkt, 0);
    sceVif1PkOpenGifTag(pkt, tag.q);
    LAUNDER_V(dbp);   /* fence: gcc hoists the GS field shifts across the calls */

    psm = psm << 56;
    sceVif1PkAddGsAD(pkt, 0x50, ((unsigned long)dbp << 32)      /* BITBLTBUF */
                                | ((unsigned long)bw << 48) | psm);
    LAUNDER_V(sx);
    sxq = (unsigned long)sx << 32;
    syq = (unsigned long)sy << 48;
    sceVif1PkAddGsAD(pkt, 0x51, sxq | syq);                     /* TRXPOS */
    rw = rrw;
    sceVif1PkAddGsAD(pkt, 0x52, rw                              /* TRXREG */
                                | ((unsigned long)rrh << 32));
    sceVif1PkAddGsAD(pkt, 0x53, 0);                             /* TRXDIR */

    sceVif1PkCloseGifTag(pkt);
    sceVif1PkCloseDirectHLCode(pkt);

    if (qwc != 0)
    {
        do
        {
            unsigned int n;
            unsigned long *p;
            unsigned long gt;
            PIN(unsigned long eop, "$4");

            n = (0x7FFF < qwc) ? 0x7FFF : qwc;

            sceVif1PkCnt(pkt, 0);
            sceVif1PkAlign(pkt, 2, 3);
            sceVif1PkAddCode(pkt, 0x51000001);
            p = (unsigned long *)sceVif1PkReserve(pkt, 4);

            /* eop first, gt second: the second one is folded in place into the
               register holding n, which is what selects movz over movn. */
            gt  = (unsigned long)n;
            eop = gt | 0x0800000000008000L;
            gt |= 0x0800000000000000L;
            if (n == qwc)
                gt = eop;
            p[1] = 0;
            p[0] = gt;

            sceVif1PkRef(pkt, addr, n, 0, n | 0x51000000, 0);

            qwc -= n;
            addr += n << 4;
        }
        while (qwc != 0);
    }
}

