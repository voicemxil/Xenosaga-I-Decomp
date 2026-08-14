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
    unsigned char nStat;     /* 0x00 */
    unsigned char nSecond;   /* 0x01 */
    unsigned char nMinute;   /* 0x02 */
    unsigned char nHour;     /* 0x03 */
    unsigned char nDay;      /* 0x04 */
    unsigned char nMonth;    /* 0x05 */
    unsigned short nYear;    /* 0x06 */
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

/* --- Calendar tables: per-month day counts, then cumulative day totals --- */
extern unsigned short D_00491660[];   /* days in each month (Feb = 28) */
extern unsigned short D_00491676[];   /* cumulative days before month m */

/* TODO: near-miss (32 words, pure REGISTER rotation). Structure, term
 * grouping and every opcode match; the whole GPR assignment is rotated
 * one slot (orig gives the accumulator $a1 with highest priority, ours
 * allocates it last -> $t1, shifting hour/const/day/min down). Tried:
 * in-place += chain (fixed the addu shapes), unsigned year (srl), nDaySec
 * constant-range lever, declaration order. */
/* Convert a calendar date to seconds since 2000-01-01 */
unsigned int xglClockDayTime2UInt(XGLDAYTIME *pTime)
{
    unsigned int nAcc;
    unsigned int nMonth;
    unsigned int nHourTerm;
    unsigned int nLeap;
    unsigned int nYear;
    unsigned int nDaySec;

    nMonth = pTime->nMonth;
    nDaySec = 86400u;
    nHourTerm = pTime->nHour * 3600u;
    nAcc = D_00491676[nMonth];
    nAcc += pTime->nDay;
    nYear = pTime->nYear;
    nAcc *= nDaySec;
    nAcc += pTime->nSecond;
    nHourTerm -= nDaySec;
    nAcc += nHourTerm;
    nLeap = (nYear - 1997) >> 2;
    nYear -= 2000;
    nAcc += pTime->nMinute * 60u;
    if ((nYear & 3) == 0 && nMonth >= 3) {
        nAcc += 86400u;
    }
    return nAcc + (nLeap * nDaySec + nYear * 31536000u);
}

/* Convert seconds since 2000-01-01 back to a calendar date */
void xglClockUInt2DayTime(XGLDAYTIME *pTime, unsigned int nTime)
{
    unsigned int nDays;
    unsigned int nRem;
    unsigned int nHour;
    unsigned int nMin;
    unsigned int nYear;
    unsigned int nYearLen;
    unsigned int nMonth;
    unsigned int nMonthLen;

    nDays = nTime / 86400u;
    nRem = nTime % 86400u;
    nYear = 0;
    pTime->nHour = nRem / 3600u;
    nRem -= pTime->nHour * 3600u;
    pTime->nMinute = nRem / 60u;
    nRem -= pTime->nMinute * 60u;
    pTime->nSecond = nRem;
    while (nDays >= (nYearLen = ((nYear & 3) == 0) ? 366u : 365u)) {
        nYear++;
        nDays -= nYearLen;
    }
    pTime->nYear = nYear + 2000;
    for (nMonth = 0; ; nMonth++) {
        nMonthLen = D_00491660[nMonth];
        if (nMonth == 1 && (pTime->nYear & 3) == 0) {
            nMonthLen = nMonthLen + 1;
        }
        if (nDays < nMonthLen) {
            break;
        }
        nDays -= nMonthLen;
    }
    pTime->nMonth = nMonth + 1;
    pTime->nDay = nDays + 1;
}
