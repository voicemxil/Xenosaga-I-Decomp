/* PS2 SDK sceCd (CD/DVD filesystem) thin wrappers. */

#include "matching.h"

extern int sceCdLayerSearchFile(int a0, int a1, int a2);

int sceCdSearchFile(int a0, int a1)
{
    return sceCdLayerSearchFile(a0, a1, 0);
}

/* sceCdGetReadPos: if the streaming-callback-installed flag reads 1
 * (streaming active), returns the value at the streaming read-position
 * variable through its uncached-accelerated alias (bit 0x20000000 set,
 * EE main-memory uncached window -- needed because the value is written
 * by IOP-side DMA); otherwise returns 0. Both variables are named fixed
 * addresses from config/symbol_addrs.txt, so referencing them by name
 * lets the linker's normal %hi/%lo relocation produce lui+addiu, same
 * as the original -- no inline asm needed. */

extern int sceCdCbfunc_num;
extern int _sceCd_Read_cur_pos;

int sceCdGetReadPos(void)
{
    PIN(int flag, "$3") = sceCdCbfunc_num;

    if (flag == 1)
        return *(volatile int *)((int)&_sceCd_Read_cur_pos | 0x20000000);
    return 0;
}

/* sceCdSt* streaming-control family: thin dispatchers into sceCdStream,
 * a 5-arg internal entry point (EE's register calling convention passes
 * up to 8 int args in $4-$11, so a 5th int argument naturally lands in
 * $8/t0 -- no different from a0-a3), each hard-coding a different opcode
 * in the 4th argument and either forwarding or zeroing the first three.
 * Some also flip the streaming-active flag `stm_status` and/or pass the
 * address of the fixed streaming-parameter block `dum_mode` as the 5th
 * arg. Both are named fixed addresses from config/symbol_addrs.txt, so
 * referencing them by name (rather than as raw numeric-cast addresses)
 * lets the linker's ordinary %hi/%lo relocation produce lui+addiu, same
 * as the original -- real C, no inline asm needed. */

extern int sceCdStream(int a0, int a1, int a2, int a3, int a4);
extern int stm_status;
extern int dum_mode;

int sceCdStInit(int a0, int a1, int a2)
{
    int mode;

    stm_status = 0;
    mode = (int)&dum_mode;
    LAUNDER_V(mode);
    return sceCdStream(a0, a1, a2, 5, mode);
}

int sceCdStSeek(int a0)
{
    int mode = (int)&dum_mode;

    LAUNDER_V(mode);
    return sceCdStream(a0, 0, 0, 4, mode);
}

int sceCdStSeekF(int a0)
{
    int mode = (int)&dum_mode;

    LAUNDER_V(mode);
    return sceCdStream(a0, 0, 0, 9, mode);
}

/* sceCdStStart: near-miss in real C -- every phrasing tried reproduces
 * the original's register choices and opcodes exactly (including the
 * a1->t0 forwarding as `daddu`, confirmed only visible through the real
 * `as -march=r5900 -mabi=eabi` pipeline; ee-gcc's own internal assembler
 * emits `or` for the same "move" text and looked like a logic
 * difference until checked against the pipeline) but the post-RA
 * delay-slot/scheduling pass keeps placing the `sw` (stm_status store)
 * one slot earlier than the original, swapped with the `a1=0` move --
 * a pure instruction-order tie-break, not a logic difference. Parked as
 * inline asm pending a scheduling lever that controls this; see
 * sceCdStInit/StSeek/StSeekF/StStop above for the same dispatcher shape
 * successfully written as real C. */

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

int sceCdStStop(void)
{
    int mode;

    stm_status = 0;
    mode = (int)&dum_mode;
    LAUNDER_V(mode);
    return sceCdStream(0, 0, 0, 3, mode);
}

/* sceCdCallback: installs a new EE-side CD event callback and returns
 * the previous one, guarding the swap with DIntr/EIntr and bailing out
 * early (returning 0, no swap) if sceCdSync(1) reports the drive still
 * busy. `sceCdCbfunc` is a named fixed address from
 * config/symbol_addrs.txt (same trick as the sceCdSt* family above), so
 * most of this compiles as real C -- every phrasing tried reproduces
 * the original's instruction count, opcodes, and shape exactly except
 * one address-temp register (v0 vs v1, a pure allocator tie-break, not
 * a logic difference). PIN only sticks when the pinned variable itself
 * appears as an asm operand (an ordinary use, e.g. a plain dereference
 * or a call argument that happens to land in that register by calling
 * convention -- see sceDeci2ExRecv/ExSend in sceDeci2.c for a case
 * where that coincided) -- forcing it here via LAUNDER after the
 * assignment makes the pin stick but defeats the %hi/%lo folding into
 * the lw/sw offset, adding a real extra addiu; PIN-ing the initializer
 * directly (`= &sceCdCbfunc` on the declaration) compiles but silently
 * drops the lui across the branch, a miscompile, not merely a
 * mismatch. Parked as inline asm pending a lever that shifts this one
 * tie-break without either side effect. */

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
