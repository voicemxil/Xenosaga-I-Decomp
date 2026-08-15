/* AGWS menu recovery setup. */

typedef struct {
    short field_00;
    short field_02;
    char pad_04[0x30];
    short field_34;
    short field_36;
} AGWS_RECOVERY;

extern AGWS_RECOVERY *func_A191C0(int id);
extern AGWS_RECOVERY *func_A11108(int id, int *work0, int *work1);

/* func_A11108 also returns an AGWS_RECOVERY *: the two shorts copied into
 * func_A191C0's record come from ITS return value, not from the record
 * itself (the original reads lhu 0/2 off the second call's v0 and writes
 * sh 0x34/0x36 through the first call's, saved in s0). */
void AgwsAllRecovery(void)
{
    int work0[4];
    int work1[4];
    int i;

    for (i = 17; i < 33; i++) {
        AGWS_RECOVERY *recovery = func_A191C0(i);
        AGWS_RECOVERY *source = func_A11108(i, work0, work1);

        recovery->field_34 = source->field_00;
        recovery->field_36 = source->field_02;
    }
}
