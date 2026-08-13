/* Model-system subsystem re-initializers (thin trampolines to CONSTRUCT_*) */

void CONSTRUCT_ALPHA_GROUP(void);
void CONSTRUCT_PARENT_BUF(void);
void CONSTRUCT_MAP_HANDLE(void *handle);
void CONSTRUCT_FADE_CONTROL(void *ctrl);
void CONSTRUCT_MODELSYSTEM(void);

/* NOTE: INIT_BACK_BUFFER is NOT a CONSTRUCT_BACK_BUFFER trampoline like
 * its siblings below -- it has its own inline body with a different
 * field-write order/set (no field 0x24 write, 0x20 before 0x14). Left
 * unimplemented here; see the final report for details. */

void INIT_ALPHA_GROUP(void)
{
    CONSTRUCT_ALPHA_GROUP();
}

void INIT_PARENT_BUF(void)
{
    CONSTRUCT_PARENT_BUF();
}

void INIT_MAP_HANDLE(void *handle)
{
    CONSTRUCT_MAP_HANDLE(handle);
}

void INIT_FADE_CONTROL(void *ctrl)
{
    CONSTRUCT_FADE_CONTROL(ctrl);
}

void INIT_MODELSYSTEM(void)
{
    CONSTRUCT_MODELSYSTEM();
}

typedef struct {
    int field_00;
    int field_04;
    int field_08;
    int field_0C;
    int field_10;
    int field_14;
    int field_18;
    int field_1C;
    int field_20;
} INIT_BACK_BUFFER_DATA;

extern INIT_BACK_BUFFER_DATA s_inBackBuffer;

/* TODO: near-match (SCHEDULING) - all target stores are present, but GCC
 * moves the 0x1C/0x20/0x18 writes relative to the original. Recover the
 * source dependency or initialization expression that fixes their order. */
void INIT_BACK_BUFFER(void)
{
    s_inBackBuffer.field_10 = -1;
    s_inBackBuffer.field_20 = 0;
    s_inBackBuffer.field_14 = 0;
    s_inBackBuffer.field_00 = 0;
    s_inBackBuffer.field_04 = 0;
    s_inBackBuffer.field_08 = 0;
    s_inBackBuffer.field_0C = 0;
    s_inBackBuffer.field_18 = 0;
    s_inBackBuffer.field_1C = 0;
}
