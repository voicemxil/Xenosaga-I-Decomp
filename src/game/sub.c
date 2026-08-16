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
    ETNODE *pNode;      /* 0x00 */
    char pad04[0x0C];
    EVEC posA;          /* 0x10 */
    char pad20[0x10];
    EVEC posB;          /* 0x30 */
} ETLINE;

extern void subTreeLineDraw(ETLINE *line, int *pType);

/* Type 1: node -> first child. Learned/unlearned picks the line style. */
void subTreeLineDraw_type_1(ETLINE *line)
{
    ETNODE *node = line->pNode;
    ETNODE *child = node->pChild[0];
    int nType;

    line->posA = node->pos;
    line->posB = child->pos;
    nType = (unsigned char)(child->bFlags & 1) ? 2 : 1;
    subTreeLineDraw(line, &nType);
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
    char pad02[0x2E];
    int nWidth;                 /* 0x30 */
} ETRIGHT;

typedef struct {
    unsigned char bFlags;       /* 0x00 */
    char pad01[0x7F];
} ETSYSTEM;

extern ETSYSTEM *EtherTreeSystem;

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

/* --- Ether tree "line2" (the animated connector to the target node) --- */

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
    char pad74[0xCC];
} ETLINE2;

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
