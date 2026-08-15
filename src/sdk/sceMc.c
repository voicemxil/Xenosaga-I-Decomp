/* PS2 SDK sceMc (memory card) thin wrappers. */

/* sceMcMkdir: opens the directory with sceMcOpen and, on failure, stores
 * a fixed error code into a fixed low-memory status word (raw-address,
 * same house issue as sceSif.c). File-scope inline asm both for the
 * raw address and to reproduce the exact branch/label layout. */

__asm__(
    ".text\n"
    ".p2align 3\n"
    ".globl sceMcMkdir\n"
    ".ent sceMcMkdir\n"
    "sceMcMkdir:\n"
    ".set noreorder\n"
    ".set nomacro\n"
    "addiu $sp,$sp,-16\n"
    "sd $31,0($sp)\n"
    "jal sceMcOpen\n"
    "li $7,64\n"
    "daddu $4,$2,$0\n"
    "bnez $4,1f\n"
    "ld $31,0($sp)\n"
    "lui $3,0x4b\n"
    "li $2,11\n"
    "sw $2,-11128($3)\n"
    "1:\n"
    "daddu $2,$4,$0\n"
    "jr $31\n"
    "addiu $sp,$sp,16\n"
    ".set macro\n"
    ".set reorder\n"
    ".end sceMcMkdir\n"
);
