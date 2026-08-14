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

/* Reserve space for a number of 32-bit values */
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
    /* TODO: near-miss (14 words).  The sq/ld union spill, the dsll32+dsra32
       sign-extractions and the dest++ / dest+3 pointer stepping all match; only
       the order of the four extractions differs.  The original emits
       hi>>32, (int)lo, (int)hi, [store], lo>>32; we emit them in source order.
       Hoisting any of them into named temps before the stores flips the two
       `ld`s (d[0] gets loaded first instead of d[1]) and costs 2 more words,
       for every ordering tried - so the extraction order is a scheduler
       artefact, not source order. */
    U128 u;
    unsigned int *dest = pkt->current;
    long lo, hi;

    u.q = value;
    lo = u.d[0];
    hi = u.d[1];
    dest[0] = (int)lo;
    dest++;
    dest[0] = (int)(lo >> 32);
    pkt->current = dest + 3;
    dest[1] = (int)hi;
    dest[2] = (int)(hi >> 32);
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

    /* Patch the size of an open DIRECT/HL code if one exists */
    if (open != 0)
    {
        int n = (((int)current - (int)open) >> 4) - 1;
        *open = *open + n;
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
       `current[i]` instead of `*current`, and hoisting the reserved store all
       reproduce the same 2-word swap, so the lever is not statement order. */
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
    /* TODO: near-miss (20 words).  ROOT CAUSE ISOLATED: gcc sinks the
       `pkt->current = cur` store out of the fill loop (our `sw v1,0(a0)` lands
       after the `bnez`), while the original keeps it inside and therefore peels
       the first iteration (`b` into the middle of the rotated loop).  Rewriting
       the body with a fresh `next` local, and `(target + 1) + mask` explicitly
       parenthesised, changes nothing - the store still sinks.  The remaining
       $v0/$v1 swap in the mask setup is downstream of that.  Next lever to try
       is something that defeats the sink (e.g. re-reading pkt->current). */
    unsigned int p = (padding + 2) & 0x1F;
    unsigned int mask = 0xFFFFFFFF >> (32 - p);
    unsigned int *cur = pkt->current;
    unsigned int target = ((unsigned int)cur & ~mask) + (boundary << 2);

    if (target < (unsigned int)cur)
        target = target + 1 + mask;

    while ((unsigned int)cur < target)
    {
        *cur++ = 0;
        pkt->current = cur;
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
    /* TODO: near-miss (25 words).  ROOT CAUSE ISOLATED: the original loop body
       is exactly sceVif1PkAddUpkData128's shape - `dest[0] = lo; dest++;
       dest[0] = lo>>32; dest[1] = hi; dest[2] = hi>>32; dest += 3;` - which the
       original compiles to `addiu a2,a2,4` + `move v0,a2` (a strength-reduced
       giv) + offsets 0/4/8 off the copy.  In a loop gcc 2.9 instead FOLDS the
       two increments into one `addiu ...,16` and rewrites the offsets, so the
       `move` never appears.  Tried: fresh `next`/`p` locals for the advanced
       pointer (folds anyway), and hoisting `src += 2` (gcc then hoists the
       `+4` out of the loop entirely, 27 words).  Needs a lever that blocks the
       induction-variable rewrite. */
    unsigned int i = count - 1;

    if (count != 0)
    {
        unsigned int *dest = pkt->current;

        do
        {
            long lo = src[0];
            long hi = src[1];
            dest[0] = (int)lo;
            dest[1] = (int)(lo >> 32);
            dest[2] = (int)hi;
            dest[3] = (int)(hi >> 32);
            dest += 4;
            src += 2;
        }
        while (--i != 0xFFFFFFFF);

        pkt->current = dest;
    }
}

/* Add multiple 128-bit UPK data units to the VIF1 packet */
void sceVif1PkAddUpkData128N(Vif1Packet *pkt, const long *src, unsigned int count)
{
    /* TODO: near-miss (29 words) - identical body to sceVif1PkAddDirectDataN
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
            dest[1] = (int)(lo >> 32);
            dest[2] = (int)hi;
            dest[3] = (int)(hi >> 32);
            dest += 4;
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
    /* TODO: near-miss (REGISTER, 13 words).  Structure now matches exactly: the
       `nloop >>= 1` really is outside the `flg != 2` test in the original (the
       movn sits in the beq's delay slot, so it runs unconditionally), and the
       fill loop walks a fresh copy `p` of `current` (that is the `move a1,t2`).
       What is left is a 3-cycle in local-alloc: flg wants $v1, `nloop >> 1`
       wants $a0 and the literal 2 wants $a1; we get $a0/$a1/$v1.  Neither
       declaration order, statement order nor `if (2 != flg)` moves it.  Pinning
       `nloop >> 1` to $4 and `p` to $5 gets it down to 5 words, but gcc 2.9
       silently ignores an __asm__ register spec on `flg` (any register), so the
       last swap cannot be forced from the source. */
    unsigned int *current = pkt->current;
    unsigned int *openGif = pkt->openGif;
    unsigned long tag = *(unsigned long *)openGif;
    unsigned int nloop = (((int)current - (int)openGif) >> 3) - 2;
    unsigned int flg = (unsigned int)(tag >> 58) & 3;
    unsigned int *p;

    if (flg != 1)
        nloop = nloop >> 1;

    if (flg != 2) {
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

/* Build a VIF1 packet to load an image into the GS */
void sceVif1PkRefLoadImage(Vif1Packet *pkt, unsigned int a1, unsigned int a2, unsigned int a3, 
                           unsigned int t0, unsigned int t1, unsigned int t2, unsigned int t3,
                           unsigned int fp)
{
    unsigned int s0, s1, s2, s3, s4, s5, s6, s7;

    /* save registers (represented as locals here) */
    s7 = (unsigned int)pkt;
    s6 = t2;
    s5 = t3;
    s4 = t0;
    s3 = a2 & 0xFF;
    s1 = a3 & 0xFFFF;
    s0 = a1 & 0xFFFF;
    fp = fp;
    
    /* start building packet */
    sceVif1PkCnt(s7, 0);
    sceVif1PkOpenDirectHLCode(s7, 0);

    /* open GIF tag for data */
    sceVif1PkOpenGifTag(s7, *(unsigned long long *)0); // value from stack/sp

    /* build GS ADDR packet entries */
    s0 = (s0 << 0) | (s1 << 16);
    s3 <<= 24;
    sceVif1PkAddGsAD(s7, 0x50, s0 | s3);

    s6 <<= 0;
    s5 <<= 16;
    sceVif1PkAddGsAD(s7, 0x51, s6 | s5);

    s2 <<= 0;
    s2 >>= 0;
    s0 = *(unsigned int *)(0xC8); // load some value
    s0 <<= 0;
    sceVif1PkAddGsAD(s7, 0x52, s2 | s0);

    /* close GIF tag and Direct HL code */
    sceVif1PkCloseGifTag(s7);
    sceVif1PkCloseDirectHLCode(s7);

    /* optional: loop for remaining image data */
    if (fp)
    {
        unsigned int s5_reg = 0x08000000 | 0x8000;
        unsigned int s3_reg = 0x13000;
        unsigned int s2_reg = 0x51000000;

        do
        {
            unsigned int s0_reg, v1, a1, a2, a0, t0, v1_alt;
            s0_reg = (fp < fp) ? fp : fp;
            sceVif1PkCnt(s7, 0);
            sceVif1PkAlign(s7, 2, 3);
            sceVif1PkAddCode(s7, 0x51000001);
            sceVif1PkReserve(s7, 4);

            v1 = s0_reg;
            a1 = s0_reg ^ fp;
            a2 = s0_reg;
            a0 = v1 | s5_reg;
            t0 = s0_reg | s2_reg;
            v1_alt = v1 | s3_reg;

            /* reference the image chunk */
            sceVif1PkRef(s7, s4, a1, a2, s0, fp);

            s0_reg <<= 4;
            fp -= s0_reg;
            s4 += s0_reg;
        } while (fp != 0);
    }

    /* restore registers */
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