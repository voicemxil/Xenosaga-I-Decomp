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

int _sceFsWaitS(void)
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

void _sceFs_Poff_Intr(int cause, PoffCb *cb)
{
    if (cb->func != 0)
        cb->func(cb->arg);
    PS2_ASM("sync.l\n\tei");
}
