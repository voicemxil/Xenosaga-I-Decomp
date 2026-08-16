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
/* MATCHES.  122 words.  Closed after a long chain of levers; the history is
   worth keeping because most of it is reusable.

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
        Naming the accumulator decides WHICH of two `x|c` values reuses the
        shared temp's register.                              6 -> 6 (4 words)
     4. `LAUNDER_V(sx)` before the TRXPOS shifts.  Without it the second
        AddGsAD's `dsll32 $s6` is hoisted into the FIRST AddGsAD's delay slot
        and its own `li $a1,0x50` is pushed above the jal.   6 -> 4
     Also still needed: `LAUNDER_V(dbp)` after sceVif1PkOpenGifTag (removing
     it costs 25 words even at the new flags).

   THE LAST 4 WORDS, and why they are a fixer and not a lever.
     The `n ^ qwc` compare temp took $a2 where the original takes $a1, and
     the two argument moves that bracket it (`move $a2,n` / `move $a1,addr`)
     came out in the opposite order.  Mechanism: -fschedule-insns runs BEFORE
     register allocation, and it hoists ONE of the two hard-register argument
     copies above the compare; local_alloc then gives the combine-folded
     compare temp the lowest-numbered register still free.  We hoist addr's
     copy, so $a1 is taken and the temp lands in $a2; the original hoists n's
     copy, so the temp lands in $a1.  Proved by blocking $a2 with a PIN'd
     dummy: the temp moved to $a3, not $a1 -- it is "lowest free", and $a1
     is not free because the scheduler already took it.
     No source shape reaches the scheduler's tie-break.  SWEPT AND RULED OUT:
     - flags: -fno-regmove, -fno-caller-saves, -fno-expensive-optimizations,
       -fno-cse-follow-jumps, -fno-rerun-cse-after-loop, -fno-force-mem,
       -fno-thread-jumps, -fomit-frame-pointer, -funroll-loops, -O1, -O3,
       -fno-schedule-insns2 (85 diffs), -fno-delayed-branch, and a second
       sweep of -fno-sched-interblock, -fno-sched-spec, -fsched-spec-load,
       -fno-peephole, -fno-function-cse, -fstrict-aliasing,
       -fno-strength-reduce, -fno-defer-pop, -fcaller-saves,
       -fno-move-all-movables, -freduce-all-givs, -fno-inline.  None moves it.
     - `PIN(unsigned int ne, "$5")` alone: IGNORED.  The pseudo is dead after
       combine merges the xor into the movz, so the register var evaporates.
     - PIN + LAUNDER / LAUNDER_V / PASSTHRU / PASSTHRU_V on it: these DO fix
       the register but every asm form is a hard scheduling barrier for this
       gcc and costs 9-14 words of order.  NEW LEVER, recorded: an empty asm
       is the only way to keep a combine-folded compare temp alive, and it
       buys the register at the price of the schedule.
     - ~40 source shapes: swapping the eop/gt statements, pinning gt to $3,
       double-pinned $3 alias, pinning n to $16, pinning addr to $20, naming
       the `n | 0x51000000` code arm in a local, `0x51000000 | n`, `addr + 0`,
       `qwc > 0x7FFF` vs `0x7FFF < qwc`, `qwc == n` vs `n == qwc`,
       `(n ^ qwc) == 0`, `!(n ^ qwc)`, a named `last = (n == qwc)` boolean,
       ?: instead of if, an if/assign instead of ?: for the min, swapping
       `qwc -= n` / `addr += n << 4`, moving either update above the call,
       hoisting the block-scope declarations to function scope, reordering
       `n` and `p`, LAUNDER_V/LAUNDER2 on n / addr / qwc / gt at the call
       site, PASSTHRU'd and PIN'd copies of either argument, swapping
       p[0]/p[1] stores, `void *`/K&R/`unsigned long` prototypes for
       sceVif1PkRef, and a 1500-iteration decomp-permuter run over a harness
       that preserves the PINs (see permuter/sceVif1PkRefLoadImage/prep.py).
       Every one of them is 4 words or worse.
     So it is closed with two fixer sites, using the new instruction-RANGE
     form of --swap-regs (fix_cc_asm.py):
         --swap-regs sceVif1PkRefLoadImage:5-6:89-98
         --swap-regs sceVif1PkRefLoadImage:16-20:91,98
     The first renames $a1<->$a2 across the ten-instruction window that holds
     the xor, the movz and the two argument moves -- every other $a1/$a2 in
     the function is an ordinary ABI argument and must not move, which is why
     the pre-existing whole-function --swap-regs could not be used.  The
     second exchanges the two argument moves' SOURCES ($s0=n, $s4=addr) on
     just those two instructions, which is the pure reorder of the pair. */
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

