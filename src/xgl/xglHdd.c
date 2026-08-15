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
int xglHddErrorScreen(void);
int xglHddActivate(int nMode);
int xglHddCheck2(void);
int xglHddMcLoadMount(void);
int xglHddMcLoadCore(XGLHDDREQ *pReq);
int xglHddMcUmount(void);

extern char D_004DC368[];
int sceDevctl(char *pDevice, int nCmd, int a2, int a3, int a4, int a5);

/* TODO: near-miss (52/54 words). Full branch structure, sceDevctl args,
 * and the range-check shape (result<3 vs result>=3, not a plain switch)
 * are all verified against the asm -- see the derivation: result==0
 * triggers a second devctl call; result==2 falls through to the default
 * -5; result==1 -> -3; result==3 -> -1; anything else -> -99. Remaining
 * blocker: the original hoists `li v0,-5` to the very top of the
 * function (shared between the HddActive>=2 early-out and the result==2
 * fallthrough), so the whole instruction stream is offset by one word
 * versus every natural C form tried (single shared return var forces an
 * extra callee-saved register across the jal calls instead). */
int xglHddCheckCore(void)
{
    int result;

    if (HddActive < 2) {
        result = sceDevctl(D_004DC368, 0x4807, 0, 0, 0, 0);
        if (result == 0) {
            result = sceDevctl(D_004DC368, 0x4804, 0, 0, 0, 0);
            if (result >= 0) {
                return 0;
            }
            if (HddActive == 1) {
                return -6;
            }
            xglHddActivate(0x102);
            return -5;
        }
        if (result != 2) {
            if (result < 3) {
                if (result == 1) {
                    return -3;
                }
                return -0x63;
            }
            if (result == 3) {
                return -1;
            }
            return -0x63;
        }
    }
    return -5;
}

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

int xglHddMcCheckCore(int *pFree);
int xglHddMcLoadMount(void);
int xglHddMcCheckYourSaves(void *pBuf);

/* Report the free space (in KB) available for saves on the HDD.
 * Register shape: the zone-size quotient needs its own variable pinned
 * to $16 fed by the zero-code tied passthrough, and the zero-clamped
 * zone-free count is spelled as an inline ternary so the mult keeps
 * $v0 as its first operand. */
int xglHddMcGetFree(void)
{
    int nFree;
    int nZoneFree;
    int nRet;
    int nBuf;
    register int nQuot __asm__("$16");

    nRet = xglHddMcLoadMount();
    if (nRet < 0) {
        return nRet;
    }
    nFree = xglHddMcCheckYourSaves((void *)-1);
    if (nFree >= 0) {
        if (sceDevctl(D_004DC368, 0x480A, 0, 0, (int)&nBuf, 4) == 0
            && nBuf > 0x1FFFFF) {
            nFree = 0x100000;
        } else {
            nFree = sceDevctl(D_004DC370, 20481, 0, 0, 0, 0);
            nZoneFree = sceDevctl(D_004DC370, 20482, 0, 0, 0, 0);
            __asm__("" : "=r"(nQuot) : "0"(nFree / 1024));
            nFree = ((nZoneFree < 0) ? 0 : nZoneFree) * nQuot;
        }
    }
    xglHddMcUmount();
    return nFree;
}



/* NOTE: in a standalone TU the first bnez comes out annulled (bnezl,
 * 1d; --branch-unlikely xglHddMcCheck:0 fixes it there) but in this
 * full TU the dbr pass picks the original non-annulled form on its
 * own. */
/* Mount the common partition and probe the memory-card image, keeping
 * the caller's free-space slot intact */
int xglHddMcCheck(int *pFree)
{
    int nSave;
    int nRet;

    if (xglHddCheck2() != 0) {
        return -1;
    }
    xglHddMcUmount();
    if (sceMount(D_004DC370, commonname, 4, 0, 0) < 0) {
        return -1;
    }
    nSave = *pFree;
    *pFree = -1;
    nRet = xglHddMcCheckCore(pFree);
    *pFree = nSave;
    xglHddMcUmount();
    return nRet;
}

extern char yoursaves[];
int sceDopen(char *pName, void *pBuf);
int Judge_MakeNewFolder(int nFd);
int Judge_MakeNewSavedata(int nFd);

/* TODO: near-miss (25/28 words). Original spills pBuf into $s0 across
 * the sceDopen call (unused afterward) instead of just moving it into
 * $a1 directly; every natural form here uses the cheaper single move. */
/* TODO: near-miss (7 words, SCHEDULING): orig hoists the yoursaves %hi
 * (lui $v0) above the sd/move prologue pair so the address tmp lands in
 * $v0; ours schedules it after, so hi+lo fold into $a0. The $16 pin +
 * barrier reproduces the original pBuf staging through $s0 (was 25/25
 * wrong: branch polarity was also inverted -- fd<0 goes to
 * Judge_MakeNewFolder, fd>=0 to Judge_MakeNewSavedata). */
/* Ensure the memory card has a "Your Saves" folder, creating the folder
 * or an empty savedata block as needed */
int xglHddMcCheckYourSaves(void *pBuf)
{
    register void *p __asm__("$16") = pBuf;
    char *pDir = yoursaves;
    int nFd;

    __asm__("" : "+r"(p));
    nFd = sceDopen(pDir, p);
    if (nFd < 0) {
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

void xglSleep(void);
void xglFontPrint(int x, int y, int nColor, char *pStr);
void xglSoundEffectNormalDirect(int nNo);
void xglDmaDirectNormal(unsigned int nCh, unsigned int nAddr, unsigned int nQwc);

typedef struct {
    char pad00[0x2A];
    unsigned short nPress;   /* 0x2A */
    char pad2C[0x68 - 0x2C];
} XGLHDDPADDATA;
extern XGLHDDPADDATA PadData[2];

typedef struct {
    char pad00[0x58];
    unsigned char nUnk58;    /* 0x58 */
} XGLHDDRENDER;
extern XGLHDDRENDER sRender;

/* Show the HDD error screen: clear + message every frame for at least
 * 30 frames, then wait for the confirm button */
int xglHddErrorScreen(void)
{
    static unsigned long TestEnv[12] __attribute__((aligned(16))) = {
        0x0000000000000000, 0x5000000500000000,
        0x4003400000008001, 0x000000000000551E,
        0x0000000000030000, 0x0000000000000047,
        0x4000000000000000, 0x8000000000000000,
        0x000071F700006FF8, 0x0000000000FFFFF0,
        0x00008DF700008FF8, 0x0000000000FFFFF0,
    };
    int i;
    register XGLHDDRENDER *pRender __asm__("$2");

    sRender.nUnk58 = 1;
    for (i = 30; ; ) {
        xglDmaDirectNormal(2, (unsigned int)TestEnv, 6);
        xglFontPrint(48, 64, 0xFFFFFF, "\013\016\001\001\000\000\000\015\003\245\317\241\274\245\311\245\307\245\243\245\271\245\257\245\311\245\351\245\244\245\326\244\313\244\242\244\353\245\274\245\316\245\265\241\274\245\254\244\316\012\012\245\242\245\327\245\352\245\261\241\274\245\267\245\347\245\363\245\307\241\274\245\277\244\254\306\311\244\337\271\376\244\341\244\336\244\273\244\363\241\243\012\012\244\263\244\354\260\312\271\337\244\317\243\304\243\326\243\304\244\253\244\351\306\311\244\337\271\376\244\337\244\362\271\324\244\244\244\336\244\271\241\243\012\012\012\012\014\200\040\040\241\373\014\200\200\200\245\334\245\277\245\363\241\247\302\263\271\324");
        if (i > 0) {
            i--;
        } else if ((PadData[0].nPress & 0x20) != 0) {
            break;
        }
        xglSleep();
    }
    xglSoundEffectNormalDirect(1);
    __asm__("" : "=r"(pRender) : "0"(&sRender));
    pRender->nUnk58 = 0;
}
