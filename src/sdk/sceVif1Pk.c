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
/* TODO: near-miss (REGISTER, 12 words, was 14).  The sq/ld spill, the ld
       order (hi first), the extraction order and the store/pointer shapes all
       match with the ordering below: hi and hi>>32 hoisted ahead of the lo
       block, lo>>32 extracted inline after the first store.  What is left is
       a pure 3-cycle in local-alloc: dest wants $v0 (we give $a2), lo wants
       $v1 (we give $v0), (int)lo wants $a2 (we give $v1).  Permuter-exhausted:
       all 5040 orderings of the head statements (with l0/h0/h1 as temps and
       dest init as a movable statement) and all 720 orderings without the
       temps compile to 12+ diffs; initialising dest late does flip dest into
       $v0 but drags lo into $a1 (the spill slot of the 128-bit arg gives lo
       an $a1 preference) for 14.  No source-level lever found for the last
       rotation. */
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

    /* TODO: near-miss (SCHEDULING, 2 words) - gcc's post-reload scheduler sinks
       the `sw` of the reserved field between the `lui` and the `or` that build
       the control word; the original emits the `or` first.  EXHAUSTED: all
       5040 orderings of the 7 statements below were compiled (tools/permute.py)
       and none reaches 0 - best is this one at 2.  `0x10000000 | flags`,
       `current[i]` instead of `*current`, hoisting the reserved store, a
       fresh temp for the or'd value, a volatile-cast reserved store and an
       early-hoisted constant temp all reproduce the same 2-word swap, so the
       lever is neither statement order nor expression shape.  gcc's own .s
       already has the swap (it is sched2, not gas), and no fix_cc_asm flag
       covers a mid-block swap. */
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

    /* TODO: near-miss (SCHEDULING, 2 words) - see sceVif1PkCnt; identical shape
       with the 0x70000000 end-of-packet code instead of 0x10000000, and the
       same permuter-exhausted `sw`/`or` swap. */
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
/* TODO: near-miss (8 words, was 21).  Three levers found and kept below:
       (1) the zero-fill store is written through `unsigned int **` so that
       under gcc 2.9's type-based aliasing it MAY alias pkt->current - a plain
       int store lets the compiler prove the in-loop `pkt->current = ...` dead
       and sink it out of the loop; (2) the loop is a do-while entered by a
       goto into its middle, reproducing the original's peeled entry (the
       plain while gets either cross-jumped or left in jump-to-test form);
       (3) `t1 = target + 1` in its own statement stops the reassociation
       into target + (mask + 1); (4) the mask-setup $v0/$v1 allocation tie
       -- called permuter-exhausted -- is fixed by PINning amt to $3 and a
       0xFFFFFFFF temp to $2 and assigning both through PASSTHRU, and the
       loop-body cse that rewrote `next = cur + 1` into `addiu v1,v1,4` is
       fixed by pinning cur to $5 and next to $3, so the addiu now reads
       cur exactly as the original does.  8 -> 4 words.
       Remaining: with --swap-adjacent sceVif1PkAlign:6 this reaches TWO
       words (not wired, since the convention is to flag only functions
       that actually match -- add it the moment the last pair falls).
       Those last two are the loop-body pair at words 23/24: the peeled
       entry and the loop body each emit the store BEFORE the pointer
       increment where the original emits it after.  Ruled out: LAUNDER
       and LAUNDER_V on either cur or next at that point (all regress to
       4), every --swap-adjacent site 14..33 with and without the force
       suffix (best 4), and all six orderings of the trailing statements.
       This is gcc sched2 hoisting a ready store over an independent
       addiu; no source lever found. */
    unsigned int p = (padding + 2) & 0x1F;
    PIN(unsigned int amt, "$3");
    PIN(unsigned int all1, "$2");
    unsigned int mask;
    PIN(unsigned int *cur, "$5");
    unsigned int target;

    PASSTHRU(amt, 32 - p);
    PASSTHRU(all1, 0xFFFFFFFFu);
    mask = all1 >> amt;
    cur = pkt->current;
    target = ((unsigned int)cur & ~mask) + (boundary << 2);

    if (target < (unsigned int)cur)
    {
        unsigned int t1 = target + 1;
        target = t1 + mask;
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
void sceVif1PkAddDirectDataN(Vif1Packet *pkt, const long *src, unsigned int count)
{
/* TODO: near-miss (26 words).  The original loop body keeps TWO pointer
       increments per iteration (`addiu a2,a2,4` ... `move v0,a2` +
       `addiu a2,v0,12`, offsets 0/0/4/8) - the body below is written in that
       split form.  gcc 2.9's loop strength reduction instead folds the two
       increments into one `addiu +16`, rewrites the store offsets (-4...)
       and inits `dest+1` in the preheader.  Tried: explicit `d = ++dest`
       intermediate (reduced to a walking giv anyway), int-casting the
       second increment (loop.c sees through it), goto-formed loop (kills
       the LOOP notes: loses the alignment nop and the hoisted 0xffffffff,
       though the increment pair then survives).  Needs a lever that makes
       the walking-giv rewrite unprofitable while keeping loop notes. */
    unsigned int i = count - 1;

    if (count != 0)
    {
        unsigned int *dest = pkt->current;

        do
        {
            long lo = src[0];
            long hi = src[1];
            dest[0] = (int)lo;
            dest++;
            dest[0] = (int)(lo >> 32);
            dest[1] = (int)hi;
            dest[2] = (int)(hi >> 32);
            dest += 3;
            src += 2;
        }
        while (--i != 0xFFFFFFFF);

        pkt->current = dest;
    }
}

/* Add multiple 128-bit UPK data units to the VIF1 packet */
void sceVif1PkAddUpkData128N(Vif1Packet *pkt, const long *src, unsigned int count)
{
/* TODO: near-miss (26 words) - identical body to sceVif1PkAddDirectDataN
       (same original instruction stream); same induction-variable folding
       problem, see the analysis on that function. */
    unsigned int i = count - 1;

    if (count != 0)
    {
        unsigned int *dest = pkt->current;

        do
        {
            long lo = src[0];
            long hi = src[1];
            dest[0] = (int)lo;
            dest++;
            dest[0] = (int)(lo >> 32);
            dest[1] = (int)hi;
            dest[2] = (int)(hi >> 32);
            dest += 3;
            src += 2;
        }
        while (--i != 0xFFFFFFFF);

        pkt->current = dest;
    }
}

/* Initialize a VIF1 packet for an UNPACK code block */
void sceVif1PkOpenUpkCode(Vif1Packet *pkt, unsigned int a1, unsigned int a2, unsigned int a3, unsigned int t0)
{
    /* TODO: near-miss (28 words, was 34).  Two causes were fixed here: the
       original walks the pointer (`*cur = w0; cur++; *cur = w1;
       pkt->current = cur + 1; pkt->openDirect = cur;`) rather than indexing
       cur[0]/cur[1], and `a3 | 0x1000000` must be its own statement or gcc
       reassociates it into `(t0 << 8 | 0x1000000) | a3`.  What is left is
       scheduling: the original defers `sw` of the second word and of
       pkt->current to the end of the block (after the mult/movn group) and
       materialises the 256 constant early into its own register, copying it
       twice (`move t4,t2` / `move t3,t2`) - we fold one copy away, so we are
       one instruction short.  Inverting the `lo < hi` test does not help. */
    unsigned int *current = pkt->current;
    unsigned int vl, vn;
    unsigned int size, lo, hi, code, c0;

    c0 = a3 | 0x1000000;
    *current = (t0 << 8) | c0;
    current++;
    vn = (a2 >> 2) & 3;
    vl = a2 & 3;
    *current = (a2 << 24) | (a1 & 0xFFFF);
    pkt->current = current + 1;
    pkt->openDirect = current;

    size = (0x20 >> vl) * (vn + 1);
    lo = a3 ? a3 : 0x100;
    hi = t0 ? t0 : 0x100;
    if (lo < hi) {
        size = size * lo;
        code = hi << 16;
    } else {
        code = 0x10000;
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
extern const long D_004D53C0[2];

/* Defined below under PS2_HARDWARE; the packet builder passes the tag as a
   single 128-bit value (the original does `lq $a1,0($sp)` at the call site). */
void sceVif1PkOpenGifTag(Vif1Packet *pkt, u128 tag);

/* Build a VIF1 packet that uploads an image to the GS: a DIRECTHL block sets up
   BITBLTBUF/TRXPOS/TRXREG/TRXDIR, then the pixel data is chained in as REF
   transfers of at most 32767 quadwords each. */
/* TODO: near-miss (58 of 122 words).  Rewritten from the disassembly: length
   parity, identical control flow and identical constants.  The whole middle of
   the REF loop (the min(), the Cnt/Align/AddCode/Reserve sequence, the GIF-tag
   movz and the qwc/addr updates) is word-exact.  What is left is a callee-saved
   allocation permutation plus entry-block scheduling:
     - two independent allocator cycles - {dpsm,rrw} want $s3/$s2 (we produce
       $s2/$s3) and {addr,dsay,dsax} want $s4/$s5/$s6 (we produce $s6/$s4/$s5);
     - gcc emits the BITBLTBUF shifts in the entry block instead of after the
       OpenGifTag call, and orders the sd/move interleave in the prologue
       differently.
   Ruled out: all 6 orderings of the three mask statements, masking inline at
   the AddGsAD call sites (114 words - much worse), separate locals for the
   masks, struct-typed / whole-struct-copy / reversed-element forms of the
   D_004D53C0 read, and swapped operand order in the TRXPOS/TRXREG expressions.
   Two levers did help and are kept below: reading the rodata halves into
   `g0`/`g1` first (aligns the lui/addiu pair, -2 words) and materialising the
   EOP GIF tag constant before the non-EOP one (-2 words).  Introducing a fresh
   `src` local for the walking address aligns the entire prologue (indices 0-28)
   but pushes `pkt` into $s8 and spills, ending up worse (62). */
void sceVif1PkRefLoadImage(Vif1Packet *pkt, unsigned int dbp, unsigned int dpsm,
                           unsigned int dbw, unsigned int addr, unsigned int qwc,
                           unsigned int dsax, unsigned int dsay,
                           unsigned int rrw, unsigned int rrh)
{
    U128 tag;
    long g0, g1;

    dpsm = dpsm & 0xFF;
    dbw  = dbw & 0xFFFF;
    dbp  = dbp & 0xFFFF;

    g0 = D_004D53C0[0];
    g1 = D_004D53C0[1];
    tag.d[0] = g0;
    tag.d[1] = g1;

    sceVif1PkCnt(pkt, 0);
    sceVif1PkOpenDirectHLCode(pkt, 0);
    sceVif1PkOpenGifTag(pkt, tag.q);

    sceVif1PkAddGsAD(pkt, 0x50, ((unsigned long)dbp << 32) | ((unsigned long)dbw << 48)
                                | ((unsigned long)dpsm << 56));
    sceVif1PkAddGsAD(pkt, 0x51, ((unsigned long)dsax << 32) | ((unsigned long)dsay << 48));
    sceVif1PkAddGsAD(pkt, 0x52, (unsigned long)rrw | ((unsigned long)rrh << 32));
    sceVif1PkAddGsAD(pkt, 0x53, 0);

    sceVif1PkCloseGifTag(pkt);
    sceVif1PkCloseDirectHLCode(pkt);

    if (qwc != 0)
    {
        do
        {
            unsigned int n;
            unsigned long *p;
            unsigned long gt;

            n = (0x7FFF < qwc) ? 0x7FFF : qwc;

            sceVif1PkCnt(pkt, 0);
            sceVif1PkAlign(pkt, 2, 3);
            sceVif1PkAddCode(pkt, 0x51000001);
            p = (unsigned long *)sceVif1PkReserve(pkt, 4);

            gt = (unsigned long)n | 0x0800000000008000L;
            if (n != qwc)
                gt = (unsigned long)n | 0x0800000000000000L;
            p[1] = 0;
            p[0] = gt;

            sceVif1PkRef(pkt, addr, n, 0, n | 0x51000000, 0);

            qwc -= n;
            addr += n << 4;
        }
        while (qwc != 0);
    }
}



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