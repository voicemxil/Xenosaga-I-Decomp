typedef struct {
    unsigned char nUnk0;
    unsigned char nUnk1;
    unsigned char nUnk2;
    unsigned char nDay;
    unsigned char nMonth;
} PARTY_TIME_DISP;

void PartyTimeDispChange(PARTY_TIME_DISP *pDisp)
{
    unsigned char nMonth;

    nMonth = pDisp->nMonth;
    if (nMonth != 0) {
        pDisp->nDay = pDisp->nDay + nMonth * 24 - 24;
    }
}
