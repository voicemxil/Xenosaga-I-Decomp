/* PS2 SDK sceTty trivial accessors (raw-address inline asm, see sceSif.c) */

void sceResetttyinit(void)
{
    __asm__ __volatile__(
        "lui $2,0x4b\n\t"
        "sw $0,-21712($2)"
    );
}
