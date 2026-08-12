/* Twin-window (TWIN) component - dialog/window widget dispose and scene-init wrappers */

typedef struct EWCOMP {
    char pad[0x1C];
} EWCOMP;

typedef struct MBUF {
    int active;
} MBUF;

typedef struct TWIN {
    char pad000[0x10];
    int nFlags;                /* 0x10 */
    char pad014[0x16 - 0x14];
    unsigned short nField16;   /* 0x16 */
    char pad018[0x20 - 0x18];
    float nField20;             /* 0x20 */
    float nField24;             /* 0x24 */
    char pad028[0x54 - 0x28];
    EWCOMP *pEwComp;             /* 0x54 */
    char pad058[0x9C - 0x58];
    MBUF *pMBuf1;                /* 0x9C */
    MBUF *pMBuf2;                /* 0xA0 */
    char pad0A4[0x186 - 0xA4];
    unsigned char nField186;    /* 0x186 */
    unsigned char nField187;    /* 0x187 */
} TWIN;

extern void EW_dispose(EWCOMP *pComp);
extern void MBUF_dispose(MBUF *buf);
extern void TWIN_init2(TWIN *t);

/* Release the twin window's debug widget and both line buffers */
void TWIN_dispose(TWIN *t)
{
    t->nFlags &= ~0x11;
    EW_dispose(t->pEwComp);
    if (t->pMBuf1 != 0) {
        MBUF_dispose(t->pMBuf1);
    }
    if (t->pMBuf2 != 0) {
        MBUF_dispose(t->pMBuf2);
    }
}

/* Tag the window as mode 3 (call-frame variant) and hand off to the shared init */
void TWIN_initCF(TWIN *t)
{
    t->nField16 = (t->nField16 & 0xFF00) | 3;
    TWIN_init2(t);
}

/* Tag the window as mode 4 (scene variant), init it, then seed its timer fields */
void TWIN_initScene(TWIN *t)
{
    t->nField186 = 0x30;
    t->nField187 = 3;
    t->nField16 = (t->nField16 & 0xFF00) | 4;
    TWIN_init2(t);
    t->nField20 = 0.0f;
    t->nField24 = 324.0f;
}
