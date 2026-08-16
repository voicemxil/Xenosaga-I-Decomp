#include "matching.h"

/* Memory-card front-end state helpers */

typedef struct {
    char nUnk00;         /* 0x00 */
    char pad01[3];
    int nState;          /* 0x04 */
    char pad08[0x78];
    unsigned char aMapName[0x22];  /* 0x80: map name, Shift-JIS, 17 pairs */
    char padA2[0xBE];    /* keeps mw out of sdata (original uses lui/lw) */
} XGLMCWORK;

extern XGLMCWORK mw;
extern char queue_top;
extern unsigned char queue_end;

int sceMcInit(void);
void xglMcSetMapName(char *pName, char *pSub);
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

/* Register shape: queue_end staged through a single unsigned local
 * (the original keeps it in $t2 across the whole body and re-uses it
 * for the final increment), pName tested in $v1 then copied into pinned
 * pSrc=$5 via the zero-code tied passthrough, pDst/i/c pinned to
 * $4/$6/$2, and a passthrough on c after each lbu keeps the (c << 24)
 * test unfolded. */
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

/* Build the Shift-JIS map name shown on the memory-card save: convert
 * the EUC name (and an optional second name, joined by a middle dot)
 * into mw.aMapName, then zero-pad the rest */
void xglMcSetMapName(char *pName, char *pSub)
{
    unsigned char aTmp[2];
    unsigned char *pDst;
    int n;

    pDst = mw.aMapName;
    n = 0;
    if (pName != 0) {
        aTmp[0] = pName[0];
        while (aTmp[0] != 0) {
            unsigned char *pLow = &aTmp[1];

            aTmp[1] = pName[1];
            pName += 2;
            xglMcEUC2SJIS(aTmp, pLow);
            pDst[0] = aTmp[0];
            pDst[1] = aTmp[1];
            n++;
            pDst += 2;
            if (n >= 16) {
                goto tail;
            }
            aTmp[0] = pName[0];
        }
        if (n < 16) {
            if (pSub != 0) {
                if (*pSub != 0) {
                    pDst[0] = 0x81;
                    pDst[1] = 0x45;
                    n++;
                    pDst += 2;
                }
            }
        }
    }
tail:
    if (pSub != 0) {
        while (n < 16) {
            unsigned char *pLow;

            aTmp[0] = pSub[0];
            if (aTmp[0] == 0) {
                break;
            }
            pLow = &aTmp[1];
            aTmp[1] = pSub[1];
            pSub += 2;
            xglMcEUC2SJIS(aTmp, pLow);
            pDst[0] = aTmp[0];
            pDst[1] = aTmp[1];
            n++;
            pDst += 2;
        }
    }
    while (n < 17) {
        pDst[0] = 0;
        pDst[1] = 0;
        n++;
        pDst += 2;
    }
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
    PIN(unsigned char *pSrc, "$5");
    PIN(char *pDst, "$4");
    PIN(int i, "$6");
    PIN(int c, "$2");
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
            /* Both pointers go through the zero-code tied passthrough:
             * pDst as a plain assignment lets sched2 hoist the first
             * `lbu` above the destination address computation, which is
             * the reverse of the original. */
            PASSTHRU(pSrc, pName);
            PASSTHRU(pDst, pReq->aName);
            c = *pSrc;
            PASSTHRU(c, c);
            /* The original's loop is rotated: entry jumps straight to
             * the store/test at the bottom.  Written as a for/while,
             * gcc 2.96 peels the first iteration instead and the
             * function comes out two words long. */
            i = 0;
            goto test;
body:
            i++;
            pSrc++;
            pDst++;
            if (i >= 108) {
                goto done;
            }
            c = *pSrc;
test:
            *pDst = c;
            if ((c << 24) != 0) {
                goto body;
            }
done:
            ;
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
