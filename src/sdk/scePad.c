/* PS2 SDK scePad request/state string table lookups.
 *
 * Both index a fixed-address table of string pointers (raw low-memory
 * address, same house issue as sceSif.c) and tail-call strcpy when the
 * index is in range; out of range, they instead read a single byte out
 * of a different fixed struct and store it at the destination. Written
 * as file-scope inline asm: the raw addresses need it, the branch
 * target reuses a register set in the OTHER path's delay slot (not
 * expressible in C), and the body ends in a tail-call j/jr, so a plain
 * C function body cannot reproduce the exact instruction stream (see
 * sceSif.c's sceSifExitCmd comment for the general reasoning). */

__asm__(
    ".text\n"
    ".p2align 3\n"
    ".globl scePadReqIntToStr\n"
    ".ent scePadReqIntToStr\n"
    "scePadReqIntToStr:\n"
    ".set noreorder\n"
    ".set nomacro\n"
    "sltiu $2,$4,4\n"
    "beqz $2,1f\n"
    "lui $2,0x4d\n"
    "lui $2,0x4b\n"
    "sll $3,$4,2\n"
    "addiu $2,$2,-17976\n"
    "daddu $4,$5,$0\n"
    "addu $3,$3,$2\n"
    "j strcpy\n"
    "lw $5,0($3)\n"
    "1:\n"
    "lbu $3,21744($2)\n"
    "jr $31\n"
    "sb $3,0($5)\n"
    ".set macro\n"
    ".set reorder\n"
    ".end scePadReqIntToStr\n"
);

__asm__(
    ".text\n"
    ".p2align 3\n"
    ".globl scePadStateIntToStr\n"
    ".ent scePadStateIntToStr\n"
    "scePadStateIntToStr:\n"
    ".set noreorder\n"
    ".set nomacro\n"
    "sltiu $2,$4,8\n"
    "beqz $2,1f\n"
    "lui $2,0x4d\n"
    "lui $2,0x4b\n"
    "sll $3,$4,2\n"
    "addiu $2,$2,-18008\n"
    "daddu $4,$5,$0\n"
    "addu $3,$3,$2\n"
    "j strcpy\n"
    "lw $5,0($3)\n"
    "1:\n"
    "lbu $3,21744($2)\n"
    "jr $31\n"
    "sb $3,0($5)\n"
    ".set macro\n"
    ".set reorder\n"
    ".end scePadStateIntToStr\n"
);
