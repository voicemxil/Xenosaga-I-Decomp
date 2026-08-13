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

/* TODO: near-match (LENGTH) - all recovered fields/constants are present,
 * but GCC reorders/coalesces zero stores (21 original vs 22 built words).
 * Recover the original initialization expression/dependency shape. */
void UnduParamInit(UNDU_PARAM *param)
{
    param->field_0D = 0x10;
    param->field_38 = 1;
    param->field_10 = 1.0f;
    param->field_14 = -1000.0f;
    param->field_1C = 0x70000000;
    param->field_00 = 0;
    param->field_04 = 0;
    param->field_08 = 0;
    param->field_0A = 0;
    param->field_0C = 0x10;
    param->field_18 = 0;
    param->field_20 = 0;
    param->field_28 = 0;
    param->field_30 = 0;
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
