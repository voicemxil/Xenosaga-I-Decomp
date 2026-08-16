/* Controller (pad) input handling */

typedef struct {
    char pad00[0x48];
    unsigned char nRepeatStart;    /* 0x48 */
    unsigned char nRepeatInterval; /* 0x49 */
    unsigned short nRepeatMask;    /* 0x4A */
    unsigned char nRepeatCount;    /* 0x4C */
    unsigned char nRepeatState;    /* 0x4D */
    unsigned char nAnaMode;        /* 0x4E */
    unsigned char nAnaMode2;       /* 0x4F */
    char pad50[0x58 - 0x50];
    unsigned char aBit[8];         /* 0x58 */
    unsigned char aTrigger[4];     /* 0x60 */
    char pad64[0x68 - 0x64];
} XGLPADDATA;

extern XGLPADDATA PadData[2];

/* Configure key-repeat for one pad (-1 = both pads) */
void xglPadSetRepeat(int nPad, unsigned short nMask, int nStart, int nInterval)
{
    XGLPADDATA *p = &PadData[nPad];

    if (nPad == -1) {
        xglPadSetRepeat(0, nMask, nStart, nInterval);
        xglPadSetRepeat(1, nMask, nStart, nInterval);
        return;
    }
    p->nRepeatStart = nStart;
    p->nRepeatInterval = nInterval;
    p->nRepeatMask = nMask;
    p->nRepeatCount = 0;
    p->nRepeatState = 0;
}

void scePadInit(int nMode);
int scePadPortOpen(int nPort, int nSlot, void *pBuf);
extern unsigned char PadDmaBuffer[];

/* Bring up both controller ports, clear their state blocks and install
 * the default repeat, button-bit and trigger tables */
void xglPadInitial(void)
{
    unsigned char *p0;
    unsigned char *p1;
    int nBit;
    unsigned int n;
    int i;
    int j;

    scePadInit(0);
    scePadPortOpen(0, 0, PadDmaBuffer);
    scePadPortOpen(1, 0, PadDmaBuffer + 256);
    p0 = (unsigned char *)&PadData[0];
    p1 = (unsigned char *)&PadData[1];
    n = 0;
    /* goto-loop: a `for` here carries a loop note and jump.c reverses the
     * dead counter into a down-counter; the original counts up. */
loop:
    n++;
    *p0++ = 0;
    *p1++ = 0;
    if (n < 0x68) {
        goto loop;
    }
    xglPadSetRepeat(0, 0xF00C, 8, 1);
    xglPadSetRepeat(1, 0xF00C, 8, 1);
    nBit = 1;
    for (i = 0; i < 8; i++) {
        PadData[0].aBit[i] = nBit;
        PadData[1].aBit[i] = nBit;
        nBit <<= 1;
    }
    PadData[0].nAnaMode = 64;
    PadData[0].nAnaMode2 = 64;
    PadData[1].nAnaMode = 64;
    PadData[1].nAnaMode2 = 64;
    for (j = 0; j < 4; j++) {
        PadData[0].aTrigger[j] = 32;
        PadData[1].aTrigger[j] = 32;
    }
}
