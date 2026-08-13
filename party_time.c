typedef struct {
    int nA;
    int nB;
} XGL_CLOCK;

void xglClockRead(XGL_CLOCK *);
void xglClockUInt2DayTime(void *, int);
int xglClockDayTime2UInt(void *);
extern int D_00491818[];
extern XGL_CLOCK _CountTime;
int PartyTimeLimitCheck(void *);

void PartyTimeInit(void)
{
    xglClockUInt2DayTime(D_00491818, 0);
    xglClockRead(&_CountTime);
}

void PartyTimePauseEnd(void)
{
    xglClockRead(&_CountTime);
}

int PartyTimeUpDate(void)
{
    XGL_CLOCK sTemp;
    int nElapsed;
    int nBase;
    int nNow;

    xglClockRead(&sTemp);
    nBase = xglClockDayTime2UInt(&_CountTime);
    nNow = xglClockDayTime2UInt(&sTemp);
    nElapsed = nNow - nBase;
    xglClockUInt2DayTime(&_CountTime, nBase + nElapsed);
    _CountTime = sTemp;
    PartyTimeLimitCheck(&_CountTime);
    return (int)&_CountTime;
}
