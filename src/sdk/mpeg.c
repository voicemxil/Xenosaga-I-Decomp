#include "matching.h"

/* IPU-assisted MPEG2 sequence-layer bitstream parsing */

typedef unsigned char u_char;
typedef unsigned short u_short;
typedef unsigned int u_int;

typedef struct {
    char pad000[0x8];
    int nUnk008;              /* 0x008 */
    char pad00C[0xAC - 0x8 - 4];
    int nUnk0AC;              /* 0x0AC */
    char pad0B0[0xD4 - 0xAC - 4];
    int nUnk0D4;              /* 0x0D4 */
    char pad0D8[0x118 - 0xD4 - 4];
    int nUnk118;              /* 0x118 */
    char pad11C[0x124 - 0x118 - 4];
    int nUnk124;               /* 0x124 */
    int nUnk128;                /* 0x128 */
    char pad12C[0x134 - 0x128 - 4];
    int nUnk134;                 /* 0x134 */
    int nUnk138;                  /* 0x138 */
    int nUnk13C;                   /* 0x13C */
    int nUnk140;                    /* 0x140 */
    int nUnk144;                     /* 0x144 */
    int nUnk148;                      /* 0x148 */
    int nUnk14C;                       /* 0x14C */
    char pad150[0x1B4 - 0x14C - 4];
    int nUnk1B4;                        /* 0x1B4 */
    char pad1B8[0x818 - 0x1B4 - 4];
    int nUnk818;                        /* 0x818 */
    char pad81C[0x820 - 0x818 - 4];
    int nUnk820;                        /* 0x820 */
    char pad824[0x840 - 0x820 - 4];
    int nUnk840;                        /* 0x840 */
    int nUnk844;                         /* 0x844 */
    int nUnk848;                          /* 0x848 */
    char pad84C[0x858 - 0x848 - 4];
    int *pUnk858;                          /* 0x858 */
} MPEGSTREAM;

extern char D_004D5A58[];   /* _sequenceHeader error string 1 */
extern char D_004D59F8[];   /* _sequenceHeader error string 2 */
extern char D_004D5A10[];   /* _sequenceExtension error string 1 */
extern char D_004D5A38[];   /* _sequenceExtension error string 2 */
extern u_char D_004AD5C0[]; /* default intra quantizer matrix */
extern u_char D_004AD600[]; /* default non-intra quantizer matrix */

u_int _nextBit(MPEGSTREAM *pStream, u_int nBits);
void _Error(MPEGSTREAM *pStream, char *pMsg);
void _waitIpuIdle(MPEGSTREAM *pStream);
void _sendIpuCommand(MPEGSTREAM *pStream, u_int nCmd);
void _setDefaultQM(MPEGSTREAM *pStream, u_int nCmd, void *pQM);
void _extensionAndUserData(MPEGSTREAM *pStream);
void _initSeq(int *pUnk858);
void _ipuSetMPEG1(int flag);

/* Levers, all of them register tie-breaks the SDK compiler (2.9-ee, -O2 -G0
 * -fno-schedule-insns) resolves the other way round:
 *   - the dead `p->nUnk0D4 = 0;` store goes BEFORE the first _nextBit() call,
 *     which is what fills the prologue delay slot;
 *   - the `v0 >= 2801` test is named in its own $a0-pinned local so it is
 *     evaluated BEFORE `p->nUnk128 = v0`.  That keeps $v0 live across the
 *     compare, which is exactly why the original puts the sltu result in $a0
 *     and the nUnk128 store in the branch delay slot.
 * Two pure post-reload reorders are left to FILE_FIX_FLAGS: an andi/srl
 * transpose at site 26 and the two _setDefaultQM calls, whose delay slot the
 * original fills with the `li $a1,cmd` rather than the quantiser-matrix
 * `addiu` (--swap-into-slot sites 7 and 12). */
/* Parse the MPEG2 sequence_header(), pick default quant matrices, kick the IPU */
void _sequenceHeader(MPEGSTREAM *pStream)
{
    MPEGSTREAM *p;
    u_int v1;
    int v0;
    PIN(int lt, "$4");

    p = pStream;
    p->nUnk0D4 = 0;
    v1 = _nextBit(p, 32);
    v0 = (v1 >> 8) & 0xFFF;
    p->nUnk124 = v1 >> 20;
    lt = (v0 < 2801);
    p->nUnk128 = v0;
    if (!lt) {
        _Error(p, D_004D5A58);
    }

    v1 = _nextBit(p, 30);
    v0 = (v1 >> 1) & 0x3FF;
    p->nUnk134 = v1 >> 12;
    p->nUnk138 = v0;

    v0 = _nextBit(p, 1);
    p->nUnk840 = v0;
    if (v0 != 0) {
        _waitIpuIdle(p);
        _sendIpuCommand(p, 0x50000000);
        _waitIpuIdle(p);
    } else {
        _setDefaultQM(p, 0x50000000, D_004AD5C0);
    }

    v0 = _nextBit(p, 1);
    p->nUnk844 = v0;
    if (v0 != 0) {
        _waitIpuIdle(p);
        _sendIpuCommand(p, 0x58000000);
        _waitIpuIdle(p);
    } else {
        _setDefaultQM(p, 0x58000000, D_004AD600);
    }

    _extensionAndUserData(p);
    _initSeq(p->pUnk858);
}

/* Shape notes.  Three things had to be right:
 *   1. `nHigh = v1 >> 0x14` is computed BEFORE the 16-bit _nextBit() call --
 *      the original puts it in that call's delay slot, which is also what
 *      keeps nHigh in $s0 and lets $s2 be reused for nExtBit.
 *   2. The tail reads all four fields and computes all four shifted deltas
 *      before combining, then stores 138/124/128/134 in that order.  Writing
 *      it as read-modify-write per field interleaves the loads with the
 *      arithmetic and costs 17 words.
 *   3. The four bit extractions and the two field reads are a register
 *      permutation gcc gets wrong; the temps are pinned to the original's
 *      $v1/$v0/$a0 and $a0/$v1.  Only these five pins are load-bearing --
 *      the shifted deltas fall into $t0/$t1/$a3/$a1 on their own. */
/* Parse sequence_extension(): chroma format, size/rate extensions, MPEG1 sanity */
void _sequenceExtension(MPEGSTREAM *pStream)
{
    u_int v1, v2;
    u_int nHigh, nExtV, nExtH, nExtRate, nExtBit;
    PIN(u_int o124, "$4");
    PIN(u_int o128, "$3");
    u_int o134, o138;
    u_int n124, n128, n134, n138;
    u_int sH;
    u_int sB;
    u_int sV;
    PIN(u_int sR, "$5");
    PIN(u_int th, "$3");
    PIN(u_int tc, "$2");
    PIN(u_int tr, "$4");

    pStream->nUnk848 = 1;
    _ipuSetMPEG1(0);

    v1 = _nextBit(pStream, 28);
    th = v1 >> 1;
    nExtH = th & 0xFFF;
    tc = v1 >> 0x11;
    tc = tc & 3;
    tr = v1 >> 0xd;
    nExtRate = tr & 3;
    nExtV = (v1 >> 0xf) & 3;
    pStream->nUnk140 = tc;
    if (tc != 1) {
        _Error(pStream, D_004D5A10);
    }

    pStream->nUnk13C = (v1 >> 0x13) & 1;
    nHigh = v1 >> 0x14;
    v2 = _nextBit(pStream, 16);
    nExtBit = v2 >> 8;
    if (nHigh != 0x48 && nHigh != 0x58 && nHigh != 0x44) {
        _Error(pStream, D_004D5A38);
    }

    sH = nExtH << 0x12;
    sB = nExtBit << 0xa;
    sV = nExtV << 0xc;
    sR = nExtRate << 0xc;
    o124 = pStream->nUnk124;
    o128 = pStream->nUnk128;
    o134 = pStream->nUnk134;
    o138 = pStream->nUnk138;
    n124 = sV | (o124 & 0xFFF);
    n128 = sR | (o128 & 0xFFF);
    n134 = o134 + sH;
    n138 = o138 + sB;
    pStream->nUnk138 = n138;
    pStream->nUnk124 = n124;
    pStream->nUnk128 = n128;
    pStream->nUnk134 = n134;
}

/* Parse sequence_display_extension(): optional colour description + display size */
void _sequenceDisplayExtension(MPEGSTREAM *pStream)
{
    u_int v0;

    _nextBit(pStream, 3);
    v0 = _nextBit(pStream, 1);
    if (v0 != 0) {
        _nextBit(pStream, 8);
        _nextBit(pStream, 8);
        pStream->nUnk144 = _nextBit(pStream, 8);
    }
    pStream->nUnk148 = _nextBit(pStream, 14);
    _nextBit(pStream, 1);
    pStream->nUnk14C = _nextBit(pStream, 14);
}

/* Scalable-coding extensions are not supported by this build */
void _sequenceScalableExtension(MPEGSTREAM *pStream)
{
    _Error(pStream, D_004D5A58);
}

/* ---- IPU register plumbing -------------------------------------------- */

#define IPU_CMD  (*(volatile u_int *)0x10002000)
#define IPU_CTRL (*(volatile u_int *)0x10002010)
#define IPU_TOP  (*(volatile u_int *)0x10002020)

extern u_int D_004AD680[];   /* per-command result register table */

extern void _flushBuf(MPEGSTREAM *pStream, u_int nBits);
extern u_int _peepBit(MPEGSTREAM *pStream, u_int nBits);
extern void _ipuVdec(MPEGSTREAM *pStream, int nTbl);
extern void _dispatchMpegCbNodata(int *pCb);

/* MPEG1 bit of IPU_CTRL (bit 23) */
void _ipuSetMPEG1(int flag)
{
    IPU_CTRL = (IPU_CTRL & 0xFF7FFFFF) | (flag << 23);
}

/* Post a command word and latch the result register it reports into. */
void _sendIpuCommand(MPEGSTREAM *pStream, u_int nCmd)
{
    IPU_CMD = nCmd;
    pStream->nUnk818 = D_004AD680[nCmd >> 28];
}

/* Spin until the IPU is idle, pumping the no-data callback if it stalls. */
void _waitIpuIdle(MPEGSTREAM *pStream)
{
    int i;

    i = 0;
    while ((IPU_CTRL & 0x80004000) == 0x80000000) {
        if (i++ > 5000) {
            _dispatchMpegCbNodata(pStream->pUnk858);
            i = 0;
        }
    }
}

/* Byte-align, then advance to the next 24-bit start code prefix. */
void _nextStartCode(MPEGSTREAM *pStream)
{
    u_int nSkip;

    _waitIpuIdle(pStream);
    nSkip = (0 - (IPU_TOP & 7)) & 7;
    if (nSkip != 0) {
        _flushBuf(pStream, nSkip);
    }
    while (_peepBit(pStream, 24) != 1) {
        _flushBuf(pStream, 8);
    }
}

/* extra_bit_* / extra_information_* : drop bytes while the marker is set */
void _extrainfo(MPEGSTREAM *pStream)
{
    while (_nextBit(pStream, 1) != 0) {
        _flushBuf(pStream, 8);
    }
}

/* slice() header, B-picture variant */
int _sliceB(MPEGSTREAM *pStream)
{
    pStream->nUnk1B4 = _nextBit(pStream, 5);
    if (_nextBit(pStream, 1) != 0) {
        _nextBit(pStream, 1);
        _flushBuf(pStream, 7);
        _extrainfo(pStream);
    }
    return 0;
}

/* dual-prime motion vector: vdec table 3 */
void _dmVector(MPEGSTREAM *pStream)
{
    _ipuVdec(pStream, 3);
}
