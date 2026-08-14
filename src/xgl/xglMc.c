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

/* TODO: near-miss (23/56, was 56/56). Solved: int (unnarrowed) args,
 * unsigned queue_end, paired load/store bodies, pName loaded between the
 * nParam1/nParam2 pairs. Remaining: orig stages pName through a
 * `move $a1,$v1` copy (same surviving-copy puzzle as xglMakeSePacket),
 * dst/count register rotation downstream of it, and the copy loop is
 * emitted b-to-bottom-test with an UNFOLDED `(c << 24) != 0` byte test
 * (every source spelling tried gets folded to beqz-on-lbu and the loop
 * rotates; likely needs the copy fixed first). */
/* Queue one memory-card request (params copied per command kind) */
int xglMcRequest(int nId, int nCmd, MCPARAM *pParam, int *pResult)
{
    MCREQUEST *pReq;
    unsigned char *pSrc;
    char *pDst;
    int i;
    int c;
    int n0;
    int n1;

    pReq = &queue[queue_end];
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
        pSrc = (unsigned char *)pParam->pName;
        pReq->nParam2 = pParam->nData2;
        if (pSrc == 0) {
            *(int *)pReq->aName = 0;
        } else {
            pDst = pReq->aName;
            c = *pSrc;
            i = 0;
            while ((c << 24) != 0) {
                *pDst = c;
                i++;
                pSrc++;
                if (i >= 108) {
                    break;
                }
                pDst++;
                c = *pSrc;
            }
        }
        break;
    case 1:
    case 9:
    case 10:
        break;
    }
    queue_end = (queue_end + 1) & 7;
    return 0;
}
