/* PS2 SDK sceCd (CD/DVD filesystem) thin wrappers.
 *
 * ---------------------------------------------------------------------
 * BUILD FLAG.  Like scePad.c, this translation unit was built WITHOUT
 * `-fno-schedule-insns`; configure.py carries
 * FILE_CFLAGS_OVERRIDE["sceCd.c"] = "-O2 -G0".  All eleven functions
 * here are plain C under it, including sceCdStStart and sceCdInitEeCB,
 * which could not be written as C at all under the old flags.  The four
 * `LAUNDER_V(mode)` steering constructs that used to sit in
 * sceCdStInit/StSeek/StSeekF/StStop are gone with it: they existed only
 * to fake, under the wrong flag, the schedule the right flag produces
 * for free.
 * ---------------------------------------------------------------------
 */

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
    return sceCdStream(a0, a1, a2, 5, mode);
}

int sceCdStSeek(int a0)
{
    int mode = (int)&dum_mode;

    return sceCdStream(a0, 0, 0, 4, mode);
}

int sceCdStSeekF(int a0)
{
    int mode = (int)&dum_mode;

    return sceCdStream(a0, 0, 0, 9, mode);
}

/* Was hand-transcribed asm while this TU was built with
 * -fno-schedule-insns; with -O2 -G0 the plain C matches. */
int sceCdStStart(int a0, int a1)
{
    stm_status = 1;
    return sceCdStream(a0, 0, 0, 1, a1);
}

int sceCdStStop(void)
{
    int mode;

    stm_status = 0;
    mode = (int)&dum_mode;
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

/* Was hand-transcribed asm while this TU was built with
 * -fno-schedule-insns; with -O2 -G0 the plain C matches. */
typedef struct sceThreadParam {
    int   status;
    void *entry;
    void *stack;
    int   stackSize;
    void *gpReg;
    int   initPriority;
    int   currentPriority;
    int   attr;
    int   option;
} sceThreadParam;

extern int cb_thid;                 /* 0x004ABA14, doubles as the "running" guard */
extern int my_thid;                 /* 0x00996C90 */
extern int my_th_info;              /* 0x00996C98 */
extern sceThreadParam cb_tp;        /* 0x00996CC8 */
extern void _Cdvd_cbLoop(void);     /* 0x0020CAA0 */
extern int _gp;                     /* 0x004DFB70, from the link script */
extern int GetThreadId(void);
extern int ReferThreadStatus(int thid, void *info);
extern int CreateThread(sceThreadParam *param);
extern int StartThread(int thid, void *arg);
extern int ChangeThreadPriority(int thid, int prio);

int sceCdInitEeCB(int prio, void *stack, int stackSize)
{
    int r = 1;

    if (cb_thid == 0) {
        my_thid = GetThreadId();
        ReferThreadStatus(my_thid, &my_th_info);
        cb_tp.stackSize = stackSize;
        cb_tp.gpReg = &_gp;
        cb_tp.entry = (void *)_Cdvd_cbLoop;
        cb_tp.stack = stack;
        cb_tp.initPriority = prio;
        cb_thid = CreateThread(&cb_tp);
        StartThread(cb_thid, 0);
    } else {
        ChangeThreadPriority(cb_thid, prio);
        r = 0;
    }
    return r;
}

/* ------------------------------------------------------------------
 * Synchronous command waiting.
 * ------------------------------------------------------------------ */

typedef struct sceSemaParam {
    int currentCount;
    int maxCount;
    int initCount;
    int numWaitThreads;
    int attr;
    int option;
} sceSemaParam;

extern int CreateSema(sceSemaParam *param);
extern int DeleteSema(int sid);
extern int WaitSema(int sid);
extern int SetAlarm(unsigned short time, void *handler, void *arg);
extern void CB_DelayTh(int id, unsigned short time, void *common);
extern int scePrintf(const char *fmt, ...);
extern int sceSifCheckStatRpc(void *rd);
extern int SCE_CD_debug;
/* libcdvd's SIF-RPC client-data block for the "S" (synchronous)
 * command channel, 0x004AD448. */
typedef struct sceCdRpcClient {
    int pad[9];                 /*  0..35 */
    void *serve;                /* 36  nonzero once the IOP side answers */
} sceCdRpcClient;

extern sceCdRpcClient _sceCd_cd_scmd;

/* NEAR-MISS, not registered: matches byte for byte at "-O2 -G0" (26
 * words) but is 7 positions out of order under -fno-schedule-insns.
 * One more instance of the flag request at the top of this file --
 * there is nothing wrong with the C.
 *
 * Sleep the calling thread for `usec` microseconds by parking on a
 * fresh semaphore that the alarm callback signals. */
void sceCdDelayThread(unsigned short usec)
{
    sceSemaParam sp;
    int sid;

    sp.maxCount = 1;
    sp.initCount = 0;
    sp.option = 0;
    sid = CreateSema(&sp);
    SetAlarm(usec, (void *)CB_DelayTh, (void *)sid);
    WaitSema(sid);
    DeleteSema(sid);
}

/* sceCdSyncS: mode 0 blocks until the drive's RPC finishes, polling
 * every 60us; any other mode just reports whether it is still busy.
 *
 * Testing `mode == 0` (not `mode != 0`) is load-bearing: it keeps the
 * blocking path as the fall-through, which is the branch polarity the
 * original has. Inverted, gcc lays the one-shot poll out first and the
 * function is a word longer. */
int sceCdSyncS(int mode)
{
    if (mode == 0) {
        if (SCE_CD_debug > 0)
            scePrintf("S cmd wait\n");
        while (sceSifCheckStatRpc(&_sceCd_cd_scmd) != 0)
            sceCdDelayThread(60);
        return 0;
    }
    return sceSifCheckStatRpc(&_sceCd_cd_scmd);
}

/* sceCdStPause / sceCdStResume / sceCdStStat: three more members of the
 * sceCdSt* dispatcher family above (opcodes 7, 8 and 6), differing only
 * in whether they touch `stm_status` first.  Unlike the earlier five
 * these carry a debug trace, gated on SCE_CD_debug the same way
 * sceCdSyncS gates "S cmd wait\n". */

extern int SCE_CD_debug;

int sceCdStPause(void)
{
    int mode;

    stm_status = 0;
    if (SCE_CD_debug > 0)
        scePrintf("sceCdStPause call\n");
    mode = (int)&dum_mode;
    return sceCdStream(0, 0, 0, 7, mode);
}

int sceCdStResume(void)
{
    int mode;

    stm_status = 1;
    if (SCE_CD_debug > 0)
        scePrintf("sceCdStResume call\n");
    mode = (int)&dum_mode;
    return sceCdStream(0, 0, 0, 8, mode);
}

int sceCdStStat(void)
{
    int mode;

    if (SCE_CD_debug > 0)
        scePrintf("sceCdStStat call\n");
    mode = (int)&dum_mode;
    return sceCdStream(0, 0, 0, 6, mode);
}

/* ------------------------------------------------------------------
 * sceCdSync: is the N-command channel still busy?
 *
 * mode 0 blocks (polling every 60us) and always returns 0; any other
 * mode is a one-shot poll returning 1 while busy.  "Busy" is two
 * separate tests -- the EE-side callback semaphore flag and the SIF RPC
 * status of the N-command client block -- written as one `||` so both
 * branches target the same loop top, which is what the original does.
 *
 * The blocking arm is a ROTATED loop (enter at the bottom test), so the
 * two %hi address temps stay hoisted in s0/s1 across the whole loop.
 * ------------------------------------------------------------------ */

/* PARKED NEAR-MISS, 3 words of 40. The mode==0 arm -- the plain `while`
 * loop with the two-test `||`, the hoisted s0/s1 %hi temps, the entry
 * branch into the bottom test -- is EXACT (the goto-loop spelling
 * duplicates the test block here; the ordinary `while` is what matches).
 * The residue is entirely the one-shot poll: the original emits
 * `bnez v0, done` with `li v0,1` in the delay slot and a `move v0,zero`
 * fall-through, where 2.9 folds our second test to `sltu v0,zero,v0`.
 * Swept without success: two explicit `return 1;`s (sltu), a shared
 * `busy:` label (bnezl + an extra move/beqz, 41 words), single-exit
 * `r = 1; ... r = 0;` and the `&&` form (both give `movz s0,zero,v0`),
 * LAUNDER on the call result (folded away, still sltu). Same class as
 * the other v0-constant tie-breaks -- permuter or a pin, not a shape. */

extern int _sceCd_c_cb_sem;
extern sceCdRpcClient _sceCd_cd_ncmd;

int sceCdSync(int mode)
{
    int r;

    if (mode == 0) {
        if (SCE_CD_debug > 0)
            scePrintf("N cmd wait\n");
        while (_sceCd_c_cb_sem != 0
               || sceSifCheckStatRpc(&_sceCd_cd_ncmd) != 0)
            sceCdDelayThread(60);
        return 0;
    }
    if (_sceCd_c_cb_sem != 0)
        return 1;
    r = sceSifCheckStatRpc(&_sceCd_cd_ncmd);
    if (r != 0)
        return 1;
    return 0;
}


/* ------------------------------------------------------------------
 * The S-command reply template.
 *
 * sceCdGetDiskType and sceCdGetError are the same nine-argument
 * sceSifCallRpc call with a different command number, differing only in
 * the prechk opcode, the RPC function number and the value returned on
 * failure.  The reply lands in the fixed 4-byte buffer
 * `_sceCd_scmdrdata`, which the IOP writes by DMA -- so it is read back
 * through the uncached-accelerated alias (| 0x20000000), exactly like
 * sceCdGetReadPos above.
 *
 * The read sits in the delay slot of the SignalSema call, i.e. the
 * value is fetched BEFORE the semaphore is released; a local holds it
 * across the call in s0.
 * ------------------------------------------------------------------ */

extern int _sceCd_scmd_prechk(int cmd);
extern int _sceCd_scmdrdata;
extern int _sceCd_scmd_semid;
extern int SignalSema(int sid);

int sceCdGetDiskType(void)
{
    int *p;
    int r;

    if (_sceCd_scmd_prechk(1) == 0)
        return 0;
    p = &_sceCd_scmdrdata;
    if (sceSifCallRpc(&_sceCd_cd_scmd, 3, 0, 0, 0, p, 4, 0, 0) < 0) {
        SignalSema(_sceCd_scmd_semid);
        return 0;
    }
    r = *(volatile int *)((int)p | 0x20000000);
    SignalSema(_sceCd_scmd_semid);
    return r;
}

int sceCdGetError(void)
{
    int *p;
    int r;

    if (_sceCd_scmd_prechk(3) == 0)
        return -1;
    p = &_sceCd_scmdrdata;
    if (sceSifCallRpc(&_sceCd_cd_scmd, 4, 0, 0, 0, p, 4, 0, 0) < 0) {
        SignalSema(_sceCd_scmd_semid);
        return -1;
    }
    r = *(volatile int *)((int)p | 0x20000000);
    SignalSema(_sceCd_scmd_semid);
    return r;
}

/* ------------------------------------------------------------------
 * The N-command channel.
 *
 * Same reply template as the S-command pair above, against the other
 * SIF client block (_sceCd_cd_ncmd), its own 4-byte reply buffer
 * (_sceCd_ncmdrdata) and its own semaphore (_sceCd_ncmd_semid).
 * ------------------------------------------------------------------ */

extern int _sceCd_ncmd_prechk(int cmd);
extern int _sceCd_ncmdrdata;
extern int _sceCd_ncmd_semid;

int sceCdNcmdDiskReady(void)
{
    int *p;
    int r;

    if (_sceCd_ncmd_prechk(2) == 0)
        return 0;
    p = &_sceCd_ncmdrdata;
    if (sceSifCallRpc(&_sceCd_cd_ncmd, 14, 0, 0, 0, p, 4, 0, 0) < 0) {
        SignalSema(_sceCd_ncmd_semid);
        return 0;
    }
    r = *(volatile int *)((int)p | 0x20000000);
    SignalSema(_sceCd_ncmd_semid);
    return r;
}

/* sceCdStatus: the S-command template plus a debug trace on the way
 * out.  `SCE_CD_debug > 1` (not >= 2 spelled any other way) is what
 * gives the `slti v0,v0,2` + `bnez` skip; the return value is loaded
 * before the trace and moved into v0 on both paths. */
int sceCdStatus(void)
{
    int *p;
    int r;

    if (_sceCd_scmd_prechk(2) == 0)
        return -1;
    p = &_sceCd_scmdrdata;
    if (sceSifCallRpc(&_sceCd_cd_scmd, 12, 0, 0, 0, p, 4, 0, 0) < 0) {
        SignalSema(_sceCd_scmd_semid);
        return -1;
    }
    r = *(volatile int *)((int)p | 0x20000000);
    SignalSema(_sceCd_scmd_semid);
    if (SCE_CD_debug > 1)
        scePrintf("status called\n");
    return r;
}

/* sceCdBreak: S-command 22, with the in-flight command code parked in
 * sceCdCbfunc_num (8 = "break") for the duration and cleared on both
 * exits -- but in a different order on each: the failure path releases
 * the semaphore first and clears afterwards, the success path clears
 * first and reads the reply in SignalSema's delay slot. */
int sceCdBreak(void)
{
    int *p;
    int r;

    if (_sceCd_scmd_prechk(30) == 0)
        return 0;
    p = &_sceCd_scmdrdata;
    sceCdCbfunc_num = 8;
    if (sceSifCallRpc(&_sceCd_cd_scmd, 22, 0, 0, 0, p, 4, 0, 0) < 0) {
        SignalSema(_sceCd_scmd_semid);
        sceCdCbfunc_num = 0;
        return 0;
    }
    sceCdCbfunc_num = 0;
    r = *(volatile int *)((int)p | 0x20000000);
    SignalSema(_sceCd_scmd_semid);
    return r;
}

/* sceCdPause: an asynchronous N-command (mode 1), so there is no reply
 * buffer -- instead the completion function `_sceCd_cd_callback` and
 * the address of the in-flight command code are handed to the RPC as
 * its 8th and 9th arguments, and the "callback armed" flag
 * _sceCd_c_cb_sem is raised for the duration.  Failure unwinds both
 * flags before releasing the semaphore. */

extern void _sceCd_cd_callback(void);

int sceCdPause(void)
{
    int *pcmd;

    if (sceCdNcmdDiskReady() == 6)
        return 0;
    if (_sceCd_ncmd_prechk(12) == 0)
        return 0;
    sceCdCbfunc_num = 7;
    pcmd = &sceCdCbfunc_num;
    _sceCd_c_cb_sem = 1;
    if (sceSifCallRpc(&_sceCd_cd_ncmd, 8, 1, 0, 0, 0, 0,
                      (void *)_sceCd_cd_callback, pcmd) < 0) {
        sceCdCbfunc_num = 0;
        _sceCd_c_cb_sem = 0;
        SignalSema(_sceCd_ncmd_semid);
        return 0;
    }
    return 1;
}

/* ------------------------------------------------------------------
 * Two-word reply variants.
 *
 * sceCdReadDvdDualInfo and sceCdPowerOff are the same function with a
 * different command number: an 8-byte S-command reply whose second word
 * is handed back through the caller's pointer and whose first word is
 * the return value.  Both are read through the uncached alias, and the
 * `lui v1,0x2000` for that alias is hoisted above the branch -- one
 * shared constant, two `or`s, which is what writing the alias twice as
 * two separate expressions off the same base gives.
 * ------------------------------------------------------------------ */

int sceCdReadDvdDualInfo(int *layer1Start)
{
    int *p;
    int r;

    if (_sceCd_scmd_prechk(39) == 0)
        return 0;
    p = &_sceCd_scmdrdata;
    if (sceSifCallRpc(&_sceCd_cd_scmd, 39, 0, 0, 0, p, 8, 0, 0) < 0) {
        SignalSema(_sceCd_scmd_semid);
        return 0;
    }
    *layer1Start = *(volatile int *)(((int)p + 4) | 0x20000000);
    r = *(volatile int *)((int)p | 0x20000000);
    SignalSema(_sceCd_scmd_semid);
    return r;
}

int sceCdPowerOff(int *result)
{
    int *p;
    int r;

    if (_sceCd_scmd_prechk(33) == 0)
        return 0;
    p = &_sceCd_scmdrdata;
    if (sceSifCallRpc(&_sceCd_cd_scmd, 33, 0, 0, 0, p, 8, 0, 0) < 0) {
        SignalSema(_sceCd_scmd_semid);
        return 0;
    }
    *result = *(volatile int *)(((int)p + 4) | 0x20000000);
    r = *(volatile int *)((int)p | 0x20000000);
    SignalSema(_sceCd_scmd_semid);
    return r;
}

/* sceCdMmode: the first S-command here with an outbound payload -- the
 * media type goes into the fixed 4-byte send buffer _sceCd_scmdsdata,
 * which must be written back out of the data cache before the IOP can
 * DMA it.  The send buffer's address is formed BEFORE the prechk call
 * (it survives in s2) and the store into it sits in the prechk branch's
 * annulled delay slot, so the argument must stay live across the call:
 * a plain local assigned from the parameter, stored afterwards. */

extern int _sceCd_scmdsdata;
extern void sceSifWriteBackDCache(void *addr, int size);

int sceCdMmode(int media)
{
    int *p;
    int *sd;
    int r;

    sd = &_sceCd_scmdsdata;
    if (_sceCd_scmd_prechk(34) == 0)
        return 0;
    *sd = media;
    sceSifWriteBackDCache(sd, 4);
    p = &_sceCd_scmdrdata;
    if (sceSifCallRpc(&_sceCd_cd_scmd, 34, 0, sd, 4, p, 4, 0, 0) < 0) {
        SignalSema(_sceCd_scmd_semid);
        return 0;
    }
    r = *(volatile int *)((int)p | 0x20000000);
    SignalSema(_sceCd_scmd_semid);
    return r;
}

/* ------------------------------------------------------------------
 * Semaphores, the alarm callback and teardown.
 * ------------------------------------------------------------------ */

extern int iSignalSema(int sid);
extern int _sceCd_scmd_semid;
extern int cb_semid;

/* The alarm handler sceCdDelayThread arms: signal the waiting thread's
 * semaphore from interrupt context, then re-enable interrupts.  `ei`
 * cannot be reached from C, so it is a PS2_ASM site (same idiom as
 * _sceFs_Poff_Intr in sceFs.c). */
void CB_DelayTh(int id, unsigned short time, void *common)
{
    iSignalSema((int)common);
    PS2_ASM("sync.l\n\tei");
}

/* PARKED NEAR-MISS, 4 words of 37 (was 7 before the sceSemaParam field
 * order was swapped to initCount-then-maxCount, which is what puts the
 * sp+4 store ahead of the sp+8 one). What is left is two delay-slot
 * fill tie-breaks in a scheduled TU: the original puts the first
 * CreateSema result's store into the SECOND call's delay slot where we
 * put the `move a0,sp`, and it reuses the dead v0 for the last %hi temp
 * where we take a0. Swept: moving `_sceCd_c_cb_sem = 0` above the third
 * CreateSema (worse -- 6 words, the store migrates into that call's
 * slot).
 *
 * cmd_sem_init: create the three libcdvd semaphores, but only if either
 * command semaphore is still -1 -- the two tests are separate `if`s
 * because the original branches on each independently rather than
 * folding them.  One sceSemaParam on the stack is filled once and
 * reused; only initCount changes between the second and third call. */
void cmd_sem_init(void)
{
    sceSemaParam sp;

    if (_sceCd_ncmd_semid != -1 && _sceCd_scmd_semid != -1)
        return;
    sp.option = 0;
    sp.initCount = 1;
    sp.maxCount = 1;
    _sceCd_ncmd_semid = CreateSema(&sp);
    _sceCd_scmd_semid = CreateSema(&sp);
    sp.initCount = 0;
    cb_semid = CreateSema(&sp);
    _sceCd_c_cb_sem = 0;
}

/* PARKED NEAR-MISS, 5 words of 32. Everything from the first
 * DeleteSema to the EIntr tail call is exact, including the `j` tail
 * call. The residue is one delay-slot choice in the cb_thid arm: the
 * original stores `sceCdCbfunc_num = -1` before the call and fills
 * SignalSema's slot with the `lw` of cb_semid; the scheduler here picks
 * the store for the slot and hoists the load. Statement order does not
 * reach it (the two are independent), and --swap-into-slot would leave
 * the `lui` for cb_semid in the wrong place.
 *
 * cdvd_exit: wake the callback thread with a poison command code if it
 * is running, drop the three semaphores, and unhook the SIF command
 * handler.  The trailing EIntr() is a real tail call -- gcc 2.9 emits
 * `j` for a void call in the tail position of a void function. */

extern void sceSifRemoveCmdHandler(unsigned int cid);

void cdvd_exit(void)
{
    if (cb_thid != 0) {
        sceCdCbfunc_num = -1;
        SignalSema(cb_semid);
    }
    DeleteSema(_sceCd_ncmd_semid);
    DeleteSema(_sceCd_scmd_semid);
    DeleteSema(cb_semid);
    DIntr();
    sceSifRemoveCmdHandler(0x80000012);
    EIntr();
}

/* ------------------------------------------------------------------
 * Power-off callback plumbing.
 * ------------------------------------------------------------------ */

extern void (*sceCdPoffCbfunc)(void *);
extern void *sceCdPoffCbdata;
extern int Init_seq;
extern int _icmd_bind;
extern void sceSifAddCmdHandler(unsigned int cid, void *func, void *data);

/* The SIF command handler the IOP fires when the power button is
 * pressed: run the user callback, unless none is installed or libcdvd
 * is still initialising. */
void _sceCd_Poff_Intr(void)
{
    if (sceCdPoffCbfunc != 0 && Init_seq == 0)
        sceCdPoffCbfunc(sceCdPoffCbdata);
}

/* Bind that handler, with Init_seq raised for the duration so a command
 * arriving mid-registration is ignored.  The 1 stored into Init_seq and
 * the 1 stored into _icmd_bind (and returned) are the same value in the
 * same register -- one local, three roles. */
int PowerOffCB(void)
{
    int one = 1;

    Init_seq = one;
    DIntr();
    sceSifAddCmdHandler(0x80000012, (void *)_sceCd_Poff_Intr, 0);
    EIntr();
    Init_seq = 0;
    _icmd_bind = one;
    return one;
}

/* Install a power-off callback, binding the SIF handler first if it has
 * never been bound (_icmd_bind still negative).  Returns the previous
 * callback; the swap is done with interrupts disabled. */
void *sceCdPOffCallback(void (*func)(void *), void *data)
{
    void (*old)(void *);

    if (_icmd_bind < 0)
        PowerOffCB();
    DIntr();
    old = sceCdPoffCbfunc;
    sceCdPoffCbdata = data;
    sceCdPoffCbfunc = func;
    EIntr();
    return (void *)old;
}

/* ------------------------------------------------------------------
 * _sceCdSetTimeout and sceCdReadClock.
 * ------------------------------------------------------------------ */

/* Two-word outbound payload version of sceCdMmode: the parameter goes
 * in the send buffer's first word inside the prechk branch's annulled
 * delay slot, the value in its second word afterwards. */
int _sceCdSetTimeout(int param, int timeout)
{
    int *p;
    int *sd;
    int r;

    sd = &_sceCd_scmdsdata;
    if (_sceCd_scmd_prechk(37) == 0)
        return 0;
    sd[0] = param;
    sd[1] = timeout;
    sceSifWriteBackDCache(sd, 8);
    p = &_sceCd_scmdrdata;
    if (sceSifCallRpc(&_sceCd_cd_scmd, 37, 0, sd, 8, p, 4, 0, 0) < 0) {
        SignalSema(_sceCd_scmd_semid);
        return 0;
    }
    r = *(volatile int *)((int)p | 0x20000000);
    SignalSema(_sceCd_scmd_semid);
    return r;
}

/* sceCdReadClock: a 16-byte reply whose second and third words are the
 * eight BCD clock bytes.  They are copied out as ONE 8-byte struct
 * assignment: the struct's 4-byte alignment is what picks gcc's
 * ldl/ldr + sdl/sdr idiom rather than four lwl/lwr pairs (see
 * "struct-copy alignment picks the move idiom" in docs/LEVERS.md).
 * Both debug traces are gated the same way sceCdSyncS gates its own. */

typedef struct sceCdCLOCK32 {
    int w[2];
} sceCdCLOCK32;

int sceCdReadClock(void *clock)
{
    int *p;
    int r;

    if (_sceCd_scmd_prechk(15) == 0)
        return 0;
    if (SCE_CD_debug > 0)
        scePrintf("Libcdvd call Clock read 1\n");
    p = &_sceCd_scmdrdata;
    if (sceSifCallRpc(&_sceCd_cd_scmd, 1, 0, 0, 0, p, 16, 0, 0) < 0) {
        SignalSema(_sceCd_scmd_semid);
        return 0;
    }
    *(sceCdCLOCK32 *)clock =
        *(sceCdCLOCK32 *)(((int)p + 4) | 0x20000000);
    if (SCE_CD_debug > 0)
        scePrintf("Libcdvd call Clock read 2\n");
    r = *(volatile int *)((int)p | 0x20000000);
    SignalSema(_sceCd_scmd_semid);
    return r;
}

/* ------------------------------------------------------------------
 * _sceCd_ncmd_prechk / _sceCd_scmd_prechk: the gate every libcdvd
 * command goes through.  Identical twins over the two channels.
 *
 * Take the channel semaphore (PollSema returns the semaphore id on
 * success, so "did I get it?" is a comparison against the id, loaded
 * again after the call), record which command is in flight, make sure
 * the drive is idle, then bring the RPC binding up if it has never been
 * bound.  The bind is a retry loop around the same fixed-count delay
 * spin sceSifInitIopHeap uses -- the entry jumps straight to the first
 * bind attempt, and only the "bound but not yet serving" case falls
 * back to the leading delay.
 *
 * PARKED NEAR-MISS, both twins at 94 words against 92. Everything from
 * the prologue through the SignalSema arm is exact, and the bind retry
 * loop is structurally right down to the annulled `bgezl` and the two
 * separate fixed-count delay spins. Three residues:
 *   - the `return 1` for an already-bound channel costs us
 *     `li v0,1; b epilogue` where the original reaches the epilogue
 *     with `bgez` and puts the `li` in its delay slot, then `b`s into
 *     the middle of the bind block;
 *   - the R5900 erratum pad nops in each delay spin sit one slot
 *     earlier (lui/li/NOP/addiu/nop*3 rather than lui/li/addiu/nop*4),
 *     and the original's loop-top .p2align pad is missing;
 *   - s1 and s2 are swapped (we keep %hi(_Xcmd_bind) in s1 and %hi of
 *     the client block in s2; the original is the other way round) --
 *     that part is what --swap-regs exists for.
 * Swept: the two delay spins written as ONE loop inside a for(;;) with
 * an if/else (gcc merges them, 79 words -- the original really does
 * duplicate the spin, so the goto form is right); `_Xcmd_bind < 0` as
 * the branch polarity (identical output); PollSema compared in both
 * operand orders (the `_Xcmd_semid != PollSema(...)` order shown here
 * is what gives the original's `beq v1,v0`).
 * ------------------------------------------------------------------ */

extern int PollSema(int sid);
extern int ncmd_sema_keep_cmd;
extern int scmd_sema_keep_cmd;
extern int _ncmd_bind;
extern int _scmd_bind;
extern int my_thid;
extern int my_th_info;
extern int ReferThreadStatus(int thid, void *info);
extern int sceSifInitRpc(int mode);
extern int sceSifBindRpc(void *cd, unsigned int sid, int mode);

int _sceCd_ncmd_prechk(int cmd)
{
    int i;

    cmd_sem_init();
    if (_sceCd_ncmd_semid != PollSema(_sceCd_ncmd_semid)) {
        if (SCE_CD_debug > 0)
            scePrintf("Ncmd fail sema cur_cmd:%d keep_cmd:%d\n",
                      cmd, ncmd_sema_keep_cmd);
        return 0;
    }
    ncmd_sema_keep_cmd = cmd;
    ReferThreadStatus(my_thid, &my_th_info);
    if (sceCdSync(1) != 0) {
        SignalSema(_sceCd_ncmd_semid);
        return 0;
    }
    sceSifInitRpc(0);
    if (_ncmd_bind >= 0)
        return 1;
    goto bind;
delay:
    for (i = 0x100000; i != -1; i--)
        ;
bind:
    if (sceSifBindRpc(&_sceCd_cd_ncmd, 0x80000595, 0) < 0) {
        if (SCE_CD_debug > 0)
            scePrintf("Libcdvd bind err N CMD\n");
        for (i = 0x100000; i != -1; i--)
            ;
        goto bind;
    }
    if (_sceCd_cd_ncmd.serve == 0)
        goto delay;
    _ncmd_bind = 0;
    return 1;
}

int _sceCd_scmd_prechk(int cmd)
{
    int i;

    cmd_sem_init();
    if (_sceCd_scmd_semid != PollSema(_sceCd_scmd_semid)) {
        if (SCE_CD_debug > 0)
            scePrintf("Scmd fail sema cur_cmd:%d keep_cmd:%d\n",
                      cmd, scmd_sema_keep_cmd);
        return 0;
    }
    scmd_sema_keep_cmd = cmd;
    ReferThreadStatus(my_thid, &my_th_info);
    if (sceCdSyncS(1) != 0) {
        SignalSema(_sceCd_scmd_semid);
        return 0;
    }
    sceSifInitRpc(0);
    if (_scmd_bind >= 0)
        return 1;
    goto bind;
delay:
    for (i = 0x100000; i != -1; i--)
        ;
bind:
    if (sceSifBindRpc(&_sceCd_cd_scmd, 0x80000593, 0) < 0) {
        if (SCE_CD_debug > 0)
            scePrintf("Libcdvd bind err S cmd\n");
        for (i = 0x100000; i != -1; i--)
            ;
        goto bind;
    }
    if (_sceCd_cd_scmd.serve == 0)
        goto delay;
    _scmd_bind = 0;
    return 1;
}
