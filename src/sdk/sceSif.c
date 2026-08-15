/* PS2 SDK sceSif (EE<->IOP SIF) low-level register/table accessors.
 *
 * These address a fixed low-memory SIF control block by raw numeric
 * constant (no ELF symbol backs it -- it is hardware/kernel-reserved
 * memory), so the base is materialized with lui+addiu. The modern
 * assembler's constant synthesis for a bare (int*)0x... cast always
 * picks lui+ori instead (verified: every C-level phrasing of the
 * constant did, including la-macro-shaped ones), so the body is
 * written as inline asm reproducing the exact original instructions,
 * the same technique kernel.c uses for the syscall stubs.
 */

int sceSifGetSreg(int idx)
{
    __asm__ __volatile__(
        "lui $2,0x99\n\t"
        "sll $4,$4,2\n\t"
        "addiu $2,$2,0x1a00\n\t"
        "addu $4,$4,$2\n\t"
        "lw $2,0($4)"
    );
}

int sceSifSetSreg(int idx, int val)
{
    __asm__ __volatile__(
        "lui $2,0x99\n\t"
        "sll $4,$4,2\n\t"
        "addiu $2,$2,0x1a00\n\t"
        "addu $4,$4,$2\n\t"
        "daddu $2,$5,$0\n\t"
        "sw $5,0($4)"
    );
}

void *sceSifSetSysCmdBuffer(void *buf, int size)
{
    __asm__ __volatile__(
        "lui $3,0x99\n\t"
        "addiu $3,$3,0x18d8\n\t"
        "lw $2,12($3)\n\t"
        "sw $5,16($3)\n\t"
        "sw $4,12($3)"
    );
}

void *sceSifSetCmdBuffer(void *buf, int size)
{
    __asm__ __volatile__(
        "lui $3,0x99\n\t"
        "addiu $3,$3,0x18d8\n\t"
        "lw $2,20($3)\n\t"
        "sw $5,24($3)\n\t"
        "sw $4,20($3)"
    );
}
