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


/* Initialize a VIF1 packet builder */
void sceVif1PkInit(Vif1Packet *pkt, unsigned int *buffer)
{
    pkt->current  = buffer;
    pkt->reserved = 0;
    pkt->base     = buffer;
}

/* Reset packet write pointer to base */
void sceVif1PkReset(Vif1Packet *pkt)
{
    pkt->reserved = 0;
    pkt->current  = pkt->base;
}

/* Append a single 32-bit value to the packet */
void sceVif1PkAddCode(Vif1Packet *pkt, unsigned int value)
{
    *pkt->current = value;
    pkt->current++;
}

/* Return number of 16-byte quadwords written */
unsigned int sceVif1PkSize(Vif1Packet *pkt)
{
    unsigned char *base = (unsigned char*)pkt->base;
    return ((unsigned int)((unsigned char*)pkt->current - base)) >> 4;
}

/* Reserve space for a number of 32-bit values; returns the reserved block.
   The return value is load-bearing: sceVif1PkRefLoadImage writes the GIF tag
   through it, and keeping $v0 live as the result is exactly what forces the
   `addu $a1,$v0,$a1` (rd==rt) form that looked like a compiler-build wall. */
unsigned int *sceVif1PkReserve(Vif1Packet *pkt, unsigned int count)
{
    unsigned int *p = pkt->current;
    pkt->current = p + count;
    return p;
}

/* Append a 64-bit GS data value (written as two 32-bit words) */
void sceVif1PkAddGsData(Vif1Packet *pkt, unsigned long long value)
{
    unsigned int *dest = pkt->current;
    unsigned int *next;
    *dest++ = (unsigned int)value;
    next = dest + 1;
    *dest = (unsigned int)(value >> 32);
    pkt->current = next;
}

/* Close a previously opened DIRECT code and patch its size */
void sceVif1PkCloseDirectCode(Vif1Packet *pkt)
{
    unsigned int *open    = pkt->openDirect;
    unsigned int *current = pkt->current;

    pkt->openDirect = 0;          /* clear open state */
    current -= 1;                 /* exclude last word */

    open[0] += (unsigned int)(current - open) >> 2;
}

/* Close a previously opened DIRECT HL code and patch its size */
void sceVif1PkCloseDirectHLCode(Vif1Packet *pkt)
{
    unsigned int *open    = pkt->openDirect;
    unsigned int *current = pkt->current;

    pkt->openDirect = 0;        /* clear open state */
    current -= 1;               /* exclude last word */

    open[0] += (unsigned int)(current - open) >> 2;
}

/* Append a GS A+D packet entry (64-bit data + 32-bit address) */
void sceVif1PkAddGsAD(Vif1Packet *pkt, unsigned int addr, unsigned long long data)
{
    unsigned int *dest = pkt->current;
    *dest++ = (unsigned int)data;
    *dest = (unsigned int)(data >> 32);
    pkt->current = dest + 3;
    dest[1] = addr;
    dest[2] = 0;
}

/* Append N 32-bit words to the packet */
void sceVif1PkAddDataN(Vif1Packet *pkt, unsigned int *src, unsigned int count)
{
    unsigned int i = count - 1;

    if (count != 0)
    {
        unsigned int *dest = pkt->current;

        do
        {
            *dest++ = *src++;
        }
        while (--i != 0xFFFFFFFF);

        pkt->current = dest;
    }
}

/* Append a 128-bit UNPACK data value (written as four 32-bit words) */
void sceVif1PkAddUpkData128(Vif1Packet *pkt, u128 value)
{
/* The ordering below is the original's: hi and hi>>32 hoisted ahead of the lo
   block, lo>>32 extracted inline after the first store.  On top of that,
   dest/lo/(int)lo are a 3-cycle in local-alloc (dest wants $v0, lo wants $v1,
   (int)lo wants $a2) which only the pins settle -- permuter-exhausted, all
   5040 orderings of the head statements and all 720 without the temps land at
   12+ diffs without them.  The last two words are a sched2 transpose, see
   FILE_FIX_FLAGS --swap-adjacent sceVif1PkAddUpkData128:10 and :15. */
    U128 u;
    PIN(unsigned int *dest, "$2");
    PIN(long lo, "$3");
    long hi;
    PIN(int l0, "$6");
    PIN(unsigned int *end, "$6");
    int h1;

    u.q = value;
    hi = u.d[1];
    h1 = (int)(hi >> 32);
    lo = u.d[0];
    PASSTHRU(dest, pkt->current);
    PASSTHRU(l0, (int)lo);
    dest[0] = l0;
    dest++;
    dest[0] = (int)(lo >> 32);
    PASSTHRU(end, dest + 3);
    pkt->current = end;
    dest[1] = (int)hi;
    dest[2] = h1;
}

/* Close a previously opened UNPACK code and patch its element count */
void sceVif1PkCloseUpkCode(Vif1Packet *pkt)
{
    unsigned int *current = pkt->current;
    unsigned int *open    = pkt->openDirect;  /* location of UNPACK code */
    unsigned int  meta    = pkt->unk1;        /* stored UNPACK parameters */

    unsigned int words;
    unsigned int cl;
    unsigned int fmt;
    unsigned int elements;

    /* Exclude last word and compute number of 32-bit words written */
    current -= 1;
    words = (unsigned int)(current - open);

    /* Extract cycle length (CL) and format bits */
    cl  = meta & 0xFFFF;         /* cycle length */
    fmt = meta & 0xFFFF0000;     /* format bits */

    /* Compute number of UNPACK elements written (ceil division) */
    elements = ((words * 32) + cl - 1) / cl;

    pkt->openDirect = 0;  /* clear open state */

    /* Patch UNPACK instruction */
    open[0] += fmt * elements;
}

/* Finalize the VIF1 packet by aligning to 16 bytes and patching any open DIRECT/HL code sizes */
unsigned int sceVif1PkTerminate(Vif1Packet *pkt)
{
    unsigned int *current = pkt->current;
    unsigned int *open    = (unsigned int *)pkt->reserved;

    /* Align current pointer to 16-byte boundary, filling padding words with zeros */
    while (((unsigned int)current & 0xC) != 0)
    {
        *current++ = 0;
    }

    /* Patch the size of an open DIRECT/HL code if one exists.
       Loading *open into its own temp BEFORE computing n is load-bearing:
       it wins the $v0/$v1 register tie-break (n in $v0, *open in $v1). */
    if (open != 0)
    {
        unsigned int v = *open;
        int n = (((int)current - (int)open) >> 4) - 1;
        *open = v + n;
    }

    /* Clear open pointer */
    pkt->reserved = 0;

    /* Update packet write pointer */
    pkt->current = current;

    return (unsigned int)current;
}

/* Finalize a packet segment and append a VIF1 control word with the given flags */
void sceVif1PkCnt(Vif1Packet *pkt, unsigned int flags)
{
    unsigned int start;
    unsigned int *current;

    /* Terminate packet to align and close any open code; remember segment start */
    start = sceVif1PkTerminate(pkt);

    /* gcc's post-reload scheduler sinks the `sw` of the reserved field between
       the `lui` and the `or` that build the control word; the original emits
       the `or` first.  EXHAUSTED at source level: all 5040 orderings of the
       seven statements below were compiled (tools/permute.py) and none
       reaches 0 -- `0x10000000 | flags`, `current[i]` instead of `*current`,
       hoisting the reserved store, a fresh temp for the or'd value, a
       volatile-cast reserved store and an early-hoisted constant temp all
       reproduce the same 2-word swap.  It is FILE_FIX_FLAGS
       --swap-adjacent sceVif1PkCnt:9. */
    current = pkt->current;
    flags |= 0x10000000;
    pkt->reserved = start;
    *current = flags;
    current++;
    pkt->openDirect = 0;
    pkt->current = current + 1;
    *current = 0;
}

/* Finalize a packet segment and append a VIF1 end-of-packet control word */
void sceVif1PkEnd(Vif1Packet *pkt, unsigned int flags)
{
    unsigned int start;
    unsigned int *current;

    /* Terminate packet to align and close any open code; remember segment start */
    start = sceVif1PkTerminate(pkt);

    /* Identical shape to sceVif1PkCnt with the 0x70000000 end-of-packet code;
       the same permuter-exhausted `sw`/`or` swap, fixed by
       --swap-adjacent sceVif1PkEnd:9. */
    current = pkt->current;
    flags |= 0x70000000;
    pkt->reserved = start;
    *current = flags;
    current++;
    pkt->openDirect = 0;
    pkt->current = current + 1;
    *current = 0;
}

/* Align the packet current pointer to a 16-byte (quadword) boundary */
void sceVif1PkAlign(Vif1Packet *pkt, unsigned int padding, unsigned int boundary)
{
    /* Shape levers (all plain C, no code emitted for them):
       (1) the zero-fill store goes through `unsigned int **` so that gcc
       2.9's type-based aliasing cannot prove the in-loop
       `pkt->current = next` dead and sink it out of the loop;
       (2) the loop is a do-while entered by a goto into its middle, which
       reproduces the original's peeled first iteration -- a plain while
       gets cross-jumped or left in jump-to-test form;
       (3) `t1 = target + 1` in its own statement stops the reassociation
       into target + (mask + 1).
       Register ties (steering only): amt/all1 settle the $v0/$v1 mask
       setup; `mask` in $a3 keeps the nor/and pair reading it directly;
       `m2` in $v0 makes the `and` reuse the register the `nor` just wrote
       instead of taking a fresh $v1; and naming the `target < cur` test in
       its own pinned local puts the sltu in $v1.  m2 and lt are a 3-cycle
       in local-alloc -- pinning either one alone just rotates the cycle.
       The last two pairs are pure post-reload-scheduler transposes (sched2
       issues the srlv before the lw, and hoists the ready store over the
       independent addiu in the loop body); no statement order reproduces
       either, so they are FILE_FIX_FLAGS --swap-adjacent sites 6 and 22. */
    unsigned int p = (padding + 2) & 0x1F;
    PIN(unsigned int amt, "$3");
    PIN(unsigned int all1, "$2");
    PIN(unsigned int mask, "$7");
    PIN(unsigned int *cur, "$5");
    PIN(unsigned int m2, "$2");
    unsigned int target;

    PASSTHRU(amt, 32 - p);
    PASSTHRU(all1, 0xFFFFFFFFu);
    mask = all1 >> amt;
    cur = pkt->current;
    PASSTHRU(m2, ~mask);
    m2 = (unsigned int)cur & m2;
    target = m2 + (boundary << 2);

    {
        PIN(unsigned int lt, "$3");

        lt = (target < (unsigned int)cur);
        if (lt)
        {
            unsigned int t1 = target + 1;
            target = t1 + mask;
        }
    }

    if ((unsigned int)cur < target)
    {
        PIN(unsigned int *next, "$3");

        next = cur + 1;
        *(unsigned int **)cur = 0;
        goto join;
        do
        {
            cur = next;
            next = cur + 1;
            *(unsigned int **)cur = 0;
    join:
            pkt->current = next;
        }
        while ((unsigned int)next < target);
    }
}

/* Open a DIRECT packet section for writing GS commands */
void sceVif1PkOpenDirectCode(Vif1Packet *pkt, unsigned int flags)
{
    unsigned int *cur;
    unsigned int directCode;

    /* Align the current pointer to a quadword boundary for DIRECT packet */
    sceVif1PkAlign(pkt, 2, 3);

    /* Load the current pointer */
    cur = pkt->current;

    /* Compute the DIRECT header value */
    directCode = 0x50000000;  /* VIF1 DIRECT code base */
    if (flags != 0)
        directCode = 0xD0000000;           /* VIF1 DIRECT code if flags set */

    /* Write DIRECT header to packet */
    pkt->current = cur + 1;   /* advance pointer by 4 bytes */
    pkt->openDirect = cur;    /* remember the start of this DIRECT block */
    *cur = directCode;
}

/* Open a HIGH-LEVEL DIRECT packet section for writing GS commands */
void sceVif1PkOpenDirectHLCode(Vif1Packet *pkt, unsigned int flags)
{
    unsigned int *cur;
    unsigned int directHLCode;

    /* Align the current pointer to a quadword boundary for DIRECT HL packet */
    sceVif1PkAlign(pkt, 2, 3);

    /* Load the current pointer */
    cur = pkt->current;

    /* Compute the DIRECT HL header value */
    directHLCode = 0x51000000;  /* VIF1 DIRECT HL base code */
    if (flags != 0)
        directHLCode = 0xD1000000;          /* VIF1 DIRECT HL if flags set */

    /* Write DIRECT HL header to packet */
    pkt->current = cur + 1;   /* advance pointer by 4 bytes */
    pkt->openDirect = cur;    /* remember the start of this DIRECT HL block */
    *cur = directHLCode;
}

/* Add N 128-bit DIRECT data units to the VIF1 packet */
/* Append N 128-bit DIRECT data units (two 64-bit halves each) to the packet.
   sceVif1PkAddUpkData128N below is the same loop over the same instruction
   stream; keep the two in step.

   Three shape levers, none of them cosmetic:

   (1) The walking store base `d` is a SEPARATE local from `dest`, taken
       after the ++ and written back as `dest = d + 3`.  Without it gcc 2.9's
       loop strength reduction folds the +4 and +12 into one `addiu +16`,
       rewrites the four store offsets to -4/0/4/8 and inits dest+1 in the
       preheader -- a 26-word divergence.  Splitting the pointer gives loop.c
       a giv it will not combine, and the original's `move v0,a2` copy comes
       back with it.
   (2) `dest = d + 3` sits BETWEEN the first and second store through d.  The
       original updates the pointer before it has finished storing (that is
       why the copy is needed at all); putting the update first or last moves
       the addiu off its slot.
   (3) `h1 = (int)(hi >> 32)` is hoisted above the first store, exactly as in
       the sceVif1PkAddUpkData128 sibling -- that is what puts lo in $v1 and
       hi in $a0 instead of $a0/$a1.

   The two PINs are steering only: gcc hands the hoisted 0xffffffff loop bound
   $t2 and the pkt copy $t1, and the original has them the other way round;
   `d` wants $v0, which it only takes when it is pinned. */
void sceVif1PkAddDirectDataN(Vif1Packet *pkt, const long *src, unsigned int count)
{
    unsigned int i = count - 1;
    PIN(Vif1Packet *p, "$10");

    PASSTHRU(p, pkt);
    if (count != 0)
    {
        unsigned int *dest = p->current;

        do
        {
            long lo, hi;
            PIN(unsigned int *d, "$2");
            int h1;

            lo = src[0];
            hi = src[1];
            h1 = (int)(hi >> 32);
            dest[0] = (int)lo;
            dest++;
            PASSTHRU(d, dest);
            d[0] = (int)(lo >> 32);
            dest = d + 3;
            d[1] = (int)hi;
            d[2] = h1;
            src += 2;
        }
        while (--i != 0xFFFFFFFF);

        p->current = dest;
    }
}

/* Add multiple 128-bit UPK data units to the VIF1 packet */
/* Add multiple 128-bit UNPACK data units to the packet.  Byte-identical body
   to sceVif1PkAddDirectDataN above (same original instruction stream); the
   three shape levers and the two PINs are explained there. */
void sceVif1PkAddUpkData128N(Vif1Packet *pkt, const long *src, unsigned int count)
{
    unsigned int i = count - 1;
    PIN(Vif1Packet *p, "$10");

    PASSTHRU(p, pkt);
    if (count != 0)
    {
        unsigned int *dest = p->current;

        do
        {
            long lo, hi;
            PIN(unsigned int *d, "$2");
            int h1;

            lo = src[0];
            hi = src[1];
            h1 = (int)(hi >> 32);
            dest[0] = (int)lo;
            dest++;
            PASSTHRU(d, dest);
            d[0] = (int)(lo >> 32);
            dest = d + 3;
            d[1] = (int)hi;
            d[2] = h1;
            src += 2;
        }
        while (--i != 0xFFFFFFFF);

        p->current = dest;
    }
}

/* Initialize a VIF1 packet for an UNPACK code block */
/* Open an UNPACK code block: writes a VIF STCYCL (CL=cl, WL=wl) followed by
   the UNPACK VIFcode itself, and stashes the element/format parameters that
   sceVif1PkCloseUpkCode needs to patch the NUM field later. */
void sceVif1PkOpenUpkCode(Vif1Packet *pkt, unsigned int addr, unsigned int upk,
                          unsigned int cl, unsigned int wl)
{
    unsigned int *current;
    unsigned int *next;
    unsigned int vl, code, c0, w1;
    PIN(unsigned int vn, "$4");
    PIN(unsigned int lo, "$11");
    PIN(unsigned int hi, "$12");
    /* One scratch local really is reused for the "0 means 256" CL/WL default
       and then for the element size.  That is not cosmetic: it is why the
       multiply lands in $t2, the register the 256 lived in.  Giving the two
       roles separate locals puts the product in $a0 (the register vn dies in)
       and costs three words. */
    unsigned int size;

    current = pkt->current;
    c0 = cl | 0x1000000;
    *current = (wl << 8) | c0;      /* STCYCL */
    current++;

    vn = (upk >> 2) & 3;
    w1 = (upk << 24) | (addr & 0xFFFF);
    vl = upk & 3;
    next = current + 1;

    size = 0x100;
    lo = cl ? cl : size;
    hi = wl ? wl : size;
    size = (0x20 >> vl) * (vn + 1);

    *current = w1;                  /* UNPACK VIFcode */
    pkt->current = next;
    pkt->openDirect = current;

    if (lo >= hi) {
        code = 0x10000;
    } else {
        size = size * lo;
        code = hi << 16;
    }
    pkt->unk1 = code | size;
}

/* Add a VIF1 reference packet with 4 words, terminating the current packet first */
void sceVif1PkRef(Vif1Packet *pkt, unsigned int a1, unsigned int a2, unsigned int a3, unsigned int t0, unsigned int t1)
{
    unsigned int *current;
    unsigned int w0, w1;

    /* terminate current packet first */
    sceVif1PkTerminate(pkt);

    w1 = a1 & 0x9FFFFFFF;
    a2 = a2 | 0x30000000;
    current = pkt->current;
    w0 = t1 | a2;
    *current++ = w0;
    *current = w1;
    pkt->current = current + 3;
    current[1] = a3;
    current[2] = t0;
}

/* Close a currently open GIF tag in the VIF1 packet */
void sceVif1PkCloseGifTag(Vif1Packet *pkt)
{
        /* The `two` temp is load-bearing: materialising the literal 2 early gives
       its pseudo a long live range and therefore a LOW local-alloc priority,
       which rotates the 3-cycle {flg, nloop>>1, 2} into the original's
       $v1/$a0/$a1 assignment.  With a bare `flg != 2` the constant is born at
       the compare, gets top priority, steals $v1, and 13 words swap registers.
       Also note `nloop >>= 1` really is outside the `flg != 2` test (the movn
       sits in the beq's delay slot, so it runs unconditionally), and the fill
       loop walks a fresh copy `p` of `current` (that is the `move a1,t2`). */
    unsigned int two = 2;
    unsigned int *current = pkt->current;
    unsigned int *openGif = pkt->openGif;
    unsigned long tag = *(unsigned long *)openGif;
    unsigned int nloop = (((int)current - (int)openGif) >> 3) - 2;
    unsigned int flg = (unsigned int)(tag >> 58) & 3;
    unsigned int *p;

    if (flg != 1)
        nloop = nloop >> 1;

    if (flg != two) {
        unsigned int nreg = (unsigned int)(tag >> 60);
        if (nreg == 0)
            nreg = 16;
        nloop = (nloop + nreg - 1) / nreg;
    }

    p = current;
    pkt->openGif = 0;
    *(unsigned long *)openGif = tag + nloop;

    while (((unsigned int)p & 0xC) != 0)
        *p++ = 0;

    pkt->current = p;
}

/* GIF tag template in rodata: NLOOP=0, EOP=0, FLG=PACKED, NREG=1, REGS={A+D} */

/* Defined below under PS2_HARDWARE; the packet builder passes the tag as a
   single 128-bit value (the original does `lq $a1,0($sp)` at the call site). */
void sceVif1PkOpenGifTag(Vif1Packet *pkt, u128 tag);

/* sceVif1PkRefLoadImage lives in its own file, src/sdk/sceVif1PkRefLoadImage.c:
   tools/tu_boundaries.py shows it is translation unit 52 all by itself, and it
   needs different flags (-O2 -G0, i.e. WITHOUT -fno-schedule-insns) from the
   rest of this archive. */

#ifdef PS2_HARDWARE

/* Store a 128-bit quadword into the packet */
void sceVif1PkAddGsPacked(Vif1Packet *pkt, unsigned int *value)
{
    unsigned int *dest = pkt->current;

    /* Equivalent to PS2 'sq' instruction */
    dest[0] = value[0];
    dest[1] = value[1];
    dest[2] = value[2];
    dest[3] = value[3];

    pkt->current += 4;  /* advance by 16 bytes */
}

/* Open a GIF tag and remember its location */
void sceVif1PkOpenGifTag(Vif1Packet *pkt, unsigned int *gifTag)
{
    unsigned int *dest = pkt->current;

    /* Store 128-bit GIF tag (sq equivalent) */
    dest[0] = gifTag[0];
    dest[1] = gifTag[1];
    dest[2] = gifTag[2];
    dest[3] = gifTag[3];

    pkt->current += 4;     /* advance by 16 bytes */
    pkt->openGif = dest;   /* save location of this tag */
}

#endif /* PS2_HARDWARE */