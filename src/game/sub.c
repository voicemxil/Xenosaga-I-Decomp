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

/* TODO: near-match (5 of 20 words, was 8). Pinning the loop bound to $a2 is
 * what fixes the i/count role swap (gcc otherwise puts the counter in $a2
 * and the bound in $a1, the mirror of the original). Do NOT also pin the
 * counter to $a1: that regresses to 15.
 * What is left: the original loads the byte into $a0, tests THAT for the
 * loop guard, and only then copies it into $a2 (`move a2,a0`); we load
 * straight into $a2, which frees a slot and lets the first `lw` float one
 * instruction earlier. Tried: a separate n temp pinned to $a0 and copied
 * into count (5, no change), LAUNDER or PASSTHRU between them (both grow
 * the function to 22 words / 19 diffs), an explicit `if (n == 0) return 0;`
 * guard ahead of the loop (25 words, 17 diffs) and the same with `<= 0`,
 * and a while-loop shape (5, no change). */
int subSeisanHissatuCheck00(void)
{
    PIN(int count, "$6");
    int i;

    count = (unsigned char)SeisanWork[6];
    for (i = 0; i < count; i++) {
        if (*(int *)(SeisanResult + 0x38 + i * 0x3C) != 0) {
            return 0x50;
        }
    }
    return 0;
}

/* TODO: near-match (5 of 20 words, was 8). Pinning the loop bound to $a2 is
 * what fixes the i/count role swap (gcc otherwise puts the counter in $a2
 * and the bound in $a1, the mirror of the original). Do NOT also pin the
 * counter to $a1: that regresses to 15.
 * What is left: the original loads the byte into $a0, tests THAT for the
 * loop guard, and only then copies it into $a2 (`move a2,a0`); we load
 * straight into $a2, which frees a slot and lets the first `lw` float one
 * instruction earlier. Tried: a separate n temp pinned to $a0 and copied
 * into count (5, no change), LAUNDER or PASSTHRU between them (both grow
 * the function to 22 words / 19 diffs), an explicit `if (n == 0) return 0;`
 * guard ahead of the loop (25 words, 17 diffs) and the same with `<= 0`,
 * and a while-loop shape (5, no change). */
int subSeisanHissatuCheck01(void)
{
    PIN(int count, "$6");
    int i;

    count = (unsigned char)SeisanWork[6];
    for (i = 0; i < count; i++) {
        if (*(int *)(SeisanResult + 0x38 + i * 0x3C) >= 5) {
            return 0x58;
        }
    }
    return 0;
}
