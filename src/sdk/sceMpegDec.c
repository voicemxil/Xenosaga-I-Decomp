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

/* A decoded-picture buffer descriptor (the _rix_/_ri0_ reference images) */
typedef struct {
    char pad00[0x28];
    int nUnk28;                  /* 0x28 */
} REFIMAGE;

/* The output-size request handed to _isOutSizeOK */
typedef struct {
    int nUnk00;
    int nWidth;                  /* 0x04 */
    int nHeight;                 /* 0x08 */
    int nUnk0C;                  /* 0x0C */
    int nUnk10;                  /* 0x10 */
} OUTREQ;

typedef struct {
    char pad000[0x8];
    int nUnk008;                 /* 0x008 */
    char pad00C[0xA0];
    int nUnk0AC;                 /* 0x0AC */
    char pad0B0[0x2C];
    int nUnk0DC;                 /* 0x0DC */
    int nUnk0E0;                 /* 0x0E0 */
    int nUnk0E4;                 /* 0x0E4 */
    int nUnk0E8;                 /* 0x0E8 */
    char pad0EC[0xC];
    int nUnk0F8;                 /* 0x0F8 */
    char pad0FC[0x1C];
    int nUnk118;                 /* 0x118 */
    char pad11C[0x4];
    int nUnk120;                 /* 0x120 */
    char pad124[0x2C];
    int nUnk150;                 /* 0x150 */
    int nUnk154;                 /* 0x154 */
    int nUnk158;                 /* 0x158 */
    int nUnk15C;                 /* 0x15C */
    int nUnk160;                 /* 0x160 */
    char pad164[0x10];
    int nUnk174;                 /* 0x174 */
    char pad178[0x2C];
    int nUnk1A4;                 /* 0x1A4 */
    int nUnk1A8;                 /* 0x1A8 */
    int nUnk1AC;                 /* 0x1AC */
    char pad1B0[0x4];
    int nUnk1B4;                 /* 0x1B4 */
    REFIMAGE *pUnk1B8;                 /* 0x1B8 */
    char pad1BC[0x4];
    REFIMAGE *pUnk1C0;                 /* 0x1C0 */
    REFIMAGE *pUnk1C4;                 /* 0x1C4 */
    REFIMAGE *pUnk1C8;                 /* 0x1C8 */
    char pad1CC[0x4];
    REFIMAGE *pUnk1D0;                 /* 0x1D0 */
    REFIMAGE *pUnk1D4;                 /* 0x1D4 */
    REFIMAGE *pUnk1D8;                 /* 0x1D8 */
    char pad1DC[0x4];
    REFIMAGE *pUnk1E0;                 /* 0x1E0 */
    REFIMAGE *pUnk1E4;                 /* 0x1E4 */
    char pad1E8[0x630];
    int nUnk818;                 /* 0x818 */
    char pad81C[0x4];
    int nUnk820;                 /* 0x820 */
    char pad824[0x1C];
    int nUnk840;                 /* 0x840 */
    int nUnk844;                 /* 0x844 */
    char pad848[0x4];
    int nUnk84C;                 /* 0x84C */
    int nUnk850;                 /* 0x850 */
    int nUnk854;                 /* 0x854 */
    int *pUnk858;                 /* 0x858 */
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

/* ---- picture layer ---------------------------------------------------- */

extern char D_004D5D10[];  /* "load_intra_quantiser_matrix" not supported */
extern char D_004D5D38[];  /* "load_chroma_..._matrix" not supported */
extern char D_004D5D68[];  /* picture_structure error */
extern char D_004D5D88[];  /* unknown picture_coding_type */
extern char D_004D5DA8[];  /* output size rejected: "%d x %d" */

extern void _Error(MPEGSTREAM *pStream, char *pMsg);
extern void _extensionAndUserData(MPEGSTREAM *pStream);
extern void _updateTempTackData(MPEGSTREAM *pStream, int nTempRef);
extern int _pictureData0(MPEGSTREAM *pStream);
extern void _dispRefImage(MPEGSTREAM *pStream, REFIMAGE *pImg, int nIdx);
extern void _dispRefImageField(MPEGSTREAM *pStream, REFIMAGE *pTop,
                               REFIMAGE *pBot, int nIdx);
extern int sprintf(char *pBuf, const char *pFmt, ...);

/* Flag the current picture as already emitted. */
void _markOutput(MPEGSTREAM *pStream)
{
    if (pStream->nUnk008 != 2) {
        pStream->nUnk008 = 2;
        pStream->nUnk0AC = pStream->nUnk118;
    }
    pStream->nUnk820 = 1;
}

/* group_of_pictures_header(): 25-bit time_code, closed_gop, broken_link */
void _groupOfPicturesHeader(MPEGSTREAM *pStream)
{
    pStream->nUnk0E8 = 0;
    pStream->nUnk854 = 1;
    pStream->nUnk84C = pStream->nUnk850 + 1;
    _nextBit(pStream, 1);                        /* drop_frame_flag */
    _nextBit(pStream, 5);                        /* time_code_hours */
    _nextBit(pStream, 6);                        /* time_code_minutes */
    _nextBit(pStream, 1);                        /* marker_bit */
    _nextBit(pStream, 6);                        /* time_code_seconds */
    _nextBit(pStream, 6);                        /* time_code_pictures */
    pStream->nUnk1A4 = _nextBit(pStream, 1);     /* closed_gop */
    pStream->nUnk1A8 = _nextBit(pStream, 1);     /* broken_link */
    _extensionAndUserData(pStream);
}

/* copyright_extension(): every field is discarded */
void _copyrightExtension(MPEGSTREAM *pStream)
{
    _nextBit(pStream, 1);    /* copyright_flag */
    _nextBit(pStream, 8);    /* copyright_identifier */
    _nextBit(pStream, 1);    /* original_or_copy */
    _nextBit(pStream, 7);    /* reserved */
    _nextBit(pStream, 1);    /* marker_bit */
    _nextBit(pStream, 20);   /* copyright_number_1 */
    _nextBit(pStream, 1);    /* marker_bit */
    _nextBit(pStream, 22);   /* copyright_number_2 */
    _nextBit(pStream, 1);    /* marker_bit */
    _nextBit(pStream, 22);   /* copyright_number_3 */
}

/* picture_header() */
void _pictureHeader(MPEGSTREAM *pStream)
{
    int nTempRef;
    int nType;

    nTempRef = _nextBit(pStream, 10);
    pStream->nUnk150 = _nextBit(pStream, 3);
    _nextBit(pStream, 16);                       /* vbv_delay */
    nType = pStream->nUnk150;
    if (nType == 2 || nType == 3) {
        pStream->nUnk154 = _nextBit(pStream, 1); /* full_pel_forward_vector */
        pStream->nUnk158 = _nextBit(pStream, 3); /* forward_f_code */
    }
    if (pStream->nUnk150 == 3) {
        pStream->nUnk15C = _nextBit(pStream, 1); /* full_pel_backward_vector */
        pStream->nUnk160 = _nextBit(pStream, 3); /* backward_f_code */
    }
    _extrainfo(pStream);
    _extensionAndUserData(pStream);
    _updateTempTackData(pStream, nTempRef);
}

/* quant_matrix_extension(): the IPU loads the tables itself */
void _quantMatrixExtension(MPEGSTREAM *pStream)
{
    pStream->nUnk840 = _nextBit(pStream, 1);
    if (pStream->nUnk840 != 0) {
        _waitIpuIdle(pStream);
        _sendIpuCommand(pStream, 0x50000000);
        _waitIpuIdle(pStream);
    }
    pStream->nUnk844 = _nextBit(pStream, 1);
    if (pStream->nUnk844 != 0) {
        _waitIpuIdle(pStream);
        _sendIpuCommand(pStream, 0x58000000);
        _waitIpuIdle(pStream);
    }
    if (_nextBit(pStream, 1) != 0) {
        _Error(pStream, D_004D5D10);
    }
    if (_nextBit(pStream, 1) != 0) {
        _Error(pStream, D_004D5D38);
    }
}

/* Emit one decoded picture to the display list. */
void _outputFrame(MPEGSTREAM *pStream, int nIdx, int nDoIt)
{
    if (nDoIt != 0) {
        int nType;

        nType = pStream->nUnk174;
        if (nType == 3) {
            nType = pStream->nUnk150;
            _dispRefImage(pStream,
                          nType == 3 ? pStream->pUnk1C4 : pStream->pUnk1B8,
                          nIdx - 1);
        } else {
            REFIMAGE *pTop;
            REFIMAGE *pBot;

            nType = pStream->nUnk150;
            if (nType == 3) {
                pTop = pStream->pUnk1D4;
                pBot = pStream->pUnk1E4;
            } else {
                pTop = pStream->pUnk1C8;
                pBot = pStream->pUnk1D8;
            }
            _dispRefImageField(pStream, pTop, pBot, nIdx - 1);
        }
    }
    if (pStream->nUnk0F8 == 1) {
        pStream->nUnk0F8 = 2;
    }
}

/* Reject an output request whose rectangle does not fit the decoded frame. */
int _isOutSizeOK(MPEGSTREAM *pStream, OUTREQ *pReq)
{
    char sBuf[256];
    int bOK;

    if (pStream->nUnk0E0 != 0) {
        if (pStream->nUnk0DC < pReq->nWidth) {
            bOK = 0;
        } else {
            bOK = pStream->nUnk0E0 >= pReq->nHeight;
            /* Keeps this arm's slt/xori tail out of the shared block that
             * gcc's cross-jumping otherwise folds the two arms into. */
            LAUNDER(bOK);
        }
    } else {
        bOK = pStream->nUnk0E4 >= pReq->nUnk0C * pReq->nUnk10;
    }
    if (bOK == 0) {
        sprintf(sBuf, D_004D5DA8, pReq->nWidth, pReq->nHeight);
        _Error(pStream, sBuf);
    }
    return bOK;
}
