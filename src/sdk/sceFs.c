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

/* sceFsReset: clears the fs-init flag and the 4-byte fs-version buffer,
 * then returns 0. Both are named fixed addresses from
 * config/symbol_addrs.txt (_fs_init, _fsversion), so referencing them
 * by name lets the linker's ordinary %hi/%lo relocation produce
 * lui+addiu, same as the original -- real C, no inline asm needed. */

extern void *memset(void *s, int c, unsigned int n);
extern int _fs_init;
extern char _fsversion[4];

int sceFsReset(void)
{
    _fs_init = 0;
    memset(_fsversion, 0, 4);
    return 0;
}

/* ------------------------------------------------------------------
 * The filesystem driver's mutual-exclusion semaphore.  `_fs_semid` is
 * a named fixed address from config/symbol_addrs.txt; it holds -1
 * until the first call creates the semaphore.
 * ------------------------------------------------------------------ */

#include "matching.h"

/* EE kernel semaphore parameter block (SDK ee_sema_t). */
typedef struct t_ee_sema {
    int          count;
    int          max_count;
    int          init_count;
    int          wait_threads;
    unsigned int attr;
    unsigned int option;
} ee_sema_t;

extern int CreateSema(ee_sema_t *param);
extern void WaitSema(int semid);
extern void SignalSema(int semid);
extern int _fs_semid;

void _sceFsSemInit(void)
{
    ee_sema_t sema;

    if (_fs_semid == -1) {
        sema.option = 0;
        sema.init_count = 1;
        sema.max_count = 1;
        _fs_semid = CreateSema(&sema);
    }
}

/* The `code` argument is the caller's fs opcode; the routine only takes
 * the lock, but every call site passes one (scePowerOffHandler below
 * sets a0 = 27 before the jal), so it is part of the signature. */
int _sceFsWaitS(int code)
{
    _sceFsSemInit();
    WaitSema(_fs_semid);
    return 0;
}

void _sceFsSigSema(void)
{
    SignalSema(_fs_semid);
}

/* _sceFs_Poff_Intr: the power-off interrupt hook.  The second argument
 * is the registered {handler, argument} pair; ExitHandler() (the SDK's
 * `sync.l; ei`) closes the interrupt context. */

typedef struct t_poff_cb {
    void (*func)(void *arg);
    void *arg;
} PoffCb;

extern PoffCb _sif_FsPoff_Data;

void _sceFs_Poff_Intr(int cause, PoffCb *cb)
{
    if (cb->func != 0)
        cb->func(cb->arg);
    PS2_ASM("sync.l\n\tei");
}

/* _sceFsIobSemaMK: the same lazy creation for the iob/fsq semaphore
 * pair. Only `_fs_iob_semid` is tested; both are made together. */

extern int _fs_iob_semid;
extern int _fs_fsq_semid;

void _sceFsIobSemaMK(void)
{
    ee_sema_t sema;

    if (_fs_iob_semid == -1) {
        sema.option = 0;
        sema.init_count = 1;
        sema.max_count = 1;
        _fs_iob_semid = CreateSema(&sema);
        _fs_fsq_semid = CreateSema(&sema);
    }
}

/* scePowerOffHandler: install the EE-side power-off callback, returning
 * the one it replaced.  The {func, arg} pair lives in the fixed block
 * `_sif_FsPoff_Data`, and the swap runs with interrupts disabled.
 *
 * The original holds &_sif_FsPoff_Data in a callee-saved register from
 * before the lock call onwards and reaches `.arg` through it while
 * `.func` uses the plain %hi/%lo offset form -- the "&STRUCT.member
 * through a local pointer" lever, one pointer local assigned at the
 * top of the function. */

extern int  sceFsInit(void);
extern int  DIntr(void);
extern int  EIntr(void);

void *scePowerOffHandler(void *func, void *arg)
{
    PoffCb *p;
    void *old;

    p = &_sif_FsPoff_Data;
    _sceFsWaitS(27);
    if (_fs_init == 0)
        sceFsInit();
    DIntr();
    old = _sif_FsPoff_Data.func;
    p->arg = arg;
    _sif_FsPoff_Data.func = func;
    EIntr();
    _sceFsSigSema();
    return old;
}
