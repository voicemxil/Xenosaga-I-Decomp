/* Controller (pad) input handling */

typedef struct {
    char pad00[0x48];
    unsigned char nRepeatStart;    /* 0x48 */
    unsigned char nRepeatInterval; /* 0x49 */
    unsigned short nRepeatMask;    /* 0x4A */
    unsigned char nRepeatCount;    /* 0x4C */
    unsigned char nRepeatState;    /* 0x4D */
    char pad4E[0x68 - 0x4E];
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
