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

void *PartyTimeUpDate(void)
{
    int nBaseUint;
    XGL_CLOCK sTemp;
    int nCountUint;
    int nTempUint;

    xglClockRead(&sTemp);
    nBaseUint = xglClockDayTime2UInt(D_00491818);
    nCountUint = xglClockDayTime2UInt(&_CountTime);
    nTempUint = xglClockDayTime2UInt(&sTemp);
    xglClockUInt2DayTime(D_00491818, nBaseUint + (nTempUint - nCountUint));
    _CountTime = sTemp;
    PartyTimeLimitCheck(&_CountTime);
    return D_00491818;
}
