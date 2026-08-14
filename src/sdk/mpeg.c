/* IPU-assisted MPEG2 sequence-layer bitstream parsing */

typedef unsigned char u_char;
typedef unsigned short u_short;
typedef unsigned int u_int;

typedef struct {
    char pad000[0xD4];
    int nUnk0D4;              /* 0x0D4 */
    char pad0D8[0x124 - 0xD4 - 4];
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
    char pad150[0x840 - 0x14C - 4];
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

/* TODO: near-miss (LOGIC, 10/73 words) - built with the 2.9-ee SDK compiler
 * (-O2 -G0 -fno-schedule-insns, confirmed via mpeg.c's SDK_FILES entry in
 * configure.py: it reproduces the original's 16-byte-per-saved-register
 * frame padding that the 2.96 game compiler cannot). Placing the dead
 * `p->nUnk0D4 = 0;` store before the first _nextBit() call (rather than
 * after, which the natural statement order would suggest) fixed the
 * prologue delay-slot filler and cut diffs from 15 to 10 -- confirmed
 * exhaustively via tools/permute.py. Remaining diffs: (1) the `v0 >= 2801`
 * check's slti target register is v0 here vs a0 in the original -- tried
 * comparing p->nUnk128 instead of the local, no change; (2) an analogous
 * swap on the `(v1>>1)&0x3FF` / `v1>>12` pair; (3) the two _setDefaultQM
 * calls' argument-setup instruction order (li a1,<cmd> vs the QM pointer
 * lui/addiu) is reordered relative to the original. All three look like
 * the same class of register/scheduling tie-break as the store-order fix
 * above, just not reachable with the source shapes tried in budget. */
/* Parse the MPEG2 sequence_header(), pick default quant matrices, kick the IPU */
void _sequenceHeader(MPEGSTREAM *pStream)
{
    MPEGSTREAM *p;
    u_int v1;
    int v0;

    p = pStream;
    p->nUnk0D4 = 0;
    v1 = _nextBit(p, 32);
    v0 = (v1 >> 8) & 0xFFF;
    p->nUnk124 = v1 >> 20;
    p->nUnk128 = v0;
    if (v0 >= 2801) {
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

/* TODO: near-miss (LOGIC, 39/76 words) - SDK-compiler (2.9-ee) prologue and
 * the delay-slot store of nUnk848=1 already match exactly. The 4-way bit
 * extraction from the 28-bit read gets the right VALUES but wrong temp
 * registers (v0/a0 instead of v1/v0 for nExtH/nChroma); tools/permute.py
 * swept all 24 statement orderings of the four extractions and the best is
 * 37 diffs (2 orderings tie), not a full match, so this is a register-
 * allocation tie-break rather than a source-order issue. The tail (field
 * update block combining old/new bits) is also unverified against the
 * original's exact store order/registers. */
/* Parse sequence_extension(): chroma format, size/rate extensions, MPEG1 sanity */
void _sequenceExtension(MPEGSTREAM *pStream)
{
    u_int v0, v1, v2;
    u_int nChroma, nHigh, nExtV, nExtH, nExtRate, nExtBit;

    pStream->nUnk848 = 1;
    _ipuSetMPEG1(0);

    v1 = _nextBit(pStream, 28);
    nExtH = (v1 >> 1) & 0xFFF;
    nChroma = (v1 >> 0x11) & 3;
    nExtRate = (v1 >> 0xd) & 3;
    nExtV = (v1 >> 0xf) & 3;
    pStream->nUnk140 = nChroma;
    if (nChroma != 1) {
        _Error(pStream, D_004D5A10);
    }

    pStream->nUnk13C = (v1 >> 0x13) & 1;
    v2 = _nextBit(pStream, 16);
    nHigh = v1 >> 0x14;
    nExtBit = v2 >> 8;
    if (nHigh != 0x48 && nHigh != 0x58 && nHigh != 0x44) {
        _Error(pStream, D_004D5A38);
    }

    v0 = pStream->nUnk124;
    v1 = pStream->nUnk128;
    pStream->nUnk138 += nExtBit << 0xa;
    pStream->nUnk124 = (nExtV << 0xc) | (v0 & 0xFFF);
    pStream->nUnk128 = (nExtRate << 0xc) | (v1 & 0xFFF);
    pStream->nUnk134 += nExtH << 0x12;
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
