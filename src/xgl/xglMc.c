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
