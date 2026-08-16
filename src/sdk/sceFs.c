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

/* ------------------------------------------------------------------
 * The iob table: 16-byte per-descriptor slots in the fixed array
 * `_iob`.  A file descriptor is the slot's INDEX, so the wrappers hand
 * back `slot - _iob`, which for a 16-byte element is the original's
 * subu + sra 4.
 * ------------------------------------------------------------------ */

typedef struct t_iob {
    int fd;                     /* +0: the IOP-side descriptor */
    int used;                   /* +4: nonzero while the slot is taken */
    int reserved[2];
} iob_t;                        /* 16 bytes */

extern iob_t _iob[];
extern iob_t *new_iob(void);
extern int _fs_iob_semid;
extern int _sceCallCode(void *arg0, int code);

/* sceDopen: open a directory.  The two arms of the result test each
 * take the iob lock themselves rather than sharing one WaitSema above
 * the branch -- that duplication is in the original, and hoisting it
 * costs a word.  Both arms fall through to one SignalSema and one
 * return, so `r` carries either the negative error or the slot index. */
int sceDopen(void *name)
{
    iob_t *p;
    int r;

    _sceFsWaitS(9);
    if (_fs_init == 0)
        sceFsInit();
    _sceFsSigSema();
    p = new_iob();
    if (p == 0)
        return -19;
    r = _sceCallCode(name, 9);
    if (r < 0) {
        WaitSema(_fs_iob_semid);
        p->used = 0;
        SignalSema(_fs_iob_semid);
        return r;
    }
    {
        /* The %hi/%lo temp holding &_iob lands in $v0 by default; the
         * original has it in $v1.  Plain C cannot reach it -- a named
         * local for the base changes the tail shape (52-53 words) --
         * so the address is pinned and fed in through the zero-cost
         * PASSTHRU, which is what makes a pin stick on a value that is
         * only used as an operand. */
        PIN(iob_t *base, "$3");
        WaitSema(_fs_iob_semid);
        p->fd = r;
        PASSTHRU(base, _iob);
        r = p - base;
    }
    SignalSema(_fs_iob_semid);
    return r;
}

/* ------------------------------------------------------------------
 * The fs RPC template.
 *
 * Every descriptor-taking fs call is the same nine-argument
 * sceSifCallRpc into the `_cd` client block, differing only in the
 * function number and in which fields of the fixed 20-byte `_send_data`
 * request block it fills.  The reply DMAs into a stack local whose
 * address is handed over in `_send_data.dst`; a separate 4-byte
 * `_rcv_data_rpc` carries the RPC's own completion word, and is read
 * back through the uncached-accelerated alias (| 0x20000000) because
 * the IOP writes it by DMA.
 *
 * The semaphore is created per call and the IOP signals it; the caller
 * parks on it only when the completion word says the transfer really
 * happened.  Reading that word BEFORE _sceFsSigSema() is what puts the
 * load in the call's delay slot, and clearing `used` straight after the
 * success branch is what makes the branch a `bgezl` with the store
 * annulled into its slot.
 * ------------------------------------------------------------------ */

typedef struct t_fs_send {
    int   semid;                /* +0  EE semaphore for the IOP to signal */
    void *dst;                  /* +4  where the reply is DMAd */
    int   size;                 /* +8  reply size */
    int   fd;                   /* +12 IOP-side descriptor */
    union {                     /* +16 one call-specific word */
        int   index;            /*     EE-side slot index */
        void *buf;              /*     caller's data buffer */
        int   arg;
    } u;
    char  path[1024];           /* +20 name argument, when there is one */
} fs_send_t;                    /* 1044 bytes */

extern fs_send_t _send_data;
extern int _rcv_data_rpc;
extern int _cd;                 /* sceSifClientData for the fs channel */
extern iob_t *get_iob(int fd);
extern int DeleteSema(int semid);
extern int sceSifCallRpc(void *cd, unsigned int fno, int mode, void *send,
                         int ssize, void *recv, int rsize, void *ef, void *ea);

int sceDclose(int fd)
{
    ee_sema_t sema;
    int result;
    iob_t *p;
    int semid;
    fs_send_t *sd;
    int done;
    int r;
    int lim;

    sd = &_send_data;
    p = get_iob(fd);
    _sceFsWaitS(10);
    if (_fs_init == 0) {
        _sceFsSigSema();
        return -1;
    }
    if (p == 0 || p->used == 0) {
        _sceFsSigSema();
        return -9;
    }
    sd->fd = p->fd;
    sema.max_count = 1;
    sema.init_count = 0;
    sema.option = 0;
    semid = CreateSema(&sema);
    _send_data.semid = semid;
    sd->dst = &result;
    sd->size = 4;
    if (sceSifCallRpc(&_cd, 10, 0, sd, 20, &_rcv_data_rpc, 4, 0, 0) < 0) {
        DeleteSema(semid);
        _sceFsSigSema();
        return -11;
    }
    p->used = 0;
    done = *(volatile int *)((int)&_rcv_data_rpc | 0x20000000);
    _sceFsSigSema();
    if (done == 0) {
        DeleteSema(semid);
        return -11;
    }
    WaitSema(semid);
    DeleteSema(semid);
    r = result;
    /* The result is normalised to 0 on success.  The original compares
     * against -1 held in a REGISTER (`li v1,-1; slt v1,v1,v0`), which
     * gcc will not emit for any spelling of the comparison -- `r >= 0`,
     * `r > -1` and `-1 < r` all canonicalise to `slti v1,v0,0`.  The
     * laundered constant is the only way to keep -1 in a register; it
     * emits nothing. */
    lim = -1;
    LAUNDER(lim);
    if (lim < r)
        r = 0;
    return r;
}

/* sceClose: sceDclose's template with the EE-side slot index added to
 * the request block, so the IOP can release its side too. */
int sceClose(int fd)
{
    ee_sema_t sema;
    int result;
    iob_t *p;
    int semid;
    fs_send_t *sd;
    int done;
    int r;
    int lim;

    sd = &_send_data;
    p = get_iob(fd);
    _sceFsWaitS(1);
    if (_fs_init == 0) {
        _sceFsSigSema();
        return -1;
    }
    if (p == 0 || p->used == 0) {
        _sceFsSigSema();
        return -9;
    }
    sd->fd = p->fd;
    sd->u.index = p - _iob;
    sema.max_count = 1;
    sema.init_count = 0;
    sema.option = 0;
    semid = CreateSema(&sema);
    _send_data.semid = semid;
    sd->dst = &result;
    sd->size = 4;
    if (sceSifCallRpc(&_cd, 1, 0, sd, 20, &_rcv_data_rpc, 4, 0, 0) < 0) {
        DeleteSema(semid);
        _sceFsSigSema();
        return -11;
    }
    p->used = 0;
    done = *(volatile int *)((int)&_rcv_data_rpc | 0x20000000);
    _sceFsSigSema();
    if (done == 0) {
        DeleteSema(semid);
        return -11;
    }
    WaitSema(semid);
    DeleteSema(semid);
    r = result;
    lim = -1;
    LAUNDER(lim);
    if (lim < r)
        r = 0;
    return r;
}

/* sceDread: read one directory entry into the caller's buffer.  The
 * result is returned verbatim (no normalising compare), and the RPC
 * failure arm waits on the semaphore instead of deleting it -- both
 * differences from sceDclose are in the original. */
int sceDread(int fd, void *buf)
{
    ee_sema_t sema;
    int result;
    iob_t *p;
    int semid;
    fs_send_t *sd;
    int done;

    sd = &_send_data;
    p = get_iob(fd);
    _sceFsWaitS(11);
    if (_fs_init == 0) {
        _sceFsSigSema();
        return -1;
    }
    if (p == 0 || p->used == 0) {
        _sceFsSigSema();
        return -9;
    }
    sd->fd = p->fd;
    sd->u.buf = buf;
    sema.max_count = 1;
    sema.init_count = 0;
    sema.option = 0;
    semid = CreateSema(&sema);
    _send_data.semid = semid;
    sd->dst = &result;
    sd->size = 4;
    if (sceSifCallRpc(&_cd, 11, 0, sd, 32, &_rcv_data_rpc, 4, 0, 0) < 0) {
        WaitSema(semid);
        _sceFsSigSema();
        return -11;
    }
    done = *(volatile int *)((int)&_rcv_data_rpc | 0x20000000);
    _sceFsSigSema();
    if (done == 0) {
        DeleteSema(semid);
        return -11;
    }
    WaitSema(semid);
    DeleteSema(semid);
    return result;
}

/* sceSync: flush a mounted filesystem by name.  The path is copied into
 * the request block a byte at a time with a 1024 cap; gcc peels the
 * first iteration itself (the i < 1024 test is known true), so the
 * source must NOT peel it by hand.  Overflowing the cap forces a
 * terminator into the last byte, which is the annulled `beql` store.
 *
 * The terminator test reads back sd->path[i], NOT the source byte: gcc
 * then tests the QImode pseudo it just stored, which needs the
 * `lbu` + `sll 24` pair the original has.  Testing `name[i]`, or a
 * `char`/`unsigned char` local holding it, gives a plain `lb` and comes
 * out three words short. */
int sceSync(char *name, int flag)
{
    ee_sema_t sema;
    int result;
    int semid;
    fs_send_t *sd;
    int done;
    int i;

    sd = &_send_data;
    _sceFsWaitS(19);
    if (_fs_init == 0)
        sceFsInit();
    for (i = 0; i < 1024; i++) {
        sd->path[i] = name[i];
        if (sd->path[i] == 0)
            break;
    }
    if (i == 1024)
        sd->path[1023] = 0;
    sd->u.arg = flag;
    sema.max_count = 1;
    sema.init_count = 0;
    sema.option = 0;
    semid = CreateSema(&sema);
    sd->dst = &result;
    sd->semid = semid;
    sd->size = 4;
    if (sceSifCallRpc(&_cd, 19, 0, &_send_data, 1044, &_rcv_data_rpc, 4,
                      0, 0) < 0) {
        DeleteSema(semid);
        _sceFsSigSema();
        return -11;
    }
    done = *(volatile int *)((int)&_rcv_data_rpc | 0x20000000);
    _sceFsSigSema();
    if (done == 0) {
        DeleteSema(semid);
        return -11;
    }
    WaitSema(semid);
    DeleteSema(semid);
    return result;
}

/* The path-argument request layout: the same block seen through a
 * header with no descriptor word, so the name starts at +16 and the
 * request length is the name length + 17. */
typedef struct t_fs_send_path {
    int   semid;
    void *dst;
    int   size;
    union {                     /* +12 the one non-name argument */
        int   mode;
        void *arg;
    } u;
    char  path[1024];           /* +16 */
} fs_send_path_t;

/* sceMkdir: create a directory.  Same copy-with-cap as sceSync, except
 * that overflowing the cap also pulls the length back to 1023 so the
 * request length still counts the terminator. */
int sceMkdir(char *name, int mode)
{
    ee_sema_t sema;
    int result;
    int semid;
    fs_send_path_t *sd;
    int done;
    int i;

    sd = (fs_send_path_t *)&_send_data;
    _sceFsWaitS(7);
    if (_fs_init == 0)
        sceFsInit();
    for (i = 0; i < 1024; i++) {
        sd->path[i] = name[i];
        if (sd->path[i] == 0)
            break;
    }
    if (i == 1024) {
        sd->path[1023] = 0;
        i = 1023;
    }
    sd->u.mode = mode;
    sema.max_count = 1;
    sema.init_count = 0;
    sema.option = 0;
    semid = CreateSema(&sema);
    sd->dst = &result;
    sd->semid = semid;
    sd->size = 4;
    if (sceSifCallRpc(&_cd, 7, 0, &_send_data, i + 17, &_rcv_data_rpc, 4,
                      0, 0) < 0) {
        DeleteSema(semid);
        _sceFsSigSema();
        return -11;
    }
    done = *(volatile int *)((int)&_rcv_data_rpc | 0x20000000);
    _sceFsSigSema();
    if (done == 0) {
        DeleteSema(semid);
        return -11;
    }
    WaitSema(semid);
    DeleteSema(semid);
    return result;
}

/* sceGetstat: stat a path into the caller's buffer.  Byte-for-byte the
 * sceMkdir template with function number 12 -- except that the copy
 * loop tests the SOURCE byte through an `unsigned char` local, so there
 * is no sll-24 QImode test and the back-edge is a plain `bnez`.  The
 * two spellings sit side by side in the original SDK sources. */
int sceGetstat(char *name, void *buf)
{
    ee_sema_t sema;
    int result;
    int semid;
    fs_send_path_t *sd;
    int done;
    int i;

    sd = (fs_send_path_t *)&_send_data;
    _sceFsWaitS(12);
    if (_fs_init == 0)
        sceFsInit();
    for (i = 0; i < 1024; i++) {
        sd->path[i] = name[i];
        if ((unsigned char)sd->path[i] == 0)
            break;
    }
    if (i == 1024) {
        sd->path[1023] = 0;
        i = 1023;
    }
    sd->u.arg = buf;
    sema.max_count = 1;
    sema.init_count = 0;
    sema.option = 0;
    semid = CreateSema(&sema);
    sd->dst = &result;
    sd->semid = semid;
    sd->size = 4;
    if (sceSifCallRpc(&_cd, 12, 0, &_send_data, i + 17, &_rcv_data_rpc, 4,
                      0, 0) < 0) {
        DeleteSema(semid);
        _sceFsSigSema();
        return -11;
    }
    done = *(volatile int *)((int)&_rcv_data_rpc | 0x20000000);
    _sceFsSigSema();
    if (done == 0) {
        DeleteSema(semid);
        return -11;
    }
    WaitSema(semid);
    DeleteSema(semid);
    return result;
}
