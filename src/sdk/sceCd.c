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
extern void CB_DelayTh(void);
extern int scePrintf(const char *fmt, ...);
extern int sceSifCheckStatRpc(void *rd);
extern int SCE_CD_debug;
/* libcdvd's SIF-RPC client-data block for the "S" (synchronous)
 * command channel, 0x004AD448. */
extern int _sceCd_cd_scmd;

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
