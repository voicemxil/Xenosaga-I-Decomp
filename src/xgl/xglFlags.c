/* Bit-packed persistent flag accessors */

int xglFlagsGet(int, int);
extern unsigned char SaveData[] __attribute__((section(".data")));
extern unsigned char D_00491824[];

/* TODO: near-match (41/48 words) - bit-packed field read/modify/write via
   stack scratch buffer; nOld extraction (chunk>>shift)&mask vs. the final
   dsll32/dsra32 sign-extend schedules differently than the original, which
   ANDs-then-sign-extends where ours sign-extends-then-ANDs. Logic is
   correct (matches xglFlagsGet's mirror-image pattern below); only
   instruction scheduling differs. LOGIC class per triage.py. */
int xglFlagsSet(int nFlag, int nSize, int nValue)
{
    unsigned char *p;
    long long nChunk;
    unsigned char *pByte = (unsigned char *)&nChunk;
    int nShift;
    int nBytes;
    int i;
    int mask;
    int nOld;

    nShift = nFlag & 7;
    mask = (1 << nSize) - 1;
    nBytes = (nSize + nShift + 7) >> 3;
    p = &D_00491824[nFlag >> 3];

    for (i = 0; i < nBytes; i++) {
        pByte[i] = p[i];
    }

    nOld = (int)((nChunk >> nShift) & mask);
    nChunk = (nChunk & ~((long long)mask << nShift)) | ((long long)(nValue & mask) << nShift);

    for (i = 0; i < nBytes; i++) {
        p[i] = pByte[i];
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
