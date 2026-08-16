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
    int nUnk11C;                 /* 0x11C */
    int nUnk120;                 /* 0x120 */
    char pad124[0x8];
    int nUnk12C;                 /* 0x12C */
    char pad130[0xC];
    int nUnk13C;                 /* 0x13C */
    char pad140[0x10];
    int nUnk150;                 /* 0x150 */
    int nUnk154;                 /* 0x154 */
    int nUnk158;                 /* 0x158 */
    int nUnk15C;                 /* 0x15C */
    int nUnk160;                 /* 0x160 */
    char pad164[0x10];
    int nUnk174;                 /* 0x174 */
    int nUnk178;                 /* 0x178 */
    char pad17C[0x8];
    int nUnk184;                 /* 0x184 */
    char pad188[0x4];
    int anUnk18C[3];             /* 0x18C */
    int anUnk198[3];             /* 0x198 */
    int nUnk1A4;                 /* 0x1A4 */
    int nUnk1A8;                 /* 0x1A8 */
    int nUnk1AC;                 /* 0x1AC */
    int nUnk1B0;                 /* 0x1B0 */
    int nUnk1B4;                 /* 0x1B4 */
    REFIMAGE *pUnk1B8;           /* 0x1B8 */
    char pad1BC[0x4];
    REFIMAGE *pUnk1C0;           /* 0x1C0 */
    REFIMAGE *pUnk1C4;           /* 0x1C4 */
    REFIMAGE *pUnk1C8;           /* 0x1C8 */
    char pad1CC[0x4];
    REFIMAGE *pUnk1D0;           /* 0x1D0 */
    REFIMAGE *pUnk1D4;           /* 0x1D4 */
    REFIMAGE *pUnk1D8;           /* 0x1D8 */
    char pad1DC[0x4];
    REFIMAGE *pUnk1E0;           /* 0x1E0 */
    REFIMAGE *pUnk1E4;           /* 0x1E4 */
    char pad1E8[0x628];
    int nUnk810;                 /* 0x810 */
    char pad814[0x4];
    int nUnk818;                 /* 0x818 */
    char pad81C[0x4];
    int nUnk820;                 /* 0x820 */
    char pad824[0x4];
    long long llUnk828;          /* 0x828 */
    long long llUnk830;          /* 0x830 */
    int nUnk838;                 /* 0x838 */
    int nUnk83C;                 /* 0x83C */
    int nUnk840;                 /* 0x840 */
    int nUnk844;                 /* 0x844 */
    int nUnk848;                 /* 0x848 */
    int nUnk84C;                 /* 0x84C */
    int nUnk850;                 /* 0x850 */
    int nUnk854;                 /* 0x854 */
    int *pUnk858;                /* 0x858 */
} MPEGSTREAM;

extern u_int _nextBit(MPEGSTREAM *pStream, u_int nBits);

/* ---- IPU register plumbing -------------------------------------------- */

#define IPU_CMD  (*(volatile u_int *)0x10002000)
#define IPU_CTRL (*(volatile u_int *)0x10002010)
#define IPU_TOP  (*(volatile u_int *)0x10002020)

extern u_int D_004AD680[];   /* per-command result register table */

extern void _flushBuf(MPEGSTREAM *pStream, u_int nBits);
extern u_int _peepBit(MPEGSTREAM *pStream, u_int nBits);
extern int _ipuVdec(MPEGSTREAM *pStream, int nTbl);
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

/* Reject an output request whose rectangle does not fit the decoded frame.
 *
 * PARKED at 5 differing words, registers only ($v0<->$v1 across the
 * area-compare arm).  Without the LAUNDER the two arms' identical
 * `slt; xori` tails get cross-jumped into one block and the function is a
 * word short; with it the arms are separate but the product lands in $v0
 * where the original has it in $v1.  Swept: LAUNDER on bOK in either arm
 * (13 diffs, $s0<->$s1 instead), PIN($3) on the product (cross-jump
 * returns), LAUNDER on the limit instead of the product (cross-jump
 * returns), a `bOK = 0` pre-initialised shape (16 diffs, LOGIC), and both
 * declaration orders of the two temporaries. */
int _isOutSizeOK(MPEGSTREAM *pStream, OUTREQ *pReq)
{
    char sBuf[256];
    int bOK;

    if (pStream->nUnk0E0 != 0) {
        if (pStream->nUnk0DC < pReq->nWidth) {
            bOK = 0;
        } else {
            bOK = pStream->nUnk0E0 >= pReq->nHeight;
        }
    } else {
        int nArea;
        int nLimit;

        /* Laundering the product keeps this arm's slt/xori tail out of the
         * shared block gcc's cross-jumping otherwise folds the arms into. */
        nLimit = pStream->nUnk0E4;
        nArea = pReq->nUnk0C * pReq->nUnk10;
        LAUNDER(nArea);
        bOK = nLimit >= nArea;
    }
    if (bOK == 0) {
        sprintf(sBuf, D_004D5DA8, pReq->nWidth, pReq->nHeight);
        _Error(pStream, sBuf);
    }
    return bOK;
}

extern char D_004D5C90[];  /* skipped macroblock in an I picture */

/* picture_display_extension(): 1..3 frame_centre offsets */
void _pictureDisplayExtension(MPEGSTREAM *pStream)
{
    int nOffs;
    int i;

    /* PARKED at 13 differing words.  The original shares ONE `nOffs = 1`
     * block between both arms (the frame-structure test's beql jumps over
     * it), while gcc gives us a second copy in an annulled delay slot.
     * Swept: ternary and if/else forms of each arm, `!= 3` vs `== 3`
     * inversion, else-if vs fully nested, and a `nOffs = 1` default
     * pre-assignment (16 diffs). */
    if (pStream->nUnk13C != 0) {
        if (pStream->nUnk184 != 0) {
            nOffs = pStream->nUnk178 ? 3 : 2;
        } else {
            nOffs = 1;
        }
    } else {
        if (pStream->nUnk174 == 3) {
            nOffs = pStream->nUnk184 ? 3 : 2;
        } else {
            nOffs = 1;
        }
    }
    for (i = 0; i < nOffs; i++) {
        pStream->anUnk18C[i] = _nextBit(pStream, 16);
        _nextBit(pStream, 1);
        pStream->anUnk198[i] = _nextBit(pStream, 16);
        _nextBit(pStream, 1);
    }
}

/* Fill in a skipped macroblock's prediction state. */
int _skipMB0(MPEGSTREAM *pStream, int *pMV, int *pType, int *pField, int *pStat)
{
    int nStruct;
    int bOK;

    bOK = 1;
    *(int *)((char *)pStream + pStream->nUnk810 * 320 + 1740) = 1;
    pStream->nUnk1B0 = 1;
    if (pStream->nUnk150 == 2) {
        pMV[5] = 0;
        pMV[4] = 0;
        pMV[1] = 0;
        pMV[0] = 0;
    }
    nStruct = pStream->nUnk174;
    if (nStruct == 3) {
        *pType = 2;
    } else {
        int bFrame;

        *pType = 1;
        bFrame = pStream->nUnk174 == 2;
        pField[1] = bFrame;
        pField[0] = bFrame;
    }
    if (pStream->nUnk150 == 1) {
        _Error(pStream, D_004D5C90);
        bOK = 0;
    }
    *pStat &= ~1;
    return bOK;
}

extern char D_004D5D68[];  /* second picture_structure in one frame */
extern char D_004D5D88[];  /* unknown picture_structure */

/* Track the presentation ordering across a temporal_reference wrap.
 *
 * PARKED at 10 differing words, registers only: the pin recovers the
 * original's `move a2,a0` copy of the stream pointer, but the base and the
 * wrap flag still land in $a3/$t0 where the original has $a0/$a3.  Swept:
 * PIN($4) on the base (12 diffs), PIN($7) on the flag (20 diffs), and both
 * declaration orders of the two. */
void _updateTempTackData(MPEGSTREAM *pStream, int nTempRef)
{
    /* The original keeps the stream pointer in $a2 and the base in $a0;
     * gcc leaves the pointer in $a0 and never makes the copy at all. */
    PIN(MPEGSTREAM *p, "$6");
    int nBase;
    int bWrapped;
    int nCur;
    int nMax;

    p = pStream;
    LAUNDER_V(p);
    bWrapped = 0;
    nBase = 0;
    if (p->nUnk150 != 3) {
        if (nTempRef != 0) {
            if (nTempRef < 0) {
                bWrapped = p->nUnk854 == 0;
            }
            p->nUnk854 = 0;
            nBase = nTempRef;
        }
    }
    nCur = p->nUnk84C + nTempRef;
    p->nUnk1AC = nCur;
    if (bWrapped != 0) {
        if (nBase >= nTempRef) {
            p->nUnk1AC = nCur + 1024;
        }
    }
    nMax = p->nUnk850;
    nCur = p->nUnk1AC;
    if (nMax < nCur) {
        nMax = nCur;
    }
    p->nUnk850 = nMax;
}

/* motion_vector(): fold one decoded delta into the running predictor. */
void _decode_motion_vector(int *pVal, int nFCode, int nDelta, int nResid,
                           int bHalf)
{
    int nLim;
    int v;

    nLim = 16 << nFCode;
    v = bHalf ? (*pVal >> 1) : *pVal;
    if (nDelta > 0) {
        v += ((nDelta - 1) << nFCode) + nResid + 1;
        if (v >= nLim) {
            v -= nLim * 2;
        }
    } else if (nDelta < 0) {
        v -= ((~nDelta) << nFCode) + nResid + 1;
        if (v < -nLim) {
            v += nLim * 2;
        }
    }
    *pVal = bHalf ? v * 2 : v;
}

/* picture_data(): decode into the reference image the structure selects.
 *
 * PARKED one word long (51 orig vs 52).  The original's decision tree has
 * case 3 fall THROUGH into the shared `s1 = pUnk1C0` load that the default
 * arm also uses, entering the error block at a second label; gcc gives
 * case 3 its own block and an extra `b`.  Swept: switch with cases in
 * 1/2/3 and 2/1/3 order, and the explicit if/else chain that mirrors the
 * tree (55 words -- the type gets reloaded per arm either way). */
int _decPicture(MPEGSTREAM *pStream)
{
    REFIMAGE *pImg;
    int nRet;

    if (pStream->nUnk174 == 3 && pStream->nUnk120 != 0) {
        _Error(pStream, D_004D5D68);
        pStream->nUnk120 = 0;
    }
    switch (pStream->nUnk174) {
    case 1:
        pImg = pStream->pUnk1D0;
        break;
    case 2:
        pImg = pStream->pUnk1E0;
        break;
    case 3:
        pImg = pStream->pUnk1C0;
        break;
    default:
        pImg = pStream->pUnk1C0;
        _Error(pStream, D_004D5D88);
        break;
    }
    nRet = _pictureData0(pStream);
    if (nRet != 0) {
        pImg->nUnk28 = 1;
    }
    return nRet;
}

/* ---- slice and header dispatch ---------------------------------------- */

extern char D_004D5C28[];  /* bad slice_start_code */
extern char D_004D5C50[];  /* slice ran off the end of the buffer */

extern void _Error1(MPEGSTREAM *pStream, char *pMsg, u_int nArg);
extern int _mbAddressIncrement(MPEGSTREAM *pStream);
extern void _sequenceHeader(MPEGSTREAM *pStream);
extern void _dispatchMpegCallback(int *pCb, void *pArg);

/* The message block handed to the per-picture callback. */
typedef struct {
    int nEvent;                  /* 0x00 */
    char pad04[0x4];
    long long llPts;             /* 0x08 */
    long long llDts;             /* 0x10 */
    char pad18[0x8];
} MPEGCBMSG;

/* slice(): header + first macroblock address, A-picture variant */
int _sliceA0(MPEGSTREAM *pStream, int nUnused, int *pMBAddr, int *pFirst,
             int *pMV)
{
    u_int nCode;
    int nQ;
    int nInc;

    pStream->nUnk11C = 0;
    _nextStartCode(pStream);
    nCode = _peepBit(pStream, 32);
    if (nCode - 257 >= 175) {
        _Error1(pStream, D_004D5C28, nCode);
        return 2;
    }
    _flushBuf(pStream, 32);
    nQ = _sliceB(pStream);
    nInc = _mbAddressIncrement(pStream);
    *pFirst = nInc;
    if (pStream->nUnk11C != 0) {
        _Error(pStream, D_004D5C50);
        return 1;
    }
    *pMBAddr = ((nQ << 7) + (nCode & 0xFF) - 1) * pStream->nUnk12C + nInc - 1;
    *pFirst = 1;
    pStream->nUnk1B0 = 1;
    pMV[5] = 0;
    pMV[4] = 0;
    pMV[1] = 0;
    pMV[0] = 0;
    pMV[7] = 0;
    pMV[6] = 0;
    pMV[3] = 0;
    pMV[2] = 0;
    return 0;
}

/* Skip to the next picture header, running the sequence/GOP parsers. */
int _nextHeader(MPEGSTREAM *pStream)
{
    MPEGCBMSG msg;
    u_int nCode;

    /* The picture-header tail lives INSIDE the loop: that is what lets
     * loop-invariant motion hoist the callback message's 5 and -1 into
     * callee-saved registers ahead of the loop. */
    for (;;) {
        _nextStartCode(pStream);
        nCode = _nextBit(pStream, 32);
        switch (nCode) {
        case 0x1B3:
            _sequenceHeader(pStream);
            break;
        case 0x1B8:
            _groupOfPicturesHeader(pStream);
            break;
        case 0x100:
            _pictureHeader(pStream);
            msg.nEvent = 5;
            msg.llPts = -1;
            msg.llDts = -1;
            _dispatchMpegCallback(pStream->pUnk858, &msg);
            pStream->llUnk830 = msg.llDts;
            pStream->llUnk828 = msg.llPts;
            return pStream->nUnk150;
        case 0x1B7:
            return 0;
        }
    }
}

/* ---- bit buffer ------------------------------------------------------- */

extern long long _waitIpuIdle64(MPEGSTREAM *pStream);

/* Drop nBits from the bit buffer, refilling the 32-bit peek window. */
void _flushBuf(MPEGSTREAM *pStream, u_int nBits)
{
    u_int nCmd;
    int nTop;
    int i;

    i = 0;
    while ((IPU_CTRL & 0x80004000) == 0x80000000) {
        if (i++ > 5000) {
            _dispatchMpegCbNodata(pStream->pUnk858);
            i = 0;
        }
    }
    nCmd = nBits | 0x40000000;
    IPU_CMD = nCmd;
    pStream->nUnk818 = D_004AD680[nCmd >> 28];
    nTop = _waitIpuIdle64(pStream);
    pStream->nUnk838 = nTop;
    pStream->nUnk83C = 32;
}

/* Look at the next nBits without consuming them. */
u_int _peepBit(MPEGSTREAM *pStream, u_int nBits)
{
    int nTop;
    int i;

    if (pStream->nUnk818 != 0 || pStream->nUnk83C < (int)nBits) {
        i = 0;
        while ((IPU_CTRL & 0x80004000) == 0x80000000) {
            if (i++ > 5000) {
                _dispatchMpegCbNodata(pStream->pUnk858);
                i = 0;
            }
        }
        IPU_CMD = 0x40000000;
        pStream->nUnk818 = D_004AD680[0x40000000 >> 28];
        nTop = _waitIpuIdle64(pStream);
        pStream->nUnk838 = nTop;
        pStream->nUnk83C = 32;
    }
    return (u_int)pStream->nUnk838 >> (32 - nBits);
}

#define IPU_CMD64 (*(volatile long long *)0x10002000)

/* Wait for the posted IPU command to retire and return its 64-bit result. */
long long _waitIpuIdle64(MPEGSTREAM *pStream)
{
    long long nRes;
    int i;

    i = 0;
    nRes = IPU_CMD64;
    while (nRes < 0 && (IPU_CTRL & 0x4000) == 0) {
        if (i++ > 5000) {
            _dispatchMpegCbNodata(pStream->pUnk858);
            i = 0;
        }
        nRes = IPU_CMD64;
    }
    return nRes;
}

#define IPU_BP   (*(volatile u_int *)0x10002020)
/* Indexed off the register-file base so the offset folds into the ld. */
#define IPU_TOP64 (((volatile long long *)0x10000000)[1030])

/* Consume and return the next nBits, refilling the peek window first. */
u_int _nextBit(MPEGSTREAM *pStream, u_int nBits)
{
    /* $a0 for the flush command word: gcc otherwise builds it in $v1 and
     * sinks it below the window-size store. */
    PIN(u_int nCmd, "$4");
    u_int nRet;
    int nTop;
    int i;

    i = 0;
    while ((IPU_CTRL & 0x80004000) == 0x80000000) {
        if (i++ > 5000) {
            _dispatchMpegCbNodata(pStream->pUnk858);
            i = 0;
        }
    }
    if (pStream->nUnk818 != 0 || pStream->nUnk83C < (int)nBits) {
        IPU_CMD = 0x40000000;
        pStream->nUnk818 = D_004AD680[0x40000000 >> 28];
        nTop = _waitIpuIdle64(pStream);
        pStream->nUnk838 = nTop;
    }
    nCmd = nBits | 0x40000000;
    pStream->nUnk83C = 32;
    nRet = (u_int)pStream->nUnk838 >> (32 - nBits);
    IPU_CMD = nCmd;
    pStream->nUnk818 = D_004AD680[nCmd >> 28];
    nTop = _waitIpuIdle64(pStream);
    pStream->nUnk838 = nTop;
    return nRet;
}

/* Run one IPU VDEC (variable-length decode) against table nTbl.
 *
 * PARKED at 3 differing words, registers only: the register-file base
 * pointer lands in $v1 where the original has it in $a0 (and the two luis
 * come out in the other order).  Swept: PIN($4) on the base with and
 * without LAUNDER_V (5 and 10 diffs), reading the bit-position register
 * before the top-of-stream one (8 diffs), and moving the base pointer's
 * declaration to the head of the block. */
int _ipuVdec(MPEGSTREAM *pStream, int nTbl)
{
    /* Through a base pointer, not a folded constant: 0x10000000 needs only
     * a lui, so the 0x2030 displacement folds into the ld. */
    volatile long long *pRegs;
    long long nRes;
    long long nTop;
    u_int nBP;
    int nCmd;
    int i;
    int j;

    j = 0;
    i = 0;
    while ((IPU_CTRL & 0x80004000) == 0x80000000) {
        if (i++ > 5000) {
            _dispatchMpegCbNodata(pStream->pUnk858);
            i = 0;
        }
    }
    nCmd = (nTbl << 26) | 0x30000000;
    IPU_CMD = nCmd;
    nRes = IPU_CMD64;
    pStream->nUnk818 = D_004AD680[nCmd >> 28];
    while (nRes < 0) {
        if (j++ > 5000) {
            _dispatchMpegCbNodata(pStream->pUnk858);
            j = 0;
        }
        nRes = IPU_CMD64;
    }
    pRegs = (volatile long long *)0x10000000;
    nTop = pRegs[1030];
    /* The bit-position register is read unconditionally, before the sign
     * test, so its load is not sunk into the else arm. */
    nBP = IPU_BP;
    pStream->nUnk838 = nTop;
    if (nTop < 0) {
        pStream->nUnk83C = (0 - (nBP & 0x1F)) & 0x1F;
    } else {
        pStream->nUnk83C = 32;
    }
    pStream->nUnk11C = (int)nRes == 0;
    return (short)nRes;
}

/* ---- macroblock address and extension dispatch ------------------------ */

extern char D_004D5BD0[];  /* bad macroblock_address_increment */

/* macroblock_address_increment(): escape and stuffing codes accumulate. */
int _mbAddressIncrement(MPEGSTREAM *pStream)
{
    int nTotal;
    int nCode;
    int nPeek;
    int bMore;

    nTotal = 0;
    do {
        nCode = _ipuVdec(pStream, 0);
        if (nCode == 34) {
            bMore = 1;                       /* macroblock_stuffing */
        } else if (nCode < 35) {
            if (nCode == 0) {
                nPeek = _peepBit(pStream, 11);
                if (pStream->nUnk848 != 0 && nPeek == 15) {
                    _flushBuf(pStream, 11);
                    bMore = 1;
                } else {
                    _Error1(pStream, D_004D5BD0, nCode);
                    pStream->nUnk11C = 1;
                    return 1;
                }
            } else {
                nTotal += nCode;
                bMore = 0;
            }
        } else if (nCode != 35) {
            nTotal += nCode;
            bMore = 0;
        } else {
            nTotal += 33;                    /* macroblock_escape */
            bMore = 1;
        }
    } while (bMore);
    return nTotal;
}

/* The extension-id dispatch table, copied to the stack on entry. */
typedef struct {
    void (*af[11])(MPEGSTREAM *pStream);
} EXTTBL;

extern EXTTBL D_004D5CE0;

/* extension_and_user_data(): run the extension parsers, skip user data. */
void _extensionAndUserData(MPEGSTREAM *pStream)
{
    EXTTBL tbl;
    u_int nCode;
    u_int nId;

    tbl = D_004D5CE0;
    _nextStartCode(pStream);
    for (;;) {
        nCode = _peepBit(pStream, 32);
        if (nCode == 0x1B5) {
            _flushBuf(pStream, 32);
            nId = _nextBit(pStream, 4);
            if (nId > 10) {
                nId = 0;
            }
            tbl.af[nId](pStream);
            _nextStartCode(pStream);
        } else if (nCode == 0x1B2) {
            _flushBuf(pStream, 32);
            _nextStartCode(pStream);
        } else {
            break;
        }
    }
}
