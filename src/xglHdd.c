/* HDD / memory-card mount, save-slot and install handling for the xgl engine */

typedef struct {
    char *pName;            /* 0x00 */
    void *pBuf;             /* 0x04 */
    int nSize;              /* 0x08 */
} XGLHDDREQ;

typedef struct {
    int (*pfnRead)(int, int, void *);   /* 0x00 */
    int nSector;                        /* 0x04 */
    void *pArg;                         /* 0x08 */
    int nResult;                        /* 0x0C */
    int nTotal;                         /* 0x10 */
} XGLHDDINSTALLCB;

unsigned char HddActive;
XGLHDDINSTALLCB *HddInstallCBparam;

extern char D_004DC370[];
extern char D_004DC398[];
extern char commonname[];
extern char partitionname[];

int sceMount(char *pDevice, char *pName, int nFlag, void *pArg, int nArgLen);
int sceUmount(char *pDevice);
int sceRemove(char *pName);
int sceOpen(char *pName, int nFlag, int nMode);
int sceRead(int nFd, void *pBuf, int nSize);
int sceClose(int nFd);
int create_file(char *pName, int nFlag, void *pBuf, int nSize);
void make_fullpath(char *pDest, char *pName, int nFlag);
int xglCdArcCheck(void);
void xglCdReadCancel(void);
int xglHddCheckCore(void);
int xglHddErrorScreen(void);
int xglHddActivate(int nMode);
int xglHddCheck2(void);
int xglHddMcLoadMount(void);
int xglHddMcLoadCore(XGLHDDREQ *pReq);
int xglHddMcUmount(void);

/* Probe the HDD, deactivating it when the media has gone away */
int xglHddCheck2(void)
{
    int nRet;

    nRet = xglHddCheckCore();
    if (nRet == -6) {
        xglHddActivate(0x103);
        return -5;
    }
    return nRet;
}

/* Probe the HDD, showing the error screen when the media has gone away */
int xglHddCheck(void)
{
    int nRet;

    if (HddActive == 3) {
        xglHddErrorScreen();
        xglHddActivate(0);
        xglHddActivate(0x102);
    }
    nRet = xglHddCheckCore();
    if (nRet == -6) {
        xglHddErrorScreen();
        xglHddActivate(0);
        xglHddActivate(0x102);
        return -5;
    }
    return nRet;
}

/* Mount the shared save partition for reading */
int xglHddMcLoadMount(void)
{
    int nRet;

    if (xglHddCheck2() != 0) {
        return -1;
    }
    nRet = sceMount(D_004DC370, commonname, 5, 0, 0);
    if (nRet < 0 && nRet != -0x10) {
        return -1;
    }
    return 0;
}

/* Unmount the shared save partition */
int xglHddMcUmount(void)
{
    return (sceUmount(D_004DC370) < 0) ? -2 : 0;
}

/* Read one save file into the caller's buffer */
int xglHddMcLoadCore(XGLHDDREQ *pReq)
{
    char szPath[0x100];
    int nFd;
    int nRet;

    make_fullpath(szPath, pReq->pName, 0);
    nFd = sceOpen(szPath, 1, 0x1B6);
    if (nFd < 0) {
        nRet = (nFd == -2) ? 1 : -6;
    } else {
        nRet = 0;
        if (sceRead(nFd, pReq->pBuf, pReq->nSize) < 0) {
            nRet = -7;
        }
        if (sceClose(nFd) < 0) {
            nRet = -8;
        }
    }
    return nRet;
}

/* Mount, read one save slot, then unmount again */
int xglHddMcLoad(XGLHDDREQ *pReq)
{
    int nRet;

    nRet = xglHddMcLoadMount();
    if (nRet < 0) {
        return nRet;
    }
    nRet = xglHddMcLoadCore(pReq);
    if (nRet < 0) {
        return nRet;
    }
    return xglHddMcUmount();
}

/* Mount, write one save slot, then unmount again */
int xglHddMcSave(XGLHDDREQ *pReq)
{
    int nRet;

    if (xglHddCheck2() != 0) {
        return -1;
    }
    if (sceMount(D_004DC370, commonname, 4, 0, 0) < 0) {
        return -1;
    }
    nRet = (create_file(pReq->pName, 0, pReq->pBuf, pReq->nSize) < 0) ? -0x18 : 0;
    xglHddMcUmount();
    return nRet;
}

/* Tear down the game's HDD partition */
int xglHddUninstall(void)
{
    int nRet;

    xglHddActivate(0);
    nRet = sceUmount(D_004DC398);
    if (nRet < 0 && nRet != -0x13) {
        return -1;
    }
    return (sceRemove(partitionname) < 0) ? -5 : 0;
}

/* Install progress callback: record the total, then pump the disc reader */
void xglHddInstallReadCB(int nMode, int nSector)
{
    XGLHDDINSTALLCB *p;

    p = HddInstallCBparam;
    if (nMode == 1) {
        p->nTotal = nSector + 1;
    } else if (nMode == 2) {
        p->nResult = p->pfnRead(6, (p->nSector << 4) + ((nSector << 4) / p->nTotal), p->pArg);
        if (p->nResult != 0) {
            xglCdReadCancel();
        }
    }
}

/* Set the HDD activity mode and report the previous one */
int xglHddActivate(int nMode)
{
    int nOld;
    int n;

    nOld = HddActive;
    if (nMode != -1 && nOld != 2) {
        n = nMode & 0xFF;
        if (n < 4) {
            if (n >= 0) {
                HddActive = nMode;
                if (nMode < 0x100) {
                    xglCdArcCheck();
                }
            }
        }
    }
    return (nOld < 2) ? nOld : 0;
}

/* Trivial stub install-callback: reports success without doing any work */
int xglHddDummyCB(void)
{
    return 0;
}

extern char yoursaves[];
int sceDopen(char *pName, void *pBuf);
int Judge_MakeNewFolder(int nFd);
int Judge_MakeNewSavedata(int nFd);

/* TODO: near-miss (25/28 words). Original spills pBuf into $s0 across
 * the sceDopen call (unused afterward) instead of just moving it into
 * $a1 directly; every natural form here uses the cheaper single move. */
/* Ensure the memory card has a "Your Saves" folder, creating the folder
 * or an empty savedata block as needed */
int xglHddMcCheckYourSaves(void *pBuf)
{
    int nFd;

    nFd = sceDopen(yoursaves, pBuf);
    if (nFd >= 0) {
        if (Judge_MakeNewFolder(nFd) == 1) {
            return 1;
        }
        return -2;
    }
    if (Judge_MakeNewSavedata(nFd) == 1) {
        return 0;
    }
    return -3;
}
