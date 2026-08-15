/* PS2 SDK filesystem driver dispatch wrappers */

extern int _sceCallCode(void *arg0, int code);

int sceRemove(void *path)
{
    return _sceCallCode(path, 6);
}

int sceRmdir(void *path)
{
    return _sceCallCode(path, 8);
}

int sceDelDrv(void *path)
{
    return _sceCallCode(path, 16);
}

int sceChdir(void *path)
{
    return _sceCallCode(path, 18);
}

int sceUmount(void *path)
{
    return _sceCallCode(path, 21);
}

/* sceFsReset: clears a fixed low-memory flag and a fixed low-memory
 * 4-byte buffer (both raw-address, same house issue as sceSif.c), then
 * returns 0. File-scope inline asm both for the raw addresses and
 * because the body ends in its own jr+delay-slot epilogue. */

__asm__(
    ".text\n"
    ".p2align 3\n"
    ".globl sceFsReset\n"
    ".ent sceFsReset\n"
    "sceFsReset:\n"
    ".set noreorder\n"
    ".set nomacro\n"
    "addiu $sp,$sp,-16\n"
    "lui $2,0x4b\n"
    "lui $4,0x99\n"
    "sd $31,0($sp)\n"
    "sw $0,-23752($2)\n"
    "addiu $4,$4,17896\n"
    "daddu $5,$0,$0\n"
    "jal memset\n"
    "li $6,4\n"
    "ld $31,0($sp)\n"
    "daddu $2,$0,$0\n"
    "jr $31\n"
    "addiu $sp,$sp,16\n"
    ".set macro\n"
    ".set reorder\n"
    ".end sceFsReset\n"
);
