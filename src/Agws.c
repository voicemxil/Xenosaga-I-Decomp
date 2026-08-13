/* AGWS menu recovery setup. */

typedef struct {
    short field_00;
    short field_02;
    char pad_04[0x30];
    short field_34;
    short field_36;
} AGWS_RECOVERY;

extern AGWS_RECOVERY *func_A191C0(int id);
extern void func_A11108(int id, int *work0, int *work1);

/* TODO: near-match (LOGIC) - the recovery iteration/copies are recovered,
 * but unknown helper interfaces and local-work-buffer source shape leave 11
 * instruction differences. Resolve the original helper prototypes. */
void AgwsAllRecovery(void)
{
    int work0[4];
    int work1[4];
    int i;

    for (i = 17; i < 33; i++) {
        AGWS_RECOVERY *recovery = func_A191C0(i);

        func_A11108(i, work0, work1);
        recovery->field_34 = recovery->field_00;
        recovery->field_36 = recovery->field_02;
    }
}
