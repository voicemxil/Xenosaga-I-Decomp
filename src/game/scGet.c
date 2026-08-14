/* Script engine register/table accessors (scGet* family) */

extern int _nowScript;

typedef struct {
    unsigned short *pTable;
    int regs[16];
    char pad[0x450 - 4 - 0x40];
} SC_SLOT;

extern SC_SLOT D_0041E7D0[];

int tracePrint(char *fmt, ...);
extern char D_004CC818[];

/* Read a script VM register: reg<0x10 selects the fixed regs[] slots
 * for the current script slot; a set 0x8000 bit re-reads through the
 * indirection; out-of-range indices are logged and read as 0 */
int scGetReg(int reg)
{
    int *adr;
    int val;

    if (reg & 0x8000) {
        reg = scGetReg(reg & 0xFFFF7FFF);
    }
    if ((unsigned int)reg >= 0x10) {
        tracePrint(D_004CC818);
        adr = 0;
    } else {
        adr = &D_0041E7D0[_nowScript].regs[reg];
    }
    val = 0;
    if (adr != 0) {
        val = *adr;
    }
    return val;
}

/* Look up a table address entry: num selects the sub-table, idx the
 * element; a table half-word of 0xFFFF/-1 is treated as "no entry" */
int scGetTableAdrIdx(int num, int idx)
{
    unsigned short *pBase;
    int v;

    pBase = 0;
    if (num != 0) {
        pBase = (unsigned short *)((char *)D_0041E7D0[_nowScript].pTable + num * 2);
    }
    v = pBase[idx];
    if (v == 0xFFFF || v == -1) {
        v = 0;
    }
    return v;
}

/* Resolve a table value from scGetTableAdrIdx into a byte address
 * within the current script slot's table */
void *scGetTableAdrImmIdx(int num, int idx)
{
    int v;
    void *ret;

    v = scGetTableAdrIdx(num, idx);
    ret = 0;
    if (v != 0) {
        ret = (char *)D_0041E7D0[_nowScript].pTable + v * 2;
    }
    return ret;
}

/* Same as scGetTableAdrImmIdx but the value is read directly from a
 * caller-supplied half-word table rather than via scGetTableAdrIdx */
void *scGetImmAdrImmIdx(unsigned short *table, int idx)
{
    int v;
    void *ret;

    v = table[idx];
    if (v == 0xFFFF || v == -1) {
        v = 0;
    }
    ret = 0;
    if (v != 0) {
        ret = (char *)D_0041E7D0[_nowScript].pTable + v * 2;
    }
    return ret;
}

/* Compute base + table[idx]*2 without the script-slot indirection */
void *scGetImmAdrImmIdx2(void *base, unsigned short *table, int idx)
{
    int v;

    v = table[idx];
    if (v == 0xFFFF || v == -1) {
        v = 0;
    }
    if (v == 0) {
        return 0;
    }
    return (char *)base + v * 2;
}

/* Like scGetTableAdrIdx but returns the raw signed half-word (no
 * 0xFFFF/-1 sentinel handling) */
int scGetTableNumIdx(int num, int idx)
{
    short *pBase;

    pBase = 0;
    if (num != 0) {
        pBase = (short *)((char *)D_0041E7D0[_nowScript].pTable + num * 2);
    }
    return pBase[idx];
}

/* Fetch a raw signed half-word directly from a caller-supplied table */
short scGetImmNumIdx(short *adr, int idx)
{
    return adr[idx];
}

typedef struct {
    int unk0[2];
    int cmdBuf[19];
    short field54;
} SCOBJ;

/* Look up an address table entry using an object's own command-slot
 * index (offset 8, selected by the object's field54) */
int scGetAdrIdx(SCOBJ *obj, int idx)
{
    int cmd;
    unsigned short *pBase;
    int v;

    cmd = obj->cmdBuf[obj->field54];
    pBase = 0;
    if (cmd != 0) {
        pBase = (unsigned short *)((char *)D_0041E7D0[_nowScript].pTable + cmd * 2);
    }
    v = pBase[idx];
    if (v == 0xFFFF || v == -1) {
        v = 0;
    }
    return v;
}
