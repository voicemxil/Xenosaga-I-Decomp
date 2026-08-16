#include "matching.h"
/* Undo-query parameter helpers. */

typedef struct UNDU_PARAM UNDU_PARAM;
extern float UnduGet2(UNDU_PARAM *param, float x, float z);

struct UNDU_PARAM {
    int field_00;
    int field_04;
    short field_08;
    short field_0A;
    unsigned char field_0C;
    unsigned char field_0D;
    short pad_0E;
    float field_10;
    float field_14;
    int field_18;
    int field_1C;
    long long field_20;
    long long field_28;
    long long field_30;
    long long field_38;
};

float UnduGet(float x, float z)
{
    return UnduGet2(0, x, z);
}

/* Store order here is load-bearing and is NOT field order: gcc 2.96
   schedules a run of independent stores, so the source order is
   observable. Field order compiles to 15 differing words; the order
   below was found by hill-climbing over all 13! statement permutations
   (pairwise swaps + single-element moves, scored by diff count -- see
   scratch/u_perm.py) and is the search's global best at 2 differing
   words. Do not "tidy" these back into field order.

   The last two words are the two constant materialisations: the
   original emits `lui 0x7000` (for field_1C) before `li 1` (for
   field_38), gcc emits them in store order. No permutation of the
   thirteen statements flips just that pair, so it is corrected by
   --swap-adjacent UnduParamInit:5 -- a pure transposition of two
   independent immediate loads, verified by tools/audit_swaps.py. */
void UnduParamInit(UNDU_PARAM *param)
{
    param->field_00 = 0;
    param->field_04 = 0;
    param->field_38 = 1;
    param->field_08 = 0;
    param->field_10 = 1.0f;
    param->field_0A = 0;
    param->field_14 = -1000.0f;
    param->field_1C = 0x70000000;
    param->field_0C = 0x10;
    param->field_18 = 0;
    param->field_20 = 0;
    param->field_30 = 0;
    param->field_0D = 0x10;
}

/* Look one entry up in a 20-byte-stride table, either by direct index
   (bit 15 set) or by scanning for a matching id. */
void *UnduDataGetHeaderSub(void *data, unsigned int count, unsigned int id)
{
    unsigned int i;
    char *entry = (char *)data;

    if (id & 0x8000) {
        id &= 0x7FFF;
        if (id < count) {
            return entry + id * 20;
        }
        goto notfound;
    }
    for (i = 0; i < count; i++) {
        if (*(unsigned short *)(entry + 2) == id) {
            return entry;
        }
        entry += 20;
    }
notfound:
    return 0;
}

/* Wall-attribute test on a map unit.  The attribute slot at +0xA8 is read
   as one 64-bit word but FILLED as two 32-bit words when it comes from the
   table, so both spellings have to live in one type: a union of a long long
   and a two-int struct keeps them in a single gcc alias set (two separate
   struct types would let gcc hoist the ld above the sw pair -- wrong code,
   not just a mismatch), and the inner struct's align-4 TYPE_ALIGN is what
   picks lw/sw over ld/sd for the table copy. */
typedef union {
    long long v;
    struct {
        int lo;
        int hi;
    } w;
} WALL_ATTR;

typedef struct {
    char pad_00[0x50];
    struct WALL_SHAPE {
        char pad_00[8];
        unsigned short nType;
    } *pShape;
    char pad_54[0x10];
    WALL_ATTR *pAttrTbl;
    char pad_68[0x18];
    float fX;
    float fY;
    float fZ;
    char pad_8C[4];
    int nPosValid;
    char pad_94[4];
    float fHeight;
    unsigned short *pWallId;
    char pad_A0[2];
    unsigned char nAttrBits;
    char pad_A3[5];
    WALL_ATTR attr;
    long long attrMask;
    long long attrWant;
} WALL_UNIT;

int CheckWallAttr(WALL_UNIT *unit, unsigned short *pId)
{
    WALL_ATTR *tbl;
    WALL_ATTR *d;
    long long attr;
    long long bit;   /* 64-bit: the attr word is 64-bit */

    tbl = unit->pAttrTbl;
    if (tbl != 0) {
        WALL_ATTR *s;

        s = &tbl[*pId];
        d = &unit->attr;
        d->w.lo = s->w.lo;
        d->w.hi = s->w.hi;
    } else {
        unit->attr.v = *pId;
    }
    attr = unit->attr.v;
    if ((attr & unit->attrMask) != unit->attrWant) {
        if (attr & 7) {
            bit = 0;
            switch (unit->pShape->nType & 0x300) {
            case 0x100:
                bit = 1;
                break;
            case 0x200:
                bit = 2;
                break;
            case 0x300:
                bit = 4;
                break;
            }
            /* `bit & attr`, in this operand order, and the two early
               `return 0`s sharing ONE exit block: the return value has to
               stay a single-basic-block pseudo so local_alloc gives it $v0.
               A `ret` variable spanning the dispatch becomes a global pseudo
               and lands in $a3 (12 words off); a flat chain of early returns
               lets gcc if-convert the last test into `sltu` (6 words short). */
            if (bit & attr) {
                return 1;
            }
        }
        return 0;
    }
    return 1;
}

typedef struct {
    char pad_00[0x54];
    int nList;
    char pad_58[8];
    char *pList;
} UNDU_CHECK;

extern float UnduCheckSub(UNDU_CHECK *chk, char *entry);
extern void UnduCheckSubHeightCheck(float height, UNDU_CHECK *chk, char *entry);

/* Height-test every entry of the check list; -1000.0f is the "no hit"
   height (the same sentinel UnduParamInit writes to field_14). */
void UnduCheckSubCheckAll(UNDU_CHECK *chk)
{
    int i;
    char *entry;
    float height;

    entry = chk->pList;
    for (i = 0; i < chk->nList; i++) {
        height = UnduCheckSub(chk, entry);
        if (height != -1000.0f) {
            UnduCheckSubHeightCheck(height, chk, entry);
        }
        entry += 24;
    }
}

typedef struct {
    unsigned short *pWallId;
    unsigned short *pWallIdPrev;
    unsigned short nFlags;
    short nAttrBits;
    char pad_0C[8];
    float fHeight;
    char pad_18[8];
    WALL_ATTR attr;
} UNDU_RESULT;

/* Publish one collision result.  Same attribute-slot rule as
   CheckWallAttr: the 64-bit slot is filled either as two words from the
   table or as one sd from the id, so both spellings go through the one
   WALL_ATTR union. */
float UnduCheckResultCheck(WALL_UNIT *w, UNDU_RESULT *res, float *pos)
{
    unsigned short flags;

    /* Statement order is load-bearing: nFlags, then nAttrBits, then
       fHeight, and pWallId cleared before the attribute slot.  gcc 2.96
       schedules this run of independent stores, so source order shows in
       the output -- the other five orders cost 7-13 words. */
    flags = res->nFlags & 0xEFFF;
    res->nFlags = flags;
    res->nAttrBits = w->nAttrBits & 3;
    res->fHeight = w->fHeight;
    if (w->pWallId != 0) {
        WALL_ATTR *tbl;

        res->pWallIdPrev = res->pWallId;
        tbl = w->pAttrTbl;
        res->pWallId = w->pWallId;
        if (tbl != 0) {
            WALL_ATTR *s;
            WALL_ATTR *d;

            s = &tbl[*w->pWallId];
            d = &res->attr;
            d->w.lo = s->w.lo;
            d->w.hi = s->w.hi;
        } else {
            res->attr.v = *w->pWallId;
        }
    } else {
        res->pWallId = 0;
        res->attr.v = 0;
        if ((flags & 7) != 1) {
            goto done;
        }
    }
    if (w->nPosValid != 0) {
        pos[0] = w->fX;
        pos[1] = w->fY;
        pos[2] = w->fZ;
    }
done:
    return w->fHeight;
}

/* EE scratchpad: the default parameter block when the caller passes none. */
#define UNDU_SPR ((UNDU_PARAM *)0x70000000)

typedef struct {
    UNDU_PARAM param;
    float fX;
    int nPad;
    float fZ;
} UNDU_WORK;

extern unsigned char *CurrentColiHead;
extern float UnduCheck(float *pos, int mode, UNDU_PARAM *param);

float UnduGet2(UNDU_PARAM *param, float x, float z)
{
    UNDU_WORK *work;
    int nType;

    /* CurrentColiHead is read twice, not held in a local: a local becomes
       a callee-saved $s0 across the UnduParamInit call and costs the
       register save/restore pair. */
    if (CurrentColiHead == 0) {
        return 0.0f;
    }
    if (param == 0) {
        UnduParamInit(UNDU_SPR);
        param = UNDU_SPR;
    }
    work = (UNDU_WORK *)param->field_1C;
    nType = *CurrentColiHead & 0x7F;
    if (work == 0) {
        work = (UNDU_WORK *)UNDU_SPR;
    }
    if (nType < 3 && nType != 0) {
        /* PARKED at 12 diffs, all register: the original keeps `work` in
           $a1 and forms the call's first argument directly in $a0
           (`addiu a0,a1,64` between the two swc1); we get work in $a3, the
           address in $v0 and an extra `move a0,v0`.  Length and schedule
           are exact.  Swept: the address expression inline at the call
           (loses the beqzl + the alignment nop -- 40 words), the `p =`
           assignment at all four positions in the store group (only
           position 0 keeps 42 words), PIN($4) on p (23 diffs, adds
           moves), function-scope vs block-scope p, and a float[3] member
           spelling of the destination.  Permuter territory. */
        float *p;

        p = &work->fX;
        work->fX = x;
        work->fZ = z;
        work->nPad = 0;
        return UnduCheck(p, 0, param);
    }
    return param->field_14;
}

/* The collision-data file header: a magic byte, three table sizes and the
   three table pointers, followed by the type-3 chain. */
typedef struct {
    unsigned char nMagic;      /* 0x00, always 0x82 */
    unsigned char nCount0;     /* 0x01 */
    unsigned char nCount1;     /* 0x02 */
    unsigned char nCount2;     /* 0x03 */
    void *pTable0;             /* 0x04 */
    void *pTable1;             /* 0x08 */
    char *pTable2;             /* 0x0C */
    int nChain;                /* 0x10 */
} UNDUHDR;

/* One type-2 group: a first id, how many ids it covers, and the 16-byte
   records they name. */
typedef struct {
    unsigned short nId;        /* 0x00 */
    unsigned short nCount;     /* 0x02 */
    char *pData;               /* 0x04 */
} UNDUGROUP;

/* One link of the type-3 chain: a signed id that goes negative at the end
   of the chain, and the table it names. */
typedef struct {
    short nId;                 /* 0x00 */
    short pad_02;
    void *pData;               /* 0x04 */
} UNDULINK;


/* PARKED at 104 words vs 101. The whole shape is right -- the four-way
   switch on the key's high byte comes out as retail's decision tree
   (`beq 0x100`, `sltiu 257`, `beq 0x200`, `beq 0x300`), both type-0/1 arms
   are real `j` tail calls, the type-2 direct-index and scan arms and the
   type-3 chain walk all match instruction for instruction, including the
   `lhu` + `sll`/`sra` pair retail uses to sign-extend the chain id for the
   call (writing the field as `short` and reading it for both the `>= 0`
   test and the argument is what produces the separate `lh`).

   Solved: the key parameter must be UNSIGNED -- that is what makes the
   switch's range test `sltiu 257` rather than `slti` -- and it has to live
   outside $a0, which retail spells as a `move t0,a0` at entry. A plain
   local copy is coalesced away, so it is pinned to $t0.

   What is left is 3 words in the type-2 SCAN arm. Retail returns the hit
   straight out of the compare's delay slot (`beq wid,id -> epilogue` with
   `move v0,p` filled from the target) and pays 2 words for the miss
   (`b epilogue; move v0,zero`); ee-gcc will not fill that slot from the
   target. Swept: `return p` inside the loop (106 words -- gcc builds a
   two-word join block for it), a `found` local with `break` (104, the
   best, but it costs a `move found,zero` init), declaring the loop locals
   in retail's order, and a function-wide `result` local initialised to 0
   to recover retail's early `move v0,zero` (still 104). Delay-slot
   territory, not a source shape.

   Resolve one collision-data lookup key against the loaded header. The
   key's high byte picks the table and its low byte indexes within it. */
void *UnduDataGetHeader(unsigned int key, unsigned int id)
{
    UNDUHDR *hdr = (UNDUHDR *)CurrentColiHead;
    PIN(unsigned int sel, "$8");

    sel = key;

    if (hdr == 0 || hdr->nMagic != 0x82) {
        return 0;
    }
    switch (sel & 0xFF00) {
    case 0x000:
        return UnduDataGetHeaderSub(hdr->pTable0, hdr->nCount0, id);
    case 0x100:
        return UnduDataGetHeaderSub(hdr->pTable1, hdr->nCount1, id);
    case 0x200: {
        unsigned int index = sel & 0xFF;
        UNDUGROUP *group;

        if (index >= hdr->nCount2) {
            return 0;
        }
        group = (UNDUGROUP *)(hdr->pTable2 + index * 8);
        if (id & 0x8000) {
            unsigned int slot = id & 0x7FFF;

            if (slot < group->nCount) {
                return group->pData + slot * 16;
            }
        } else {
            int n = group->nCount;
            int i = 0;
            char *p = group->pData;
            int wid = group->nId;

            char *found = 0;

            if (n != 0) {
                do {
                    if (wid == id) {
                        found = p;
                        break;
                    }
                    i++;
                    p += 16;
                    wid++;
                } while (i < n);
            }
            return found;
        }
        return 0;
    }
    case 0x300: {
        UNDULINK *link;

        if (hdr->pTable0 == (void *)&hdr->nChain) {
            return 0;
        }
        link = (UNDULINK *)hdr->nChain;
        while (link->nId >= 0) {
            if ((sel & 0xFF) == 0) {
                return UnduDataGetHeaderSub(link->pData, link->nId, id);
            }
            sel--;
            link++;
        }
        return 0;
    }
    }
    return 0;
}
