/* PS2 SDK sceCd (CD/DVD filesystem) thin wrappers. */

extern int sceCdLayerSearchFile(int a0, int a1, int a2);

int sceCdSearchFile(int a0, int a1)
{
    return sceCdLayerSearchFile(a0, a1, 0);
}

/* sceCdSt* streaming-control family: thin dispatchers into sceCdStream,
 * a 5-arg (a0-a3 + t0) internal entry point, each hard-coding a
 * different opcode in a3 and either forwarding or zeroing a0-a2. Some
 * also flip a fixed low-memory streaming-active flag (base 0x4B0000-ish,
 * same raw-address issue as sceSif.c/sceGs.c/sceTty.c) and/or load a
 * fixed low-memory buffer address for t0 (0x996F18). Written as
 * file-scope inline asm for both reasons plus the usual jal-delay-slot
 * control (see sceSif.c's sceSifExitCmd comment for why a plain C
 * function body can't be used once the function ends in its own
 * jr+delay-slot epilogue). `move` is spelled out as `daddu $x,$0,$0`
 * -- the pseudo-op assembles as `or`, but the original encodes daddu. */

__asm__(
    ".text\n"
    ".p2align 3\n"
    ".globl sceCdStInit\n"
    ".ent sceCdStInit\n"
    "sceCdStInit:\n"
    ".set noreorder\n"
    ".set nomacro\n"
    "addiu $sp,$sp,-16\n"
    "lui $2,0x4b\n"
    "lui $8,0x99\n"
    "sd $31,0($sp)\n"
    "sw $0,-11152($2)\n"
    "addiu $8,$8,28440\n"
    "jal sceCdStream\n"
    "li $7,5\n"
    "ld $31,0($sp)\n"
    "jr $31\n"
    "addiu $sp,$sp,16\n"
    ".set macro\n"
    ".set reorder\n"
    ".end sceCdStInit\n"
);

__asm__(
    ".text\n"
    ".p2align 3\n"
    ".globl sceCdStSeek\n"
    ".ent sceCdStSeek\n"
    "sceCdStSeek:\n"
    ".set noreorder\n"
    ".set nomacro\n"
    "addiu $sp,$sp,-16\n"
    "lui $8,0x99\n"
    "sd $31,0($sp)\n"
    "addiu $8,$8,28440\n"
    "daddu $5,$0,$0\n"
    "daddu $6,$0,$0\n"
    "jal sceCdStream\n"
    "li $7,4\n"
    "ld $31,0($sp)\n"
    "jr $31\n"
    "addiu $sp,$sp,16\n"
    ".set macro\n"
    ".set reorder\n"
    ".end sceCdStSeek\n"
);

__asm__(
    ".text\n"
    ".p2align 3\n"
    ".globl sceCdStSeekF\n"
    ".ent sceCdStSeekF\n"
    "sceCdStSeekF:\n"
    ".set noreorder\n"
    ".set nomacro\n"
    "addiu $sp,$sp,-16\n"
    "lui $8,0x99\n"
    "sd $31,0($sp)\n"
    "addiu $8,$8,28440\n"
    "daddu $5,$0,$0\n"
    "daddu $6,$0,$0\n"
    "jal sceCdStream\n"
    "li $7,9\n"
    "ld $31,0($sp)\n"
    "jr $31\n"
    "addiu $sp,$sp,16\n"
    ".set macro\n"
    ".set reorder\n"
    ".end sceCdStSeekF\n"
);

__asm__(
    ".text\n"
    ".p2align 3\n"
    ".globl sceCdStStart\n"
    ".ent sceCdStStart\n"
    "sceCdStStart:\n"
    ".set noreorder\n"
    ".set nomacro\n"
    "addiu $sp,$sp,-16\n"
    "lui $3,0x4b\n"
    "li $2,1\n"
    "daddu $8,$5,$0\n"
    "sd $31,0($sp)\n"
    "daddu $5,$0,$0\n"
    "sw $2,-11152($3)\n"
    "daddu $6,$0,$0\n"
    "jal sceCdStream\n"
    "li $7,1\n"
    "ld $31,0($sp)\n"
    "jr $31\n"
    "addiu $sp,$sp,16\n"
    ".set macro\n"
    ".set reorder\n"
    ".end sceCdStStart\n"
);

__asm__(
    ".text\n"
    ".p2align 3\n"
    ".globl sceCdStStop\n"
    ".ent sceCdStStop\n"
    "sceCdStStop:\n"
    ".set noreorder\n"
    ".set nomacro\n"
    "addiu $sp,$sp,-16\n"
    "lui $2,0x4b\n"
    "lui $8,0x99\n"
    "sd $31,0($sp)\n"
    "sw $0,-11152($2)\n"
    "addiu $8,$8,28440\n"
    "daddu $4,$0,$0\n"
    "daddu $5,$0,$0\n"
    "daddu $6,$0,$0\n"
    "jal sceCdStream\n"
    "li $7,3\n"
    "ld $31,0($sp)\n"
    "jr $31\n"
    "addiu $sp,$sp,16\n"
    ".set macro\n"
    ".set reorder\n"
    ".end sceCdStStop\n"
);
