/* PS2 SDK sceCd (CD/DVD filesystem) thin wrappers. */

extern int sceCdLayerSearchFile(int a0, int a1, int a2);

int sceCdSearchFile(int a0, int a1)
{
    return sceCdLayerSearchFile(a0, a1, 0);
}

/* sceCdGetReadPos: if a fixed low-memory status flag reads 1 (streaming
 * active), returns the value at a fixed KSEG1-mapped (uncached) address;
 * otherwise returns 0. Raw addresses throughout, so file-scope inline
 * asm (same house issue as sceSif.c). */

__asm__(
    ".text\n"
    ".p2align 3\n"
    ".globl sceCdGetReadPos\n"
    ".ent sceCdGetReadPos\n"
    "sceCdGetReadPos:\n"
    ".set noreorder\n"
    ".set nomacro\n"
    "lui $2,0x4b\n"
    "li $4,1\n"
    "lw $3,-17836($2)\n"
    "bne $3,$4,1f\n"
    "lui $3,0x4b\n"
    "lui $2,0x2000\n"
    "addiu $3,$3,-13376\n"
    "or $3,$3,$2\n"
    "jr $31\n"
    "lw $2,0($3)\n"
    "1:\n"
    "jr $31\n"
    "daddu $2,$0,$0\n"
    ".set macro\n"
    ".set reorder\n"
    ".end sceCdGetReadPos\n"
);

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

/* sceCdCallback: installs a new EE-side CD event callback and returns
 * the previous one, guarding the swap with DIntr/EIntr and bailing out
 * early (returning 0, no swap) if sceCdSync(1) reports the drive still
 * busy. The callback slot is a fixed low-memory pointer (raw-address
 * house issue, same block scePad's RPC family and sceCdInitEeCB share)
 * loaded before EIntr and stored in EIntr's own jal delay slot, so this
 * is file-scope inline asm both for the raw address and to reproduce
 * that exact load/call/store interleaving. */

__asm__(
    ".text\n"
    ".p2align 3\n"
    ".globl sceCdCallback\n"
    ".ent sceCdCallback\n"
    "sceCdCallback:\n"
    ".set noreorder\n"
    ".set nomacro\n"
    "addiu $sp,$sp,-48\n"
    "sd $17,16($sp)\n"
    "daddu $17,$4,$0\n"
    "sd $31,32($sp)\n"
    "sd $16,0($sp)\n"
    "jal sceCdSync\n"
    "li $4,1\n"
    "bnez $2,1f\n"
    "daddu $2,$0,$0\n"
    "jal DIntr\n"
    "nop\n"
    "lui $3,0x99\n"
    "lw $16,27776($3)\n"
    "jal EIntr\n"
    "sw $17,27776($3)\n"
    "daddu $2,$16,$0\n"
    "1:\n"
    "ld $31,32($sp)\n"
    "ld $17,16($sp)\n"
    "ld $16,0($sp)\n"
    "jr $31\n"
    "addiu $sp,$sp,48\n"
    ".set macro\n"
    ".set reorder\n"
    ".end sceCdCallback\n"
);

/* sceCdInitEeCB: lazily spins up the EE-side CD event thread (a fixed
 * low-memory thread-id slot, raw-address house issue, doubles as the
 * "already running" guard) the first time it's called, storing the
 * caller's callback args into the thread's param/priority struct;
 * later calls just re-prioritize the existing thread. File-scope
 * inline asm for the raw address and the jal-delay-slot-exact calls
 * into the kernel thread API. */

__asm__(
    ".text\n"
    ".p2align 3\n"
    ".globl sceCdInitEeCB\n"
    ".ent sceCdInitEeCB\n"
    "sceCdInitEeCB:\n"
    ".set noreorder\n"
    ".set nomacro\n"
    "addiu $sp,$sp,-96\n"
    "sd $20,64($sp)\n"
    "sd $17,16($sp)\n"
    "lui $20,0x4b\n"
    "daddu $17,$4,$0\n"
    "sd $19,48($sp)\n"
    "sd $18,32($sp)\n"
    "li $19,1\n"
    "sd $16,0($sp)\n"
    "daddu $18,$5,$0\n"
    "lw $4,-17900($20)\n"
    "daddu $16,$6,$0\n"
    "bnez $4,1f\n"
    "sd $31,80($sp)\n"
    "jal GetThreadId\n"
    "nop\n"
    "lui $3,0x99\n"
    "lui $5,0x99\n"
    "sw $2,27792($3)\n"
    "daddu $4,$2,$0\n"
    "jal ReferThreadStatus\n"
    "addiu $5,$5,27800\n"
    "lui $3,0x99\n"
    "lui $2,0x4e\n"
    "lui $5,0x21\n"
    "addiu $3,$3,27848\n"
    "addiu $2,$2,-1168\n"
    "addiu $5,$5,-13664\n"
    "sw $16,12($3)\n"
    "daddu $4,$3,$0\n"
    "sw $2,16($3)\n"
    "sw $5,4($3)\n"
    "sw $18,8($3)\n"
    "jal CreateThread\n"
    "sw $17,20($3)\n"
    "daddu $5,$0,$0\n"
    "sw $2,-17900($20)\n"
    "jal StartThread\n"
    "daddu $4,$2,$0\n"
    "b 2f\n"
    "daddu $2,$19,$0\n"
    "1:\n"
    "daddu $5,$17,$0\n"
    "jal ChangeThreadPriority\n"
    "daddu $19,$0,$0\n"
    "daddu $2,$19,$0\n"
    "2:\n"
    "ld $31,80($sp)\n"
    "ld $20,64($sp)\n"
    "ld $19,48($sp)\n"
    "ld $18,32($sp)\n"
    "ld $17,16($sp)\n"
    "ld $16,0($sp)\n"
    "jr $31\n"
    "addiu $sp,$sp,96\n"
    ".set macro\n"
    ".set reorder\n"
    ".end sceCdInitEeCB\n"
);
