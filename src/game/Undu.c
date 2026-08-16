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

/* TODO: near-match (LENGTH) - the direct-index and linear-id lookup logic
 * are recovered, but its loop/control-flow shape is 33 built vs 29 original
 * instructions. Recover the original source loop form. */
void *UnduDataGetHeaderSub(void *data, unsigned int count, unsigned int id)
{
    unsigned int i;
    char *entry = (char *)data;

    if (id & 0x8000) {
        id &= 0x7FFF;
        if (id < count) {
            return entry + id * 20;
        }
        return 0;
    }
    if (count == 0) {
        return 0;
    }
    for (i = 0; i < count; i++) {
        if (*(unsigned short *)(entry + 2) == id) {
            return entry;
        }
        entry += 20;
    }
    return 0;
}
