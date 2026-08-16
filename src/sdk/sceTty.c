/* PS2 SDK sceTty trivial accessors (raw-address inline asm, see sceSif.c) */

void sceResetttyinit(void)
{
    __asm__ __volatile__(
        "lui $2,0x4b\n\t"
        "sw $0,-21712($2)"
    );
}

/* ------------------------------------------------------------------
 * sceTtyInit: open the DECI2 TTY protocol channel.
 *
 * `tinfo` is the driver's state block and `wbuf` / `rbuf` are its two
 * DMA buffers; both buffer pointers are stored as uncached-accelerated
 * aliases (| 0x20000000) because the DECI2 hardware writes them, and
 * the outgoing packet header is filled in through that same alias.
 * All three are named fixed addresses from config/symbol_addrs.txt.
 *
 * PARKED NEAR-MISS, 14 diffs of 47 words, RIGHT LENGTH, and the
 * differing words are the SAME instruction multiset in a different
 * order: the original interleaves the three zero-stores into `tinfo`
 * with the two `%hi/%lo | 0x20000000` address computations, where we
 * emit them in two groups.
 *
 * The `sock` field must be volatile -- the original stores the
 * sceDeci2Open result and then RELOADS it for the sign test, which no
 * non-volatile spelling reproduces (that alone was 47 vs 46 words).
 *
 * NOTE FOR THE NEXT AGENT: the interleaving is scheduler output, so
 * this TU looks like another `FILE_CFLAGS_OVERRIDE = "-O2 -G0"` file
 * (scePad.c / sceCd.c / sceSif.c / sceMc.c).  With that flag set the
 * residue drops from 19-20 diffs to 14; without it, nothing gets below
 * 19.  The override is NOT set, because 14 is not a match and an
 * unproven flag on a shared file is worse than none -- set it again
 * when a second sceTty function confirms it.  Statement orders swept
 * at 14: rbuf-alias before wbuf-alias and after, the zero-stores
 * before/between/after the alias computations, and both orders of the
 * final `w->pad` / `w->src` stores.
 * ------------------------------------------------------------------ */

typedef struct t_ttyinfo {
    volatile int sock;          /* +0  DECI2 socket, negative on failure */
    int   rpos;                 /* +4  */
    int   wpos;                 /* +8  */
    int   state;                /* +12 */
    void *wbuf;                 /* +16 */
    void *rbuf;                 /* +20 */
    void *queue;                /* +24 */
} ttyinfo_t;

/* The DECI2 packet header that sits at the head of `wbuf`. */
typedef struct t_deci2hdr {
    short          len;         /* +0  filled in per send */
    short          pad;         /* +2  */
    short          size;        /* +4  */
    unsigned char  proto;       /* +6  */
    unsigned char  dst;         /* +7  */
    int            src;         /* +8  */
} deci2hdr_t;

extern ttyinfo_t tinfo;
extern char wbuf[];
extern char rbuf[];
extern void FlushCache(int mode);
extern int sceDeci2Open(int proto, void *param, void *handler);
extern void sceTtyHandler(void);
extern void *QueueInit(int size);

int sceTtyInit(void)
{
    deci2hdr_t *w;
    void *r;

    FlushCache(0);
    tinfo.sock = sceDeci2Open(528, &tinfo, (void *)sceTtyHandler);
    if (tinfo.sock < 0)
        return 0;
    tinfo.state = 0;
    tinfo.rpos = 0;
    tinfo.wpos = 0;
    r = (void *)((int)rbuf | 0x20000000);
    w = (deci2hdr_t *)((int)wbuf | 0x20000000);
    tinfo.rbuf = r;
    tinfo.wbuf = w;
    w->size = 528;
    w->proto = 69;
    w->dst = 72;
    w->pad = 0;
    w->src = 0;
    tinfo.queue = QueueInit(256);
    return 1;
}
