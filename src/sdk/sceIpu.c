/* PS2 SDK sceIpu (Image Processing Unit) synchronization. */

typedef int u128 __attribute__((mode(TI)));

extern void __gnu_compiled_c_00210510(int enable);
extern const volatile u128 iqval[];
extern const volatile u128 vqval[];

#define IPU_CMD  (*(volatile unsigned int *)0x10002000)
#define IPU_FIFO (*(volatile u128 *)0x10007010)

/* IPU_CTRL, read through the uncached KSEG1 window. */
#define IPU_CTRL (*(volatile unsigned int *)0x10002010)

/* sceIpuSync: mode 0 spins on IPU_CTRL's BUSY bit (bit 31) until it
 * clears and returns 0; mode 1 reads the register once and returns that
 * bit without spinning; any other mode returns 0 immediately.
 *
 * The `switch` is load-bearing. An if/else-if chain -- in either branch
 * polarity, with or without early returns -- inverts the first test and
 * inlines the spin block, giving 21 words against the original's 26.
 * The switch is what makes gcc emit the two forward tests up front and
 * lay the two bodies out afterwards with a shared `move v0,v1` join,
 * which is the original's shape. The nops padding the volatile
 * IPU_CTRL accesses are gcc's own (it emits them for the volatile load
 * on this target); they are not hand-inserted stall pads. */

int sceIpuSync(int mode)
{
    int r = 0;

    switch (mode) {
    case 0:
        while ((int)IPU_CTRL < 0)
            ;
        r = 0;
        break;
    case 1:
        r = IPU_CTRL >> 31;
        break;
    }
    return r;
}

void sceIpuInit(void)
{
    __gnu_compiled_c_00210510(1);

    IPU_CTRL = 0x40000000;
    while ((int)IPU_CTRL < 0)
        ;

    IPU_CMD = 0;
    while ((int)IPU_CTRL < 0)
        ;

    IPU_FIFO = iqval[0];
    IPU_FIFO = iqval[1];
    IPU_FIFO = iqval[2];
    IPU_FIFO = iqval[3];
    IPU_FIFO = iqval[4];
    IPU_FIFO = iqval[4];
    IPU_FIFO = iqval[4];
    IPU_FIFO = iqval[4];

    IPU_CMD = 0x50000000;
    while ((int)IPU_CTRL < 0)
        ;

    IPU_CMD = 0x58000000;
    while ((int)IPU_CTRL < 0)
        ;

    IPU_FIFO = vqval[0];
    IPU_FIFO = vqval[1];

    IPU_CMD = 0x60000000;
    while ((int)IPU_CTRL < 0)
        ;

    IPU_CMD = 0x90000000;
    while ((int)IPU_CTRL < 0)
        ;

    IPU_CTRL = 0x40000000;
    while ((int)IPU_CTRL < 0)
        ;

    IPU_CMD = 0;
    while ((int)IPU_CTRL < 0)
        ;
}
