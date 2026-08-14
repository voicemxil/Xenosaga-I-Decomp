/* Memory-card front-end state helpers */

typedef struct {
    char nUnk00;         /* 0x00 */
    char pad01[3];
    int nState;          /* 0x04 */
    char pad08[0x158];   /* keeps mw out of sdata (original uses lui/lw) */
} XGLMCWORK;

extern XGLMCWORK mw;
extern char queue_top;
extern unsigned char queue_end;

int sceMcInit(void);
void xglMcSetMapName(char *pName, int nArg);
void xglMcReset(void);

/* Current memory-card state-machine state */
int xglMcGetState(void)
{
    return mw.nState;
}

/* Clear the request queue, map name and state byte */
void xglMcReset(void)
{
    queue_top = 0;
    queue_end = 0;
    xglMcSetMapName(0, 0);
    mw.nUnk00 = 0;
}

/* Bring up the memory-card library and reset the front-end */
void xglMcInitial(void)
{
    sceMcInit();
    xglMcReset();
}

/* Convert a 2-byte EUC code to Shift-JIS in place */
void xglMcEUC2SJIS(unsigned char *pHigh, unsigned char *pLow)
{
    unsigned char c1 = *pHigh + 128;
    unsigned int c2 = (unsigned char)(*pLow + 128);
    unsigned char nOdd = c1 & 1;

    if (nOdd != 0) {
        c2 += 31;
        c1 = (c1 >> 1) + 113;
    } else {
        c2 += 125;
        c1 = (c1 >> 1) + 112;
    }
    if (c1 >= 160) {
        c1 += 64;
    }
    c2 &= 0xFF;
    if (c2 >= 127) {
        c2 = (c2 + 1) & 0xFF;
    }
    if (c1 == 135) {
        if (c2 == 84) {
            c1 = 130;
            c2 = 80;
        }
    }
    *pHigh = c1;
    *pLow = c2;
}

/* --- Request queue --- */
typedef struct {
    short nId;               /* 0x00 */
    short nCmd;              /* 0x02 */
    int *pResult;            /* 0x04 */
    int nParam0;             /* 0x08 */
    int nParam1;             /* 0x0C */
    int nParam2;             /* 0x10 */
    char aName[0x6C];        /* 0x14 */
} MCREQUEST;

typedef struct {
    int nData0;              /* 0x00 */
    char *pName;             /* 0x04 */
    int nData1;              /* 0x08 */
    int nData2;              /* 0x0C */
} MCPARAM;

extern MCREQUEST queue[8];

/* TODO: near-miss (21/56, was 23). Wave 3 solved: queue_end staged
 * through a single unsigned local (orig keeps it in $t2 across the
 * whole body and re-uses it for the final increment), pName tested in
 * $v1 then copied into pinned pSrc=$5 via the zero-code tied
 * passthrough, pDst/i pinned to $4/$6, and a passthrough on c after
 * each lbu keeps the (c << 24) test unfolded (all verified). The copy
 * loop is also store-first (the original writes the NUL terminator: the
 * sb sits in the bottom bnez's always-executed delay slot). Remaining:
 * gcc copies the loop header (peels the first test: lbu/sll/beqz + li
 * $a2,1 at entry) where the original keeps one bottom test entered via
 * b-to-bottom with i=0 in the b's slot; volatile passthroughs, plain
 * while, and for(;;)+breaks all still peel. */
int sceMcGetInfo(int, int, int *, int *, int *);
int sceMcSync(int, int *, int *);
int sceMcOpen(int, int, char *, int);
int sceMcRead(int, void *, int);
int sceMcWrite(int, void *, int);
int sceMcClose(int);

/* One-shot blocking load of a whole file from memory card 0 */
int xglMcEasyLoad(char *pName, void *pBuf, int nSize)
{
    int nFormat;
    int nCmd;
    int nResult;
    int nFd;
    int nRead;

    sceMcGetInfo(0, 0, 0, 0, &nFormat);
    sceMcSync(0, &nCmd, &nResult);
    if (nFormat == 0) {
        return -1;
    }
    if (nResult < -1) {
        return nResult;
    }
    sceMcOpen(0, 0, pName, 1);
    sceMcSync(0, &nCmd, &nResult);
    nFd = nResult;
    if (nFd < 0) {
        return nFd;
    }
    sceMcRead(nFd, pBuf, nSize);
    sceMcSync(0, &nCmd, &nResult);
    nRead = nResult;
    if (nRead < 0) {
        return nRead;
    }
    sceMcClose(nFd);
    sceMcSync(0, &nCmd, &nResult);
    return (nResult < 0) ? nResult : nRead;
}

/* One-shot blocking save of a whole file to memory card 0 */
int xglMcEasySave(char *pName, void *pBuf, int nSize)
{
    int nFormat;
    int nCmd;
    int nResult;
    int nFd;

    sceMcGetInfo(0, 0, 0, 0, &nFormat);
    sceMcSync(0, &nCmd, &nResult);
    if (nResult < -1) {
        return -1;
    }
    if (nFormat == 0) {
        return -1;
    }
    sceMcOpen(0, 0, pName, 514);
    sceMcSync(0, &nCmd, &nResult);
    nFd = nResult;
    if (nFd < 0) {
        return -1;
    }
    sceMcWrite(nFd, pBuf, nSize);
    sceMcSync(0, &nCmd, &nResult);
    if (nResult < 0) {
        return -1;
    }
    sceMcClose(nFd);
    sceMcSync(0, &nCmd, &nResult);
    return (nResult < 0) ? -1 : 0;
}

/* Write the two map-number digits ('O'+digit) into an EUC name buffer */
void xglMcWriteMapName(char *pName, int nNo)
{
    int n;

    n = nNo + 1;
    pName[229] = n % 10 + 79;
    pName[227] = n / 10 % 10 + 79;
}

int xglMcRequest(int nId, int nCmd, MCPARAM *pParam, int *pResult)
{
    MCREQUEST *pReq;
    register unsigned char *pSrc __asm__("$5");
    register char *pDst __asm__("$4");
    register int i __asm__("$6");
    int c;
    int n0;
    int n1;
    unsigned int nEnd;
    unsigned char *pName;

    nEnd = queue_end;
    pReq = &queue[nEnd];
    *pResult = 0;
    pReq->nId = nId;
    pReq->pResult = pResult;
    pReq->nCmd = nCmd;
    switch (nCmd) {
    case 2:
    case 4:
    case 6:
    case 8:
        n0 = pParam->nData0;
        n1 = pParam->nData1;
        pReq->nParam0 = n0;
        pReq->nParam1 = n1;
        pReq->nParam2 = pParam->nData2;
        break;
    case 3:
    case 5:
    case 7:
        n0 = pParam->nData0;
        n1 = pParam->nData1;
        pReq->nParam0 = n0;
        pReq->nParam1 = n1;
        pName = (unsigned char *)pParam->pName;
        pReq->nParam2 = pParam->nData2;
        if (pName == 0) {
            *(int *)pReq->aName = 0;
        } else {
            __asm__("" : "=r"(pSrc) : "0"(pName));
            pDst = pReq->aName;
            c = *pSrc;
            __asm__("" : "=r"(c) : "0"(c));
            i = 0;
            for (;;) {
                *pDst = c;
                if ((c << 24) == 0) {
                    break;
                }
                i++;
                pSrc++;
                if (i >= 108) {
                    break;
                }
                pDst++;
                c = *pSrc;
                __asm__("" : "=r"(c) : "0"(c));
            }
        }
        break;
    case 1:
    case 9:
    case 10:
        break;
    }
    queue_end = (nEnd + 1) & 7;
    return 0;
}
