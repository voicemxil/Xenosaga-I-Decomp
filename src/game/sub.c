#include "matching.h"

/* Small menu and Ether-tree subhelpers. */

int subSeisanEtherCheck(void)
{
    return 0;
}

int subSeisanItemCheck(void)
{
    return 0x80;
}

void subTreeLineDraw_type_0(void)
{
}

typedef struct {
    int text;
    int box;
    unsigned char field_08;
} SUB_LIST;

extern int *MenuTextGet(int id);
extern int MenuBoxChk(int id);
extern int MenuBoxMoneyGet(int id, int type);
extern char *SeisanWork;
extern char *SeisanResult;

void subListMake00(SUB_LIST *list_, int id_)
{
    PIN(SUB_LIST *list, "$16") = list_;
    PIN(int id, "$17") = id_;

    list->text = *MenuTextGet(id);
    list->box = MenuBoxChk(id);
    list->field_08 = 0;
}

void subListMake01(SUB_LIST *list_, int id_)
{
    PIN(SUB_LIST *list, "$16") = list_;
    PIN(int id, "$17") = id_;

    list->text = *MenuTextGet(id);
    list->box = MenuBoxMoneyGet(id, 0);
    list->field_08 = 0;
}

/* The child pointer is at data+8+index*4 (a 4-byte-stride pointer array).
   The last two diffs were a register tie-break: the original does the
   sll/addu in place in $v0 and puts the index on the LEFT of the addu.
   Both fall out of doing the address arithmetic in int, in steps, on one
   reused variable -- writing it as pointer arithmetic makes gcc keep the
   index in a fresh register and canonicalise the addu with the pointer
   first. Do not reshape this back into `(char *)data + index * 4 + 8`, and
   do not put a barrier on the index: this function is self-tail-recursive
   and a barrier defeats gcc's tail-call-to-loop transform entirely. */
void sub2JoutoYGet(void *data, float *result)
{
    int index = *(unsigned short *)((char *)data + 8);

    if (index != 0) {
        index <<= 2;
        index += (int)data;
        sub2JoutoYGet(*(void **)(index + 8), result);
    } else {
        *result = *(float *)((char *)data + 0x24);
    }
}

/* Any unspent skill slot blocks the "seisan" (settlement) confirm.

   The loop bound is the expression, not a local computed above the loop:
   that is what makes gcc load the count into its own register for the
   zero guard and copy it into the bound register (`move a2,a0`), freeing
   the first register for the walking pointer. Hoisting it into a local --
   with or without pinning the local to $a2 -- loads straight into the
   bound register, loses the copy and floats the first lw a slot earlier. */
int subSeisanHissatuCheck00(void)
{
    int i;

    for (i = 0; i < (unsigned char)SeisanWork[6]; i++) {
        if (*(int *)(SeisanResult + 0x38 + i * 0x3C) != 0) {
            return 0x50;
        }
    }
    return 0;
}

/* Same scan, rejecting slots whose value has reached 5 (see above for why
   the bound is spelled as the expression). */
int subSeisanHissatuCheck01(void)
{
    int i;

    for (i = 0; i < (unsigned char)SeisanWork[6]; i++) {
        if (*(int *)(SeisanResult + 0x38 + i * 0x3C) >= 5) {
            return 0x58;
        }
    }
    return 0;
}

/* --- Ether tree node graph --- */

/* 8-byte aligned so a whole-node position copy uses ld/sd rather than the
   unaligned ldl/ldr/sdl/sdr sequence. */
struct EVECS {
    float x;
    float y;
    float z;
    float w;
};
typedef struct EVECS __attribute__((aligned(8))) EVEC;
/* Same struct tag, natural 4-byte alignment: a copy through EVEC4 uses the
   unaligned ldl/ldr idiom, and because it is a VARIANT of the same record
   type rather than a second identical struct, it shares gcc's alias set --
   a distinct type here lets gcc hoist the read above the store. */
typedef struct EVECS EVEC4;

typedef struct ETNODE {
    unsigned short nId;          /* 0x00 */
    unsigned short nUnk02;       /* 0x02 */
    struct ETNODE *pParent;      /* 0x04 */
    unsigned short nChildren;    /* 0x08 */
    char pad0A[2];
    struct ETNODE *pChild[3];    /* 0x0C */
    unsigned char bFlags;        /* 0x18 */
    char pad19[7];
    EVEC pos;                    /* 0x20 */
    char pad30[0x40];
} ETNODE;

extern ETNODE *EtherTreeObject;
extern unsigned char *EtherTreeFirstDataGet(void);
extern ETNODE *EtherTreeObjectWorkGet(void);
extern unsigned char *MenuEtherDataGet(int nId);

/* Place every branch node halfway between its first child and whichever of
   the other two children its node type selects.

   PARKED at 13 diffs -- pure register rotation, right instructions in the
   right order. Original: a=$v0, b=$v1, p=$a0; we get a=$a0, b=$v0, p=$v1,
   and the prologue schedules `lw p,gp` before the 0.5f `lui/mtc1` instead
   of after. Swept: swapping the a/b declaration order, scoping a and b
   inside the loop body, both leave the 13 diffs untouched -- the switch
   shape (beql pair with the pChild[0] load duplicated into both likely
   delay slots) is already exactly right, so this is a global_alloc
   priority tie-break, permuter territory. */
void subPosSet3(void)
{
    ETNODE *p;
    ETNODE *a;
    ETNODE *b;
    int i;

    p = EtherTreeObject;
    for (i = 0; i < 80; i++, p++) {
        switch (p->nChildren) {
        case 2:
            a = p->pChild[0];
            b = p->pChild[1];
            break;
        case 3:
            a = p->pChild[0];
            b = p->pChild[2];
            break;
        default:
            continue;
        }
        p->pos.y = (b->pos.y - a->pos.y) * 0.5f + a->pos.y;
    }
}

/* Shift a node and its whole subtree by pOfs, in place. */
void subPosSet2(ETNODE *node, EVEC *pOfs)
{
    EVEC pos;
    int i;

    if (node != 0) {
        pos = node->pos;
        if (pOfs != 0) {
            pos.x += pOfs->x;
            pos.y += pOfs->y;
        }
        for (i = 0; i < node->nChildren; i++) {
            subPosSet2(node->pChild[i], &pos);
        }
        node->pos = pos;
    }
}

/* Build the child list of one tree node from the ether data table, then
   recurse into each child. */
void sub2ParentChildSet(ETNODE *node)
{
    int i;
    unsigned char nId;
    ETNODE *p;

    node->nChildren = 0;
    node->bFlags |= 4;
    for (i = 0; i < 3; i++) {
        nId = MenuEtherDataGet(node->nId)[i];
        if (nId != 0) {
            p = EtherTreeObjectWorkGet();
            p->nId = nId;
            node->pChild[i] = p;
            p->pParent = node;
            node->nChildren = node->nChildren + 1;
            sub2ParentChildSet(node->pChild[i]);
        } else {
            node->pChild[i] = 0;
        }
    }
}

/* Build the root node's child list from the first-data table and recurse. */
void subParentChildSet(void)
{
    unsigned char *pData;
    ETNODE *root;
    ETNODE *p;
    unsigned char nId;
    int i;

    pData = EtherTreeFirstDataGet();
    root = EtherTreeObjectWorkGet();
    root->nChildren = 0;
    root->bFlags = 0x20;
    for (i = 0; i < 3; i++) {
        nId = pData[i];
        if (nId != 0) {
            p = EtherTreeObjectWorkGet();
            p->nId = nId;
            root->pChild[i] = p;
            root->nChildren = root->nChildren + 1;
            sub2ParentChildSet(p);
        } else {
            root->pChild[i] = 0;
        }
    }
}

/* One line segment of the ether-tree wireframe: endpoints are the node's
   own position and its first child's. */
typedef struct {
    EVEC posA;          /* 0x00 */
    char pad10[0x10];
    EVEC posB;          /* 0x20 */
    char pad30[0x10];
} ETSEG;

typedef struct {
    ETNODE *pNode;      /* 0x00 */
    char pad04[0x0C];
    ETSEG seg[6];       /* 0x10, stride 0x40 */
} ETLINE;

extern void subTreeLineDraw(ETLINE *line, int *pType);

/* Type 1: node -> first child. Learned/unlearned picks the line style. */
void subTreeLineDraw_type_1(ETLINE *line)
{
    ETNODE *node = line->pNode;
    ETNODE *child = node->pChild[0];
    int nType;

    line->seg[0].posA = node->pos;
    line->seg[0].posB = child->pos;
    nType = (unsigned char)(child->bFlags & 1) ? 2 : 1;
    subTreeLineDraw(line, &nType);
}

/* PARKED, subTreeLineDraw_type_2 at 56 diffs / 80 words vs 81, and
   subTreeLineDraw_type_3 at 87 / 92 vs 90. The bodies are semantically
   right and, apart from one copy chain, instruction-for-instruction
   right. Two things stand between them and a match:

     * The original copies `line` into $a2 at entry and back into $a0 for
       the call, keeping the CHILD pointer in $a0 inside the loop; we
       keep `line` in $a0 throughout. Pure global_alloc tie-break.
     * The original rematerialises `node->pChild` (`addiu t0,t1,12`) for
       the geometry loop; we keep the loop-1 giv's initial value alive in
       a spare register and copy it (one extra word). Not reachable by
       spelling: walked pointer, indexed access, separate loop counters
       and every mix of the two were tried, and the CSE survives all of
       them -- it is a consequence of the register assignment above.

   The elbow geometry itself IS solved and should not be re-swept: the
   store order comes from writing posB before posA and chaining the
   second copy off the first (`posA = posB`, so cse forwards the stored
   value instead of reloading node->pos through the same alias set), and
   the giv anchor (line+176) comes from making the seg[i*2+2] pair
   posA-first while the seg[i*2+1] pair stays posB-first. Swapping either
   pair moves the anchor by 32 or mirrors four stores. Block-local ETSEG
   pointers, EVEC pointers to the members and permuter run #1 (best 8080
   vs base 8780) all failed to close it -- permuter territory.

   Type 2: a two-child node. One elbow per child -- a vertical stub out of
   the node (segment 0, pushed 28.5 to the right), then for each child a
   riser to the child's height and a run out to the child. The style array
   is one entry for the stub plus two per child. */
void subTreeLineDraw_type_2(ETLINE *line)
{
    int nType[7];
    ETNODE *node;
    ETNODE *c;
    ETNODE **pc;
    int i;
    int j;

    node = line->pNode;
    /* Nested, not `||`: the original materialises `li 2` twice, once in
       each branch's delay slot. */
    if ((unsigned char)(node->pChild[0]->bFlags & 1)) {
        nType[0] = 2;
    } else if ((unsigned char)(node->pChild[1]->bFlags & 1)) {
        nType[0] = 2;
    } else {
        nType[0] = 1;
    }
    /* Indexed, not walked: both loops want their own LSR-built giv over
       pChild, and a hand-walked pointer CSEs the two bases together. */
    for (j = 0; j < 2; j++) {
        c = node->pChild[j];
        if ((unsigned char)(c->bFlags & 1)) {
            nType[j * 2 + 2] = 2;
            nType[j * 2 + 1] = 2;
        } else {
            nType[j * 2 + 2] = 1;
            nType[j * 2 + 1] = 1;
        }
    }

    /* Chained, not two copies from node->pos: the second copy's source is
       the first copy's destination, so CSE forwards the just-stored value
       and the whole pair is two ld and four sd. Copying node->pos twice
       reloads it -- the store to seg[0] is in the same alias set. */
    line->seg[0].posB = node->pos;
    line->seg[0].posA = line->seg[0].posB;
    line->seg[0].posB.x = line->seg[0].posB.x + 28.5f;

    pc = node->pChild;
    for (i = 0; i < 2; i++) {
        c = *pc++;
        line->seg[i * 2 + 1].posB = line->seg[0].posB;
        line->seg[i * 2 + 1].posA = line->seg[i * 2 + 1].posB;
        line->seg[i * 2 + 1].posB.y = c->pos.y;
        line->seg[i * 2 + 2].posA = c->pos;
        line->seg[i * 2 + 2].posB = line->seg[i * 2 + 2].posA;
        line->seg[i * 2 + 2].posA.x = line->seg[i * 2 + 1].posB.x;
    }
    subTreeLineDraw(line, nType);
}

/* Type 3: a three-child node. The elbows are drawn for the outer two
   children and the middle child gets a single straight run in segment 5. */
void subTreeLineDraw_type_3(ETLINE *line)
{
    int nType[7];
    ETNODE *node;
    ETNODE *c;
    ETNODE **pc;
    int i;
    int j;

    nType[0] = 1;
    node = line->pNode;
    /* The original really re-tests pChild[0] every pass -- the load is not
       hoisted because the loop is rotated, so this reads as written. */
    for (i = 0; i < 3; i++) {
        if ((unsigned char)(node->pChild[0]->bFlags & 1)) {
            nType[0] = 2;
            break;
        }
    }
    for (j = 0; j < 3; j++) {
        c = node->pChild[j];
        if ((unsigned char)(c->bFlags & 1)) {
            nType[j * 2 + 2] = 2;
            nType[j * 2 + 1] = 2;
        } else {
            nType[j * 2 + 2] = 1;
            nType[j * 2 + 1] = 1;
        }
    }

    /* Chained, not two copies from node->pos: the second copy's source is
       the first copy's destination, so CSE forwards the just-stored value
       and the whole pair is two ld and four sd. Copying node->pos twice
       reloads it -- the store to seg[0] is in the same alias set. */
    line->seg[0].posB = node->pos;
    line->seg[0].posA = line->seg[0].posB;
    line->seg[0].posB.x = line->seg[0].posB.x + 28.5f;

    pc = node->pChild;
    for (i = 0; i < 2; i++) {
        c = *pc;
        pc += 2;
        line->seg[i * 2 + 1].posB = line->seg[0].posB;
        line->seg[i * 2 + 1].posA = line->seg[i * 2 + 1].posB;
        line->seg[i * 2 + 1].posB.y = c->pos.y;
        line->seg[i * 2 + 2].posA = c->pos;
        line->seg[i * 2 + 2].posB = line->seg[i * 2 + 2].posA;
        line->seg[i * 2 + 2].posA.x = line->seg[i * 2 + 1].posB.x;
    }

    c = node->pChild[1];
    line->seg[5].posB = c->pos;
    line->seg[5].posA = line->seg[5].posB;
    line->seg[5].posA.x = line->seg[0].posB.x;
    subTreeLineDraw(line, nType);
}

/* Ease *pCur toward *pDst by a fraction of the remaining gap, plus a 1.0
   floor so it always closes, clamping so it never overshoots. */
void subMoveSlide(float *pCur, float *pDst, float fRate)
{
    float d = *pCur - *pDst;

    if (d != 0.0f) {
        if (*pDst < *pCur) {
            *pCur = *pCur - (d * fRate + 1.0f);
            if (*pCur < *pDst) {
                *pCur = *pDst;
            }
        } else {
            *pCur = *pCur - (d * fRate - 1.0f);
            if (*pDst < *pCur) {
                *pCur = *pDst;
            }
        }
    }
}

/* --- Ether tree right-hand info panel --- */

typedef struct {
    unsigned char bFlags;       /* 0x00 */
    unsigned char nMode;        /* 0x01 */
    char pad02[0x0E];
    float fX;                   /* 0x10 */
    float fY;                   /* 0x14 */
    char pad18[0x08];
    ETNODE *pTarget;            /* 0x20 */
    short nX;                   /* 0x24 */
    short nY;                   /* 0x26 */
    unsigned int nZ;            /* 0x28 */
    char pad2C[0x04];
    int nWidth;                 /* 0x30 */
} ETRIGHT;

typedef struct {
    unsigned char bFlags;       /* 0x00 */
    char pad01;
    unsigned short wId;         /* 0x02 */
    char pad04[0x0C];
    float fCenterX;             /* 0x10 */
    float fCenterY;             /* 0x14 */
    char pad18[0x18];
    float fBaseX;               /* 0x30 */
    float fBaseY;               /* 0x34 */
    float fBaseZ;               /* 0x38 */
    float fScale;               /* 0x3C */
    char pad40[0x14];
    ETNODE *pJouto;             /* 0x54 */
    char pad58[0x28];
} ETSYSTEM;

extern ETSYSTEM *EtherTreeSystem;
extern float D_004D7E84;
extern void endPrintExtFunc(int nKind, int nType, void *pData);

/* Ease the right panel toward its target node, then hand the screen-space
   position to the print list. The target pointer is re-read for the second
   ease because the first call may have changed it. */
void subRightDraw(ETRIGHT *p)
{
    float fRate;
    ETSYSTEM *sys;
    float *pBase;
    float *pCenter;

    fRate = D_004D7E84;
    sys = EtherTreeSystem;
    subMoveSlide(&p->fX, &p->pTarget->pos.x, fRate);
    /* Held member ADDRESSES, not sys-relative offsets: the original keeps
       both group bases in callee-saved registers across the second call. */
    pBase = &sys->fBaseX;
    pCenter = &sys->fCenterX;
    subMoveSlide(&p->fY, &p->pTarget->pos.y, fRate);
    p->nX = (short)(p->fX + pBase[0] + pCenter[0]);
    p->nY = (short)(p->fY + pBase[1] + pCenter[1]);
    p->nZ = (unsigned int)(pBase[2] + 4.0f);
    endPrintExtFunc(0, 17, &p->nX);
}

/* Slide the right panel open (mode 1) and shut (mode 3) eight pixels a
   frame; mode 2 is the held-open state that mode 1 falls through into. */
void subEtherTreeRightMain(ETRIGHT *p)
{
    unsigned char bFlags;
    int n;

    if ((unsigned char)(EtherTreeSystem->bFlags & 1) != 0) {
        bFlags = p->bFlags;
        if ((unsigned char)(bFlags & 1) != 0) {
            switch (p->nMode) {
            /* The idle case must be spelled out. With only cases 1..3 the
               node list has three entries and balance_case_nodes splits at
               the middle, rooting the decision tree at 2; a fourth node
               roots it at 1, which is the `beq 1` / `slti 2` / `beq 2` /
               `beql 3` chain the original has. */
            case 0:
                break;
            case 1:
                n = p->nWidth + 8;
                p->nWidth = n;
                if (n < 48) {
                    break;
                }
                p->nMode = 2;
                /* fallthrough */
            case 2:
                p->nWidth = 48;
                break;
            case 3:
                n = p->nWidth - 8;
                p->nWidth = n;
                if (n > 0) {
                    break;
                }
                p->nWidth = 0;
                p->nMode = 0;
                p->bFlags = bFlags & 0xFE;
                break;
            }
        }
    }
}

extern int MenuEtherWhoCheck(int nIndex);
extern int func_A19578(unsigned short wId, int nIndex);
extern void sub2JoutoYGet(void *data, float *result);

/* Lay out the "jouto" (transfer) grid: every ether whose owner is not the
   tree's own character and which passes func_A19578 gets a slot in a
   four-wide grid, 57 pixels across and 72 down. */
void subJoutoPosSet(void)
{
    float fY;
    ETNODE *root;
    ETNODE *p;
    float *pBase;
    unsigned short wId;
    float fX0;
    float fDX;
    float fDY;
    float fRow;
    unsigned char nFlags;
    int n;
    int i;

    /* Three separate float locals, not three literals: cse turns the
       second 57.0 into `mov.s f22,f20` and the 72.0 step into a copy of
       the temp that added it to fY. Spelling the constants inline instead
       gives one register each and the function comes out three words
       short. */
    fX0 = 57.0f;
    fDX = 57.0f;
    n = 0;
    /* Held member address again -- the base group survives both calls. */
    pBase = &EtherTreeSystem->fBaseX;
    wId = EtherTreeSystem->wId;
    root = EtherTreeObjectWorkGet();
    EtherTreeSystem->pJouto = root;
    p = root;
    sub2JoutoYGet(EtherTreeObject, &fY);
    fY = fY + 72.0f;
    fDY = 72.0f;
    for (i = 1; i < 80; i++) {
        if (wId != (unsigned short)MenuEtherWhoCheck(i) &&
            func_A19578(wId, i) != 0) {
            nFlags = p->bFlags;
            fRow = (float)(n / 4) * fDY;
            p->bFlags = nFlags | 8;
            p->nId = i;
            p->pos.x = fX0 + (float)(n % 4) * fDX;
            p->pos.y = fY + fRow;
            p->pos.z = pBase[2];
            n++;
            p++;
        }
    }
    root->nChildren = n;
}

/* --- Ether tree "line2" (the animated connector to the target node) --- */

/* One drawn quad of the connector: two transformed endpoints, each
   followed by the 16-byte colour block EtherTreeLineColorGet fills. */
typedef struct {
    float x;                    /* 0x00 */
    float y;                    /* 0x04 */
    float z;                    /* 0x08 */
    char pad0C[0x04];
    char color0[0x10];          /* 0x10 */
    float x2;                   /* 0x20 */
    float y2;                   /* 0x24 */
    float z2;                   /* 0x28 */
    char pad2C[0x04];
    char color1[0x10];          /* 0x30 */
} ETOUT;

/* The base[] copy is 4-aligned where position[] is 8-aligned; that is what
   picks ldl/ldr for base = position but plain ld/sd for position = node. */
typedef struct {
    unsigned char bFlags;       /* 0x00 */
    unsigned char nMode;        /* 0x01 */
    unsigned char nState;       /* 0x02 */
    unsigned char nType;        /* 0x03 */
    int nSelect;                /* 0x04 */
    char pad08[0x08];
    EVEC4 base[3];              /* 0x10 */
    EVEC position[3];           /* 0x40 */
    ETNODE *pTarget;            /* 0x70 */
    char pad74[0x0C];
    ETOUT out[3];               /* 0x80, stride 0x40 */
} ETLINE2;

extern void EtherTreeLineColorGet(void *pDst, int nType);

/* Transform all three connector segments into screen space and queue
   them: each endpoint is offset by the tree's base and centre and scaled,
   and both endpoints get a colour block. */
void subLine2_DrawType_1(ETLINE2 *p)
{
    ETSYSTEM *sys;
    float *pBase;
    float *pCenter;
    float *pIn;
    int i;

    sys = EtherTreeSystem;
    pBase = &sys->fBaseX;
    pCenter = &sys->fCenterX;
    /* One float walk over BOTH endpoint arrays: the original's induction
       variable sits on &position[i].y and reaches base[i] at -52/-48, an
       anchor no struct-member spelling of base[i]/position[i] produces --
       indexing the two arrays separately builds four givs. */
    pIn = &p->position[0].y;
    for (i = 0; i < 3; i++) {
        ETOUT *q = &p->out[i];

        q->x = (pIn[-13] + pBase[0] + pCenter[0]) * pBase[3];
        q->y = (pIn[-12] + pBase[1] + pCenter[1]) * pBase[3];
        q->z = pBase[2] - 1.0f;
        q->x2 = (pIn[-1] + pBase[0] + pCenter[0]) * pBase[3];
        q->y2 = (pIn[0] + pBase[1] + pCenter[1]) * pBase[3];
        q->z2 = pBase[2] - 1.0f;
        EtherTreeLineColorGet(q->color0, 3);
        EtherTreeLineColorGet(q->color1, 3);
        endPrintExtFunc(0, 16, q);
        pIn += 4;
    }
}

/* Grow the connector toward the target's first child; 1 once it arrives. */
int subLine2_OpenType_0(ETLINE2 *p)
{
    ETNODE *node = p->pTarget;
    float x;
    float xEnd;

    switch (p->nState) {
    case 0:
        p->position[0] = node->pos;
        p->base[0] = *(EVEC4 *)&p->position[0];
        p->nState = 1;
        /* fallthrough */
    case 1:
        x = p->position[0].x + 4.0f;
        p->position[0].x = x;
        xEnd = node->pChild[0]->pos.x;
        if (x < xEnd) {
            return 0;
        }
        p->position[0].x = xEnd;
        return 1;
    }
    return 0;
}
