/* Undo-query parameter helpers. */

extern float UnduGet2(int mode, float x, float z);

typedef struct {
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
} UNDU_PARAM;

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
    char pad_68[0x40];
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
