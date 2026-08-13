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
int scWaitParseCntScript(SC_CNT_OBJ *o) { return (--o->field5E < 1); }

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
int scWaitParseMovieScript(void *p) { return func_A33248(p) < 1; }

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
