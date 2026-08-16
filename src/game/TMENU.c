/* Text-menu (TMENU) component - create/dispose plumbing and message-queue push */

/* 0x10 and 0x14 of an EW component are read both as a word and as a
 * halfword by TMENU_init, so they are spelled as unions of one struct
 * rather than two struct types (alias-set trap). */
typedef union EWVAL {
    int n;
    short h;
    struct { short h0; short h2; } w;
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
    EWVAL u14;                     /* 0x14 (and 0x16 through .w.h2) */
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
    short field_8;
    short field_A;
    unsigned short field_C;
    unsigned short field_E;
    short field_10;                /* 0x10 */
    short field_12;                /* 0x12 */
    char pad014[0x1C - 0x14];
    unsigned short field_1C;       /* 0x1C */
} MSGQUEUE;

typedef struct TMENU {
    char pad000[0x0C];
    short h0C;                     /* 0x0C: running max line width */
    unsigned short h0E;            /* 0x0E: addQuery2 reads it with lhu */
    int nFlags;                    /* 0x10 */
    unsigned short h14;            /* 0x14: drawDefault reads it with lhu */
    char pad016[0x20 - 0x16];
    float nField20;              /* 0x20 */
    float nField24;              /* 0x24 */
    char pad028[0x30 - 0x28];
    short h30;                     /* 0x30: per-mode phase counter */
    char pad032[0x50 - 0x32];
    int n50;                       /* 0x50 */
    signed char b54;               /* 0x54: drawDefault reads it with lb */
    signed char b55;               /* 0x55 */
    unsigned char nItemCount;      /* 0x56: occupied item slots */
    unsigned char b57;             /* 0x57: widest line so far */
    short h58;                     /* 0x58 */
    short h5A;                     /* 0x5A: per-line height factor */
    char pad05C[0x60 - 0x5C];
    MBUF *pMBuf1;                 /* 0x60 */
    MBUF *pMBuf2;                 /* 0x64 */
    unsigned char b68;             /* 0x68 */
    char pad069[0xD8 - 0x69];
    MSGQUEUE queue;                /* 0xD8 */
    int nTexF8;                    /* 0xF8: cursor sprite texture handle */
    EWCOMP *pEwComp;               /* 0xFC */
    EWCOMP *pComp[16];             /* 0x100: child component table */
    unsigned char nItemMax;        /* 0x140: item slot capacity */
    unsigned char b141;            /* 0x141 */
    signed char b142;              /* 0x142 */
    signed char b143;              /* 0x143 */
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

/* Convert an array of already-converted message lines into the menu's text
 * arena: tag markup (/[..]) is stripped, double-byte characters are copied
 * as pairs, newlines close a line and grow the widest-line counter.  An
 * all-zero three-byte header ends the query block; the remaining entries
 * are handed to TMENU_addItem.  Finally the line-pointer table is laid out
 * on a 4-byte boundary after the text and the panel is refit.
 *
 * NEAR-MISS, 2 words long (202 orig vs 204 built), everything else in
 * shape.  The surplus is the "i >= nCount" loop exit: retail spells it
 * `beqzl v0,tail` with `lw t2,340(s1)` in the annulled delay slot,
 * while gcc puts the load in a block of its own reached by a `b` (+2).
 * Swept without moving it: break vs goto done for either or both exits;
 * a redundant else arm around the empty-line block (the LEVERS layout
 * lever); hoisting `pTop = t->pText154` above the i++ (208, worse);
 * dropping the pTop local so the tail re-reads the member (204, the
 * same surplus just moves to the empty-line -> tail edge instead);
 * and every loop form for the tokenizer -- while, for(;;)+break,
 * while(1)+break, do/while(1), and split load/test all give 204, while
 * the explicit goto form gives 200 because it loses loop-invariant
 * motion of the 10/47/91 constants and the two .p2align pad nops.
 * The movn/branch-likely/store shapes elsewhere already match. */
void TMENU_addQuery2(TMENU *t, unsigned char **ppSrc, int nCount)
{
    unsigned char *pOut;
    unsigned char *pTop;
    unsigned char *pSrc;
    int nFirst;
    int nLines;
    int i;
    int j;
    int c;
    int v;
    int w;

    pTop = t->pText154;
    if (t->pText148 == 0) {
        t->pText148 = pTop;
    }
    pOut = pTop;
    nFirst = -1;
    nLines = 0;
    if (nCount > 0) {
        i = 0;
        pSrc = ppSrc[0];
        c = pSrc[0];
        if (((c << 16) | (pSrc[1] << 8) | pSrc[2]) == 0) {
            pOut[0] = 0;
            pOut[1] = 0;
            pOut += 2;
            t->pText154 = pOut;
            pTop = pOut;
            nFirst = 1;
        } else {
            for (;;) {
                while ((c = *pSrc++) != 0) {
                    if (c == 47) {
                        if (*pSrc == 91) {
                            do {
                                v = *pSrc++;
                                if (v >= 0xA1) {
                                    *pOut++ = v;
                                    *pOut++ = *pSrc++;
                                }
                            } while (v != 93);
                        } else {
                            *pOut++ = c;
                        }
                    } else if (c == 10) {
                        w = pOut - t->pText154;
                        if (t->b57 < w) {
                            t->b57 = w;
                        }
                        *pOut++ = c;
                        t->pText154 = pOut;
                        nLines++;
                    } else {
                        *pOut++ = c;
                        if (c >= 0xA0) {
                            *pOut++ = *pSrc++;
                        }
                    }
                }
                pTop = t->pText154;
                w = pOut - pTop;
                if (t->b57 < w) {
                    t->b57 = w;
                }
                v = pOut[-1];
                if (i == nCount - 1) {
                    if (v == 10) {
                        pOut[-1] = 0;
                        *pOut++ = 0;
                    } else {
                        *pOut++ = 0;
                        *pOut = 0;
                        pOut++;
                        t->pText154 = pOut;
                        nLines++;
                    }
                } else if (v != 10) {
                    *pOut++ = 10;
                    t->pText154 = pOut;
                    nLines++;
                }
                i++;
                if (i >= nCount) {
                    pTop = t->pText154;
                    goto done;
                }
                pSrc = ppSrc[i];
                c = pSrc[0];
                if (((c << 16) | (pSrc[1] << 8) | pSrc[2]) != 0) {
                    continue;
                }
                *pOut++ = 0;
                *pOut++ = 0;
                t->pText154 = pOut;
                nFirst = i + 1;
                pTop = pOut;
                goto done;
            }
        }
    }
done:
    {
        int n = t->b57 * 10;

        v = n + 56;
        if (t->pText148 != 0) {
            v = n + 136;
        }
        if (t->h0C < v) {
            t->h0C = v;
        }
    }
    {
        int *pLine = (int *)((((unsigned int)pTop + 3) >> 2) * 4);

        t->ppLine144 = (unsigned char **)pLine;
        t->pText154 = (unsigned char *)(pLine + nLines);
        for (j = 0; j < nLines; j++) {
            t->ppLine144[j] = t->pText154;
            *t->pText154 = 0;
            t->pText154 = t->pText154 + t->b57 + 32;
        }
    }
    t->b141 = nLines;
    t->b142 = 0;
    t->b143 = 0;
    t->h0E = t->h0E + nLines * 24 + 24;
    if (nFirst > 0) {
        for (i = nFirst; i < nCount; i++) {
            TMENU_addItem(t, ppSrc[i]);
        }
    }
}

extern void EW_sprtSetCursorUV(EWCOMP *pComp, int nIdx, int nTex);

/* Lay out the menu's nine EW children for this frame: frame, shadow,
 * backdrop, title text, item text, the two scroll arrows and the cursor.
 * Positions come from the panel's float origin, the item count and the
 * current selection; the arrows and cursor are shown or hidden by
 * clearing bit 0x4000 of the component's flag word.
 *
 * NEAR-MISS, 43 diffs of 223 words, LENGTH already correct.  The whole
 * residue is one register-allocation channel: retail puts nField20 in
 * $f1/$s3 and nField24 in $f0/$s2, and this build has them the other way
 * round, which then renames $v0/$v1 through every component block.
 * Swept: both orders of the x/y assignments, both orders of the two
 * (int)(field + 16.0f) stores, swapping the x/y declaration order, and
 * dropping the x/y locals so CSE invents the temporaries (243 words --
 * CSE does not fold the repeated (int)field + 16).  A full
 * adjacent-swap hill climb over every p-> store statement in the
 * function took it 65 -> 43 and then stalled; the four store swaps it
 * found are already applied here.  What is left needs whatever decides
 * which of two equally-used float pseudos is created first. */
void TMENU_drawDefault(TMENU *t)
{
    EWCOMP *p;
    int x;
    int y;
    int nOff;
    int nCur;
    int n;

    if (t->nFlags & 0x10) {
        /* 0xFFFF, not -1: retail materialises the addend with li+addu. */
        if ((unsigned short)(t->h14 + 0xFFFF) < 2) {
            return;
        }
        y = (int)t->nField24 + 16;
        x = (int)t->nField20 + 16;
        p = t->pComp[1];
        p->n08 = 0xFFFFF1;
        p->h06 = (int)(t->nField24 + 16.0f);
        p->h04 = (int)(t->nField20 + 16.0f);
        p->h0C = t->h0C;
        p->h0E = t->h0E;
        p->h00 |= 0x4000;
        p = t->pComp[5];
        p->h04 = x;
        p->h06 = y;
        p->n08 = 0xFFFFF0;
        p->h00 |= 0x4000;
        p->h0C = t->h0C;
        p->h0E = t->h0E;
        if (t->b141 != 0) {
            n = t->b141;
            nOff = n * 24 + 12;
        } else {
            nOff = 0;
        }
        p = t->pComp[0];
        p->h04 = x;
        p->h06 = y;
        p->n08 = 0xFFFFF1;
        p->h0C = t->h0C;
        p->h0E = t->h0E;
        p->h00 |= 0x4000;
        p = t->pComp[3];
        p->n24 = (int)t->ppLine144;
        p->h04 = x;
        p->h06 = y;
        p->n08 = 0xFFFFFF;
        p->u14.h = t->b141;
        p->u14.w.h2 = 0;
        p->h00 |= 0x4000;
        p = t->pComp[2];
        p->h04 = x + 20;
        p->h06 = y + nOff;
        p->n08 = 0xFFFFFF;
        p->u14.w.h2 = t->h58;
        p->n24 = (int)t->ppItem;
        p->h00 |= 0x4000;
        p->u14.h = t->h5A + 1;
        p = t->pComp[6];
        p->h04 = t->h0C + x - 16;
        p->n08 = 0xFFFFFF;
        p->h06 = y;
        EW_sprtSetCursorUV(p, 1, t->nTexF8);
        if (t->h58 > 0) {
            p->h00 |= 0x4000;
        } else {
            p->h00 &= 0xBFFF;
        }
        p = t->pComp[7];
        p->n08 = 0xFFFFFF;
        p->h04 = t->h0C + x - 16;
        p->h06 = t->h0E + y - 16;
        EW_sprtSetCursorUV(p, 2, t->nTexF8);
        if (t->h58 + t->h5A < t->nItemCount) {
            p->h00 |= 0x4000;
        } else {
            p->h00 &= 0xBFFF;
        }
        nCur = 12;
        if (t->b54 >= 0) {
            nCur = (t->b54 - t->h58) * 24 + 12;
        }
        if (t->b141 != 0) {
            n = t->b141;
            nOff = n * 24 + 12;
        } else {
            nOff = 0;
        }
        p = t->pComp[8];
        p->n08 = 0xFFFFFF;
        p->h04 = (int)(t->nField20 + 20.0f + 0.0f + 0.0f);
        p->h06 = (int)((float)nCur + ((t->nField24 + 16.0f) - 6.0f + (float)nOff));
        EW_sprtSetCursorUV(p, 0, t->nTexF8);
        p->h00 |= 0x4000;
    }
}


typedef struct XGLPADDATA {
    char pad000[0x32];
    unsigned short nTrig;          /* 0x32 */
    unsigned short nRepeat;        /* 0x34 */
    char pad036[0x68 - 0x36];
} XGLPADDATA;

extern XGLPADDATA PadData[2];

extern void xglSoundEffectNormalID(int nId, int nArg);
extern void MSG_queueGetInfo(MSGQUEUE *q, short *pInfo, int nArg);
extern int MSG_queuePop(MSGQUEUE *q, unsigned char *pDst, int nArg);
extern void MSG_queueReset(MSGQUEUE *q, int nArg);
extern void TW_setPos(TMENU *t);

/* Per-frame state machine for a default text menu.  h14 is the mode and
 * h30 the phase counter; the mode is dispatched twice, once for the
 * "not yet running" half (bit 2 of nFlags clear) and once for the live
 * half.  The live half opens/closes the panel with a two-channel fade,
 * drains the message queue a character at a time and moves the cursor
 * from the pad. */
void TMENU_updateDefault(TMENU *t)
{
    EWCOMP *p;
    short info[4];
    int nFlags;
    int nA;
    int nB;
    int nCount;
    int c;
    short n;
    short h58;
    signed char cur;
    signed char nNew;

    nFlags = t->nFlags;
    if (!(nFlags & 0x10)) {
        return;
    }
    if (t->nItemCount < t->h5A) {
        t->h5A = t->nItemCount;
    }
    t->nTexF8 = t->nTexF8 + 1;
    if (!(nFlags & 2)) {
        switch (t->h14) {
        case 16:
            t->nFlags |= 2;
            p = t->pComp[1];
            t->h30 = 0;
            p->u10.n |= 0x100;
            p->u14.n = 0x808000;
            return;
        case 1:
            t->nFlags |= 2;
            t->h30 = 0;
            t->h0E = t->h5A * 24 + 4;
            MSG_queueGetInfo(&t->queue, info, 0);
            if (info[1] > 0) {
                t->h0E = t->h0E + (info[1] * 24 + 12);
            }
            if (t->h0C < info[2] + 36) {
                t->h0C = info[2] + 36;
            }
            t->b141 = info[1];
            return;
        case 14:
        case 15:
            t->h30 = 0;
            t->nFlags |= 2;
            /* fall through */
        case 13:
            t->nFlags |= 2;
            return;
        case 2:
            t->h30 = 10;
            t->nFlags |= 2;
            t->pEwComp->h00 &= 0xBFFF;
            return;
        }
    } else if (nFlags & 4) {
        switch (t->h14) {
        case 15:
            t->nFlags &= ~2;
            if (t->n50 & 1) {
                t->h14 = 2;
            } else {
                t->h14 = 0;
            }
            return;
        case 14:
            if (PadData[0].nTrig & 0x40) {
                xglSoundEffectNormalID(2, 0);
                t->nFlags &= ~2;
                t->h14 = 15;
                t->b55 = -1;
                return;
            }
            cur = t->b54;
            if (PadData[0].nTrig & 0x20) {
                xglSoundEffectNormalID(1, 0);
                t->nFlags &= ~2;
                t->h14 = 15;
                return;
            }
            t->h30 = (t->h30 + 1) & 0xFF;
            if ((PadData[0].nRepeat & 0x1000) && cur > 0 && t->b55 == 0) {
                xglSoundEffectNormalID(3, 0);
                t->b54 = t->b54 - 1;
                t->h30 = 0;
            }
            if (PadData[0].nRepeat & 0x4000) {
                if (t->b54 < t->nItemCount - 1) {
                    xglSoundEffectNormalID(3, 0);
                    t->b54 = t->b54 + 1;
                    t->h30 = 0;
                }
            }
            if (t->b54 == cur) {
                return;
            }
            if (t->b54 < 0) {
                t->b54 = 0;
            }
            if (t->b54 >= t->h58 + t->h5A) {
                t->h58 = t->h58 + 1;
            }
            if (t->b54 >= t->nItemCount) {
                t->b54 = t->nItemCount - 1;
            }
            if (t->b54 < t->h58) {
                t->h58 = t->h58 - 1;
            }
            if (t->h58 < 0) {
                t->h58 = 0;
            }
            if (t->h58 >= t->nItemCount - t->h5A) {
                t->h58 = t->nItemCount - t->h5A;
            }
            return;
        case 13:
            t->h30 = t->h30 - 1;
            if (t->h30 < 0) {
                t->nFlags &= ~2;
                t->h14 = 16;
            }
            return;
        case 2:
            t->h30 = t->h30 - 1;
            if (t->h30 < 0) {
                TMENU_dispose(t);
            }
            return;
        case 1:
            t->h30 = t->h30 + 1;
            TW_setPos(t);
            if (t->h30 >= 11) {
                /* Retail performs BOTH nFlags stores and keeps a dead
                 * read of pText148 in the block; without the volatile
                 * read the (x | 1) & ~2 pair folds into a single store. */
                *(unsigned char *volatile *)&t->pText148;
                nFlags = t->nFlags | 1;
                *(volatile int *)&t->nFlags = nFlags;
                t->h14 = 16;
                t->nFlags = nFlags & ~2;
            }
            nFlags = t->nFlags;
            if (nFlags & 0x200) {
                t->b54 = t->b68;
                t->nFlags = nFlags & ~0x200;
            }
            return;
        case 16:
            n = t->h30;
            if (n < 8) {
                nA = (int)(128.0f - (float)n * 0.125f * 128.0f);
            } else {
                nA = 1;
            }
            if (t->h30 < 4) {
                nB = (int)(128.0f - (float)t->h30 * 0.25f * 128.0f);
            } else {
                nB = 1;
            }
            t->h30 = t->h30 + 1;
            c = t->h30;
            p = t->pComp[1];
            p->u14.n = (nB << 16) | (nA << 8);
            if (c >= 8) {
                t->pComp[2]->n18 = 128;
                t->pComp[3]->n18 = 128;
            }
            p->n08 = 0xFFFFF1;
            p->h04 = (int)(t->nField20 + 16.0f);
            p->h06 = (int)(t->nField24 + 16.0f);
            p->h0C = t->h0C;
            p->h0E = t->h0E;
            if (t->queue.field_A > 0) {
                c = MSG_queuePop(&t->queue, t->ppLine144[t->b142] + t->b143, 1);
                t->b143 = t->b143 + c;
                c = t->queue.field_12;
                if (c >= 128) {
                    switch (c - 128) {
                    case 0:
                        break;
                    case 1:
                        break;
                    case 2:
                        break;
                    case 3:
                        t->h14 = 13;
                        t->h30 = t->queue.field_1C;
                        break;
                    case 4:
                        break;
                    case 5:
                        break;
                    }
                } else if (t->queue.field_10 >= t->h0C || c == 10) {
                    t->b142 = t->b142 + 1;
                    t->b143 = 0;
                    MSG_queueReset(&t->queue, 10);
                }
            }
            if (t->queue.field_8 >= t->queue.field_A) {
                if (t->h30 >= 9) {
                    MSG_queueReset(&t->queue, 0);
                    t->nFlags &= ~2;
                    t->h14 = 14;
                    t->pComp[1]->u14.n = 0x10100;
                }
            }
            return;
        }
        t->h30 = t->h30 + 1;
    }
}

/* TMENU_updateDefault @ 0x0025F410, 0x734 bytes (461 instructions) -- NOT
 * WRITTEN.  Notes for whoever picks it up:
 *
 * m2c refuses it ("two delay slot instructions in a row", the
 * beql/bgezl pair at +0x318), so it has to be read from disasm.py.
 *
 * It is a state machine with THREE computed-goto dispatches, all keyed
 * on 16-entry tables in .rodata.  Dumped from the ROM image:
 *
 *   switch (t->h14 - 1)          table 0x004C1C80, targets (fn+):
 *     0 ->0x0b8  1 ->0x160  2..11 ->end  12 ->0x150  13,14 ->0x140
 *     15 ->0x088
 *   switch (t->h30)              table 0x004C1CC0, targets (fn+):
 *     0 ->0x438  1 ->0x410  2..11 ->0x710  12 ->0x3e0  13 ->0x1f0
 *     14 ->0x1c0  15 ->0x4a8
 *   switch (t->queue.h0A - 128)  table 0x004C1D00, six live entries
 *
 * So h14 is a top-level mode (2..11 idle) and h30 (a new short at
 * offset 0x30, cleared by several arms) is the per-mode phase counter.
 * It calls xglSoundEffectNormalID, MSG_queueGetInfo, MSG_queuePop,
 * MSG_queueReset, TW_setPos and TMENU_dispose, and it reads the pad
 * through the global at 0x00490D90 (+0x32 and +0x34 bit tests).
 * Offsets touched that this file does not name yet: 0x30 (short),
 * 0x68 (byte), 0xE0/0xE2/0xE8/0xEA/0xF4 inside the queue block. */
