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

/* TODO: near-match (LOGIC) - recovered calls/field stores still differ in
 * call-result and object-layout source shape (12 instruction differences). */
void subListMake00(SUB_LIST *list, int id)
{
    list->text = *MenuTextGet(id);
    list->field_08 = 0;
    list->box = MenuBoxChk(id);
}

/* TODO: near-match (LOGIC) - recovered calls/field stores still differ in
 * call-result and object-layout source shape (12 instruction differences). */
void subListMake01(SUB_LIST *list, int id)
{
    list->text = *MenuTextGet(id);
    list->field_08 = 0;
    list->box = MenuBoxMoneyGet(id, 0);
}

/* TODO: near-match (LOGIC) - recursive traversal is recovered; four
 * instructions still differ, likely from the original child data layout. */
void sub2JoutoYGet(void *data, float *result)
{
    unsigned short index = *(unsigned short *)((char *)data + 8);

    if (index != 0) {
        sub2JoutoYGet(*(void **)((char *)data + index * 20 + 8), result);
    } else {
        *result = *(float *)((char *)data + 0x24);
    }
}

/* TODO: near-match (LOGIC) - recovered count/entry predicate differs in
 * global/object representation and loop shape (8 instruction differences). */
int subSeisanHissatuCheck00(void)
{
    int i;
    int count = (unsigned char)SeisanWork[6];

    for (i = 0; i < count; i++) {
        if (*(int *)(SeisanResult + 0x38 + i * 0x3C) != 0) {
            return 0x50;
        }
    }
    return 0;
}

/* TODO: near-match (LOGIC) - recovered count/entry predicate differs in
 * global/object representation and loop shape (8 instruction differences). */
int subSeisanHissatuCheck01(void)
{
    int i;
    int count = (unsigned char)SeisanWork[6];

    for (i = 0; i < count; i++) {
        if (*(int *)(SeisanResult + 0x38 + i * 0x3C) >= 5) {
            return 0x58;
        }
    }
    return 0;
}
