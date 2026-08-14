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
    char pad000[0x0C];
    short h0C;                     /* 0x0C: running max line width */
    short h0E;                     /* 0x0E */
    int nFlags;                    /* 0x10 */
    char pad014[0x20 - 0x14];
    float nField20;              /* 0x20 */
    float nField24;              /* 0x24 */
    char pad028[0x56 - 0x28];
    unsigned char nItemCount;      /* 0x56: occupied item slots */
    char pad057[0x5A - 0x57];
    short h5A;                     /* 0x5A: per-line height factor */
    char pad05C[0x60 - 0x5C];
    MBUF *pMBuf1;                 /* 0x60 */
    MBUF *pMBuf2;                 /* 0x64 */
    char pad068[0xD8 - 0x68];
    MSGQUEUE queue;                /* 0xD8 */
    char pad0EA[0xFC - 0xEA];
    EWCOMP *pEwComp;               /* 0xFC */
    char pad100[0x140 - 0x100];
    unsigned char nItemMax;        /* 0x140: item slot capacity */
    unsigned char b141;            /* 0x141 */
    char pad142[0x144 - 0x142];
    unsigned char **ppLine144;     /* 0x144 */
    unsigned char *pText148;        /* 0x148 */
    unsigned char **ppItem;        /* 0x14C: item pointer slots */
    char pad150[0x154 - 0x150];
    unsigned char *pText154;        /* 0x154 */
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
