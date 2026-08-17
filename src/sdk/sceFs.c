/* PS2 SDK filesystem driver dispatch wrappers */

#include <stdarg.h>

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

typedef struct t_fs_rpc_client {
    char reserved[0x24];
    int server;
} fs_rpc_client_t;

typedef struct __attribute__((packed)) t_fs_version_word {
    int value;
} fs_version_word_t;

extern int sceSifInitRpc(int mode);
extern void sceSifAddCmdHandler(unsigned int cid, void *handler, void *data);
extern int sceSifBindRpc(void *client, unsigned int sid, int mode);
extern fs_rpc_client_t _cd;
extern char _sif_FsRcv_Data[];
extern int _rcv_data_cmd;
extern int _rcv_data_rpc;
extern int *rcv_adr_30 __asm__("rcv_adr.30");
extern void _sceFs_Rcv_Intr(void);

int sceFsInit(void)
{
    PoffCb *poff;

    poff = &_sif_FsPoff_Data;
    sceSifInitRpc(0);
    _sif_FsPoff_Data.func = 0;
    poff->arg = 0;
    DIntr();
    sceSifAddCmdHandler(0x80000011, (void *)_sceFs_Rcv_Intr,
                        _sif_FsRcv_Data);
    sceSifAddCmdHandler(0x80000013, (void *)_sceFs_Poff_Intr,
                        poff);
    EIntr();

    for (;;) {
        int limit;
        int i;

        if (sceSifBindRpc(&_cd, 0x80000001, 0) < 0)
            return -1;
        i = 0x100000;
        if (_cd.server != 0)
            break;
        limit = -1;
        do {
            i--;
        } while (i != limit);
    }

    _sceFsIobSemaMK();
    WaitSema(_fs_iob_semid);
    {
        PIN(iob_t *p, "$3");
        iob_t *end;

        /* Preserve the SDK object's coalesced %hi/%lo descriptor cursor;
         * the tied pass-through emits no instructions. */
        PASSTHRU(p, _iob);
        end = p + 32;
        for (; p < end; p++)
            p->used = 0;
    }
    SignalSema(_fs_iob_semid);

    rcv_adr_30 = &_rcv_data_cmd;
    if (sceSifCallRpc(&_cd, 0xFF, 0, &rcv_adr_30, 4,
                      &_rcv_data_rpc, 4, 0, 0) < 0)
        return 0xFFFEFFFF;

    *(fs_version_word_t *)_fsversion =
        *(fs_version_word_t *)((int)&_rcv_data_rpc | 0x20000000);
    _fs_init = 1;
    return 0;
}

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
extern fs_rpc_client_t _cd;     /* sceSifClientData for the fs channel */
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

/* The open request layout: flags at +12, mode at +16, the name at +20
 * and the EE-side slot index in a trailing word, 1048 bytes in all. */
typedef struct t_fs_send_open {
    int   semid;
    void *dst;
    int   size;
    int   flags;                /* +12 */
    int   mode;                 /* +16 */
    char  path[1024];           /* +20 */
    int   index;                /* +1044 */
} fs_send_open_t;               /* 1048 bytes */

extern int _fs_version(void);

/* sceOpen: the mode argument is variadic (open(path, flags) is legal),
 * which is what spills a2..t3 into the top of the frame.  The flags
 * word is masked to 0x6fffffff on the way out but stored UNMASKED into
 * the iob slot afterwards. */
int sceOpen(char *name, int flags, ...)
{
    va_list ap;
    ee_sema_t sema;
    int result;
    int semid;
    fs_send_open_t *sd;
    iob_t *p;
    int done;
    int i;
    int idx;
    int mode;
    int r;

    sd = (fs_send_open_t *)&_send_data;
    _sceFsWaitS(0);
    if (_fs_init == 0)
        sceFsInit();
    if (_fs_version() != 0) {
        _sceFsSigSema();
        return 0xfffefffc;
    }
    p = new_iob();
    if (p == 0) {
        _sceFsSigSema();
        return -19;
    }
    va_start(ap, flags);
    mode = va_arg(ap, int);
    va_end(ap);
    for (i = 0; i < 1024; i++) {
        sd->path[i] = name[i];
        if (sd->path[i] == 0)
            break;
    }
    if (i == 1024)
        sd->path[1023] = 0;
    idx = p - _iob;
    sd->flags = flags & 0x6fffffff;
    sd->mode = mode;
    sd->index = idx;
    sema.max_count = 1;
    sema.init_count = 0;
    sema.option = 0;
    semid = CreateSema(&sema);
    sd->dst = &result;
    sd->semid = semid;
    sd->size = 4;
    if (sceSifCallRpc(&_cd, 0, 0, &_send_data, 1048, &_rcv_data_rpc, 4,
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
    if (result < 0) {
        WaitSema(_fs_iob_semid);
        p->used = 0;
        SignalSema(_fs_iob_semid);
        r = result;
    } else {
        r = idx;
        WaitSema(_fs_iob_semid);
        p->used |= flags;
        p->fd = result;
        SignalSema(_fs_iob_semid);
    }
    return r;
}

/* ------------------------------------------------------------------
 * The no-wait (0x8000) descriptor mode.
 *
 * An iob opened with 0x8000 set does not block: the request is queued,
 * its semaphore id is parked in the 32-slot `_sceFs_q` table with its
 * SIGN FLIPPED in the request block (that is how the IOP side tells a
 * queued request from a synchronous one), and the call returns 0
 * immediately.  gcc peels the i == 0 probe out of the scan loop by
 * itself, so the source must not.  Storing offset before whence preserves
 * the original request-field lifetimes.  LAUNDER(flags) between the two
 * 0x8000 tests is required so the second test reloads the descriptor state
 * after the RPC path may have observed it.
 */

typedef struct t_fs_send_seek {
    int   semid;
    void *dst;
    int   size;
    int   fd;                   /* +12 */
    int   offset;               /* +16 */
    int   whence;               /* +20 */
    int   index;                /* +24 */
} fs_send_seek_t;               /* 28 bytes */

extern int _sceFs_q[32];
extern int _fs_fsq_semid;

int sceLseek(int fd, int offset, int whence)
{
    ee_sema_t sema;
    int result;
    int semid;
    fs_send_seek_t *sd;
    iob_t *p;
    int done;
    int i;
    int flags;
    int id;

    sd = (fs_send_seek_t *)&_send_data;
    p = get_iob(fd);
    _sceFsWaitS(4);
    if (_fs_init == 0) {
        _sceFsSigSema();
        return -1;
    }
    if (p == 0) {
        _sceFsSigSema();
        return -9;
    }
    flags = p->used;
    if (flags == 0) {
        _sceFsSigSema();
        return -9;
    }
    sd->fd = p->fd;
    sd->offset = offset;
    sd->whence = whence;
    sd->index = p - _iob;
    sema.max_count = 1;
    sema.init_count = 0;
    sema.option = 0;
    semid = CreateSema(&sema);
    sd->dst = &result;
    sd->size = 4;
    _send_data.semid = semid;
    if (flags & 0x8000) {
        WaitSema(_fs_fsq_semid);
        for (i = 0; i < 32; i++) {
            if (_sceFs_q[i] == -1) {
                id = sd->semid;
                _sceFs_q[i] = id;
                sd->semid = -id;
                break;
            }
        }
        SignalSema(_fs_fsq_semid);
    }
    LAUNDER(flags);
    if (sceSifCallRpc(&_cd, 4, 0, &_send_data, 28, &_rcv_data_rpc, 4,
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
    if (flags & 0x8000) {
        DeleteSema(semid);
        return 0;
    }
    WaitSema(semid);
    DeleteSema(semid);
    return result;
}

/* The readlink request layout: the caller's buffer and its length, then
 * the path at +20. */
typedef struct t_fs_send_link {
    int   semid;
    void *dst;
    int   size;
    int   len;                  /* +12 */
    void *buf;                  /* +16 */
    char  path[1024];           /* +20 */
} fs_send_link_t;

extern void sceSifWriteBackDCache(void *addr, int size);

/* sceReadlink: the reply DMAs straight into the caller's buffer, so it
 * is written back out of the data cache first.  The length is clamped
 * to 1023; gcc hoists the `len < 1024` test into the copy loop's
 * preheader as loop-invariant and lands the clamp as a movz. */
int sceReadlink(char *name, void *buf, unsigned int len)
{
    ee_sema_t sema;
    int result;
    int semid;
    fs_send_link_t *sd;
    int done;
    int i;

    sd = (fs_send_link_t *)&_send_data;
    _sceFsWaitS(17);
    if (_fs_init == 0)
        sceFsInit();
    for (i = 0; i < 1024; i++) {
        sd->path[i] = name[i];
        if (sd->path[i] == 0)
            break;
    }
    if (i == 1024)
        sd->path[1023] = 0;
    if (len >= 1024)
        len = 1023;
    sd->buf = buf;
    sd->len = len;
    sceSifWriteBackDCache(buf, len);
    sema.max_count = 1;
    sema.init_count = 0;
    sema.option = 0;
    semid = CreateSema(&sema);
    sd->dst = &result;
    sd->semid = semid;
    sd->size = 4;
    if (sceSifCallRpc(&_cd, 25, 0, &_send_data, 2060, &_rcv_data_rpc, 4,
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

/* The two-path request layout: two 1024-byte names back to back
 * starting at +12, 2060 bytes on the wire. */
typedef struct t_fs_send_2path {
    int   semid;
    void *dst;
    int   size;
    char  path1[1024];          /* +12 */
    char  path2[1024];          /* +1036 */
} fs_send_2path_t;

/* sceSymlink: the same capped copy twice over.  Only the first copy
 * pulls its terminator store back into the beql slot from the block
 * that follows; the second one has no following block, which is why the
 * second loop's cap store is the last thing in the region. */
int sceSymlink(char *existing, char *newpath)
{
    ee_sema_t sema;
    int result;
    int semid;
    fs_send_2path_t *sd;
    int done;
    int i;

    sd = (fs_send_2path_t *)&_send_data;
    _sceFsWaitS(17);
    if (_fs_init == 0)
        sceFsInit();
    for (i = 0; i < 1024; i++) {
        sd->path1[i] = existing[i];
        if (sd->path1[i] == 0)
            break;
    }
    if (i == 1024)
        sd->path1[1023] = 0;
    for (i = 0; i < 1024; i++) {
        sd->path2[i] = newpath[i];
        if (sd->path2[i] == 0)
            break;
    }
    if (i == 1024)
        sd->path2[1023] = 0;
    sema.max_count = 1;
    sema.init_count = 0;
    sema.option = 0;
    semid = CreateSema(&sema);
    sd->dst = &result;
    sd->semid = semid;
    sd->size = 4;
    if (sceSifCallRpc(&_cd, 24, 0, &_send_data, 2060, &_rcv_data_rpc, 4,
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

/* sceRename: sceSymlink's two-path template with function number 17,
 * plus a writeback of the whole 2060-byte request out of the data
 * cache before the call. */
int sceRename(char *oldpath, char *newpath)
{
    ee_sema_t sema;
    int result;
    int semid;
    fs_send_2path_t *sd;
    int done;
    int i;

    sd = (fs_send_2path_t *)&_send_data;
    _sceFsWaitS(17);
    if (_fs_init == 0)
        sceFsInit();
    for (i = 0; i < 1024; i++) {
        sd->path1[i] = oldpath[i];
        if (sd->path1[i] == 0)
            break;
    }
    if (i == 1024)
        sd->path1[1023] = 0;
    for (i = 0; i < 1024; i++) {
        sd->path2[i] = newpath[i];
        if (sd->path2[i] == 0)
            break;
    }
    if (i == 1024)
        sd->path2[1023] = 0;
    sema.max_count = 1;
    sema.init_count = 0;
    sema.option = 0;
    semid = CreateSema(&sema);
    sd->size = 4;
    sd->dst = &result;
    sd->semid = semid;
    sceSifWriteBackDCache(&_send_data, 2060);
    if (sceSifCallRpc(&_cd, 17, 0, &_send_data, 2060, &_rcv_data_rpc, 4,
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

/* The mount request layout: two names, then an opaque argument block,
 * the flag word and the argument length -- 3092 bytes on the wire. */
typedef struct t_fs_send_mount {
    int   semid;
    void *dst;
    int   size;
    char  path1[1024];          /* +12   fs name */
    char  path2[1024];          /* +1036 device name */
    char  data[1024];           /* +2060 driver argument */
    int   flag;                 /* +3084 */
    int   arglen;               /* +3088 */
} fs_send_mount_t;              /* 3092 bytes */

/* sceMount: the argument-length bound check is loop-invariant, so gcc
 * hoists its `slti` all the way up into the first copy loop's preheader
 * and only branches on it after both names are in the block. */
int sceMount(char *fsname, char *devname, int flag, char *arg, int arglen)
{
    ee_sema_t sema;
    int result;
    int semid;
    fs_send_mount_t *sd;
    int done;
    int i;

    sd = (fs_send_mount_t *)&_send_data;
    _sceFsWaitS(20);
    if (_fs_init == 0)
        sceFsInit();
    for (i = 0; i < 1024; i++) {
        sd->path1[i] = fsname[i];
        if (sd->path1[i] == 0)
            break;
    }
    if (i == 1024)
        sd->path1[1023] = 0;
    for (i = 0; i < 1024; i++) {
        sd->path2[i] = devname[i];
        if (sd->path2[i] == 0)
            break;
    }
    if (i == 1024)
        sd->path2[1023] = 0;
    if (arglen > 1024) {
        _sceFsSigSema();
        return -7;
    }
    for (i = 0; i < arglen; i++)
        sd->data[i] = arg[i];
    sd->arglen = arglen;
    sd->flag = flag;
    sema.max_count = 1;
    sema.init_count = 0;
    sema.option = 0;
    semid = CreateSema(&sema);
    sd->size = 4;
    sd->dst = &result;
    sd->semid = semid;
    sceSifWriteBackDCache(&_send_data, 3092);
    if (sceSifCallRpc(&_cd, 20, 0, &_send_data, 3092, &_rcv_data_rpc, 4,
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

/* The format request layout: sceMount's without the flag word. */
typedef struct t_fs_send_format {
    int   semid;
    void *dst;
    int   size;
    char  path1[1024];          /* +12   device */
    char  path2[1024];          /* +1036 block device */
    char  data[1024];           /* +2060 driver argument */
    int   arglen;               /* +3084 */
} fs_send_format_t;             /* 3088 bytes */

int sceFormat(char *dev, char *blockdev, char *arg, int arglen)
{
    ee_sema_t sema;
    int result;
    int semid;
    fs_send_format_t *sd;
    int done;
    int i;

    sd = (fs_send_format_t *)&_send_data;
    _sceFsWaitS(14);
    if (_fs_init == 0)
        sceFsInit();
    for (i = 0; i < 1024; i++) {
        sd->path1[i] = dev[i];
        if (sd->path1[i] == 0)
            break;
    }
    if (i == 1024)
        sd->path1[1023] = 0;
    if (blockdev == 0) {
        sd->path2[0] = 0;
    } else {
        for (i = 0; i < 1024; i++) {
            sd->path2[i] = blockdev[i];
            if (sd->path2[i] == 0)
                break;
        }
        if (i == 1024)
            sd->path2[1023] = 0;
    }
    if (arglen > 1024) {
        _sceFsSigSema();
        return -7;
    }
    for (i = 0; i < arglen; i++)
        sd->data[i] = arg[i];
    sd->arglen = arglen;
    sema.max_count = 1;
    sema.init_count = 0;
    sema.option = 0;
    semid = CreateSema(&sema);
    sd->size = 4;
    sd->dst = &result;
    sd->semid = semid;
    sceSifWriteBackDCache(&_send_data, 3088);
    if (sceSifCallRpc(&_cd, 14, 0, &_send_data, 3088, &_rcv_data_rpc, 4,
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

/* The devctl request layout. */
typedef struct t_fs_send_devctl {
    int   semid;
    void *dst;
    int   size;
    char  path[1024];           /* +12 */
    char  data[1024];           /* +1036 */
    int   cmd;                  /* +2060 */
    unsigned int arglen;        /* +2064 */
    void *bufp;                 /* +2068 */
    unsigned int buflen;        /* +2072 */
} fs_send_devctl_t;             /* 2076 bytes */

int sceDevctl(char *name, int cmd, char *arg, unsigned int arglen,
              void *bufp, unsigned int buflen)
{
    ee_sema_t sema;
    int result;
    int semid;
    fs_send_devctl_t *sd;
    int done;
    int i;

    sd = (fs_send_devctl_t *)&_send_data;
    _sceFsWaitS(23);
    if (_fs_init == 0)
        sceFsInit();
    for (i = 0; i < 1024; i++) {
        sd->path[i] = name[i];
        if (sd->path[i] == 0)
            break;
    }
    if (i == 1024)
        sd->path[1023] = 0;
    if (arglen > 1024 || buflen > 1024) {
        _sceFsSigSema();
        return -22;
    }
    for (i = 0; i < arglen; i++)
        sd->data[i] = arg[i];
    sd->arglen = arglen;
    sd->cmd = cmd;
    sema.max_count = 1;
    sema.init_count = 0;
    sema.option = 0;
    semid = CreateSema(&sema);
    sd->buflen = buflen;
    sd->dst = &result;
    sd->size = 4;
    sd->bufp = bufp;
    sd->semid = semid;
    sceSifWriteBackDCache(&_send_data, 2076);
    if (sceSifCallRpc(&_cd, 23, 0, &_send_data, 2076, &_rcv_data_rpc, 4,
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

/* sceAddDrv: register an EE-side device driver.  16-byte request, and
 * the failure value is -1 rather than the -11 the rest of the family
 * uses. */
int sceAddDrv(void *drv)
{
    ee_sema_t sema;
    int result;
    int semid;
    fs_send_t *sd;
    int done;

    sd = &_send_data;
    _sceFsWaitS(15);
    if (_fs_init == 0)
        sceFsInit();
    sd->fd = (int)drv;
    sema.max_count = 1;
    sema.init_count = 0;
    sema.option = 0;
    semid = CreateSema(&sema);
    _send_data.semid = semid;
    sd->dst = &result;
    sd->size = 4;
    if (sceSifCallRpc(&_cd, 15, 0, sd, 16, &_rcv_data_rpc, 4, 0, 0) < 0) {
        DeleteSema(semid);
        _sceFsSigSema();
        return -1;
    }
    done = *(volatile int *)((int)&_rcv_data_rpc | 0x20000000);
    _sceFsSigSema();
    if (done == 0) {
        DeleteSema(semid);
        return -1;
    }
    WaitSema(semid);
    DeleteSema(semid);
    return result;
}

/* The ioctl2 request layout. */
typedef struct t_fs_send_ioctl2 {
    int   semid;
    void *dst;
    int   size;
    int   fd;                   /* +12 */
    int   cmd;                  /* +16 */
    char  data[1024];           /* +20 */
    void *bufp;                 /* +1044 */
    unsigned int buflen;        /* +1048 */
    unsigned int arglen;        /* +1052 */
} fs_send_ioctl2_t;             /* 1056 bytes */

extern void *memcpy(void *dst, const void *src, unsigned int n);

/* sceIoctl2: the NULL-argument arm zeroes the length word even though
 * the unconditional `sd->arglen = arglen` below overwrites it -- gcc
 * 2.9 does not eliminate the dead store, and the original has it. */
int sceIoctl2(int fd, int cmd, char *arg, unsigned int arglen,
              void *bufp, unsigned int buflen)
{
    ee_sema_t sema;
    int result;
    int semid;
    fs_send_ioctl2_t *sd;
    iob_t *p;
    int done;

    sd = (fs_send_ioctl2_t *)&_send_data;
    p = get_iob(fd);
    _sceFsWaitS(26);
    if (_fs_init == 0)
        sceFsInit();
    if (p == 0 || p->used == 0) {
        _sceFsSigSema();
        return -9;
    }
    if (arglen > 1024 || buflen > 1024) {
        _sceFsSigSema();
        return -22;
    }
    if (arg == 0)
        sd->arglen = 0;
    else
        memcpy(sd->data, arg, arglen);
    sd->fd = p->fd;
    sd->cmd = cmd;
    sd->arglen = arglen;
    sema.max_count = 1;
    sema.init_count = 0;
    sema.option = 0;
    semid = CreateSema(&sema);
    sd->buflen = buflen;
    sd->dst = &result;
    sd->size = 4;
    sd->bufp = bufp;
    sd->semid = semid;
    sceSifWriteBackDCache(&_send_data, 1056);
    if (sceSifCallRpc(&_cd, 26, 0, &_send_data, 1056, &_rcv_data_rpc, 4,
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

/* The read/write request layout.  The 0x20000000 flag bit means the
 * caller's buffer is already an uncached-accelerated pointer, so it
 * needs no writeback -- and gcc shares that constant with the
 * uncached alias built for the completion word further down.  Reading
 * p->used for the first queue gate keeps that test distinct from the
 * cached `flags` value used after the RPC calls; this recovers the
 * original saved-register roles and makes the final 0x8000 test a fresh
 * `andi`.  The configured fail-closed retimer removes gcc's conservative
 * reload across CreateSema and restores the peeled queue probe's retail
 * delay-slot schedule. */
typedef struct t_fs_send_rw {
    int   semid;
    void *dst;
    int   size;
    int   fd;                   /* +12 */
    void *buf;                  /* +16 */
    int   len;                  /* +20 */
    int   reserved;             /* +24 */
    int   index;                /* +28 */
} fs_send_rw_t;                 /* 32 bytes */

extern int _rcv_data_cmd;

int sceRead(int fd, void *buf, int len)
{
    ee_sema_t sema;
    int result;
    int semid;
    fs_send_rw_t *sd;
    iob_t *p;
    int done;
    int i;
    int flags;
    int id;

    sd = (fs_send_rw_t *)&_send_data;
    p = get_iob(fd);
    _sceFsWaitS(2);
    if (_fs_init == 0) {
        _sceFsSigSema();
        return -1;
    }
    if (p == 0) {
        _sceFsSigSema();
        return -9;
    }
    flags = p->used;
    if (flags == 0) {
        _sceFsSigSema();
        return -9;
    }
    sd->fd = p->fd;
    sd->index = p - _iob;
    sd->buf = buf;
    sd->len = len;
    sema.max_count = 1;
    sema.init_count = 0;
    sema.option = 0;
    semid = CreateSema(&sema);
    sd->size = 4;
    sd->dst = &result;
    _send_data.semid = semid;
    if (p->used & 0x8000) {
        WaitSema(_fs_fsq_semid);
        for (i = 0; i < 32; i++) {
            if (_sceFs_q[i] == -1) {
                id = sd->semid;
                _sceFs_q[i] = id;
                sd->semid = -id;
                break;
            }
        }
        SignalSema(_fs_fsq_semid);
    }
    if ((flags & 0x20000000) == 0)
        sceSifWriteBackDCache(buf, len);
    sceSifWriteBackDCache(&_rcv_data_cmd, 164);
    sceSifWriteBackDCache(sd, 32);
    if (sceSifCallRpc(&_cd, 2, 0, &_send_data, 32, &_rcv_data_rpc, 4,
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
    if (flags & 0x8000) {
        DeleteSema(semid);
        return 0;
    }
    WaitSema(semid);
    DeleteSema(semid);
    return result;
}

/* The chstat request layout.  The 64-byte stat block is copied by
 * struct assignment; at align 4 gcc 2.9 does that as eight ldl/ldr +
 * sdl/sdr pairs, which is exactly what the original has. */
typedef struct t_fs_stat {
    int w[16];
} fs_stat_t;                    /* 64 bytes, align 4 */

typedef struct t_fs_send_chstat {
    int   semid;
    void *dst;
    int   size;
    int   cmask;                /* +12 */
    fs_stat_t stat;             /* +16 */
    char  path[1024];           /* +80 */
} fs_send_chstat_t;             /* 1104 bytes */

int sceChstat(char *name, void *stat, int cmask)
{
    ee_sema_t sema;
    int result;
    int semid;
    fs_send_chstat_t *sd;
    int done;
    int i;

    sd = (fs_send_chstat_t *)&_send_data;
    _sceFsWaitS(13);
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
    sd->stat = *(fs_stat_t *)stat;
    sd->cmask = cmask;
    sema.max_count = 1;
    sema.init_count = 0;
    sema.option = 0;
    semid = CreateSema(&sema);
    sd->size = 4;
    sd->dst = &result;
    sd->semid = semid;
    sceSifWriteBackDCache(&_send_data, 1104);
    if (sceSifCallRpc(&_cd, 13, 0, &_send_data, i + 81, &_rcv_data_rpc, 4,
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
