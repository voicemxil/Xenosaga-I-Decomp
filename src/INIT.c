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
