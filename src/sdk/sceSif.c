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

void *sceSifGetDataTable(void)
{
    __asm__ __volatile__(
        "lui $2,0x99\n\t"
        "addiu $2,$2,0x18d8"
    );
}

/* Thin dispatch wrappers around the underscore-prefixed internal SIF
 * loader/heap primitives (same house pattern as sceFs.c). */

extern int sceSifFreeSysMemory(int a0, int a1);
extern int _sceSifLoadElfPart(int a0, int a1, int a2, int a3);
extern int _sceSifLoadModuleBuffer(int a0, int a1, int a2, int a3);
extern int _sceSifLoadModule(int a0, int a1, int a2, int a3, int a4);

int sceSifFreeIopHeap(int a0, int a1)
{
    return sceSifFreeSysMemory(a0, a1);
}

int sceSifLoadElfPart(int a0, int a1, int a2)
{
    return _sceSifLoadElfPart(a0, a1, a2, 1);
}

int sceSifLoadModuleBuffer(int a0, int a1, int a2)
{
    int local;

    return _sceSifLoadModuleBuffer(a0, a1, a2, (int)&local);
}

int sceSifLoadStartModule(int a0, int a1, int a2, int a3)
{
    return _sceSifLoadModule(a0, a1, a2, a3, 0);
}

int sceSifLoadStartModuleBuffer(int a0, int a1, int a2, int a3)
{
    return _sceSifLoadModuleBuffer(a0, a1, a2, a3);
}

int sceSifLoadModule(int a0, int a1, int a2)
{
    int local;

    return _sceSifLoadModule(a0, a1, a2, (int)&local, 0);
}

/* sceSifExitCmd/sceSifExitRpc: teardown that mixes calls to kernel
 * primitives with a fixed low-memory control-block store, so (as with
 * the raw-address accessors above) the body is written as inline asm.
 *
 * Unlike the leaf accessors above, these already end with their own
 * jr/delay-slot epilogue, so they cannot be a normal C function body:
 * GCC always appends its own trailing "j $31" epilogue after a
 * fall-off-the-end asm block (there's no `return` for it to see),
 * producing a dead but byte-real duplicate jr. Written as file-scope
 * __asm__ (a bare .ent/.end block, no enclosing C function) so GCC's
 * epilogue synthesis never runs; ".set noreorder" stops the assembler
 * from auto-filling jal delay slots by re-scheduling the following
 * instruction (its default "reorder" mode moved a later `lw` into an
 * earlier jal's delay slot instead of the `li` actually written there,
 * scrambling the order). */

__asm__(
    ".text\n"
    ".p2align 3\n"
    ".globl sceSifExitCmd\n"
    ".ent sceSifExitCmd\n"
    "sceSifExitCmd:\n"
    ".set noreorder\n"
    ".set nomacro\n"
    "addiu $sp,$sp,-16\n"
    "sd $31,0($sp)\n"
    "jal DisableDmac\n"
    "li $4,5\n"
    "lui $3,0x99\n"
    "li $4,5\n"
    "jal RemoveDmacHandler\n"
    "lw $5,0x18d4($3)\n"
    "lui $3,0x4b\n"
    "ld $31,0($sp)\n"
    "sw $0,-23888($3)\n"
    "jr $31\n"
    "addiu $sp,$sp,16\n"
    ".set macro\n"
    ".set reorder\n"
    ".end sceSifExitCmd\n"
);

/* sceSifIsAliveIop: the boolean cast of (x & 0x10000) is a single-bit
 * test, and GCC's own idiom substitution turns that into a shift+andi
 * (sra/andi) rather than the and+sltu the original uses, regardless of
 * how the C is phrased -- so this is inline asm (file-scope, see
 * sceSifExitCmd above, since it still needs to call through sceSifGetReg
 * with a controlled delay slot). */

__asm__(
    ".text\n"
    ".p2align 3\n"
    ".globl sceSifIsAliveIop\n"
    ".ent sceSifIsAliveIop\n"
    "sceSifIsAliveIop:\n"
    ".set noreorder\n"
    ".set nomacro\n"
    "addiu $sp,$sp,-16\n"
    "sd $31,0($sp)\n"
    "jal sceSifGetReg\n"
    "li $4,4\n"
    "lui $3,0x1\n"
    "ld $31,0($sp)\n"
    "and $2,$2,$3\n"
    "sltu $2,$0,$2\n"
    "jr $31\n"
    "addiu $sp,$sp,16\n"
    ".set macro\n"
    ".set reorder\n"
    ".end sceSifIsAliveIop\n"
);

/* sceSifLoadElf: the fixed 0x4D4BE8 constant hits the same lui+ori vs
 * lui+addiu issue as the raw-address accessors, so this is inline asm
 * too (file-scope, for the same jal-delay-slot-control reason). */

__asm__(
    ".text\n"
    ".p2align 3\n"
    ".globl sceSifLoadElf\n"
    ".ent sceSifLoadElf\n"
    "sceSifLoadElf:\n"
    ".set noreorder\n"
    ".set nomacro\n"
    "daddu $6,$5,$0\n"
    "addiu $sp,$sp,-16\n"
    "lui $5,0x4d\n"
    "sd $31,0($sp)\n"
    "addiu $5,$5,19432\n"
    "jal _sceSifLoadElfPart\n"
    "li $7,1\n"
    "ld $31,0($sp)\n"
    "jr $31\n"
    "addiu $sp,$sp,16\n"
    ".set macro\n"
    ".set reorder\n"
    ".end sceSifLoadElf\n"
);

/* sceSifRemoveCmdHandler: picks one of two SIF command-table pointers
 * out of the fixed low-memory _data_table block (base 0x9918D8, same
 * block sceSifSetSysCmdBuffer/sceSifSetCmdBuffer index into above) by
 * the sign of the handler index, then zeroes the selected slot. Written
 * as inline asm both for the raw-address loads (lui+addiu house issue)
 * and to reproduce the exact branch/label layout -- C control flow for
 * this shape reliably compiled 2 words longer (a macro-expanded lw of
 * the 32-bit address plus branch-target alignment padding). */

__asm__(
    ".text\n"
    ".p2align 3\n"
    ".globl sceSifRemoveCmdHandler\n"
    ".ent sceSifRemoveCmdHandler\n"
    "sceSifRemoveCmdHandler:\n"
    ".set noreorder\n"
    ".set nomacro\n"
    "bgez $4,1f\n"
    "sll $3,$4,3\n"
    "lui $2,0x99\n"
    "b 2f\n"
    "lw $4,6372($2)\n"
    "1:\n"
    "lui $2,0x99\n"
    "lw $4,6380($2)\n"
    "2:\n"
    "addu $3,$3,$4\n"
    "jr $31\n"
    "sw $0,0($3)\n"
    ".set macro\n"
    ".set reorder\n"
    ".end sceSifRemoveCmdHandler\n"
);

/* sceSifAddCmdHandler: same table-select shape as sceSifRemoveCmdHandler
 * above, storing the handler function (a1) and its argument (a2) into
 * the selected slot instead of zeroing it. */

__asm__(
    ".text\n"
    ".p2align 3\n"
    ".globl sceSifAddCmdHandler\n"
    ".ent sceSifAddCmdHandler\n"
    "sceSifAddCmdHandler:\n"
    ".set noreorder\n"
    ".set nomacro\n"
    "bgez $4,1f\n"
    "sll $3,$4,3\n"
    "lui $2,0x99\n"
    "b 2f\n"
    "lw $4,6372($2)\n"
    "1:\n"
    "lui $2,0x99\n"
    "lw $4,6380($2)\n"
    "2:\n"
    "addu $3,$3,$4\n"
    "sw $6,4($3)\n"
    "jr $31\n"
    "sw $5,0($3)\n"
    ".set macro\n"
    ".set reorder\n"
    ".end sceSifAddCmdHandler\n"
);

/* sceSifSyncIop: reads sceSifGetReg(4) (an IOP sync/status register),
 * and if bit 0x40000 is set, re-inits the debug tty and always returns
 * 1; otherwise returns 0. File-scope inline asm because the body ends
 * in its own jr+delay-slot epilogue after a jal (see sceSifExitCmd
 * above for why a plain C function body can't reproduce this). */

__asm__(
    ".text\n"
    ".p2align 3\n"
    ".globl sceSifSyncIop\n"
    ".ent sceSifSyncIop\n"
    "sceSifSyncIop:\n"
    ".set noreorder\n"
    ".set nomacro\n"
    "addiu $sp,$sp,-16\n"
    "sd $31,0($sp)\n"
    "jal sceSifGetReg\n"
    "li $4,4\n"
    "lui $3,0x4\n"
    "and $2,$2,$3\n"
    "beqz $2,1f\n"
    "daddu $2,$0,$0\n"
    "jal sceResetttyinit\n"
    "nop\n"
    "li $2,1\n"
    "1:\n"
    "ld $31,0($sp)\n"
    "jr $31\n"
    "addiu $sp,$sp,16\n"
    ".set macro\n"
    ".set reorder\n"
    ".end sceSifSyncIop\n"
);

__asm__(
    ".text\n"
    ".p2align 3\n"
    ".globl sceSifExitRpc\n"
    ".ent sceSifExitRpc\n"
    "sceSifExitRpc:\n"
    ".set noreorder\n"
    ".set nomacro\n"
    "addiu $sp,$sp,-16\n"
    "sd $31,0($sp)\n"
    "jal sceSifExitCmd\n"
    "nop\n"
    "lui $2,0x4b\n"
    "ld $31,0($sp)\n"
    "sw $0,-23884($2)\n"
    "jr $31\n"
    "addiu $sp,$sp,16\n"
    ".set macro\n"
    ".set reorder\n"
    ".end sceSifExitRpc\n"
);
