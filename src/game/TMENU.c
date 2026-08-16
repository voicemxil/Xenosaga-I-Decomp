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

/* NEAR-MISS, 26 diffs of 461 words, LENGTH CORRECT (was 443 on the first
 * draft).  Nothing structural is left -- all three jump tables, both
 * dispatches, the pad block, the fade and the queue drain are
 * instruction-for-instruction right.  The residue is four register /
 * scheduling clusters:
 *
 *  1. open arm (h14 == 16, no bit 2): 3 words.  Retail issues
 *     `lw a0,260(s0)` second and `sh zero,48(s0)` sixth; this build
 *     rotates the two.  A --rotate TMENU_updateDefault:N:3 fixer would
 *     close it.  Swept: all four statement orders of the arm (the
 *     block-local EWCOMP* already recovered retail's $a0 -- see below).
 *  2. cursor block: 2 words, one adjacent `sra ..,0x10` / `sra ..,0x18`
 *     pair emitted the other way round.  Pure scheduling; a
 *     --swap-adjacent site would close it.
 *  3. h14 == 1 live arm: 12 words.  Retail keeps `t->nFlags | 1` in $a0
 *     and the reload in $v0; this build has $a1/$a0, and puts the first
 *     store after the `and` instead of before it.  Caused by the
 *     volatile-store spelling below.  Swept: plain |= / &= (loses the
 *     store entirely), the volatile read alone in three positions,
 *     volatile store with and without a separate local, h14 before and
 *     after the mask, and a fresh local for the outer reload.
 *  4. fade arm (h14 == 16, live): 9 words, a pure $v0/$v1/$a1 rotation
 *     around nB and the phase counter -- retail does `addiu a1,a1,1`
 *     in place, this build lands the sum in $v0.  Swept: int vs short
 *     phase local, reading t->h30 back instead, the store before and
 *     after the u14 write, and an if/else duplication of the u14 store.
 *
 * Also swept with NO effect: the order of the local declarations (four
 * orders, byte-identical output -- gcc 2.96 allocates by use order, do
 * not spend runs on it), and a block-local EWCOMP* in the fade arm
 * (helps only in the open arm).
 *
 * The two --mtc1-nop sites in configure.py are REQUIRED for the length:
 * ee-as pads two slots after the last mtc1 of each float-constant pair
 * before the first COP1 compute and this toolchain emits only the first
 * hazard nop.
 *
 * SECOND SWEEP (30 further variants, all measured, NONE below 26):
 *
 *  - Clusters 3 and 4 are INERT to source form.  Every one of these
 *    emits BYTE-IDENTICAL code to the current spelling: both nFlags
 *    stores volatile; the volatile store with the mask folded in;
 *    `nFlags &= ~2` as its own statement; the dead pText148 read moved
 *    between the two stores; the dead read spelled
 *    `*(volatile int *)&t->pText148`; dropping the `n` local so both
 *    fade tests read t->h30; `if (t->h30 >= 8)` instead of
 *    `if (nPhase >= 8)`; and the fade arm rotation p/nPhase/u14/h30
 *    (A,C,B,D,E and the decl-then-assign forms of the open arm too).
 *    gcc normalises all of them before scheduling, so no amount of
 *    respelling moves the $v0/$a0 and $v0/$v1 rotations.
 *
 *  - Things that are strictly WORSE, do not retry: putting the volatile
 *    store's mask on a reload of t->nFlags (35); `int nPhase` (27);
 *    `short nB` (35); `(nA << 8) | (nB << 16)` operand order (33);
 *    reusing `n` as the phase variable, `n = n + 1` (39) or
 *    `n = t->h30 + 1` (33); `nPhase = n + 1` (32); the read-modify-write
 *    `t->h30 = t->h30 + 1` with `if (t->h30 >= 8)` (36 -- the compare
 *    reloads with `lh`, it does NOT CSE back to the stored register);
 *    the nB test reading `n` while the nA test reads `n` (179);
 *    open-arm orders A,B,D,E,C (32) / A,C,D,E,B (35) / A,B,D,C,E (28);
 *    fade-arm order p,u14,nPhase (83).
 *
 *  - Two spellings that LOSE the store (length blows up to ~205), i.e.
 *    the volatile must be on the STORE and on the POINTER OBJECT:
 *    `*(volatile unsigned char **)&t->pText148` (volatile on the
 *    pointee, so the read is dead and deleted) and a volatile LOAD with
 *    plain stores.  `*(unsigned char *volatile *)&t->pText148` is the
 *    only spelling of the dead read that survives, along with the
 *    equivalent `*(volatile int *)&t->pText148`.
 *
 *  - The open arm (cluster 1) IS source-order sensitive -- four of the
 *    seven orders tried move the diff count -- but no order reaches
 *    below 26, so it really is the --rotate site described above.
 *
 * CONCLUSION: clusters 1 and 2 (5 words) are fixer-flag sites and
 * clusters 3 and 4 (21 words) are pure register naming that this
 * toolchain will not produce from any C spelling found so far.  The
 * next thing to try is NOT another respelling -- it is either a
 * --swap-regs site (which would mask, so only after clusters 1/2 are
 * flagged and the residue is provably nothing but naming) or a
 * different steering device for the double nFlags store entirely.
 */
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
    short nPhase;
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
            {
                /* Block-local: a function-wide EWCOMP* local spans the
                 * other arms and the allocator then keeps this arm off
                 * retail's $a0.  Scoping it here recovers the register. */
                EWCOMP *pOpen = t->pComp[1];

                t->nFlags |= 2;
                t->h30 = 0;
                pOpen->u10.n |= 0x100;
                pOpen->u14.n = 0x808000;
            }
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
                t->nFlags = nFlags & ~0x200;
                t->b54 = t->b68;
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
            nPhase = t->h30 + 1;
            p = t->pComp[1];
            p->u14.n = (nB << 16) | (nA << 8);
            t->h30 = nPhase;
            if (nPhase >= 8) {
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

