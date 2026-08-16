#include "matching.h"

/* MPEG2 picture/slice-layer decoder helpers (the IPU-driving half of the
 * SDK's MPEG library).  This is a SEPARATE translation unit from mpeg.c:
 * mpeg.c's sequence-layer parsers only match with -fno-schedule-insns,
 * while this TU was plainly built WITH the first scheduling pass -- its
 * address materialisation is interleaved around an intervening store,
 * which -fno-schedule-insns can never produce. */

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

extern u_int _nextBit(MPEGSTREAM *pStream, u_int nBits);

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
