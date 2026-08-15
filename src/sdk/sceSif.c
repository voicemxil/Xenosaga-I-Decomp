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

#include "matching.h"

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
/* (path, section name, destination, mode) -- the section name is what
 * sceSifLoadElf below hard-codes to "all". */
extern int _sceSifLoadElfPart(const char *path, const char *sec, void *dest,
                              int mode);
extern int _sceSifLoadModuleBuffer(int a0, int a1, int a2, int a3);
extern int _sceSifLoadModule(int a0, int a1, int a2, int a3, int a4);

int sceSifFreeIopHeap(int a0, int a1)
{
    return sceSifFreeSysMemory(a0, a1);
}

int sceSifLoadElfPart(const char *path, const char *sec, void *dest)
{
    return _sceSifLoadElfPart(path, sec, dest, 1);
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

/* sceSifExitCmd: tears down the SIF cmd layer -- disables the SIF0 DMA
 * channel, removes its handler, and clears the init-check flag.
 * `sif0_handleid` and `_cmd_init_check` are named fixed addresses from
 * config/symbol_addrs.txt.
 *
 * The v0/v1 address-temp difference that had this parked as assembly
 * was not an allocator tie-break: DisableDmac() and RemoveDmacHandler()
 * return the previous state / handler id, so with their true `int`
 * return type the call result occupies v0 and both address temps move
 * to v1 by themselves -- the same wrong-prototype cause as
 * sceCdCallback in sceCd.c. No PIN or LAUNDER needed. */

extern int DisableDmac(int channel);
extern int RemoveDmacHandler(int channel, int handlerid);
extern int sif0_handleid;
extern int _cmd_init_check;

void sceSifExitCmd(void)
{
    DisableDmac(5);
    RemoveDmacHandler(5, sif0_handleid);
    _cmd_init_check = 0;
}

/* sceSifIsAliveIop: bit 0x10000 of SIF register 4 is the "IOP is up"
 * flag.
 *
 * The phrasing is load-bearing. Written as a boolean expression --
 * `return (x & 0x10000) != 0;`, `!!(...)`, or any cast of it -- gcc
 * folds the single-bit test into sra+andi (2 words) and the function is
 * one word short. Written as an explicit branch it keeps the mask in a
 * register (`lui`+`and`) and materialises the boolean with sltu, which
 * is what the original does. `x & 0x10000; return x > 0;` also matches;
 * the branch form is kept because it is the clearer source. */

int sceSifIsAliveIop(void)
{
    if (sceSifGetReg(4) & 0x10000)
        return 1;
    return 0;
}

/* sceSifLoadElf: load every section of an ELF from the IOP-side loader.
 * The "fixed unnamed 0x4D4BE8 constant" that had this parked as
 * assembly is just a string literal: asm/data/cod/2BE280.rodata.s
 * labels 0x004D4BE8 as `.asciz "all"`, the section-name argument
 * _sceSifLoadElfPart takes (compare sceSifLoadElfPart above, which
 * passes the caller's own section name through). */

int sceSifLoadElf(const char *path, void *dest)
{
    return _sceSifLoadElfPart(path, "all", dest, 1);
}

/* sceSifRemoveCmdHandler / sceSifAddCmdHandler: pick one of two SIF
 * command-table pointers out of the fixed low-memory _data_table block
 * (base 0x9918D8, named `_data_table_009918D8` in
 * config/symbol_addrs.txt, same block sceSifSetSysCmdBuffer/
 * sceSifSetCmdBuffer index into above) by the sign of the handler
 * index, then zero (Remove) or store (Add) the selected slot. */

extern int _data_table_009918D8[8];

void sceSifRemoveCmdHandler(int idx)
{
    int off = idx << 3;
    PIN(int base, "$4");

    if (idx < 0)
        base = _data_table_009918D8[3];
    else
        base = _data_table_009918D8[5];
    off = off + base;
    *(int *)off = 0;
}

void sceSifAddCmdHandler(int idx, void (*handler)(void), void *arg)
{
    int off = idx << 3;
    PIN(int base, "$4");
    int *slot;

    if (idx < 0)
        base = _data_table_009918D8[3];
    else
        base = _data_table_009918D8[5];
    off = off + base;
    slot = (int *)off;
    slot[1] = (int)arg;
    slot[0] = (int)handler;
}

/* sceSifSyncIop: reads sceSifGetReg(4) (an IOP sync/status register),
 * and if bit 0x40000 is set, re-inits the debug tty and always returns
 * 1; otherwise returns 0. A plain C if/return body gets GCC's own
 * synthesized epilogue, which already matches (see sceSifExitCmd's
 * comment for why that works once the function is a straight
 * fall-off-the-end/return shape rather than a shared-epilogue branch),
 * so this compiles as real C. */

extern int sceSifGetReg(int a0);
extern void sceResetttyinit(void);

int sceSifSyncIop(void)
{
    if (sceSifGetReg(4) & 0x40000) {
        sceResetttyinit();
        return 1;
    }
    return 0;
}

/* sceSifExitRpc: tears down the SIF cmd layer via sceSifExitCmd, then
 * clears the RPC init-check flag `_sceSifInitCheck`, a named fixed
 * address from config/symbol_addrs.txt -- so this compiles as real C
 * (same reasoning as sceSifExitCmd/sceSifSyncIop above). */

void sceSifExitCmd(void);
extern int _sceSifInitCheck;

void sceSifExitRpc(void)
{
    sceSifExitCmd();
    _sceSifInitCheck = 0;
}

/* sceSifUnloadModule: version-checks the loadfile linkage (_lf_bind /
 * _lf_version), then dispatches an "unload" opcode through the same
 * SIF-RPC boilerplate as the scePad RPC family in scePad.c (fixed
 * low-memory payload block, named `_senddata` in
 * config/symbol_addrs.txt, base 0x994840, and client-data block, named
 * `cd_00994A40`, base 0x994A40), returning the reply on success or a
 * fixed error code otherwise. */

extern int _lf_bind(int a0);
extern int _lf_version(void);
extern int sceSifCallRpc(void *pCd, unsigned int nFno, int nMode,
                          void *pSend, int nSSize, void *pRecv, int nRSize,
                          void *pEndFunc, void *pEndArg);
extern int cd_00994A40;
extern int _senddata;

int sceSifUnloadModule(int modId)
{
    PIN(int id, "$17") = modId;

    if (_lf_bind(id) < 0)
        return (int)0xffff0000;
    if (_lf_version() != 0)
        return (int)0xfffefffc;
    _senddata = id;
    {
        void *pCd = &cd_00994A40;

        LAUNDER_V(pCd);
        if (sceSifCallRpc(pCd, 8, 0, &_senddata, 4, &_senddata, 4, 0, 0) < 0)
            return (int)0xfffeffff;
    }
    return _senddata;
}
