/* Text-menu (TMENU) component - create/dispose plumbing and message-queue push */

typedef struct EWCOMP {
    char pad[0x1C];
} EWCOMP;

typedef struct MBUF {
    int active;
} MBUF;

typedef struct MSGQUEUE {
    unsigned short field_0;
    unsigned short field_2;
    unsigned short field_4;
    unsigned short field_6;
    unsigned short field_8;
    unsigned short field_A;
    unsigned short field_C;
    unsigned short field_E;
    unsigned short field_10;
} MSGQUEUE;

typedef struct TMENU {
    char pad000[0x10];
    int nFlags;                    /* 0x10 */
    char pad014[0x20 - 0x14];
    float nField20;              /* 0x20 */
    float nField24;              /* 0x24 */
    char pad028[0x60 - 0x28];
    MBUF *pMBuf1;                 /* 0x60 */
    MBUF *pMBuf2;                 /* 0x64 */
    char pad068[0xD8 - 0x68];
    MSGQUEUE queue;                /* 0xD8 */
    char pad0EA[0xFC - 0xEA];
    EWCOMP *pEwComp;               /* 0xFC */
    char pad100[0x148 - 0x100];
    int nField148;                  /* 0x148 */
    char pad14C[0x154 - 0x14C];
    int nField154;                  /* 0x154 */
} TMENU;

extern void EW_dispose(EWCOMP *pComp);
extern void MBUF_dispose(MBUF *buf);
extern TMENU *TWSYS_createComponent(int type, int arg);
extern void TMENU_init(TMENU *t);
extern int MSG_convert(unsigned char *dest, int size, unsigned char *src, int arg);
extern int MSG_queuePush(MSGQUEUE *q, unsigned char *buf, int size, int arg);

/* Allocate a text-menu component of the given type and seed its default scroll speeds */
TMENU *TMENU_create(int type)
{
    TMENU *t;

    t = TWSYS_createComponent(type, 1);
    if (t == 0) {
        return 0;
    }
    TMENU_init(t);
    t->nField20 = 48.0f;
    t->nField24 = 64.0f;
    return t;
}

/* Release the menu's debug widget and both line buffers */
void TMENU_dispose(TMENU *t)
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

/* Convert a message and push it onto the menu's print queue, refreshing the read index */
void TMENU_addQuery(TMENU *t, unsigned char *src)
{
    unsigned char buf[0x400];

    MSG_convert(buf, 0x400, src, -1);
    MSG_queuePush(&t->queue, buf, 0x400, -1);
    if (t->nField148 == 0) {
        t->nField148 = t->nField154;
    }
}
