/* Text-menu (TMENU) component - create/dispose plumbing and message-queue push */

/* 0x10 and 0x14 of an EW component are read both as a word and as a
 * halfword by TMENU_init, so they are spelled as unions of one struct
 * rather than two struct types (alias-set trap). */
typedef union EWVAL {
    int n;
    short h;
} EWVAL;

typedef struct EWCOMP {
    unsigned short h00;            /* 0x00 */
    short h02;
    short h04;
    short h06;
    int n08;                       /* 0x08 */
    short h0C;                     /* 0x0C */
    short h0E;                     /* 0x0E */
    EWVAL u10;                     /* 0x10 */
    EWVAL u14;                     /* 0x14 */
    int n18;                       /* 0x18 */
    char pad01C[0x24 - 0x1C];
    int n24;                       /* 0x24 */
} EWCOMP;

typedef struct MBUF {
    int active;
    char pad004[0x08 - 0x04];
    int *p08;                      /* 0x08 */
    int *p0C;                      /* 0x0C */
} MBUF;

typedef struct MSGQUEUE {
    int *pBuf;                     /* 0x00 */
    unsigned short field_4;
    unsigned short field_6;
    unsigned short field_8;
    unsigned short field_A;
    unsigned short field_C;
    unsigned short field_E;
    unsigned short field_10;
} MSGQUEUE;

typedef struct TMENU {
    char pad000[0x0C];
    short h0C;                     /* 0x0C: running max line width */
    short h0E;                     /* 0x0E */
    int nFlags;                    /* 0x10 */
    short h14;                     /* 0x14 */
    char pad016[0x20 - 0x16];
    float nField20;              /* 0x20 */
    float nField24;              /* 0x24 */
    char pad028[0x50 - 0x28];
    int n50;                       /* 0x50 */
    unsigned char b54;             /* 0x54 */
    unsigned char b55;             /* 0x55 */
    unsigned char nItemCount;      /* 0x56: occupied item slots */
    unsigned char b57;             /* 0x57: widest line so far */
    short h58;                     /* 0x58 */
    short h5A;                     /* 0x5A: per-line height factor */
    char pad05C[0x60 - 0x5C];
    MBUF *pMBuf1;                 /* 0x60 */
    MBUF *pMBuf2;                 /* 0x64 */
    char pad068[0xD8 - 0x68];
    MSGQUEUE queue;                /* 0xD8 */
    char pad0EC[0xFC - 0xEC];  /* MSGQUEUE is word-aligned, so it rounds up to 0xEC */
    EWCOMP *pEwComp;               /* 0xFC */
    EWCOMP *pComp[16];             /* 0x100: child component table */
    unsigned char nItemMax;        /* 0x140: item slot capacity */
    unsigned char b141;            /* 0x141 */
    unsigned char b142;            /* 0x142 */
    unsigned char b143;            /* 0x143 */
    unsigned char **ppLine144;     /* 0x144 */
    unsigned char *pText148;        /* 0x148 */
    unsigned char **ppItem;        /* 0x14C: item pointer slots */
    char pad150[0x154 - 0x150];
    unsigned char *pText154;        /* 0x154 */
    unsigned char *aSlot158[16];   /* 0x158: the item slot array itself */
    int nLine198[3];               /* 0x198: line-metrics block */
    unsigned char aText1A4[4];     /* 0x1A4: start of the text arena */
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
    if (t->pText148 == 0) {
        t->pText148 = t->pText154;
    }
}

extern int MSG_copyln(unsigned char **ppDst, unsigned char **ppSrc, int arg);

/* Store an item pointer into the given slot (or the first free slot when
 * idx is negative), measure its first line and grow the menu width */
void TMENU_setItem(TMENU *t, int idx, unsigned char *item)
{
    unsigned char buf[0x400];
    unsigned char *loc[2];
    int w;
    int v;
    int n = t->nItemMax;

    if (idx < 0) {
        int i = 0;

        if (n != 0) {
            unsigned char **arr = t->ppItem;

            if (arr[i] != 0) {
                goto step;
            }
            idx = i;
            goto skip;
step:
            i++;
            if (i >= n) {
                goto skip;
            }
            if (arr[i] != 0) {
                goto step;
            }
            idx = i;
        }
skip:
        if (idx < 0) {
            return;
        }
        t->nItemCount++;
    }
    t->ppItem[idx] = item;
    loc[1] = item;
    loc[0] = buf;
    w = MSG_copyln(&loc[0], &loc[1], 0) + 56;
    v = t->h5A * 24 + 4;
    t->h0E = v;
    if (t->h0C < w) {
        t->h0C = w;
    }
}

/* Convert a multi-line message into the item slots (PARKED 2-word
 * scheduling diff: retail materializes the MSG_convert args as li a3,-1
 * before li a1,1024; every source form tried emits them swapped --
 * needs a swap-adjacent fixer site): each line is copied
 * off the running text pointer, slotted at the current count, and the
 * menu width/height are refit to the widest line and final line count */
void TMENU_addItem(TMENU *t, unsigned char *src)
{
    unsigned char buf[0x400];
    unsigned char *cell0;
    unsigned char *cell1;
    unsigned char *dst;
    int w;
    int wmax;
    int cnt;
    int n;
    MSG_convert(buf, sizeof(buf), src, -1);
    dst = t->pText154;
    cnt = t->nItemCount;
    wmax = t->h0C;
    __asm__ volatile("");
    cell0 = dst;
    cell1 = buf;
    do {
        t->ppItem[cnt++] = dst;
        w = MSG_copyln(&cell0, &cell1, 0) + 56;
        if (wmax < w) {
            wmax = w;
        }
        dst = cell0 + 1;
        cell0 = dst;
        t->pText154 = dst;
    } while (cell1 != 0);
    cnt = 0;
    n = t->nItemMax;
    if (n != 0) {
        unsigned char **p = t->ppItem;
        int k = n;

        do {
            if (*p != 0) {
                cnt++;
            }
            p++;
            k--;
        } while (k != 0);
    }
    t->nItemCount = cnt;
    if (t->pText148 != 0) {
        int x = t->h5A * 3;
        int y = t->b141 * 3;

        t->h0E = (y + x) * 8 + 16;
    } else {
        int x = t->h5A * 24 + 4;
        int y = t->b141 * 24;

        t->h0E = x + y;
    }
    if (t->h0C < wmax) {
        t->h0C = wmax;
    }
}

extern MBUF *MBUF_create(int size, int count);
extern EWCOMP *EW_create(int id, int type);
extern int EW_addComponent(EWCOMP *parent, int idx, EWCOMP *child);

/* Build a text-menu component: allocate its message buffer, clear the item
 * and child-component tables, seed the default geometry, then create and
 * attach the nine EW children that draw frame, cursor, text and icons. */
void TMENU_init(TMENU *t)
{
    MBUF *m;
    EWCOMP *p;
    EWCOMP *e;
    int i;
    /* retail parks t+0x198 in a callee-saved register across MBUF_create,
     * so the line block is a local named before the call, not an address
     * rebuilt at each use. */
    int nMax = 16;
    int *pLine = (int *)t->aSlot158;

    t->ppItem = (unsigned char **)pLine;
    pLine = t->nLine198;
    t->n50 = 1;
    m = MBUF_create(0x80, 3);
    t->queue.pBuf = m->p0C;
    t->queue.field_C = 0x100;
    t->pMBuf1 = m;
    t->nItemMax = nMax;
    t->queue.field_8 = 0;
    t->queue.field_A = 0;
    t->queue.field_E = 0;
    t->queue.field_10 = 0;
    t->pMBuf2 = 0;
    for (i = 0; i < 16; i++) {
        t->ppItem[i] = 0;
    }
    t->ppLine144 = (unsigned char **)pLine;
    pLine += 3;
    t->b142 = 0;
    t->b143 = 0;
    t->b141 = 0;
    t->pText148 = 0;
    {
        int *ps = t->pMBuf1->p08;
        int *pd = (int *)t->ppLine144;

        for (i = 0; i < 3; i++) {
            pd[i] = ps[i];
        }
    }
    t->pText154 = (unsigned char *)pLine;
    t->nFlags = 0x15;
    t->h14 = 1;
    t->nField20 = 120.0f;
    t->nField24 = 100.0f;
    t->h0E = 32;
    t->h5A = 4;
    t->nItemCount = 0;
    t->h58 = 0;
    t->b55 = 0;
    t->b54 = 0;
    t->h0C = 0;
    for (i = 0; i < 16; i++) {
        t->pComp[i] = 0;
    }
    p = EW_create(-1, 4);
    p->u14.n = (int)t->pComp;
    p->u10.h = 16;
    p->h00 |= 0x4000;
    t->pEwComp = p;
    e = EW_create(-1, 9);
    e->h0C = 16;
    e->h0E = 16;
    EW_addComponent(p, 0, e);
    e = EW_create(-1, 10);
    e->h0C = 16;
    e->h0E = 16;
    EW_addComponent(p, 1, e);
    {
        EWCOMP *q = t->pComp[1];

        q->u14.n = 0x808000;
        q->u10.n |= 0x100;
    }
    e = EW_create(-1, 7);
    e->n18 = 0;
    e->n24 = (int)t->ppItem;
    e->u14.h = -1;
    EW_addComponent(p, 2, e);
    e = EW_create(-1, 7);
    e->n18 = 0;
    e->n24 = (int)t->ppLine144;
    e->u14.h = -1;
    EW_addComponent(p, 3, e);
    e = EW_create(-1, 9);
    e->h0C = 512;
    e->h0E = 448;
    e->h04 = 0;
    e->h06 = 0;
    e->n08 = 0;
    EW_addComponent(p, 4, e);
    e->h00 |= 0x4000;
    e = EW_create(-1, 1);
    EW_addComponent(p, 5, e);
    e = EW_create(-1, 5);
    e->h0C = 16;
    e->h0E = 16;
    EW_addComponent(p, 6, e);
    e = EW_create(-1, 5);
    e->h0C = 16;
    e->h0E = 16;
    EW_addComponent(p, 7, e);
    e = EW_create(-1, 5);
    e->h0C = 16;
    e->h0E = 16;
    EW_addComponent(p, 8, e);
    t->nFlags = 0x55;
    t->h14 = 1;
}
