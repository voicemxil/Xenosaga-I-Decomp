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
