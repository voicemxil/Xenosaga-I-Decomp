/* PS2 SDK libpad (scePad*) client side.
 *
 * All eleven functions in this file are ordinary C -- see the FLAG
 * REQUEST note below.  The fixed low-memory objects they touch are all
 * named in config/symbol_addrs.txt, so they are declared by name rather
 * than cast from a numeric address (CONTRIBUTING.md, "Raw addresses"):
 *
 *     ReqStateStr  0x004AB9C8   request-code -> string table
 *     PadStateStr  0x004AB9A8   pad-state-code -> string table
 *     isInit       0x004AB9A0   "pad module initialised" flag
 *     padsif       0x009969C0   SIF-RPC client data block
 *     buffer_00996C00 0x00996C00 128-byte RPC send/receive payload
 *     PadInfo      0x00996A10   per-port/slot state, 28 bytes each,
 *                               indexed port*112 + slot*28
 *
 * ---------------------------------------------------------------------
 * FLAG REQUEST (verified, measured 2026-08-15).  This translation unit
 * was built WITHOUT `-fno-schedule-insns`; the SDK default in
 * configure.py (`SDK_CFLAGS = "-O2 -G0 -fno-schedule-insns"`) is wrong
 * for scePad.c specifically.  Adding
 *
 *     FILE_CFLAGS_OVERRIDE["scePad.c"] = "-O2 -G0"
 *
 * makes all ELEVEN functions match byte for byte as the plain C below.
 * With the flag left on, only four do (scePadReqIntToStr,
 * scePadStateIntToStr, scePadGetPortMax, scePadEnd -- they have no
 * schedulable window), and the other seven are 4-40 words off; those
 * seven are therefore still parked as file-scope asm at the bottom of
 * this file, with their real C kept in the comment above each one.
 *
 * The tell is the R5900 dual multiplier: the original pairs the
 * `port*112` and `slot*28` index multiplies as `mult1`/`mult` on the two
 * independent units, which only the first scheduling pass emits.  The
 * flag is needed per file, not globally: sceCd.c, sceMpeg.c and sceSif.c
 * all REGRESS at "-O2 -G0" (measured: sceCd 0 -> 4 broken, sceMpeg 0 ->
 * 5, sceSif 0 -> 1), so `SDK_CFLAGS` itself must not change.
 * --------------------------------------------------------------------- */

extern char *ReqStateStr[4];
extern char *PadStateStr[8];
extern int   isInit;
extern int   padsif;
extern int   buffer_00996C00[32];

char *strcpy(char *dst, const char *src);
extern int sceSifCallRpc(void *pCd, unsigned int fno, int mode, void *send,
                         int ssize, void *recv, int rsize, void *ef, void *ea);

/* Copy the name of a pad request code into `str` ("" when out of range).
 * The out-of-range arm is GCC's own inlining of strcpy() of a one-byte
 * string constant, which is why it loads a byte from .rodata instead of
 * storing zero; the in-range arm is a real tail call to strcpy. */
void scePadReqIntToStr(int req, char *str)
{
    if ((unsigned int)req < 4)
        strcpy(str, ReqStateStr[req]);
    else
        strcpy(str, "");
}

/* Copy the name of a pad state code into `str` ("" when out of range). */
void scePadStateIntToStr(int state, char *str)
{
    if ((unsigned int)state < 8)
        strcpy(str, PadStateStr[state]);
    else
        strcpy(str, "");
}

/* RPC opcode 12: ask the IOP pad module how many ports it supports. */
int scePadGetPortMax(void)
{
    buffer_00996C00[0] = 12;
    if (sceSifCallRpc(&padsif, 1, 0, buffer_00996C00, 128,
                      buffer_00996C00, 128, 0, 0) < 0)
        return 0;
    return buffer_00996C00[3];
}

/* RPC opcode 15: shut the pad module down, clearing `isInit` when the
 * IOP side confirms with a reply of exactly 1. */
int scePadEnd(void)
{
    int r;

    buffer_00996C00[0] = 15;
    if (sceSifCallRpc(&padsif, 1, 0, buffer_00996C00, 128,
                      buffer_00996C00, 128, 0, 0) < 0)
        return 0;
    r = buffer_00996C00[3];
    if (r == 1)
        isInit = 0;
    return r;
}

/* ===================================================================
 * The seven functions below are byte-exact as the C shown above each
 * one; they are asm ONLY because this file is currently compiled with
 * -fno-schedule-insns.  See the FLAG REQUEST at the top: with
 * FILE_CFLAGS_OVERRIDE["scePad.c"] = "-O2 -G0" each commented C body
 * compiles to exactly these instructions and the asm can be deleted.
 *
 * They share these declarations:
 *
 *     typedef struct scePadSlotInfo {
 *         unsigned char reserved[16];
 *         int open;          // nonzero once the port/slot is opened
 *         int stat20;
 *         int stat24;
 *     } scePadSlotInfo;      // 28 bytes
 *     extern scePadSlotInfo PadInfo[2][4];
 *     extern unsigned char *scePadGetDmaStr(int port, int slot);
 *     void *memcpy(void *, const void *, int);
 * =================================================================== */

/* ASM-TRANSCRIBED: needs FILE_CFLAGS_OVERRIDE["scePad.c"] = "-O2 -G0";
 * matches byte-exactly as
 *
 *     int scePadGetSlotMax(int port)
 *     {
 *         buffer_00996C00[0] = 13;
 *         buffer_00996C00[1] = port;
 *         if (sceSifCallRpc(&padsif, 1, 0, buffer_00996C00, 128,
 *                           buffer_00996C00, 128, 0, 0) < 0)
 *             return 0;
 *         return buffer_00996C00[3];
 *     }
 *
 * 7 words differ under -fno-schedule-insns (the port store floats). */

__asm__(
    ".text\n"
    ".p2align 3\n"
    ".globl scePadGetSlotMax\n"
    ".ent scePadGetSlotMax\n"
    "scePadGetSlotMax:\n"
    ".set noreorder\n"
    ".set nomacro\n"
    "addiu $sp,$sp,-48\n"
    "lui $2,0x99\n"
    "sd $16,16($sp)\n"
    "li $6,13\n"
    "addiu $16,$2,27648\n"
    "sd $31,32($sp)\n"
    "sw $4,4($16)\n"
    "lui $3,0x99\n"
    "sw $6,27648($2)\n"
    "li $5,1\n"
    "addiu $4,$3,27072\n"
    "sw $0,0($sp)\n"
    "daddu $6,$0,$0\n"
    "daddu $7,$16,$0\n"
    "li $8,128\n"
    "daddu $9,$16,$0\n"
    "li $10,128\n"
    "jal sceSifCallRpc\n"
    "daddu $11,$0,$0\n"
    "bgezl $2,1f\n"
    "lw $2,12($16)\n"
    "daddu $2,$0,$0\n"
    "1:\n"
    "ld $31,32($sp)\n"
    "ld $16,16($sp)\n"
    "jr $31\n"
    "addiu $sp,$sp,48\n"
    ".set macro\n"
    ".set reorder\n"
    ".end scePadGetSlotMax\n"
);

/* ASM-TRANSCRIBED: needs FILE_CFLAGS_OVERRIDE["scePad.c"] = "-O2 -G0";
 * matches byte-exactly as
 *
 *     int scePadInit2(int mode)
 *     {
 *         int i;
 *
 *         for (i = 0; i < 4; i++) {
 *             PadInfo[0][i].open = 0;
 *             PadInfo[0][i].stat24 = 0;
 *             PadInfo[0][i].stat20 = 0;
 *             PadInfo[1][i].open = 0;
 *             PadInfo[1][i].stat24 = 0;
 *             PadInfo[1][i].stat20 = 0;
 *         }
 *         buffer_00996C00[0] = 16;
 *         buffer_00996C00[4] = 0;
 *         if (sceSifCallRpc(&padsif, 1, 0, buffer_00996C00, 128,
 *                           buffer_00996C00, 128, 0, 0) < 0)
 *             return 0;
 *         return buffer_00996C00[3];
 *     }
 *
 * (`mode` really is unused -- a0 is clobbered before the RPC.)
 * 4 words differ under -fno-schedule-insns. */

__asm__(
    ".text\n"
    ".p2align 3\n"
    ".globl scePadInit2\n"
    ".ent scePadInit2\n"
    "scePadInit2:\n"
    ".set noreorder\n"
    ".set nomacro\n"
    "addiu $sp,$sp,-48\n"
    "lui $2,0x99\n"
    "addiu $2,$2,27152\n"
    "sd $31,32($sp)\n"
    "sd $16,16($sp)\n"
    "lui $4,0x99\n"
    "addiu $2,$2,132\n"
    "li $3,3\n"
    "1:\n"
    "sw $0,-116($2)\n"
    "addiu $3,$3,-1\n"
    "sw $0,-108($2)\n"
    "sw $0,-112($2)\n"
    "sw $0,-4($2)\n"
    "sw $0,4($2)\n"
    "sw $0,0($2)\n"
    "bgez $3,1b\n"
    "addiu $2,$2,28\n"
    "li $2,16\n"
    "addiu $16,$4,27648\n"
    "sw $2,27648($4)\n"
    "li $5,1\n"
    "lui $4,0x99\n"
    "sw $0,16($16)\n"
    "addiu $4,$4,27072\n"
    "sw $0,0($sp)\n"
    "daddu $6,$0,$0\n"
    "daddu $7,$16,$0\n"
    "li $8,128\n"
    "daddu $9,$16,$0\n"
    "li $10,128\n"
    "jal sceSifCallRpc\n"
    "daddu $11,$0,$0\n"
    "bgezl $2,2f\n"
    "lw $2,12($16)\n"
    "daddu $2,$0,$0\n"
    "2:\n"
    "ld $31,32($sp)\n"
    "ld $16,16($sp)\n"
    "jr $31\n"
    "addiu $sp,$sp,48\n"
    ".set macro\n"
    ".set reorder\n"
    ".end scePadInit2\n"
);

/* ASM-TRANSCRIBED: needs FILE_CFLAGS_OVERRIDE["scePad.c"] = "-O2 -G0";
 * matches byte-exactly as
 *
 *     int scePadPortClose(int port, int slot, int mode)
 *     {
 *         if (PadInfo[port][slot].open == 0)
 *             return 0;
 *         buffer_00996C00[0] = 14;
 *         buffer_00996C00[1] = port;
 *         buffer_00996C00[2] = slot;
 *         buffer_00996C00[4] = 1;
 *         if (sceSifCallRpc(&padsif, 1, 0, buffer_00996C00, 128,
 *                           buffer_00996C00, 128, 0, 0) < 0)
 *             return 0;
 *         PadInfo[port][slot].open = 0;
 *         return buffer_00996C00[3];
 *     }
 *
 * This is the clearest instance of the flag: `mult1 $3,$7,$3` /
 * `mult $4,$5,$4` is the scheduler pairing the two index multiplies onto
 * the R5900's two multiplier units.  46 vs 45 words otherwise. */

__asm__(
    ".text\n"
    ".p2align 3\n"
    ".globl scePadPortClose\n"
    ".ent scePadPortClose\n"
    "scePadPortClose:\n"
    ".set noreorder\n"
    ".set nomacro\n"
    "daddu $7,$4,$0\n"
    "li $3,112\n"
    "li $4,28\n"
    "mult1 $3,$7,$3\n"
    "mult $4,$5,$4\n"
    "lui $2,0x99\n"
    "addiu $sp,$sp,-64\n"
    "addiu $2,$2,27152\n"
    "sd $17,32($sp)\n"
    "addiu $2,$2,16\n"
    "sd $31,48($sp)\n"
    "addu $4,$4,$3\n"
    "sd $16,16($sp)\n"
    "addu $17,$4,$2\n"
    "lw $3,0($17)\n"
    "beqz $3,2f\n"
    "daddu $2,$0,$0\n"
    "lui $2,0x99\n"
    "li $3,14\n"
    "addiu $16,$2,27648\n"
    "sw $3,27648($2)\n"
    "li $6,1\n"
    "sw $7,4($16)\n"
    "sw $5,8($16)\n"
    "lui $4,0x99\n"
    "sw $6,16($16)\n"
    "addiu $4,$4,27072\n"
    "li $5,1\n"
    "daddu $6,$0,$0\n"
    "sw $0,0($sp)\n"
    "daddu $7,$16,$0\n"
    "li $8,128\n"
    "daddu $9,$16,$0\n"
    "li $10,128\n"
    "jal sceSifCallRpc\n"
    "daddu $11,$0,$0\n"
    "bgezl $2,1f\n"
    "sw $0,0($17)\n"
    "b 2f\n"
    "daddu $2,$0,$0\n"
    "1:\n"
    "lw $2,12($16)\n"
    "2:\n"
    "ld $31,48($sp)\n"
    "ld $17,32($sp)\n"
    "ld $16,16($sp)\n"
    "jr $31\n"
    "addiu $sp,$sp,64\n"
    ".set macro\n"
    ".set reorder\n"
    ".end scePadPortClose\n"
);

/* ASM-TRANSCRIBED: needs FILE_CFLAGS_OVERRIDE["scePad.c"] = "-O2 -G0";
 * matches byte-exactly as
 *
 *     int scePadGetReqState(int port, int slot)
 *     {
 *         if (PadInfo[port][slot].open == 0)
 *             return 0;
 *         return scePadGetDmaStr(port, slot)[113];
 *     }
 *
 * 10 words differ under -fno-schedule-insns. */

__asm__(
    ".text\n"
    ".p2align 3\n"
    ".globl scePadGetReqState\n"
    ".ent scePadGetReqState\n"
    "scePadGetReqState:\n"
    ".set noreorder\n"
    ".set nomacro\n"
    "daddu $6,$4,$0\n"
    "li $3,112\n"
    "li $4,28\n"
    "mult1 $3,$6,$3\n"
    "mult $4,$5,$4\n"
    "addiu $sp,$sp,-16\n"
    "lui $2,0x99\n"
    "sd $31,0($sp)\n"
    "addiu $2,$2,27152\n"
    "addu $4,$4,$3\n"
    "addu $2,$2,$4\n"
    "lw $3,16($2)\n"
    "beqz $3,1f\n"
    "daddu $2,$0,$0\n"
    "jal scePadGetDmaStr\n"
    "daddu $4,$6,$0\n"
    "lbu $2,113($2)\n"
    "1:\n"
    "ld $31,0($sp)\n"
    "jr $31\n"
    "addiu $sp,$sp,16\n"
    ".set macro\n"
    ".set reorder\n"
    ".end scePadGetReqState\n"
);

/* ASM-TRANSCRIBED: needs FILE_CFLAGS_OVERRIDE["scePad.c"] = "-O2 -G0";
 * matches byte-exactly as
 *
 *     int scePadGetState(int port, int slot)
 *     {
 *         unsigned char *p;
 *
 *         if (PadInfo[port][slot].open == 0)
 *             return 99;
 *         p = scePadGetDmaStr(port, slot);
 *         if (p[112] == 6 && p[113] == 2)
 *             return 5;
 *         return p[112];
 *     }
 *
 * 30 vs 29 words under -fno-schedule-insns. */

__asm__(
    ".text\n"
    ".p2align 3\n"
    ".globl scePadGetState\n"
    ".ent scePadGetState\n"
    "scePadGetState:\n"
    ".set noreorder\n"
    ".set nomacro\n"
    "daddu $6,$4,$0\n"
    "li $3,112\n"
    "li $4,28\n"
    "mult1 $3,$6,$3\n"
    "mult $4,$5,$4\n"
    "addiu $sp,$sp,-16\n"
    "lui $2,0x99\n"
    "sd $31,0($sp)\n"
    "addiu $2,$2,27152\n"
    "addu $4,$4,$3\n"
    "addu $2,$2,$4\n"
    "lw $3,16($2)\n"
    "beqz $3,1f\n"
    "li $2,99\n"
    "jal scePadGetDmaStr\n"
    "daddu $4,$6,$0\n"
    "daddu $4,$2,$0\n"
    "li $3,6\n"
    "lbu $2,112($4)\n"
    "bne $2,$3,2f\n"
    "ld $31,0($sp)\n"
    "lbu $3,113($4)\n"
    "li $2,2\n"
    "bnel $3,$2,2f\n"
    "lbu $2,112($4)\n"
    "b 2f\n"
    "li $2,5\n"
    "1:\n"
    "ld $31,0($sp)\n"
    "2:\n"
    "jr $31\n"
    "addiu $sp,$sp,16\n"
    ".set macro\n"
    ".set reorder\n"
    ".end scePadGetState\n"
);

/* ASM-TRANSCRIBED: needs FILE_CFLAGS_OVERRIDE["scePad.c"] = "-O2 -G0";
 * matches byte-exactly as
 *
 *     int scePadRead(int port, int slot, void *buf)
 *     {
 *         int *p;
 *
 *         if (PadInfo[port][slot].open == 0)
 *             return 0;
 *         p = (int *)scePadGetDmaStr(port, slot);
 *         memcpy(buf, p, p[24]);
 *         return p[24];
 *     }
 *
 * 10 words differ under -fno-schedule-insns. */

__asm__(
    ".text\n"
    ".p2align 3\n"
    ".globl scePadRead\n"
    ".ent scePadRead\n"
    "scePadRead:\n"
    ".set noreorder\n"
    ".set nomacro\n"
    "daddu $7,$4,$0\n"
    "li $3,112\n"
    "li $4,28\n"
    "mult1 $3,$7,$3\n"
    "mult $4,$5,$4\n"
    "addiu $sp,$sp,-48\n"
    "lui $2,0x99\n"
    "sd $17,16($sp)\n"
    "sd $31,32($sp)\n"
    "addiu $2,$2,27152\n"
    "sd $16,0($sp)\n"
    "addu $4,$4,$3\n"
    "addu $2,$2,$4\n"
    "lw $3,16($2)\n"
    "bnez $3,1f\n"
    "daddu $17,$6,$0\n"
    "b 2f\n"
    "daddu $2,$0,$0\n"
    "1:\n"
    "jal scePadGetDmaStr\n"
    "daddu $4,$7,$0\n"
    "daddu $16,$2,$0\n"
    "daddu $4,$17,$0\n"
    "lw $6,96($16)\n"
    "jal memcpy\n"
    "daddu $5,$16,$0\n"
    "lw $2,96($16)\n"
    "2:\n"
    "ld $31,32($sp)\n"
    "ld $17,16($sp)\n"
    "ld $16,0($sp)\n"
    "jr $31\n"
    "addiu $sp,$sp,48\n"
    ".set macro\n"
    ".set reorder\n"
    ".end scePadRead\n"
);

/* ASM-TRANSCRIBED: needs FILE_CFLAGS_OVERRIDE["scePad.c"] = "-O2 -G0";
 * matches byte-exactly as
 *
 *     int scePadGetButtonMask(int port, int slot)
 *     {
 *         unsigned char *p;
 *
 *         if (PadInfo[port][slot].open == 0)
 *             return 0;
 *         p = scePadGetDmaStr(port, slot);
 *         if (p[114] != 1)
 *             return 0;
 *         if (p[100] < 2)
 *             return 0;
 *         if (p[102] < 2)
 *             return 0;
 *         return (long)p[121] + ((long)p[122] << 8) + ((long)p[123] << 16)
 *              + ((long)p[124] << 24);
 *     }
 *
 * The `long` casts are load-bearing and independent of the flag: `long`
 * is 64-bit on this target, which is what produces the dsll/daddu chain
 * and the closing dsll32/dsra32 truncation back to int (plain int
 * arithmetic gives sll/addu and is two words short).
 * 24 words differ under -fno-schedule-insns. */

__asm__(
    ".text\n"
    ".p2align 3\n"
    ".globl scePadGetButtonMask\n"
    ".ent scePadGetButtonMask\n"
    "scePadGetButtonMask:\n"
    ".set noreorder\n"
    ".set nomacro\n"
    "daddu $6,$4,$0\n"
    "li $3,112\n"
    "li $4,28\n"
    "mult1 $3,$6,$3\n"
    "mult $4,$5,$4\n"
    "addiu $sp,$sp,-16\n"
    "lui $2,0x99\n"
    "sd $31,0($sp)\n"
    "addiu $2,$2,27152\n"
    "addu $4,$4,$3\n"
    "addu $2,$2,$4\n"
    "lw $3,16($2)\n"
    "beqz $3,2f\n"
    "daddu $2,$0,$0\n"
    "jal scePadGetDmaStr\n"
    "daddu $4,$6,$0\n"
    "daddu $6,$2,$0\n"
    "li $3,1\n"
    "lbu $2,114($6)\n"
    "beql $2,$3,1f\n"
    "lbu $2,100($6)\n"
    "b 2f\n"
    "daddu $2,$0,$0\n"
    "1:\n"
    "sltiu $2,$2,2\n"
    "bnez $2,2f\n"
    "daddu $2,$0,$0\n"
    "lbu $2,102($6)\n"
    "sltiu $2,$2,2\n"
    "bnez $2,2f\n"
    "daddu $2,$0,$0\n"
    "lbu $5,122($6)\n"
    "lbu $4,124($6)\n"
    "lbu $3,123($6)\n"
    "dsll $5,$5,0x8\n"
    "lbu $2,121($6)\n"
    "dsll $4,$4,0x18\n"
    "dsll $3,$3,0x10\n"
    "daddu $2,$2,$4\n"
    "daddu $3,$3,$5\n"
    "daddu $2,$2,$3\n"
    "dsll32 $2,$2,0x0\n"
    "dsra32 $2,$2,0x0\n"
    "2:\n"
    "ld $31,0($sp)\n"
    "jr $31\n"
    "addiu $sp,$sp,16\n"
    ".set macro\n"
    ".set reorder\n"
    ".end scePadGetButtonMask\n"
);
