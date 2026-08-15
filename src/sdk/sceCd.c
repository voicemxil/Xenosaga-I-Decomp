/* PS2 SDK sceCd (CD/DVD filesystem) thin wrappers.
 *
 * ---------------------------------------------------------------------
 * FLAG REQUEST (verified, measured 2026-08-15).  Like scePad.c, this
 * translation unit was built WITHOUT `-fno-schedule-insns`.  Adding
 *
 *     FILE_CFLAGS_OVERRIDE["sceCd.c"] = "-O2 -G0"
 *
 * makes ALL NINE functions here match as plain C, and it lets the four
 * `LAUNDER_V(mode)` steering constructs in sceCdStInit/StSeek/StSeekF/
 * StStop be deleted: they exist only to fake, under the wrong flag, a
 * schedule the real flag produces for free.  Measured at "-O2 -G0" with
 * those four LAUNDER_V lines removed: 9 match, 0 not.  Under the current
 * flags two functions (sceCdStStart, sceCdInitEeCB) cannot be written as
 * C at all and stay parked as asm below.
 *
 * The flag must be per file: sceSif.c regresses at "-O2 -G0"
 * (sceSifAddCmdHandler goes 0 -> 2 scheduling diffs), and sceMpeg.c /
 * mpeg.c / sceVif1Pk.c are unaffected either way.  Do not change
 * SDK_CFLAGS itself.
 * --------------------------------------------------------------------- */

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

/* ASM-TRANSCRIBED: needs FILE_CFLAGS_OVERRIDE["sceCd.c"] = "-O2 -G0"
 * (see the FLAG REQUEST at the top of this file).  Byte-exact as
 *
 *     int sceCdStStart(int a0, int a1)
 *     {
 *         stm_status = 1;
 *         return sceCdStream(a0, 0, 0, 1, a1);
 *     }
 *
 * -- the same dispatcher shape as sceCdStInit/StSeek/StSeekF/StStop
 * above.  Under -fno-schedule-insns the `sw` of stm_status lands one
 * slot earlier than the original (3 words differ); with the scheduler
 * enabled it matches with no steering constructs at all. */

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
 * config/symbol_addrs.txt (same trick as the sceCdSt* family above).
 *
 * The one thing that had this parked as assembly was the address temp
 * landing in v0 where the original uses v1. It is not an allocator
 * tie-break at all: DIntr() and EIntr() really do RETURN the previous
 * interrupt-enable state (`int DIntr(void)`, not `void`). Declared with
 * their true return type, the call clobbers v0 with a result the
 * allocator must route around, and the address temp moves to v1 on its
 * own -- no PIN, no LAUNDER. A wrong extern prototype, not a compiler
 * quirk. */

extern int sceCdSync(int mode);
extern int DIntr(void);
extern int EIntr(void);
extern void *sceCdCbfunc;

void *sceCdCallback(void *func)
{
    void *old;

    if (sceCdSync(1) != 0)
        return 0;
    DIntr();
    old = sceCdCbfunc;
    sceCdCbfunc = func;
    EIntr();
    return old;
}

/* ASM-TRANSCRIBED: needs FILE_CFLAGS_OVERRIDE["sceCd.c"] = "-O2 -G0"
 * (see the FLAG REQUEST at the top of this file).  Byte-exact as
 *
 *     typedef struct sceThreadParam {
 *         int   status;
 *         void *entry;
 *         void *stack;
 *         int   stackSize;
 *         void *gpReg;
 *         int   initPriority;
 *         int   currentPriority;
 *         int   attr;
 *         int   option;
 *     } sceThreadParam;
 *
 *     extern int cb_thid;             // 0x004ABA14
 *     extern int my_thid;             // 0x00996C90
 *     extern int my_th_info;          // 0x00996C98
 *     extern sceThreadParam cb_tp;    // 0x00996CC8
 *     extern void _Cdvd_cbLoop(void); // 0x0020CAA0
 *     extern int _gp;                 // 0x004DFB70, from the link script
 *
 *     int sceCdInitEeCB(int prio, void *stack, int stackSize)
 *     {
 *         int r = 1;
 *
 *         if (cb_thid == 0) {
 *             my_thid = GetThreadId();
 *             ReferThreadStatus(my_thid, &my_th_info);
 *             cb_tp.entry = (void *)_Cdvd_cbLoop;
 *             cb_tp.stack = stack;
 *             cb_tp.stackSize = stackSize;
 *             cb_tp.gpReg = &_gp;
 *             cb_tp.initPriority = prio;
 *             cb_thid = CreateThread(&cb_tp);
 *             StartThread(cb_thid, 0);
 *         } else {
 *             ChangeThreadPriority(cb_thid, prio);
 *             r = 0;
 *         }
 *         return r;
 *     }
 *
 * Lazily spins up the EE-side CD event thread the first time it is
 * called (`cb_thid` doubles as the "already running" guard); later
 * calls only re-prioritise the existing thread.  Note the `+16` field
 * really is the callee's $gp: 0x004DFB70 is exactly the link script's
 * `_gp = cod_LIT4_START + 0x7FF0`, which is what identified the struct
 * as a stock sceThreadParam.
 *
 * `int r = 1;` is load-bearing and flag-independent: written as
 * `return 1;` / `return 0;` in the two arms, gcc materialises the
 * constants at the returns and drops a callee-saved register, giving a
 * 48-word function against the original's 53.  Under
 * -fno-schedule-insns this is 52 words vs 53 -- the scheduler is what
 * groups the three lui/addiu pairs for cb_tp/_gp/_Cdvd_cbLoop and keeps
 * the cb_tp base in v1 with a separate `move a0,v1` for the call. */

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
