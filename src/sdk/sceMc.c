/* PS2 SDK sceMc (memory card) client side.
 *
 * ---------------------------------------------------------------------
 * BUILD FLAG.  Like scePad.c and sceCd.c, this translation unit was
 * built WITHOUT `-fno-schedule-insns`; configure.py carries
 * FILE_CFLAGS_OVERRIDE["sceMc.c"] = "-O2 -G0", under which every
 * function below matches byte for byte.
 *
 * Every call here follows one template: grab the module semaphore
 * without blocking, refuse if the IOP-side module never bound, marshal
 * the arguments into the fixed `sifParamOrd` block, and fire an RPC.
 * On success the semaphore is deliberately NOT released -- it stays
 * held until sceMcSync() collects the result, which is what makes the
 * API asynchronous -- and `mcRunCmdNo` records which command is in
 * flight.  On failure the semaphore is released immediately.
 *
 * Writing that tail as `if (rc != 0) { release; return rc; } record;
 * return rc;` costs a word: gcc then materialises the second return
 * constant separately instead of sharing the `move v0,s0`.  The
 * if/else below is the original shape.
 */

extern int PollSema(int sid);
extern int SignalSema(int sid);
extern int sceSifCallRpc(void *pCd, unsigned int fno, int mode, void *send,
                         int ssize, void *recv, int rsize, void *ef, void *ea);

extern int mcClientID[16];   /* 0x00996F40, SIF-RPC client data */
extern int sifParamOrd[16];  /* 0x00996FC0, 48-byte RPC send block */
extern int retval;           /* 0x00998500, RPC reply word */
extern int mcRunCmdNo;       /* 0x004AD488, command currently in flight */
extern int semaidRegFunc;    /* 0x004AD48C, module semaphore id */

extern int sceMcOpen(int a0, int a1, int a2, int a3);

/* sceMcMkdir: opens the directory with sceMcOpen (mode 64, i.e.
 * O_CREAT-ish) and, if it returns a falsy fd, records the error code. */
int sceMcMkdir(int a0, int a1, int a2)
{
    int fd = sceMcOpen(a0, a1, a2, 64);

    if (!fd)
        mcRunCmdNo = 11;
    return fd;
}


int sceMcClose(int fd)
{
    int rc;

    if (PollSema(semaidRegFunc) < 0)
        return -200;
    if (mcClientID[9] == 0) {
        SignalSema(semaidRegFunc);
        return -100;
    }
    sifParamOrd[0] = fd;
    rc = sceSifCallRpc(mcClientID, 3, 1, sifParamOrd, 48, &retval, 4, 0, 0);
    if (rc == 0)
        mcRunCmdNo = 3;
    else
        SignalSema(semaidRegFunc);
    return rc;
}

int sceMcFlush(int fd)
{
    int rc;

    if (PollSema(semaidRegFunc) < 0)
        return -200;
    if (mcClientID[9] == 0) {
        SignalSema(semaidRegFunc);
        return -100;
    }
    sifParamOrd[0] = fd;
    rc = sceSifCallRpc(mcClientID, 10, 1, sifParamOrd, 48, &retval, 4, 0, 0);
    if (rc == 0)
        mcRunCmdNo = 10;
    else
        SignalSema(semaidRegFunc);
    return rc;
}

int sceMcFormat(int port, int slot)
{
    int rc;

    if (PollSema(semaidRegFunc) < 0)
        return -200;
    if (mcClientID[9] == 0) {
        SignalSema(semaidRegFunc);
        return -100;
    }
    sifParamOrd[2] = slot;
    sifParamOrd[1] = port;
    rc = sceSifCallRpc(mcClientID, 16, 1, sifParamOrd, 48, &retval, 4, 0, 0);
    if (rc == 0)
        mcRunCmdNo = 16;
    else
        SignalSema(semaidRegFunc);
    return rc;
}

int sceMcUnformat(int port, int slot)
{
    int rc;

    if (PollSema(semaidRegFunc) < 0)
        return -200;
    if (mcClientID[9] == 0) {
        SignalSema(semaidRegFunc);
        return -100;
    }
    sifParamOrd[2] = slot;
    sifParamOrd[1] = port;
    rc = sceSifCallRpc(mcClientID, 17, 1, sifParamOrd, 48, &retval, 4, 0, 0);
    if (rc == 0)
        mcRunCmdNo = 17;
    else
        SignalSema(semaidRegFunc);
    return rc;
}

int sceMcSeek(int fd, int offset, int whence)
{
    int rc;

    if (PollSema(semaidRegFunc) < 0)
        return -200;
    if (mcClientID[9] == 0) {
        SignalSema(semaidRegFunc);
        return -100;
    }
    sifParamOrd[0] = fd;
    sifParamOrd[4] = offset;
    sifParamOrd[5] = whence;
    rc = sceSifCallRpc(mcClientID, 4, 1, sifParamOrd, 48, &retval, 4, 0, 0);
    if (rc == 0)
        mcRunCmdNo = 4;
    else
        SignalSema(semaidRegFunc);
    return rc;
}

int sceMcChangeThreadPriority(int prio)
{
    int rc;

    if (PollSema(semaidRegFunc) < 0)
        return -200;
    if (mcClientID[9] == 0) {
        SignalSema(semaidRegFunc);
        return -100;
    }
    sifParamOrd[5] = prio;
    rc = sceSifCallRpc(mcClientID, 20, 1, sifParamOrd, 48, &retval, 4, 0, 0);
    if (rc == 0)
        mcRunCmdNo = 20;
    else
        SignalSema(semaidRegFunc);
    return rc;
}

int sceMcGetSlotMax(int port)
{
    int rc;

    if (PollSema(semaidRegFunc) < 0)
        return -200;
    if (mcClientID[9] == 0) {
        SignalSema(semaidRegFunc);
        return -100;
    }
    sifParamOrd[1] = port;
    rc = sceSifCallRpc(mcClientID, 21, 0, sifParamOrd, 48, &retval, 4, 0, 0);
    if (rc != 0) {
        SignalSema(semaidRegFunc);
        return rc;
    }
    SignalSema(semaidRegFunc);
    return retval;
}
