#include "matching.h"
/* Bit-packed persistent flag accessors */

int xglFlagsGet(int, int);
extern unsigned char SaveData[] __attribute__((section(".data")));
extern unsigned char D_00491824[];

/* TODO: near-match (37/48 words, 11 differing).
   Fixed this session: the byte loops index D_00491824[nOfs + i] and
   ((unsigned char *)&nChunk)[i] directly -- a `p` pointer local kills
   retail's per-preheader lui/addiu rematerialisation, and a `pByte`
   local costs an extra `move t4,sp`.  The two PINs are the register
   tie-break (nShift $9, nOld $7); without them every register in the
   function is shifted up by one (38 diffs).

   RESIDUE: retail masks BEFORE the int truncation --
       dsrav v0,v0,t1 ; and v0,v0,t2 ; dsll32 a3,v0 ; dsra32 a3,a3
   while gcc folds trunc(x & m) -> trunc(x) & trunc(m) and emits
       dsll32 a3,a3 ; dsra32 a3,a3 ; and a3,a3,t2
   plus a v0/v1 rotation in the read-modify-write that follows from it.
   Swept, all still 11: implicit conversion; (long long)/(unsigned int)/
   (long long)(unsigned int) casts on mask; `long long mask`;
   `unsigned long long mask`; `unsigned long long nChunk`; a named
   long long temp with LAUNDER / LAUNDER_V (both +16 bytes); a local
   `long long v = nChunk` copy feeding both the extract and the RMW;
   a PIN'd long long temp (+16 bytes); computing the new chunk into a
   temp first (31 diffs); swapping the extract and the RMW (31 diffs).
   All four statement orderings of nShift/mask/nBytes/nOfs are
   codegen-identical -- gcc reorders them freely.
   No source spelling found that blocks the combine fold. */
int xglFlagsSet(int nFlag, int nSize, int nValue)
{
    long long nChunk;
    PIN(int nShift, "$9");
    int nBytes;
    int i;
    int mask;
    PIN(int nOld, "$7");
    int nOfs;

    nShift = nFlag & 7;
    mask = (1 << nSize) - 1;
    nBytes = (nSize + nShift + 7) >> 3;
    nOfs = nFlag >> 3;

    for (i = 0; i < nBytes; i++) {
        ((unsigned char *)&nChunk)[i] = D_00491824[nOfs + i];
    }

    nOld = (int)((nChunk >> nShift) & mask);
    nChunk = (nChunk & ~((long long)mask << nShift)) | ((long long)(nValue & mask) << nShift);

    for (i = 0; i < nBytes; i++) {
        D_00491824[nOfs + i] = ((unsigned char *)&nChunk)[i];
    }

    return nOld;
}
int xglFlagsSet1(int nFlag, int nValue) { return xglFlagsSet(nFlag, 1, nValue); }
int xglFlagsSet2(int nFlag, int nValue) { return xglFlagsSet(nFlag, 2, nValue); }
int xglFlagsSet4(int nFlag, int nValue) { return xglFlagsSet(nFlag, 4, nValue); }
int xglFlagsSet8(int nFlag, int nValue) { return xglFlagsSet(nFlag, 8, nValue); }
int xglFlagsSet16(int nFlag, int nValue) { return xglFlagsSet(nFlag, 0x10, nValue); }
int xglFlagsSet32(int nFlag, int nValue) { return xglFlagsSet(nFlag, 0x20, nValue); }

long long xglFlagsSet64(int nFlag, long long nValue)
{
    long long nLow;
    long long nHigh;

    nLow = xglFlagsSet(nFlag, 0x20, (int)nValue);
    nHigh = xglFlagsSet(nFlag + 0x20, 0x20, nValue >> 32);
    return (nHigh << 32) + nLow;
}

int xglFlagsGet1(int nFlag) { return xglFlagsGet(nFlag, 1); }
int xglFlagsGet2(int nFlag) { return xglFlagsGet(nFlag, 2); }
int xglFlagsGet4(int nFlag) { return xglFlagsGet(nFlag, 4); }
int xglFlagsGet8(int nFlag) { return xglFlagsGet(nFlag, 8); }
int xglFlagsGet16(int nFlag) { return xglFlagsGet(nFlag, 0x10); }
int xglFlagsGet32(int nFlag) { return xglFlagsGet(nFlag, 0x20); }

long long xglFlagsGet64(int nFlag)
{
    long long nLow;
    long long nHigh;

    nLow = xglFlagsGet(nFlag, 0x20);
    nHigh = xglFlagsGet(nFlag + 0x20, 0x20);
    return (nHigh << 32) + nLow;
}

void xglFlagsInitial(void)
{
    unsigned char *pFlag;
    int nCount;

    pFlag = SaveData;
    pFlag += 0x74;
    nCount = 0x10000;
    do {
        nCount--;
        *pFlag++ = 0;
    } while (nCount != 0);
}

/* Read an nSize-bit field starting at bit nFlag out of the packed flag
 * array (mirror image of xglFlagsSet's staging loop) */
/* TODO: near-miss (~5 words). Indexed copies (both sides spelled with
 * [i] so the source pointer strength-reduces AFTER the guard and the
 * stack side stays sp-relative, no pByte local) match the original's
 * loop; the residue is the original staging the count through $a2 with
 * a `move t0,a2` copy (ours lands in $8 directly, leaving a scheduler
 * nop where the move sat). Pinning the count to $6 and an explicit
 * if-guard both regressed. */
int xglFlagsGet(int nFlag, int nSize)
{
    long long nChunk;
    int nShift;
    int i;

    nShift = nFlag & 7;
    /* The loop bound is written inline, not via a named nBytes local:
       jump.c's duplicate_loop_exit_test copies the test ahead of the
       loop, and LICM then hoists the invariant as a SECOND pseudo, which
       is retail's `move t0,a2`. A named local coalesces the two. */
    for (i = 0; i < (nSize + nShift + 7) >> 3; i++) {
        ((unsigned char *)&nChunk)[i] = D_00491824[(nFlag >> 3) + i];
    }
    return (int)(nChunk >> nShift) & ((1 << nSize) - 1);
}
