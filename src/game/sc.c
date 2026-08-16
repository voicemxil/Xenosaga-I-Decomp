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
    o->cmdBuf[(short)uidx] = o->cmdBuf[(short)uidx] + 1;
    return *adr;
}

/* Indirect register write: reg with bit 0x8000 set means "the register
 * number is itself stored in register (reg & ~0x8000)". */
void scSetReg(int reg, int val) {
    int *p;
    if ((reg & 0x8000) != 0) {
        reg = scGetReg(reg & 0xFFFF7FFF);
    }
    if ((unsigned int)reg >= 16) {
        tracePrint(D_004CC818, reg);
        p = 0;
    } else {
        p = &D_0041E7D0[_nowScript].regs[reg];
    }
    if (p != 0) {
        *p = val;
    }
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
    if (v1 == 0) return 0;
    return (char *)D_0041E7D0[_nowScript].pTable + v1 * 2;
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

typedef struct { unsigned short flags; } SC_TASK_FLAGS;
int scWaitParseEveScript(SCOBJ *o) {
    SC_TASK_FLAGS *p = (SC_TASK_FLAGS *)scGetTaskAdr(_nowScript, o->field60);
    return (p == 0) || (p->flags == 0);
}

extern char D_004CC830[];
int scGOSUBScript(SCOBJ *o) {
    int adr = scGetAdrScript(o);
    unsigned short raw = o->field54;

    if (o->field54 < 3) {
        int idx = raw + 1;
        o->field54 = idx;
        o->cmdBuf[(short)idx] = adr;
    } else {
        tracePrint(D_004CC830);
    }
    return 1;
}

void scDeleteTaskAll(int a)
{
    int i, j;
    for (i = 0; i < 16; i++) {
        if (a < 0 || i == a) {
            for (j = 0; j < 8; j++) {
                scDeleteTask(i, j);
            }
        }
    }
}

int scRDIVScript(SCOBJ *o) {
    int reg = scGetCmdScript(o);
    int a = scGetNumScript(o);
    if (a != 0) {
        scSetReg(reg, scGetReg(reg) / a);
    }
    return 1;
}

int scRMODScript(SCOBJ *o) {
    int reg = scGetCmdScript(o);
    int a = scGetNumScript(o);
    if (a != 0) {
        scSetReg(reg, scGetReg(reg) % a);
    }
    return 1;
}

extern void sefDestroyScriptScheduler2(int slot);
extern char D_004CC7B8[];
/* TODO: near-miss (LOGIC, 6/41 words) -- the original speculatively
   hoists the `move a2,s1` (a1 staged for the tracePrint call) above the
   `bne slot,16` branch test, and similarly reorders the lui/ld/addiu
   setup for the tracePrint call and the stack-restore epilogue; our
   build computes everything in its natural post-branch order. Same
   class as the scGOSUBScript/EtherTreeRightDraw speculative-hoist-
   across-branch wall. Tried: a trailing memory barrier after the
   tracePrint call (this is NOT a tail call in the original -- the
   barrier badly destabilized the whole function, going from 6 to 32+
   diffs; reverted immediately). Leave as-is. */
void scDestroyScript2(int slot, int a1) {
    if ((unsigned int)slot >= 16) {
        tracePrint(D_004CC7B8, slot, a1);
        return;
    }
    sefDestroyScriptScheduler2(slot);
    if (D_0041E7D0[slot].pTable == 0) {
        return;
    }
    scDeleteTask(slot, a1);
}

/* ------------------------------------------------------------------ *
 * Script command dispatch: the per-object wait/move state machines and
 * the opcode handlers that drive them.
 * ------------------------------------------------------------------ */

typedef struct {
    unsigned short flags;      /* 0x00 */
    char pad02[6];
    int cmdBuf[15];            /* 0x08 */
    short eftNo;               /* 0x44 */
    char pad46[14];
    short field54;             /* 0x54 */
    char pad56[4];
    unsigned short waitState;  /* 0x5A */
    unsigned short moveState;  /* 0x5C */
    char pad5E[2];
    short field60;             /* 0x60 */
    char pad62[4];
    short field66;             /* 0x66 */
    short field68;             /* 0x68 */
    char pad6A[6];
    int field70;               /* 0x70 */
} SCTASK;

int scWaitParseNopScript(void) { return 1; }

/* The gosub return-address stack is cmdBuf[]; field54 is its depth. As in
 * scGetCmdScript, field54 is read once signed (for the depth test) and once
 * through (unsigned short) (for the store and the subscript), which is what
 * gives the original's lh+lhu pair rather than a single load. */
int scONGOSUBScript(SCTASK *o)
{
    int adr = scONGOSUB((SCOBJ *)o);
    int depth = o->field54;
    int idx = (unsigned short)o->field54;
    if (depth < 3) {
        idx = idx + 1;
        o->field54 = idx;
        o->cmdBuf[(short)idx] = adr;
    }
    return 1;
}

extern int _nowEvent;
extern short _scMslCate[4];
extern void srsAnalyzeEftNo(short no, void *out, short *cate);

int scMISSILEScript(SCTASK *o)
{
    int work[4];
    int adr = (int)scGetAdrImmScript((SCOBJ *)o);
    if (adr != 0 && _nowEvent == 0) {
        o->field70 = adr;
        o->moveState = 1;
        o->flags |= 0x20;
        o->field66 = 0;
        o->field68 = 0;
        srsAnalyzeEftNo(o->eftNo, work, _scMslCate);
    }
    return 1;
}

/* TODO: near-miss (7/32 words, correct length). One register tie-break
   cascades: the original keeps the resolved address in $a0 (the dying
   incoming argument register) and loads the srsAnalyzeEftNo effect number
   into $a0 only at the call; our build parks the address in $t0, which
   leaves $a0 free and lets the scheduler hoist that `lh` above the store
   block, permuting the five stores. Swept: all store orders around
   field66/field68 (no effect -- the scheduler picks address order either
   way), field70-first vs moveState-first (moveState-first is what fixes
   the LENGTH, keep it), negating the delay in place (17 diffs), a 4-arg
   srsAnalyzeEftNo (wrong -- scMISSILEScript matches with 3 and sets no
   $a3). The tie-break itself is what needs a lever. */
int scMISSILE3Script(SCTASK *o)
{
    int work[4];
    int delay = scGetNumScript((SCOBJ *)o);
    int adr = (int)scGetAdrImmScript((SCOBJ *)o);
    if (adr != 0) {
        short lead = -delay * _nowEvent;
        o->moveState = 1;
        o->field70 = adr;
        o->flags |= 0x20;
        o->field66 = lead;
        o->field68 = 0;
        srsAnalyzeEftNo(o->eftNo, work, _scMslCate);
    }
    return 1;
}

extern int sefIsDeadSchduler(int id);

int scWaitParseEftScript(SCTASK *o)
{
    int done = sefIsDeadSchduler(o->field60);
    if (done != 0) {
        short no = o->eftNo;
        if (o->flags & 0x100) {
            sdvSetAmbState2(2, no);
        } else {
            sdvSetAmbState(2, no);
        }
    }
    return (short)done;
}

extern int scWaitParseCntScript();
extern int scWaitParseEveScript();
extern int scWaitParseMovieScript();
extern int scWaitParseMovScript();
extern int scWaitMissileScript();

/* State out of range resets the machine; otherwise the handler decides.
 * A nonzero handler result clears the "waiting" flag bit and is returned
 * narrowed to short. */
short scWaitParseScript(SCTASK *o)
{
    static int (*waitHandlerTbl[])() = {
        scWaitParseNopScript, scWaitParseCntScript, scWaitParseEveScript,
        scWaitParseEftScript, scWaitParseMovieScript, scWaitParseMovScript,
    };
    int r;
    if (o->waitState >= 6) {
        o->waitState = 0;
        return 1;
    }
    r = waitHandlerTbl[(short)o->waitState](o);
    if (r != 0) {
        o->flags &= 0xFFEF;
    }
    return r;
}

short scMoveParseScript(SCTASK *o)
{
    static int (*moveHandlerTbl[])() = {
        scWaitParseNopScript, scWaitMissileScript,
    };
    int r;
    if (o->moveState >= 2) {
        o->moveState = 0;
        return 1;
    }
    r = moveHandlerTbl[(short)o->moveState](o);
    if (r != 0) {
        o->flags &= 0xFFDF;
    }
    return r;
}

extern int scParseScript(SCTASK *o);

/* Flags==0 means the task is free; bit 2 means suspended. Bit 4 selects the
 * wait state machine, bit 5 the move one; the wait handler may clear bit 4,
 * in which case the parser is skipped for this frame. */
/* TODO: near-miss (1/32 words). Every instruction is identical; the
   original's last test is `beqzl` (branch-likely) where we emit a plain
   `beqz` with the same `li v0,1` in the slot. reorg.c only annuls when
   the slot insn is not redundant on the fall-through path, and both
   builds have an identical `li v0,1` after the scMoveParseScript call.
   Swept: `return 1` inside the if as well as after it, an else-arm with
   a duplicated scParseScript call -- all still beqz. */
int scDispatchScript(SCTASK *o)
{
    unsigned short f = o->flags;
    if (f == 0) {
        return 0;
    }
    if (f & 4) {
        return 4;
    }
    if (f & 0x10) {
        scWaitParseScript(o);
        if (o->flags & 0x10) {
            goto move;
        }
    }
    scParseScript(o);
move:
    if (o->flags & 0x20) {
        scMoveParseScript(o);
    }
    return 1;
}

extern int rand(void);

int scRRNDScript(SCOBJ *o)
{
    int reg = scGetCmdScript(o);
    int x = scGetNumScript(o);
    int y = scGetNumScript(o);
    int v;
    if (y < x) {
        /* v does double duty: the swap temporary here and the random
           value below, so both land in the same register. */
        v = y;
        y = x;
        x = v;
    }
    /* y is reused as the range: one C local is one pseudo, so the range
       inherits the upper bound's callee-saved register instead of needing
       a fourth one (and a copy for the divide-by-zero trap check). */
    y = (short)(y - x);
    v = 0;
    if (y != 0) {
        v = (short)(rand() % y);
    }
    scSetReg(reg, v + x);
    return 1;
}

extern int scGetAdrIdx(SCOBJ *o, int idx);

/* ONGOSUB: a computed branch. The operand count is read first, then the
 * jump-table base; a count in range selects a table entry, otherwise the
 * fall-through address (cursor + base) is taken. Either way the cursor
 * advances past the table. field54 is read signed for the fetch and
 * re-read through (unsigned short) for the store -- the call in between
 * means the original really does load it twice. */
int scONGOSUB(SCOBJ *o)
{
    int *cmd = &o->cmdBuf[0];
    int cnt = scGetNumScript(o);
    int base = scGetCmdScript(o);
    int next = cmd[o->field54] + base;
    int ret = next;
    if (cnt >= 0 && cnt < base) {
        ret = scGetAdrIdx(o, cnt);
    }
    cmd[(short)(unsigned short)o->field54] = next;
    return ret;
}

extern int strlen(const char *s);

/* PRINT: the operand is an inline NUL-terminated string in the script
 * table; the cursor steps over it rounded up to a whole number of
 * half-words, plus the terminator. */
int scPRINTScript(SCOBJ *o)
{
    int *cmd = &o->cmdBuf[0];
    char *s = 0;
    int adr = cmd[o->field54];
    int len;
    int step;
    if (adr != 0) {
        s = (char *)D_0041E7D0[_nowScript].pTable + adr * 2;
    }
    len = strlen(s);
    step = (len & 1) ? len + 1 : len + 2;
    step = step / 2;
    cmd[o->field54] = cmd[o->field54] + step;
    return 1;
}

extern int sefCreateScheduler(void *adr, void *a, void *b, void *tbl, int e);
extern void sefCreateBattleActorTbl(short no);
extern void scFreezeCamera(short no);

/* EFFECT: hand the resolved script address to the effect scheduler; a
 * successful schedule freezes the camera on the effect's own actor. */
int scEFFECTScript(SCOBJ *o)
{
    void *adr = scGetAdrImmScript(o);
    if (adr != 0) {
        int sch = sefCreateScheduler(adr, (char *)o + 0x30, (char *)o + 0x20,
                                     D_0041E7D0[_nowScript].pTable, -1);
        o->field56 = sch;
        sefCreateBattleActorTbl(((SCTASK *)o)->eftNo);
        if (sch >= 0) {
            scSetAmbient((SC_AMB_OBJ *)o);
            scFreezeCamera(((SCTASK *)o)->eftNo);
        }
    }
    return 1;
}
