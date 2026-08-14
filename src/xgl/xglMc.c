/* Memory-card front-end state helpers */

typedef struct {
    char nUnk00;         /* 0x00 */
    char pad01[3];
    int nState;          /* 0x04 */
    char pad08[0x158];   /* keeps mw out of sdata (original uses lui/lw) */
} XGLMCWORK;

extern XGLMCWORK mw;
extern char queue_top;
extern char queue_end;

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
