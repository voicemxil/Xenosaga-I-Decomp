/* CD/DVD access helpers for the xgl engine */

typedef unsigned int u_int;
typedef unsigned char u_char;

typedef struct {
    u_char aPad00[0x30];
    char nStatus;
    u_char aPad31[0xD];
    char nUnk3E;
    char nUnk3F;
} XGLCDWORK;

typedef struct {
    u_char aPad00[0x24];
    u_char nType;
    u_char aPad25[3];
    int nFd;
    int nSize;
} XGLCDFILEPOS;

extern XGLCDWORK LW;
extern int D_0093DC44[4];

void (*callback)();
int StrList;
int WorkEnd;

int sceCdPause(void);
int sceCdSync(int nMode);
int sceClose(int nFd);
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
