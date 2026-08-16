#include "matching.h"

/* CD/DVD access helpers for the xgl engine */

typedef unsigned int u_int;
typedef unsigned char u_char;

/* One pending read request in LW's 32-entry ring */
typedef struct {
    u_int nAddr;         /* 0x00 */
    int nUnk04;          /* 0x04 */
    int nUnk08;          /* 0x08 */
    int nSize;           /* 0x0C */
    char aName[0x70];    /* 0x10 */
} XGLCDQUEUE;

typedef struct {
    int nFd;             /* 0x00 */
    u_char aPad04[4];
    int nUnk08;          /* 0x08 */
    int nUnk0C;          /* 0x0C */
    void (*pProgress)(int, int); /* 0x10 */
    u_char aPad14[0x1C];
    char nStatus;
    u_char aPad31[3];
    char nPowerOff;      /* 0x34 */
    u_char aPad35[0x9];
    u_char nUnk3E;
    u_char nUnk3F;
    XGLCDQUEUE aQueue[32];   /* 0x40 */
} XGLCDWORK;

/* CD stream work area (prefix of the sound XGL_STREAM) */
typedef struct {
    int nUnk00;          /* 0x00 */
    short nSectors;      /* 0x04 */
    short nRingSects;    /* 0x06 */
    char nUnk08;         /* 0x08 */
    unsigned char nType; /* 0x09 */
    char pad0A[2];
    int nUnk0C;          /* 0x0C */
    int nFd;             /* 0x10 */
    int nUnk14;          /* 0x14 */
    int nUnk18;          /* 0x18 */
    int nUnk1C;          /* 0x1C */
    int nUnk20;          /* 0x20 */
    int nRingBytes;      /* 0x24 */
    int nReadPos;        /* 0x28 */
    int nWritePos;       /* 0x2C */
} XGLCDSTREAM;

typedef struct {
    int nUnk00;
    u_char aPad04[0x20];
    u_char nType;
    u_char aPad25[3];
    int nFd;
    int nSize;
} XGLCDFILEPOS;

typedef struct {
    char nReady;         /* 0x00 */
    char pad01[3];
    char *pBuf;          /* 0x04 */
    int nLsn;            /* 0x08 */
} XGLCDARC;

void xglCdArcInitSub0(XGLCDFILEPOS *pPos, char *pBuf, int nBlock);
int sceCdDiskReady(int nMode);
int sceCdRead(int nLsn, int nSectors, void *pBuf, void *pMode);

extern XGLCDWORK LW;
extern int D_0093DC44[4];

void (*callback)();
int StrList;
int WorkEnd;

int sceCdPause(void);
int sceCdSync(int nMode);
int sceClose(int nFd);
int sceCdStSeek(int nLsn);
int sceLseek(int nFd, int nOffset, int nWhence);
int xglCdGetFilePos(XGLCDFILEPOS *pPos, char *pName, void (*pFunc)());
int xglCdReadFilePart(char *pName, u_int nAddr, int nOfs, int nSize, int nA, int nB);
int xglCdArcInit(void);
int StreamReadRingCoreXss(void *pStr);
int StreamReadRingCoreNormal(void *pStr);

/* Default completion callback (nothing to do) */
int xglArxExtract(void *pDst, void *pSrc);

/* Copy nSize bytes from pSrc to pDst, unpacking an "ARX" archive in place
 * if the source carries one.  nSize = 0 after the unpack is dead but it is
 * what the original wrote: without a statement after the call gcc turns the
 * call into a sibling jump. */
void extract(u_int *pDst, u_int *pSrc, int nSize)
{
    u_int nWord;

    if (*pSrc == 0x00585241 && pSrc[3] == 0) {
        xglArxExtract(pDst, pSrc);
        nSize = 0;
    } else if (pDst != pSrc && nSize > 0) {
        do {
            nWord = *pSrc;
            nSize -= 4;
            *pDst = nWord;
            pSrc++;
            pDst++;
        } while (nSize > 0);
    }
}

/* Issue the next queued read; returns 1 when the ring is empty */
int queue_next(void)
{
    XGLCDQUEUE *pEnt;

    LW.pProgress(4, (LW.nUnk3F - LW.nUnk3E) & 0x1F);
    LW.nStatus = 0;
    if (LW.nUnk3F != LW.nUnk3E) {
        pEnt = &LW.aQueue[LW.nUnk3E];
        xglCdReadFilePart(pEnt->aName, pEnt->nAddr, 1, pEnt->nSize, pEnt->nUnk04,
                          pEnt->nUnk08);
        LW.nUnk3E = (LW.nUnk3E + 1) & 0x1F;
        return 0;
    }
    return 1;
}

void xglCdDefaultCallback(void)
{
}

/* Dummy completion callback for synchronous reads */
void xglCdDummyCallback(void)
{
}

/* Reset the CD state and stop the drive */
int xglCdReset(void)
{
    LW.nUnk3E = 0;
    LW.nUnk3F = 0;
    callback = xglCdDefaultCallback;
    LW.nStatus = 0;
    StrList = 0;
    sceCdPause();
    return sceCdSync(0);
}

/* Install a read-completion callback, returning the previous one */
void (*xglCdSetCallback(int nFunc))()
{
    void (*pOld)();

    pOld = callback;
    switch (nFunc) {
    case 0:
        callback = xglCdDefaultCallback;
        break;
    case 1:
        callback = xglCdDummyCallback;
        break;
    case -1:
        break;
    default:
        callback = (void (*)())nFunc;
        break;
    }
    return pOld;
}

/* Look up a file's position data, returning 0 on success */
int xglCdGetFileData(char *pName, u_int nAddr)
{
    return xglCdGetFilePos((XGLCDFILEPOS *)nAddr, pName, xglCdDummyCallback) ? 0 : -1;
}

/* Return the size of a file, or -1 if it cannot be found */
int xglCdGetFileSize(char *pName)
{
    XGLCDFILEPOS sPos;
    int n;

    n = xglCdGetFilePos(&sPos, pName, xglCdDummyCallback);
    if (n == 0) {
        return -1;
    }
    n = sPos.nType;
    if (n == 0) {
        return sPos.nSize;
    }
    if (n >= 0) {
        if (n < 3) {
            sceClose(sPos.nFd);
        }
    }
    return sPos.nSize;
}

/* Read a whole file into memory */
int xglCdReadFile(char *pName, u_int nAddr, int nOfs, int nSize)
{
    return xglCdReadFilePart(pName, nAddr, nOfs, nSize, 0, -1);
}

int sceCdBreak(void);
int sceCdPause(void);
int sceClose(int nFd);

/* TODO: near-miss (2 words, SCHEDULING): only the epilogue restore pair
 * is swapped -- orig `ld $17,8/ld $16,0`, ours `ld $16,0/ld $17,8`
 * (the original sched2's WAR delay after `addiu $2,$17,%lo(LW)`).
 * Exactly the --war-restore-swap class, but (a) xglCd.c has no
 * FILE_FIX_FLAGS entry to extend, (b) RE_WAR_ALU lacks addiu, and
 * (c) the reader addiu is separated from the ld pair by the $L36 label
 * and the pinned passthrough's #APP/#NO_APP markers, so the pass's
 * 3-line adjacency never fires (verified with a .scratch-xgl2 fixer
 * copy). Everything else matches: goto-shared break/close blocks,
 * $5/$16/$2 pinned passthroughs, memory-clobber on the nUnk3E copy to
 * keep the lbu above the status lb. */
void xglCdReadCancel(void)
{
    PIN(XGLCDWORK *pLW, "$5");
    PIN(XGLCDWORK *pFd, "$16");
    PIN(XGLCDWORK *pEnd, "$2");
    PIN(int nTmp, "$2");
    int n;

    PASSTHRU(pLW, &LW);
    __asm__("" : "=r"(nTmp) : "0"(pLW->nUnk3E) : "memory");
    pLW->nUnk3F = nTmp;
    if (pLW->nStatus == 1) {
        n = pLW->nUnk0C;
        if (n == 0) {
            goto do_break;
        }
        if (n >= 0) {
            if (n < 3) {
                pLW->nUnk08 = 0;
            }
        }
        goto done;
    } else if (pLW->nStatus == 2) {
        n = pLW->nUnk0C;
        if (n == 0) {
            goto do_break;
        }
        if (n >= 0) {
            if (n < 3) {
                goto do_close;
            }
        }
        goto done;
    }
    goto done;

do_break:
    sceCdBreak();
    sceCdPause();
    goto done;

do_close:
    PASSTHRU(pFd, &LW);
    if (pFd->nFd >= 0) {
        sceClose(pFd->nFd);
        pFd->nFd = -1;
    }

done:
    PASSTHRU(pEnd, &LW);
    pEnd->nStatus = 0;
    /* The epilogue restores $s1 before $s0 in the original -- the mirror
     * of its own prologue save order -- while gcc emits the two `ld`s in
     * increasing register number.  Pure scheduling:
     * --swap-adjacent xglCdReadCancel:41! (forced, because swap_ok will
     * not reorder two memory ops it cannot prove disjoint; these are two
     * loads from distinct stack slots). */
}

/* Return whether a read request is still in progress */
int xglCdSync(void)
{
    return LW.nStatus != 0;
}

extern unsigned char loaded_overlay;
extern char arcfileaddr[];
extern char D_00491708[];
int sceSifLoadElf(char *pName, void *pData);
int FlushCache(int nMode);

/* TODO: near-miss (~4 real diffs in a 1-word length shift). Structure,
 * strings, both copy loops and the div digits all line up; the residue
 * is one scheduling nop the original carries before the first copy
 * loop's bottom bnez (ours fills tighter, 60 vs 61 words) plus the
 * entry `move t0,s0` sitting after the sll instead of after the lbu.
 * No FILE_FIX_FLAGS entry exists for xglCd.c to scope a slot-nop fix. */
/* Load the numbered engine overlay ELF ("OV%02d" + arcfileaddr suffix
 * appended to the cdrom0 prefix) unless it is already resident */
void xglCdLoadOverlay(int nNo)
{
    char aData[16];
    char aBuf[256];
    PIN(u_char *pSrc, "$5");
    char *pDst;
    PIN(int c, "$2");

    if (nNo != loaded_overlay) {
        loaded_overlay = nNo;
        pSrc = (u_char *)D_00491708;
        c = *pSrc;
        pDst = aBuf;
        PASSTHRU(c, c);
        for (;;) {
            *pDst = c;
            if ((c << 24) == 0) {
                break;
            }
            pSrc++;
            pDst++;
            c = *pSrc;
            PASSTHRU(c, c);
        }
        pDst[1] = 'V';
        pDst[0] = 'O';
        pDst[2] = nNo / 10 + 48;
        pDst[3] = nNo % 10 + 48;
        pDst += 4;
        pSrc = (u_char *)arcfileaddr;
        for (;;) {
            c = *pSrc;
            PASSTHRU(c, c);
            *pDst = c;
            if ((c << 24) == 0) {
                break;
            }
            pSrc++;
            pDst++;
        }
        FlushCache(0);
        FlushCache(2);
        sceSifLoadElf(aBuf, aData);
        FlushCache(0);
        FlushCache(2);
    }
}


int StreamReadRingCoreSub(XGLCDSTREAM *pStr, char *pBuf, int nSectors);

/* Blocking stream read: pull whole sectors through the ring core, track
 * the bytes remaining, and zero-pad the tail up to the ring granule */
int xglCdStreamRead(XGLCDSTREAM *pStr, char *pBuf, int nBytes)
{
    int nRead;
    int nRest;
    int nPad;

    nRead = StreamReadRingCoreSub(pStr, pBuf, nBytes >> 11);
    nRest = pStr->nUnk18 - nRead;
    if (nRest <= 0) {
        pStr->nUnk08 = 1;
        nRest = 0;
    }
    nPad = nRead & (pStr->nUnk1C - 1);
    pStr->nUnk18 = nRest;
    if (nPad > 0) {
        pBuf += nRead;
        nRead += nPad;
        do {
            nPad--;
            *pBuf++ = 0;
        } while (nPad > 0);
    }
    return nRead;
}

/* Read from a stream ring buffer using the appropriate backend */
int xglCdStreamReadRingCore(void *pStr)
{
    if (*(int *)((char *)pStr + 0xC) != 0) {
        return StreamReadRingCoreXss(pStr);
    }
    return StreamReadRingCoreNormal(pStr);
}

/* Advance the ring write cursor by a sector-aligned byte count, kick the
 * ring core and spin until the read cursor catches up */
int xglCdStreamReadRing(XGLCDSTREAM *pStr, int nBytes)
{
    PIN(int nBusy, "$2");

    nBytes = (nBytes + 2047) & -2048;
    pStr->nWritePos = (pStr->nWritePos + nBytes) % pStr->nRingBytes;
    for (;;) {
        xglCdStreamReadRingCore(pStr);
        PASSTHRU(nBusy, pStr->nUnk18);
        if (nBusy == 0) {
            break;
        }
        if (pStr->nReadPos == pStr->nWritePos) {
            break;
        }
    }
    return 0;
}

extern u_char ArcHeader[];
extern u_char system_cnf[];
int sceCdInit(int nMode);
int sceCdMmode(int nMedia);
int sceCdReadDvdDualInfo(int *pOnDual);
void xglHddActivate(int nArg);
int xglHddMount(void);
int xglCdReadFile(char *pName, u_int nAddr, int nOfs, int nSize);

/* Bring up the CD/DVD subsystem: reset state, note dual-layer media,
 * invalidate the archive-header slots, read system.cnf into the work
 * buffer and mirror it, then mount the HDD and archive tables.
 * Register shape: pArc/nM1 need the zero-code tied passthroughs while
 * nTwo/nDualByte only pin (asm forms there push the lbu below the
 * lui/lw pair); ArcHeader store order back-solved to 72,24,48,0. */
void xglCdInitial(void)
{
    u_char *p;
    u_char *pSrc;
    u_int nAddr;
    PIN(u_char *pArc, "$3");
    PIN(int nM1, "$2");
    PIN(int nDualByte, "$9");
    PIN(int nTwo, "$10");
    u_char *pDst;
    int i;
    int nDual;

    p = (u_char *)&LW;
    p[49] = 0;
    p[50] = 0;
    p[51] = 0;
    p[52] = 0;
    p[28] = 0;
    p[29] = 0;
    sceCdInit(0);
    sceCdMmode(2);
    xglCdReset();
    nDual = 0;
    sceCdReadDvdDualInfo(&nDual);
    nTwo = 2;
    nDualByte = *(u_char *)&nDual;
    p[51] = nDualByte;
    nAddr = WorkEnd;
    PASSTHRU(pArc, ArcHeader);
    PASSTHRU(nM1, -1);
    pArc[72] = nM1;
    loaded_overlay = nTwo;
    pArc[24] = nM1;
    pArc[48] = nM1;
    pArc[0] = nM1;
    pDst = system_cnf;
    xglCdReadFile("system.cnf", nAddr, 0, 1);
    pSrc = (u_char *)nAddr;
    for (i = 79; i >= 0; i--) {
        *pDst++ = *pSrc++;
    }
    xglHddActivate(256);
    xglHddMount();
    xglCdArcInit();
}

/* Probe the archive table using the reserved work area */
int xglCdArcCheck(void)
{
    int nSave;
    int nRet;

    nRet = D_0093DC44[0];
    nSave = WorkEnd;
    WorkEnd = nRet;
    nRet = xglCdArcInit();
    WorkEnd = nSave;
    return nRet;
}

/* IOP power-off callback: just raise the flag for the control thread */
void xglCdPowerOffCB(void)
{
    LW.nPowerOff = 1;
}

/* Reset a CD stream descriptor to its default geometry */
void xglCdStreamParamInit(XGLCDSTREAM *pStream)
{
    pStream->nSectors = 64;
    pStream->nRingSects = 16;
    pStream->nFd = -1;
    pStream->nUnk00 = 0;
    pStream->nUnk08 = 0;
    pStream->nUnk0C = 0;
    pStream->nUnk20 = 0;
}

/* Skip over an archive TOC entry chain and return the 64-byte-aligned
 * address just past it, zero-terminating the next entry */
char *xglCdArcInitSub2(char *pArc)
{
    PIN(unsigned char *p, "$3") = (unsigned char *)pArc + 1;
    PIN(unsigned int c, "$5");

    c = *p;

    if (c != 0) {
        do {
            unsigned int t80 = c & 0x80;
            unsigned int t40 = c & 0x40;
            c &= 0x3F;
            if (t80 == 0) {
                unsigned char *r;
                p += 7;
                r = p + 3;
                if (t40 != 0) {
                    p = r;
                }
            }
            p += c;
            c = *p;
        } while (c != 0);
    }
    p = (unsigned char *)(((unsigned int)p + 64) & ~63);
    p[1] = 0;
    p[0] = 0;
    return (char *)p;
}

/* Rewind a stream to its start (CD streams seek, HDD/host files lseek) */
int xglCdStreamRewind(XGLCDSTREAM *pStream)
{
    int nType;

    pStream->nUnk08 = 0;
    pStream->nUnk18 = pStream->nUnk14;
    nType = pStream->nType;
    switch (nType) {
    case 0:
        sceCdStSeek(pStream->nFd);
        break;
    case 1:
    case 2:
        sceLseek(pStream->nFd, 0, 0);
        break;
    }
    return 0;
}

/* Register one archive TOC block: record the buffer and size, clear the
 * ready flag and prime the per-entry file positions */
int xglCdArcInitSub1(XGLCDARC *pArc, char *pName, char *pBuf)
{
    XGLCDFILEPOS pos;
    int nCount;

    if (xglCdGetFilePos(&pos, pName, xglCdDummyCallback) == 0) {
        return 0;
    }
    pArc->pBuf = pBuf;
    pArc->nLsn = pos.nUnk00;
    pArc->nReady = 0;
    xglCdArcInitSub0(&pos, pBuf, 0);
    nCount = *(unsigned char *)pBuf;
    if (nCount >= 2) {
        xglCdArcInitSub0(&pos, pBuf + 0x800, nCount - 1);
    }
    return nCount + 1;
}

/* Read one archive TOC block (or nCount sectors past it) synchronously */
void xglCdArcInitSub0(XGLCDFILEPOS *pPos, char *pBuf, int nCount)
{
    char aMode[4];
    int nLsn;

    sceCdSync(0);
    sceCdDiskReady(0);
    if (nCount == 0) {
        nLsn = pPos->nUnk00;
        nCount = 1;
    } else {
        nLsn = pPos->nUnk00 + 1;
    }
    aMode[1] = 1;
    aMode[0] = 0;
    aMode[2] = 0;
    sceCdRead(nLsn, nCount, pBuf, aMode);
    while (sceCdSync(1) > 0) {
    }
}

/* --- Stream close --- */
int sceCdStStop(void);
int sceSifFreeIopHeap(void *);
extern XGLCDSTREAM *listnow[2];   /* active-stream slots (.sbss) */
extern char D_0093CC30;          /* stream-busy flag (uncached mirror) */

/* TODO: WIP (~30 diffs, built 148 vs orig 224 bytes). Two blockers:
 * (1) the one-and-a-bit-iteration slot-search loop collapses (gcc folds
 * i<=0 after i++ and deletes the back edge; the original keeps a peeled
 * first compare + live loop); (2) `nType >= 0` on the int copy of the
 * u_char field folds away (orig emits a real bltz), and the built picks
 * beql/bnezl likely forms where orig has plain branches. */
/* Close a CD stream: drop it from the active slots, stop/close the
 * underlying transport and clear the busy flag */
int xglCdStreamClose(XGLCDSTREAM *pStream)
{
    XGLCDSTREAM **pp;
    int nType;
    int i;

    i = 0;
    pp = listnow;
    do {
        if (*pp == pStream) {
            *pp = 0;
            break;
        }
        i++;
        pp++;
    } while (i <= 0);

    if (pStream->nFd != -1) {
        nType = pStream->nType;
        if (nType == 0) {
            sceCdStStop();
            if (pStream->nUnk00 != 0) {
                sceSifFreeIopHeap((void *)pStream->nUnk00);
            }
            pStream->nFd = -1;
        } else if (nType >= 0 && nType < 3) {
            sceClose(pStream->nFd);
        }
        D_0093CC30 = 0;
    }
    return 0;
}

int sceSifLoadModule(char *pPath, int nArgLen, char *pArgs);

/* Build "cdrom0:\IOP\<NAME>.IRX;1" (the module name upper-cased) and load
 * it into the IOP */
void xglCdSifLoadModule(char *pName, char *pArgs)
{
    static char head[] = "cdrom0:\\IOP\\";
    static char tail[] = ".IRX;1";
    char szPath[256];
    char *p;
    char *q;
    char *a;
    int nArgLen;

    p = szPath;
    q = head;
    while ((*p = *q) != 0) {
        q++;
        p++;
    }
    q = pName;
    while (*q != 0) {
        if (*q < 'a') {
            *p = *q;
        } else {
            *p = *q - 32;
        }
        q++;
        p++;
    }
    q = tail;
    while ((*p = *q) != 0) {
        q++;
        p++;
    }
    nArgLen = 0;
    if (pArgs != 0) {
        a = pArgs;
        nArgLen = 1;
        while (*a != 0 || a[1] != 0) {
            a++;
            nArgLen++;
        }
    }
    sceSifLoadModule(szPath, nArgLen, pArgs);
}

int sceCdStRead(int nSectors, void *pBuf, int nMode, u_int *pErr);
int sceRead(int nFd, void *pBuf, int nSize);

/* Pull whole sectors for one stream, either through the CD streaming
 * engine (type 0) or through a plain file descriptor (types 1 and 2) */
int StreamReadRingCoreSub(XGLCDSTREAM *pStr, char *pBuf, int nSectors)
{
    u_int nErr;
    int nResult;

    nResult = 0;
    switch (pStr->nType) {
    case 0:
        nResult = sceCdStRead(nSectors, pBuf, 1, &nErr) << 11;
        break;
    case 1:
    case 2:
        nResult = sceRead(pStr->nFd, pBuf, nSectors << 11);
        break;
    }
    return nResult;
}
