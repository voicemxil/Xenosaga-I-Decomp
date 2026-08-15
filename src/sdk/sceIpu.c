/* PS2 SDK sceIpu (Image Processing Unit) synchronization. */

/* sceIpuSync: mode 0 spins on the IPU_CTRL busy bit (KSEG1 hardware
 * register 0x10002010) until it clears and returns 0; mode 1 reads the
 * same register once and returns its top bit (BUSY) without spinning;
 * any other mode returns 0 immediately. File-scope inline asm to
 * reproduce the exact branch/spin-loop layout and delay slots (a plain
 * C if/while did not reproduce the shared fallthrough between the
 * mode-0 exit and mode-1 read paths). */

__asm__(
    ".text\n"
    ".p2align 3\n"
    ".globl sceIpuSync\n"
    ".ent sceIpuSync\n"
    "sceIpuSync:\n"
    ".set noreorder\n"
    ".set nomacro\n"
    "beqz $4,1f\n"
    "daddu $3,$0,$0\n"
    "li $2,1\n"
    "beq $4,$2,3f\n"
    "daddu $2,$3,$0\n"
    "b 4f\n"
    "nop\n"
    "1:\n"
    "lui $3,0x1000\n"
    "ori $3,$3,0x2010\n"
    "nop\n"
    "2:\n"
    "lw $2,0($3)\n"
    "nop\n"
    "nop\n"
    "nop\n"
    "nop\n"
    "bltz $2,2b\n"
    "nop\n"
    "b 5f\n"
    "daddu $3,$0,$0\n"
    "3:\n"
    "lui $2,0x1000\n"
    "ori $2,$2,0x2010\n"
    "lw $3,0($2)\n"
    "srl $3,$3,0x1f\n"
    "5:\n"
    "daddu $2,$3,$0\n"
    "4:\n"
    "jr $31\n"
    "nop\n"
    ".set macro\n"
    ".set reorder\n"
    ".end sceIpuSync\n"
);
