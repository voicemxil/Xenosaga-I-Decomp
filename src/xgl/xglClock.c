/* Real-time clock access (RTC read + calendar conversions) */

typedef struct {
    unsigned char stat;      /* 0x00 */
    unsigned char second;    /* 0x01 */
    unsigned char minute;    /* 0x02 */
    unsigned char hour;      /* 0x03 */
    unsigned char pad;       /* 0x04 */
    unsigned char day;       /* 0x05 */
    unsigned char month;     /* 0x06 */
    unsigned char year;      /* 0x07 */
} XGLCDCLOCK;

typedef struct {
    char nStat;              /* 0x00 */
    char nSecond;            /* 0x01 */
    char nMinute;            /* 0x02 */
    char nHour;              /* 0x03 */
    char nDay;               /* 0x04 */
    char nMonth;             /* 0x05 */
    short nYear;             /* 0x06 */
} XGLDAYTIME;

extern char ReadClockInterval;
extern XGLCDCLOCK PresentTime;

int sceCdReadClock(XGLCDCLOCK *pClock);

/* Two-digit BCD -> binary */
static int BCD2INT(int nBcd)
{
    unsigned int n = nBcd & 0xFF;
    return (n >> 4) * 10 + (n & 0xF);
}

/* Read the RTC (at most once per interval window) and unpack it */
void xglClockRead(XGLDAYTIME *pTime)
{
    if (ReadClockInterval == 0) {
        ReadClockInterval = 10;
        sceCdReadClock(&PresentTime);
    }
    pTime->nYear = BCD2INT(PresentTime.year) + 2000;
    pTime->nMonth = BCD2INT(PresentTime.month);
    pTime->nDay = BCD2INT(PresentTime.day);
    pTime->nHour = BCD2INT(PresentTime.hour);
    pTime->nMinute = BCD2INT(PresentTime.minute);
    pTime->nSecond = BCD2INT(PresentTime.second);
    pTime->nStat = 0;
}
