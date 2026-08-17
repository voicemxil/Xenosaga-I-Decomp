#include "matching.h"
/* Battle scene ambient/alter-state functions (sdv* family) */

extern int _sdvAmbFrame;
extern int _sdvAmbState;
void sdvInitAmbient(void) {
    _sdvAmbFrame = 0;
    _sdvAmbState = 0;
}

extern void sdvSetAmbStateSub(int a, int b, int c);
void sdvSetAmbState(int a, int b) { sdvSetAmbStateSub(a, b, 0); }
void sdvSetAmbState2(int a, int b) { sdvSetAmbStateSub(a, b, 1); }

extern void xglSoundEffectNormalID(int id, int a);
/* b and c are dead here, but sdvScheduleSound really does pass three
 * arguments, so the prototype has to carry them. */
void sdvPlaySound(int id, int b, int c) { xglSoundEffectNormalID(id, 0); }

extern void sefMemZero(void *p, int size);
void sdvInitAlter(void *p) { sefMemZero(p, 0x1280); }
void sdvDestroyAlter(void *p) { sefMemZero(p, 0x1280); }

extern int _sdvMapRgb[];
extern int _sdvAmbient[];
extern void sdvSetAmbient(void *a, void *b);
void sdvRestoreAmbient(void) { sdvSetAmbient(_sdvMapRgb, _sdvAmbient); }

extern short _sdvSpecialBuf[];
extern void *memset(void *, int, unsigned int);
void sdvInitSpecialWork(void) { memset(_sdvSpecialBuf, 0, 0x10); }

/* --- ambient light plumbing --- */

extern void *xglStudioGetLight2(void);
extern void xglLightIntensityAmbient(void *pLight, void *pIntensity);
extern void func_A2C3D8(void *p);

void sdvSetAmbient(void *pRgb, void *pAmbient)
{
    xglLightIntensityAmbient(xglStudioGetLight2(), pAmbient);
    func_A2C3D8(pRgb);
}

/* --- the 16 alter-state slots (0x1280 bytes apiece) --- */

typedef struct
{
    char pad0000[0x1278];
    unsigned short nUsed;    /* 0x1278 */
    short pad127A;
    short nSerial;           /* 0x127C */
    short pad127E;
} SDV_ALTER;

/* A third view of the record, based at its 0x1270 tail: sdvCreateAlter
 * walks this one while the block copy goes to base + byte offset. The
 * stride is still the full record. */
typedef struct
{
    int nFlags;              /* 0x1270 */
    short f1274;
    short f1276;
    unsigned short nUsed;    /* 0x1278 */
    short pad127A;
    short nSerial;           /* 0x127C */
    char pad127E[0x1280 - 0x0E];
} SDV_ALTER_TAIL;

/* The 112 bytes actually copied in from the caller's template. */
typedef struct { long q[14]; } SDV_ALTER_COPY;

/* A second view of the same record, based 0x60 in: the draw loop walks
 * this one (stride is still the full record) while the calls take
 * &_sdvAlter[i], which is what gives the loop its two induction
 * variables and the +0x60 bias on every field load. */
typedef struct
{
    char pad0060[0x0E];
    short nOwner;            /* 0x6E */
    char pad0070[0x1208];
    unsigned short nUsed;    /* 0x1278 */
    char pad127A[0x66];
} SDV_ALTER_BODY;

extern SDV_ALTER _sdvAlter[];

void sdvInitAlters(void)
{
    int i;
    SDV_ALTER *p;

    p = _sdvAlter;
    for (i = 0; i < 16; i++) {
        sdvInitAlter(p);
        p++;
    }
}

void sdvDestroyAlters(void)
{
    int i;
    SDV_ALTER *p;

    p = _sdvAlter;
    for (i = 0; i < 16; i++) {
        sdvDestroyAlter(p);
        p++;
    }
}

/* Snapshot the current ambient/map colours into _sdvAmbient/_sdvMapRgb
 * and reset the ambient fade. The two saves are 128-bit quadword copies
 * (lq/sq) -- MMI, not expressible in C. */
extern void *func_A2C3F8(void);
extern int _sdvAmbient[4];
extern int _sdvMapRgb[4];

void sdvSaveAmbient(void)
{
    void *pLight;
    void *pRgb;

    pLight = xglStudioGetLight2();
    pRgb = func_A2C3F8();
    __asm__ __volatile__("lq $8, 0x0(%1)\n\tsq $8, 0x0(%0)"
                         : : "r"(_sdvAmbient), "r"(pLight) : "$8", "memory");
    __asm__ __volatile__("lq $8, 0x0(%1)\n\tsq $8, 0x0(%0)"
                         : : "r"(_sdvMapRgb), "r"(pRgb) : "$8", "memory");
    _sdvAmbFrame = 0;
    _sdvAmbState = 0;
}

/* Tear down the eight "special" slots, telling the game loop to drop the
 * defocus each one requested */
extern void GameDefocusSet(int nIdx, int a, int b);
extern short _sdvSpecialWork[] __asm__("_sdvSpecialBuf");

void sdvClearSpecialWork(void)
{
    short *p;
    int i;

    p = _sdvSpecialWork;
    for (i = 0; i < 8; i++) {
        if (*p != 0) {
            GameDefocusSet(i, 0, 0);
            *p = 0;
        }
        p++;
    }
}

/* Kill the alter slot named by a packed (serial << 16 | index) handle,
 * but only if the slot is still live and still carries that serial.
 * The live/serial pair sits in a header at +0x1270 that the original
 * addresses through its own pointer -- hence the separate `addiu +4720`
 * rather than 0x1278/0x127C folded into the loads -- and the slot's byte
 * offset is kept in its own unsigned variable so gcc cannot reassociate
 * base+offset+constant into one address. */
typedef struct
{
    char pad00[8];
    unsigned short nUsed;    /* +0x08 (slot +0x1278) */
    short pad0A;
    short nSerial;           /* +0x0C (slot +0x127C) */
    short pad0E;
} ALTER_HDR;

void sdvKillAlter(int nHandle)
{
    char *pBase;
    unsigned int nOfs;
    unsigned int nHdrOfs;
    int nSerial;
    ALTER_HDR *pHdr;

    if (nHandle < 0) {
        return;
    }
    nSerial = nHandle >> 16;
    pBase = (char *)_sdvAlter;
    nOfs = (nHandle & 0xFFFF) * 0x1280;
    nHdrOfs = nOfs + 0x1270;
    pHdr = (ALTER_HDR *)(pBase + nHdrOfs);
    if (pHdr->nUsed == 0) {
        return;
    }
    if (pHdr->nSerial != nSerial) {
        return;
    }
    sdvDestroyAlter(pBase + nOfs);
}

extern void sdvExecAlter(void *p);

void sdvExecAlters(void)
{
    int i;

    for (i = 0; i < 16; i++) {
        if (_sdvAlter[i].nUsed != 0) {
            sdvExecAlter(&_sdvAlter[i]);
        }
    }
}

/* --- scheduled (sequence-table driven) sound playback --- */

typedef struct
{
    short *pTbl;     /* 0x00 -- table of 2-byte cells */
    char pad04[0x10 - 0x4];
    int nStride;     /* 0x10 -- cells per record */
    int nLastSound;  /* 0x14 */
} SDV_SEQ;

extern int sdvExecSeqTbl(SDV_SEQ *p);

/* Advance the sequence table and, if the record it lands on names a
 * sound, play it. The record's flag short is read in the blez delay
 * slot -- unconditionally -- so it has to be loaded before the "is there
 * a sound" test; the write-back of the sound id is annulled into the
 * bgtzl, so it is the flag>0 arm.
 *
 * One fixer flag (--branch-unlikely sdvScheduleSound:1): gcc annuls the
 * `p->pTbl == 0` branch while the original leaves it a plain beqz with
 * the same epilogue load stolen into the slot. Swept for a source shape
 * first -- early returns, shared `goto ret`, and fully nested ifs all
 * produce the identical annulled branch, and the very next branch in
 * the same function comes out plain either way. */
void sdvScheduleSound(SDV_SEQ *p)
{
    int nCell;
    int nSound;
    int nFlag;
    char *pRec;

    if (p != 0) {
        if (p->pTbl != 0) {
            nCell = sdvExecSeqTbl(p);
            if ((unsigned int)nCell < 1024) {
                pRec = (char *)((nCell * p->nStride) * 2 + (int)p->pTbl);
                nSound = *(int *)(pRec + 4);
                nFlag = *(short *)(pRec + 2);
                if (nSound > 0) {
                    sdvPlaySound(nSound, 0, 0);
                    if (nFlag > 0) {
                        p->nLastSound = nSound;
                    }
                }
            }
        }
    }
}

/* Claim a special-effect work slot: slot 0 is the reserved one, slots
 * 1..7 are the general pool. Returns the slot index, or -1 if busy. */
int sdvAllocSpecialWork(int nType)
{
    int i;

    if (nType == 0) {
        if (_sdvSpecialBuf[0] == 0) {
            _sdvSpecialBuf[0] = 1;
            return 0;
        }
        return -1;
    }
    for (i = 1; i < 8; i++) {
        if (_sdvSpecialBuf[i] == 0) {
            _sdvSpecialBuf[i] = 1;
            return i;
        }
    }
    return -1;
}

/* Ambient-state latch. The effect number is decoded once into two
 * function statics; only category-14 effects (or the one special ID
 * 0xB25) may change the state, and only when the effect number falls
 * outside the 0x8FC..0x9B1 window.
 *
 * PARKED at 6 diffs -- identical instruction multiset, a pure
 * cross-block placement tie-break: retail keeps the `sltiu` in the
 * eftCate block and materialises `li 2853` in the second block; every
 * shape below puts one or the other on the wrong side of the `beq`.
 * Swept: inline ternary in the if (8, sltiu gets its own pseudo);
 * `nDelta` local before the if (7) and inside the if (8); boolean
 * `bOutside = ... >= 0xB6` before the if (32 words -- the truth value
 * picks up an xori to invert for movn); `bInside ? _sdvAmbState :
 * nState` (28 words, gcc folds the self-assign); `(bInside == 0) ?`
 * before the if (6, this one -- correct movz and correct register
 * reuse, only the placement wrong); early-return spelling of the
 * guard (6); a separate `nCate = eftCate` local (6). Permuter
 * territory.
 */
extern void srsAnalyzeEftNo(int nEffect, int *pCharID, int *pCate);

void sdvSetAmbStateSub(int nState, int nEffect, int bForce)
{
    static int charID;
    static int eftCate;
    unsigned int nDelta;
    int bInside;

    if (bForce != 0) {
        _sdvAmbState = nState;
        return;
    }
    srsAnalyzeEftNo(nEffect, &charID, &eftCate);
    nDelta = (unsigned int)(nEffect - 0x8FC);
    bInside = nDelta < 0xB6U;
    if (eftCate == 14 || nEffect == 0xB25) {
        _sdvAmbState = (bInside == 0) ? nState : _sdvAmbState;
    }
}

/* Draw every active alter that belongs to nOwner. The GS environment is
 * set up once, on the first one actually drawn. */
extern int _packet[];
extern void SGsInitEnv(void);
extern void sdvDrawAlter(void *p, void *pPacket);

void sdvDrawAlters(int nOwner)
{
    SDV_ALTER_BODY *q;
    int i;
    int nDraw;
    /* Steering: gcc gives the loop-invariant base the earlier
     * callee-saved register and the byte offset the later one; retail
     * has it the other way round. Emits no code. */
    PIN(int nOfs, "$18");
    char *pBase;

    pBase = (char *)_sdvAlter;
    q = (SDV_ALTER_BODY *)(pBase + 0x60);
    nDraw = 0;
    for (i = 0, nOfs = 0; i < 16; i++, nOfs += 0x1280) {
        if (q->nUsed != 0 && q->nOwner == nOwner) {
            nDraw++;
            if (nDraw == 1) {
                SGsInitEnv();
            }
            /* Integer-first addition: retail's addu has the byte offset as
             * rs and the base as rt. */
            sdvDrawAlter((char *)(nOfs + (int)pBase), _packet);
        }
        q++;
    }
}

/* Byte-exact. The three independent loop-invariant initializers use a
 * two-step audited scheduling permutation to reproduce the retail order.
 *
 * Claim the first unused alter slot, copy the caller's 112-byte
 * template into it and stamp it with the next 8-bit serial. The handle
 * packs that serial into the high half and the slot index into the low.
 * The serial is re-read for the store because the block copy may alias
 * it. */
int sdvCreateAlter(SDV_ALTER_COPY *pSrc)
{
    static int serial;
    SDV_ALTER_TAIL *q;
    int i;
    int nOfs;
    int nSerial;
    char *pBase;

    pBase = (char *)_sdvAlter;
    q = (SDV_ALTER_TAIL *)(pBase + 0x1270);
    for (i = 0, nOfs = 0; i < 16; i++, nOfs += 0x1280) {
        if (q->nUsed == 0) {
            nSerial = (serial + 1) & 0xFF;
            serial = nSerial;
            *(SDV_ALTER_COPY *)(char *)(nOfs + (int)pBase) = *pSrc;
            q->nSerial = serial;
            q->f1276 = 0;
            q->nFlags = 0;
            q->nUsed = 1;
            q->f1274 = 0;
            return (nSerial << 16) | i;
        }
        q++;
    }
    return -1;
}

/* --- sm*: a first-header doubly-linked best-fit heap over 16-byte cells.
 * Every cell header carries a magic word, 0x1234 free / 0x5678 in use;
 * nSize counts the 16-byte cells that FOLLOW the header. --- */

typedef struct SM_BLK
{
    int            nMagic;   /* 0x00 */
    unsigned int   nSize;    /* 0x04 */
    struct SM_BLK *pPrev;    /* 0x08 */
    struct SM_BLK *pNext;    /* 0x0C */
} SM_BLK;

typedef struct
{
    void        *pHeap;      /* 0x00 */
    unsigned int nCells;     /* 0x04 */
    int          nUsed;      /* 0x08 */
    int          nFree;      /* 0x0C */
    int          nUsedMax;   /* 0x10 */
    int          nTotal;     /* 0x14 */
} SM_MEMINFO;

extern SM_MEMINFO stMemInfo;
extern SM_BLK *pmcHead;
extern unsigned int nMemCell;

void smInitMemInfo(SM_MEMINFO *p)
{
    unsigned int n = nMemCell;
    int nBytes = (n << 4) + 16;

    p->nCells = n;
    p->pHeap = pmcHead;
    p->nFree = nBytes;
    p->nUsedMax = 0;
    p->nTotal = nBytes;
    p->nUsed = 0;
}

void smInitilize(void *pMem, unsigned int nSize)
{
    SM_BLK *p = (SM_BLK *)pMem;
    unsigned int n = nSize >> 4;

    nMemCell = n;
    pmcHead = p;
    p->nMagic = 0x1234;
    p->nSize = n - 1;
    p->pPrev = 0;
    p->pNext = 0;
    smInitMemInfo(&stMemInfo);
}

void smPrintInfo(void)
{
    SM_BLK *p = pmcHead;

    while (p != 0) {
        int nMagic = p->nMagic;

        if (nMagic != 0x1234 && nMagic != 0x5678) {
            return;
        }
        p = p->pNext;
    }
}

void *smAlloc(unsigned int nBytes)
{
    SM_BLK *p;
    SM_BLK *pBest;
    unsigned int nNeed;
    unsigned int nBest;
    unsigned int n;

    nNeed = ((nBytes + 2047) & ~2047) >> 4;
    p = pmcHead;
    pBest = 0;
    nBest = nMemCell;
    while (p != 0) {
        /* ONE local for the magic word and the size: retail keeps both in
         * $v1, and splitting them costs the whole register assignment. */
        n = p->nMagic;
        if (n == 0x1234) {
            n = p->nSize;
            if (n >= nNeed && n < nBest) {
                /* PASSTHRU, not `nBest = n`: the two are equal here, so CSE
                 * hands the exact-fit test n's register, where retail tests
                 * nBest's. Costs no instruction. */
                PASSTHRU(nBest, n);
                pBest = p;
                if (nBest == nNeed) {
                    break;
                }
            }
        }
        p = p->pNext;
    }
    if (pBest == 0) {
        smPrintInfo();
        return 0;
    }
    n = pBest->nSize;
    pBest->nMagic = 0x5678;
    if (n != nNeed) {
        /* The dead search cursor carries the split block: retail reuses
         * its register rather than taking a fresh one. */
        p = (SM_BLK *)((char *)pBest + (nNeed << 4) + 16);
        p->nMagic = 0x1234;
        p->pPrev = pBest;
        p->pNext = pBest->pNext;
        p->nSize = pBest->nSize - nNeed - 1;
        if (p->pNext != 0) {
            p->pNext->pPrev = p;
        }
        pBest->nSize = nNeed;
        pBest->pNext = p;
    }
    return (char *)pBest + 16;
}

void smFree(void *pMem)
{
    SM_BLK *p;
    SM_BLK *q;
    SM_BLK *r;

    if (pMem == 0) {
        return;
    }
    p = (SM_BLK *)((char *)pMem - 16);
    p->nMagic = 0x1234;
    q = p->pNext;
    if (q != 0 && q->nMagic == 0x1234) {
        unsigned int n = p->nSize + q->nSize + 1;

        r = q->pNext;
        p->pNext = r;
        p->nSize = n;
        if (r != 0) {
            r->pPrev = p;
        }
    }
    q = p->pPrev;
    if (q == 0) {
        return;
    }
    if (q->nMagic != 0x1234) {
        return;
    }
    {
        unsigned int n = q->nSize + p->nSize + 1;

        r = p->pNext;
        q->pNext = r;
        q->nSize = n;
        if (r != 0) {
            r->pPrev = q;
        }
    }
}

/* --- MOutputDebugString: prefix the caller's format string, then print
 * the caller's own varargs through it. --- */

#define va_start(ap, last) __builtin_stdarg_start(ap, last)
typedef __builtin_va_list va_list;

extern char dstr[];
extern char D_004CCA40[];
extern char D_004CCA58[];
extern int sprintf(char *pBuf, const char *pFmt, ...);
extern int vprintf(const char *pFmt, va_list ap);

void MOutputDebugString(char *pFmt, ...)
{
    va_list ap;

    va_start(ap, pFmt);
    sprintf(dstr, D_004CCA40, pFmt);
    vprintf(dstr, ap);
}

void MOutputDebugStringWarn(char *pFmt, ...)
{
    va_list ap;

    va_start(ap, pFmt);
    sprintf(dstr, D_004CCA58, pFmt);
    vprintf(dstr, ap);
}
