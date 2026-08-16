/* MIF2 - message-script tag emitters: expand parsed tags into the message byte stream */

typedef struct {
    int nCode;                      /* 0x00 */
    unsigned int nArg;              /* 0x04 */
} MIF2_ARG;

/* Emit a font-change tag wrapping the argument name, stopping at '/' */
char *MIF2_font(char *pDst, int nUnused, MIF2_ARG *pArg)
{
    unsigned char *pSrc;
    int c;

    pSrc = (unsigned char *)pArg->nArg;
    *pDst++ = 0x19;
    *pDst = 0x01;
    for (;;) {
        pDst++;
        c = *pSrc++;
        if (c == 0x2F) {
            break;
        }
        if (c == 0) {
            break;
        }
        *pDst = c;
    }
    *pDst++ = 0x19;
    *pDst++ = 0;
    *pDst = 0;
    return pDst;
}

/* Emit a colour tag from the packed RGB argument */
char *MIF2_color(char *pDst, int nUnused, MIF2_ARG *pArg)
{
    unsigned int nRGB;
    char *pEnd;

    nRGB = pArg->nArg;
    pEnd = pDst + 4;
    pDst[1] = nRGB >> 16;
    pDst[2] = nRGB >> 8;
    pDst[3] = nRGB;
    pDst[0] = 0x0C;
    *pEnd = 0;
    return pEnd;
}

/* Emit a label tag carrying the argument string */
char *MIF2_label(char *pDst, int nUnused, MIF2_ARG *pArg)
{
    unsigned char *pSrc;
    unsigned char *p;

    pSrc = (unsigned char *)pArg->nArg;
    p = (unsigned char *)pDst;
    *p++ = 0x2F;
    *p++ = 0x5B;
    *p++ = 0x85;
    while ((*p++ = *pSrc++) != 0) {
        ;
    }
    *p++ = 0x5D;
    *p = 0;
    return (char *)p;
}

/* Emit a clear-window tag */
char *MIF2_clear(char *pDst, int nUnused, MIF2_ARG *pArg)
{
    char *pEnd;

    pDst[0] = 0x2F;
    pDst[1] = 0x5B;
    pDst[2] = (char)0x81;
    pDst[3] = 0x5D;
    pEnd = pDst + 4;
    *pEnd = 0;
    return pEnd;
}

/* Emit a close-window tag */
char *MIF2_close(char *pDst, int nUnused, MIF2_ARG *pArg)
{
    char *pEnd;

    pDst[0] = 0x2F;
    pDst[1] = 0x5B;
    pDst[2] = (char)0x84;
    pDst[3] = 0x5D;
    pEnd = pDst + 4;
    *pEnd = 0;
    return pEnd;
}

/* Emit a wait-for-keypress tag */
char *MIF2_waitkey(char *pDst, int nUnused, MIF2_ARG *pArg)
{
    unsigned int nVal;
    char *pEnd;

    nVal = pArg->nArg;
    pDst[0] = 0x2F;
    pDst[1] = 0x5B;
    pDst[2] = (char)0x80;
    pDst[3] = nVal;
    pDst[4] = 0x5D;
    pEnd = pDst + 5;
    *pEnd = 0;
    return pEnd;
}

/* Emit a timed-wait tag */
char *MIF2_wait(char *pDst, int nUnused, MIF2_ARG *pArg)
{
    unsigned int nVal;
    char *pEnd;

    nVal = pArg->nArg;
    pDst[0] = 0x2F;
    pDst[1] = 0x5B;
    pDst[2] = (char)0x83;
    pDst[3] = nVal;
    pDst[4] = 0x5D;
    pEnd = pDst + 5;
    *pEnd = 0;
    return pEnd;
}

/* Emit a two-byte external (gaiji) character code */
char *MIF2_gaiji(char *pDst, int nUnused, MIF2_ARG *pArg)
{
    unsigned int nVal;
    char *pEnd;

    nVal = pArg->nArg;
    pEnd = pDst + 2;
    pDst[0] = (char)0xAD;
    pDst[1] = nVal + 0xA0;
    *pEnd = 0;
    return pEnd;
}

/* Emit a single raw control code */
char *MIF2_code(char *pDst, int nUnused, MIF2_ARG *pArg)
{
    char *pEnd;

    pEnd = pDst + 1;
    pDst[0] = pArg->nArg;
    *pEnd = 0;
    return pEnd;
}

/* A tag that expands to nothing: burn nCount iterations and emit no bytes */
char *MIF2_dummy(char *pDst, int nCount, MIF2_ARG *pArg)
{
    int i = nCount;

    if (i > 0) {
        do {
        } while (--i != 0);
    }
    return pDst;
}
