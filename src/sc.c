/* Script engine functions (sc* family, other than the already-matched scGet*
 * accessors in scGet.c). See scGet.c for the SC_SLOT register-table shape
 * this file reuses. */

int scERRORScript(void) { return 0; }
int scEXITScript(void) { return 0; }

extern short _cmdPut;
int scCMDPUTONScript(void) { _cmdPut = 1; return 1; }
int scCMDPUTOFFScript(void) { _cmdPut = 0; return 1; }

typedef struct { char pad[0x5A]; short field5A; } SC_MOVIE_OBJ;
int scWAITMOVIEScript(SC_MOVIE_OBJ *o) { o->field5A = 4; return 2; }
int scWAITMISSILEScript(SC_MOVIE_OBJ *o) { o->field5A = 5; return 2; }

typedef struct { unsigned short flags; } SC_FLAGS_OBJ;
int scPAUSEScript(SC_FLAGS_OBJ *o) { o->flags |= 4; return 1; }
int scFADEONScript(SC_FLAGS_OBJ *o) { o->flags |= 0x100; return 1; }

typedef struct { char pad[0x5E]; short field5E; } SC_CNT_OBJ;
int scWaitParseCntScript(SC_CNT_OBJ *o) {
    int v = o->field5E - 1;
    o->field5E = v;
    return v < 1;
}

typedef struct { unsigned short flags; } SC_MOV_OBJ;
int scWaitParseMovScript(SC_MOV_OBJ *o) { return ((o->flags >> 5) ^ 1) & 1; }

extern int sefSearchMapperIndex(int a);
int scFindScriptData(int a) { return sefSearchMapperIndex(a); }

extern void scExecScript(void);
extern void sefExecScheduler(void);
void scExecEffect(void) { scExecScript(); sefExecScheduler(); }

/* Sentinel idiom: a table half-word of 0xFFFF/-1 means "no entry" */
int scOfsToAdr(int a0) {
    if (a0 == 0xFFFF || a0 == -1) a0 = 0;
    return a0;
}

extern int func_A33248(void *p);
int scWaitParseMovieScript(void *p) { return (unsigned)func_A33248(p) < 1; }

extern int args_jump[2];
extern short D_0033868E[];
extern void Java_xeno_util_Runtime_jump_sub(int a, int b);
void scriptReset_jump(void) {
    int a0 = args_jump[0];
    int a1 = args_jump[1];
    D_0033868E[0] = 0;
    Java_xeno_util_Runtime_jump_sub(a0, a1);
}

typedef struct {
    unsigned short *pTable;
    int regs[16];
    char pad[0x450 - 4 - 0x40];
} SC_SLOT;
extern SC_SLOT D_0041E7D0[];
extern int _nowScript;

/* Resolve a table offset into an absolute address in the current script
 * slot's table; ofs==0 returns NULL. Same 0x450-stride slot table as
 * scGet.c. */
void *scAdrToImm(int ofs) {
    void *v = 0;
    if (ofs != 0) {
        v = (char *)D_0041E7D0[_nowScript].pTable + ofs * 2;
    }
    return v;
}

typedef struct { char pad[0x450]; } SCRIPTWORK_SLOT;
extern SCRIPTWORK_SLOT _scriptWork[];
/* Index arithmetic: (slot<<4)+slot; <<2; +slot; <<2 == slot*0x450, same
 * shape as the SC_SLOT indexing documented in scGet.c */
void *scGetTaskAdr(int slot, int task) {
    if (task < 0) return 0;
    return (char *)&_scriptWork[slot] + task * 128;
}

typedef struct {
    int unk0[2];
    int cmdBuf[19];
    short field54;
    short field56;
    char pad2[0x5A - 0x56 - 2];
    short field5A;
    char pad3[0x60 - 0x5A - 2];
    short field60;
} SCOBJ;

extern int scGetCmdScript(SCOBJ *o);
extern int scGetReg(int reg);
int scRPUTScript(SCOBJ *o) {
    scGetReg(scGetCmdScript(o));
    return 1;
}

extern int scGetNumScript(SCOBJ *o);
int scWAITEVEScript(SCOBJ *o) {
    o->field60 = scGetNumScript(o);
    o->field5A = 2;
    return 2;
}

int scWAITEFTScript(SCOBJ *o) {
    unsigned short v;
    scGetNumScript(o);
    v = o->field56;
    o->field5A = 3;
    o->field60 = v;
    return 2;
}

extern int scGetAdrScript(SCOBJ *o);
int scGOScript(SCOBJ *o) {
    o->cmdBuf[o->field54] = scGetAdrScript(o);
    return 1;
}

extern int scONGOSUB(SCOBJ *o);
int scONGOScript(SCOBJ *o) {
    o->cmdBuf[o->field54] = scONGOSUB(o);
    return 1;
}

extern void scDestroyScript(int idx);
void scDestroyScriptAll(void) {
    int i;
    for (i = 0; i < 16; i++) {
        scDestroyScript(i);
    }
}

/* Same "two genuinely separate compares" sentinel idiom as scGet.c, here
 * applied to an int masked from a call result rather than a raw lhu load */
int scGetAdrScript(SCOBJ *o) {
    int v = scGetCmdScript(o) & 0xFFFF;
    if (v == 0xFFFF || v == -1) {
        v = 0;
    }
    return v;
}

extern void tracePrint(char *fmt, ...);
extern char D_004CC818[];
extern char D_004CC850[];

/* scGetCmdScript: reads the current opcode word for the running task,
 * advancing its cmdBuf[field54] cursor (a halfword index into the
 * current slot's script table) by one each call. */
/* field54 is read twice with two different lvalue types -- once as the
 * naturally-signed `short` (lh, direct sll*4 pointer scale) and once cast
 * through `(unsigned short)` (lhu, needs a sign-extension shift-pair when
 * later scaled for pointer math) -- which defeats CSE and reproduces the
 * original's real lh+lhu pair. */
int scGetCmdScript(SCOBJ *o) {
    short *adr = 0;
    int idx = o->field54;
    int uidx = (unsigned short)o->field54;
    int val = o->cmdBuf[idx];
    if (val != 0) {
        adr = (short *)((char *)D_0041E7D0[_nowScript].pTable + val * 2);
    }
    o->cmdBuf[uidx] = o->cmdBuf[uidx] + 1;
    return *adr;
}

/* Indirect register write: reg with bit 0x8000 set means "the register
 * number is itself stored in register (reg & ~0x8000)". */
void scSetReg(int reg, int val) {
    if ((reg & 0x8000) != 0) {
        reg = scGetReg(reg & 0xFFFF7FFF);
    }
    if ((unsigned int)reg >= 16) {
        tracePrint(D_004CC818, reg);
        return;
    }
    D_0041E7D0[_nowScript].regs[reg] = val;
}

/* scGetNumScript: resolve a script operand -- register indirection if the
 * high bit is set, else a 14-bit unsigned immediate. */
int scGetNumScript(SCOBJ *o) {
    int v1 = scGetCmdScript(o);
    if ((v1 & 0x8000) != 0) {
        if ((v1 & 0x4000) == 0) {
            v1 &= 0x3FFF;
        }
    } else {
        v1 = (short)scGetReg(v1);
    }
    return v1;
}

/* Same D_0041E7D0[_nowScript].pTable + ofs*2 shape as scAdrToImm, using the
 * pending cmdBuf slot as the offset instead of a resolved operand. */
void *scGetCmdAdrScript(SCOBJ *o) {
    void *result = 0;
    int v1 = o->cmdBuf[o->field54];
    if (v1 != 0) {
        result = (char *)D_0041E7D0[_nowScript].pTable + v1 * 2;
    }
    return result;
}

/* Same signed/unsigned double-read idiom as scGetCmdScript: the signed
 * read feeds the compare + cmdBuf pointer scale, the unsigned-cast read
 * feeds the plain (non-scaling) decrement. */
int scRETURNScript(SCOBJ *o) {
    int idx = o->field54;
    int uidx = (unsigned short)o->field54;
    if (idx > 0) {
        o->field54 = uidx - 1;
        o->cmdBuf[idx] = 0;
    } else {
        tracePrint(D_004CC850);
    }
    return 1;
}

void *scGetAdrImmScript(SCOBJ *o) {
    int v1 = scGetAdrScript(o);
    void *result = (void *)v1;
    if (v1 != 0) {
        result = (char *)D_0041E7D0[_nowScript].pTable + v1 * 2;
    }
    return result;
}

int scGetRegScript(SCOBJ *o) {
    int v1 = scGetCmdScript(o);
    int result;
    if ((v1 & 0x8000) != 0) {
        result = (short)scGetReg(v1 & 0xFFFF7FFF);
    } else {
        result = v1;
    }
    return result;
}

/* Same 0x450-stride slot table as SC_SLOT, reaching a short field just past
 * the regs[16] array (offset 0x46) that SC_SLOT's own padding covers. */
typedef struct { char pad[0x46]; short field46; char pad2[0x450 - 0x48]; } SC_SLOT_REVE;
extern SC_SLOT_REVE D_0041E7D0_reve[] __asm__("D_0041E7D0");
int scREVEScript(SCOBJ *o) {
    int reg = scGetCmdScript(o);
    scSetReg(reg, D_0041E7D0_reve[_nowScript].field46);
    return 1;
}

int scRADDScript(SCOBJ *o) {
    int reg = scGetCmdScript(o);
    int a = scGetNumScript(o);
    scSetReg(reg, scGetReg(reg) + a);
    return 1;
}

int scRSUBScript(SCOBJ *o) {
    int reg = scGetCmdScript(o);
    int a = scGetNumScript(o);
    scSetReg(reg, scGetReg(reg) - a);
    return 1;
}

int scR_ANDScript(SCOBJ *o) {
    int reg = scGetCmdScript(o);
    int a = scGetNumScript(o);
    scSetReg(reg, scGetReg(reg) & a);
    return 1;
}

int scR_ORScript(SCOBJ *o) {
    int reg = scGetCmdScript(o);
    int a = scGetNumScript(o);
    scSetReg(reg, scGetReg(reg) | a);
    return 1;
}

int scR_XORScript(SCOBJ *o) {
    int reg = scGetCmdScript(o);
    int a = scGetNumScript(o);
    scSetReg(reg, scGetReg(reg) ^ a);
    return 1;
}

int scRMULScript(SCOBJ *o) {
    int reg = scGetCmdScript(o);
    int a = scGetNumScript(o);
    scSetReg(reg, scGetReg(reg) * a);
    return 1;
}

int scRSETScript(SCOBJ *o) {
    int reg = scGetCmdScript(o);
    int a = scGetNumScript(o);
    scSetReg(reg, a);
    return 1;
}

int scRDECScript(SCOBJ *o) {
    int reg = scGetCmdScript(o);
    scSetReg(reg, scGetReg(reg) - 1);
    return 1;
}

int scRINCScript(SCOBJ *o) {
    int reg = scGetCmdScript(o);
    scSetReg(reg, scGetReg(reg) + 1);
    return 1;
}

int scR_NEGScript(SCOBJ *o) {
    int reg = scGetCmdScript(o);
    scSetReg(reg, -scGetReg(reg));
    return 1;
}

int scR_NOTScript(SCOBJ *o) {
    int reg = scGetCmdScript(o);
    scSetReg(reg, ~scGetReg(reg));
    return 1;
}

typedef struct { char pad[0x5A]; short field5A; char pad2[2]; short field5E; } SC_WAITCNT_OBJ;
int scWAITCNTScript(SC_WAITCNT_OBJ *o) {
    int n = scGetNumScript((SCOBJ *)o);
    if (n <= 0) {
        return 1;
    }
    o->field5A = 1;
    o->field5E = n;
    return 2;
}

extern void scDeleteTaskAll(int a);
extern void scDeleteTask(int a, int b);
extern int _nowEvent;
int scABORTScript(SCOBJ *o) {
    int n = scGetNumScript(o);
    if (n < 0) {
        scDeleteTaskAll(_nowEvent);
    } else {
        scDeleteTask(_nowScript, n);
    }
    return 1;
}

typedef struct { unsigned short flags; char pad[0x44 - 2]; short field44; } SC_AMB_OBJ;
extern int _nowEvent;
extern void sdvSetAmbState(int a, int b);
extern void sdvSetAmbState2(int a, int b);
void scSetAmbient(SC_AMB_OBJ *o) {
    if (_nowEvent == 0) {
        int v = o->field44;
        if ((o->flags & 0x100) != 0) {
            sdvSetAmbState2(1, v);
        } else {
            sdvSetAmbState(1, v);
        }
    }
}

int *scGetRegAdr(int reg) {
    if ((unsigned int)reg >= 16) {
        tracePrint(D_004CC818, reg);
        return 0;
    }
    return &D_0041E7D0[_nowScript].regs[reg];
}

typedef struct { char pad[0x8]; short nTaskCnt; char pad2[0x450 - 0xA]; } SC_SLOT_CNT;
extern SC_SLOT_CNT D_0041E810[];

typedef struct {
    unsigned short flags;   /* 0x00 */
    char pad2[8 - 2];
    int f08;                 /* 0x08 */
    char pad3[0x54 - 0x0C];
    short f54;                /* 0x54 */
} TASK_OBJ;

/* Clear task <task> in script slot <slot>: decrement the slot's active-task
 * counter (offset 0x48 of the SC_SLOT header) if the task was live, then
 * zero the task's flags/field54 and reset its field08 to -1. Same 0x450
 * slot stride, 0x80 task stride as scGetTaskAdr. */
void scDeleteTask(int slot, int task)
{
    TASK_OBJ *p;
    SC_SLOT_CNT *cp;

    if ((unsigned int)task >= 9) {
        return;
    }
    if (task < 0) {
        p = 0;
    } else {
        p = (TASK_OBJ *)((char *)&_scriptWork[slot] + task * 128);
    }
    if (p->flags != 0) {
        cp = &D_0041E810[slot];
        cp->nTaskCnt--;
    }
    p->flags = 0;
    p->f54 = 0;
    p->f08 = -1;
}

extern int _nowScript;
extern short D_0041E426[];
extern int scCreateTask(int nowScript, int a1, void *adr, void *a3);

/* OBJEVE opcode: spawn a sub-task at the resolved address, storing the
 * event's operand value into the per-(slot,task) short table at
 * D_0041E426 (same 0x450/0x80 stride as _scriptWork). Always returns 1. */
int scOBJEVEScript(SCOBJ *o) {
    int n;
    int adr;
    int taskId;

    n = scGetNumScript(o);
    adr = scGetAdrScript(o);
    taskId = scCreateTask(_nowScript, -1, (void *)adr, (char *)o + 0x30);
    if (taskId >= 0) {
        *(short *)(taskId * 128 + (char *)D_0041E426 + _nowScript * 1104) = n;
    }
    return 1;
}

extern int sprintf(char *buf, const char *fmt, ...);

/* RDUMP debug opcode: sprintf all 16 script registers (2 rows of 8) into a
 * scratch buffer via scGetReg; always returns 1 (the buffer itself is
 * write-only/discarded here, matching the original's dead final store). */
int scRDUMPScript(SCOBJ *o) {
    char buf[0x80];
    int pos;
    int row;
    int col;
    int v;

    pos = 0;
    for (row = 0; row < 2; row++) {
        for (col = 0; col < 8; col++) {
            v = scGetReg(row * 8 + col);
            pos += sprintf(buf + pos, "%d ", v);
        }
        buf[0] = 0;
    }
    return 1;
}
