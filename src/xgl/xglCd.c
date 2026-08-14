/* CD/DVD access helpers for the xgl engine */

typedef unsigned int u_int;
typedef unsigned char u_char;

typedef struct {
    u_char aPad00[0x30];
    char nStatus;
    u_char aPad31[3];
    char nPowerOff;      /* 0x34 */
    u_char aPad35[0x9];
    char nUnk3E;
    char nUnk3F;
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
    char pad1C[4];
    int nUnk20;          /* 0x20 */
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

/* Return whether a read request is still in progress */
int xglCdSync(void)
{
    return LW.nStatus != 0;
}

/* Read from a stream ring buffer using the appropriate backend */
int xglCdStreamReadRingCore(void *pStr)
{
    if (*(int *)((char *)pStr + 0xC) != 0) {
        return StreamReadRingCoreXss(pStr);
    }
    return StreamReadRingCoreNormal(pStr);
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
    register unsigned char *p __asm__("$3") = (unsigned char *)pArc + 1;
    register unsigned int c __asm__("$5");

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
    char nCount;

    if (xglCdGetFilePos(&pos, pName, xglCdDummyCallback) != 0) {
        pArc->pBuf = pBuf;
        pArc->nLsn = pos.nUnk00;
        pArc->nReady = 0;
        xglCdArcInitSub0(&pos, pBuf, 0);
        nCount = *pBuf;
        if (nCount >= 2) {
            xglCdArcInitSub0(&pos, pBuf + 0x800, nCount - 1);
        }
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
