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

typedef struct sceMcSemaParam {
    int currentCount;
    int maxCount;
    int initCount;
    int numWaitThreads;
    int attr;
    int option;
} sceMcSemaParam;

extern int CreateSema(sceMcSemaParam *param);
extern int WaitSema(int sid);
extern int sceSifInitRpc(int mode);
extern int sceSifBindRpc(void *client, unsigned int sid, int mode);
extern int sceMcSync(int mode, int *cmd, int *result);
extern int printf(const char *fmt, ...);
extern char D_004D5870[];
extern char D_004D5888[];
extern char D_004D58B0[];

/* TODO: Ghidra-derived natural-C reconstruction currently emits 103
 * instructions versus the original 109.  The compiler collapses the
 * client/send/reply aliases that the original kept in saved registers. */
int sceMcInit(void)
{
    sceMcSemaParam sema;
    int i;
    int rpcResult;
    int *client;
    int *rpcClient;
    int *send;
    int *reply;

    if (semaidRegFunc < 0) {
        sema.option = 0;
        sema.initCount = 1;
        sema.maxCount = 1;
        semaidRegFunc = CreateSema(&sema);
    }

    sceMcSync(0, 0, 0);
    WaitSema(semaidRegFunc);
    sceSifInitRpc(0);
    send = sifParamOrd;
    reply = &retval;

    goto bind;

delay:
    for (i = 0x100000; i != 0; i--)
        ;

bind:
    rpcResult = sceSifBindRpc(mcClientID, 0x80000400, 0);
    if (rpcResult < 0) {
        printf(D_004D5870);
        for (;;)
            ;
    }
    client = mcClientID;
    if (client[9] == 0)
        goto delay;
    rpcClient = client;

    rpcResult = sceSifCallRpc(rpcClient, 0xFE, 0, send, 48,
                              reply, 12, 0, 0);
    SignalSema(semaidRegFunc);
    if (rpcResult < 0) {
        rpcClient[9] = 0;
        return rpcResult - 100;
    }
    if (reply[1] < 0x20A) {
        printf(D_004D5888);
        rpcClient[9] = 0;
        return -120;
    }
    if (reply[2] < 0x20E) {
        printf(D_004D58B0);
        client[9] = 0;
        return -121;
    }
    return reply[0];
}

extern int sceMcOpen(int port, int slot, const char *name, int flags);

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

/* ------------------------------------------------------------------
 * Module-internal helpers.
 * ------------------------------------------------------------------ */

#include "matching.h"

/* _lmcGetClientPtr: hand the two fixed blocks back to the caller and
 * refresh the module semaphore id the reply block carries at +60.  The
 * return value is the SIF client-data block itself. */
void *_lmcGetClientPtr(void **ppRetval, void **ppCmdNo)
{
    int *p;

    p = &retval;
    *ppRetval = p;
    *ppCmdNo = &mcRunCmdNo;
    p[15] = semaidRegFunc;
    return mcClientID;
}

/* mcHearAlarm: the alarm handler mcDelayThread arms.  Wakes the sleeping
 * thread from interrupt context; `ei` is not reachable from C. */

extern int iWakeupThread(int thid);

void mcHearAlarm(int id, unsigned short time, void *common)
{
    iWakeupThread((int)common);
    PS2_ASM("sync.l\n\tei");
}

/* mcDelayThread: sleep this thread for `usec` microseconds.  The alarm
 * handler address is materialised BEFORE GetThreadId so it survives the
 * call in s0, and SleepThread is a real tail call. */

extern int GetThreadId(void);
extern int SetAlarm(unsigned short time, void *handler, void *common);
extern void SleepThread(void);

void mcDelayThread(unsigned short usec)
{
    SetAlarm(usec, (void *)mcHearAlarm, (void *)GetThreadId());
    SleepThread();
}

/* mceGetInfoApdx: scatter the three fields of a card-info reply into
 * whichever of the three caller-supplied destinations were registered.
 * The reply was written by IOP DMA, so it is read through the
 * uncached-accelerated alias -- the `or` sits in the first test's delay
 * slot, i.e. it is unconditional. */

extern int *typeAddr;
extern int *freeAddr;
extern int *formAddr;

void mceGetInfoApdx(void *info)
{
    int *p;

    p = (int *)((int)info | 0x20000000);
    if (typeAddr != 0)
        *typeAddr = p[0];
    if (freeAddr != 0)
        *freeAddr = p[1];
    if (formAddr != 0)
        *formAddr = p[36];
}

/* ------------------------------------------------------------------
 * sceMcSync: collect the result of whatever command is in flight.
 *
 * mode 0 blocks (polling every 60us) and mode 1 is a one-shot test.
 * The return value is "the command has finished" -- the `sltiu s0,s0,1`
 * that turns the busy flag into a done flag sits in the *cmd store's
 * branch delay slot, so it is written before that store in the source.
 * Only a finished command releases the semaphore taken by the call that
 * started it.
 * ------------------------------------------------------------------ */

extern int sceSifCheckStatRpc(void *cd);
extern void mcDelayThread(unsigned short usec);

int sceMcSync(int mode, int *cmd, int *result)
{
    int done;

    if (mcRunCmdNo == 0)
        return -1;
    done = sceSifCheckStatRpc(mcClientID);
    if (mode == 0 && done != 0) {
        while (sceSifCheckStatRpc(mcClientID) != 0)
            mcDelayThread(60);
        done = 0;
    }
    done = (done == 0);
    if (cmd != 0)
        *cmd = mcRunCmdNo;
    if (done != 0) {
        mcRunCmdNo = 0;
        if (result != 0)
            *result = retval;
        SignalSema(semaidRegFunc);
    }
    return done;
}

/* ------------------------------------------------------------------
 * sceMcGetEntSpace: the long-payload variant of the module template.
 * The pathname goes into the 1044-byte `sifParamFname` block (port and
 * slot at +0/+4, the name strncpy'd into +20 and hard-terminated), and
 * an empty or absent name is rejected with -210 before the block is
 * touched at all.
 * ------------------------------------------------------------------ */

extern int sifParamFname[261];      /* 0x00996FF0, 1044 bytes */
extern char *strncpy(char *dst, const char *src, int n);

int sceMcGetEntSpace(int port, int slot, const char *name)
{
    int rc;

    if (PollSema(semaidRegFunc) < 0)
        return -200;
    if (mcClientID[9] == 0) {
        SignalSema(semaidRegFunc);
        return -100;
    }
    if (name == 0 || *name == 0) {
        SignalSema(semaidRegFunc);
        return -210;
    }
    sifParamFname[0] = port;
    sifParamFname[1] = slot;
    strncpy((char *)sifParamFname + 20, name, 1023);
    *((char *)sifParamFname + 1043) = 0;
    rc = sceSifCallRpc(mcClientID, 18, 1, sifParamFname, 1044,
                       &retval, 4, 0, 0);
    if (rc == 0)
        mcRunCmdNo = 18;
    else
        SignalSema(semaidRegFunc);
    return rc;
}

/* ------------------------------------------------------------------
 * Pathname commands: sceMcOpen and sceMcDelete.
 *
 * Same 1044-byte sifParamFname payload as sceMcGetEntSpace, but here
 * the original keeps the ADDRESS OF THE NAME FIELD live and reaches the
 * block header backwards from it (`addiu v1,s0,-20`) rather than
 * indexing the block twice -- so the strncpy destination is the named
 * local and the header pointer is derived from it.
 * ------------------------------------------------------------------ */

int sceMcOpen(int port, int slot, const char *name, int flags)
{
    char *dst;
    int *p;
    int rc;

    if (PollSema(semaidRegFunc) < 0)
        return -200;
    if (mcClientID[9] == 0) {
        SignalSema(semaidRegFunc);
        return -100;
    }
    if (name == 0 || *name == 0) {
        SignalSema(semaidRegFunc);
        return -210;
    }
    dst = (char *)sifParamFname + 20;
    strncpy(dst, name, 1023);
    p = (int *)(dst - 20);
    p[0] = port;
    p[2] = flags;
    p[1] = slot;
    ((char *)p)[1043] = 0;
    rc = sceSifCallRpc(mcClientID, 2, 1, p, 1044, &retval, 4, 0, 0);
    if (rc == 0)
        mcRunCmdNo = 2;
    else
        SignalSema(semaidRegFunc);
    return rc;
}

int sceMcDelete(int port, int slot, const char *name)
{
    char *dst;
    int *p;
    int rc;

    if (PollSema(semaidRegFunc) < 0)
        return -200;
    if (mcClientID[9] == 0) {
        SignalSema(semaidRegFunc);
        return -100;
    }
    if (name == 0 || *name == 0) {
        SignalSema(semaidRegFunc);
        return -210;
    }
    dst = (char *)sifParamFname + 20;
    strncpy(dst, name, 1023);
    p = (int *)(dst - 20);
    p[0] = port;
    p[1] = slot;
    ((char *)p)[1043] = 0;
    p[2] = 0;
    rc = sceSifCallRpc(mcClientID, 15, 1, p, 1044, &retval, 4, 0, 0);
    if (rc == 0)
        mcRunCmdNo = 15;
    else
        SignalSema(semaidRegFunc);
    return rc;
}

/* ------------------------------------------------------------------
 * sceMcRead: the caller's buffer and the 192-byte alignment scratch
 * block are both flushed out of the data cache before the IOP is told
 * about them, and the RPC carries an end-function
 * (mceIntrReadFixAlign) plus the scratch block as its end-data -- that
 * is what puts the misaligned head and tail bytes back after the DMA.
 * ------------------------------------------------------------------ */

extern int sifParamNext[48];        /* 0x00997440, 192-byte align scratch */
extern void sceSifWriteBackDCache(void *addr, int size);
extern void mceIntrReadFixAlign(void *arg);

int sceMcRead(int fd, void *buf, int size)
{
    int *p;
    int rc;

    if (PollSema(semaidRegFunc) < 0)
        return -200;
    if (mcClientID[9] == 0) {
        SignalSema(semaidRegFunc);
        return -100;
    }
    p = sifParamOrd;
    p[0] = fd;
    p[7] = (int)sifParamNext;
    p[6] = (int)buf;
    p[3] = size;
    sceSifWriteBackDCache(buf, size);
    sceSifWriteBackDCache(sifParamNext, 192);
    rc = sceSifCallRpc(mcClientID, 5, 1, p, 48, &retval, 4,
                       (void *)mceIntrReadFixAlign, sifParamNext);
    if (rc == 0)
        mcRunCmdNo = 5;
    else
        SignalSema(semaidRegFunc);
    return rc;
}

/* sceMcChdir: pathname command 12.  The current-directory buffer the
 * IOP fills in is flushed before the call and copied out afterwards by
 * the RPC end function mceStorePwd, whose end-data is the caller's
 * destination pointer. */

extern char currentDir[1024];       /* 0x00997500 */
extern void mceStorePwd(char *dst);

int sceMcChdir(int port, int slot, const char *name, char *pwd)
{
    int *p;
    int rc;

    if (PollSema(semaidRegFunc) < 0)
        return -200;
    if (mcClientID[9] == 0) {
        SignalSema(semaidRegFunc);
        return -100;
    }
    if (name == 0 || *name == 0) {
        SignalSema(semaidRegFunc);
        return -210;
    }
    p = sifParamFname;
    p[0] = port;
    p[4] = (int)currentDir;
    p[1] = slot;
    strncpy((char *)p + 20, name, 1023);
    ((char *)p)[1043] = 0;
    sceSifWriteBackDCache(currentDir, 1024);
    rc = sceSifCallRpc(mcClientID, 12, 1, p, 1044, &retval, 4,
                       (void *)mceStorePwd, pwd);
    if (rc == 0)
        mcRunCmdNo = 12;
    else
        SignalSema(semaidRegFunc);
    return rc;
}

/* sceMcGetDir: pathname command 13.  Six arguments -- the EE register
 * convention puts the fifth and sixth in $t0/$t1 -- and the caller's
 * entry array is flushed only when the requested count is not negative
 * (a negative count means "just count them", no buffer). */
int sceMcGetDir(int port, int slot, const char *name, int flags,
                int maxent, void *table)
{
    int *p;
    int rc;

    if (PollSema(semaidRegFunc) < 0)
        return -200;
    if (mcClientID[9] == 0) {
        SignalSema(semaidRegFunc);
        return -100;
    }
    if (name == 0 || *name == 0) {
        SignalSema(semaidRegFunc);
        return -210;
    }
    p = sifParamFname;
    p[0] = port;
    p[1] = slot;
    p[2] = flags;
    p[3] = maxent;
    p[4] = (int)table;
    strncpy((char *)p + 20, name, 1023);
    ((char *)p)[1043] = 0;
    if (maxent >= 0)
        sceSifWriteBackDCache(table, maxent << 6);
    rc = sceSifCallRpc(mcClientID, 13, 1, p, 1044, &retval, 4, 0, 0);
    if (rc == 0)
        mcRunCmdNo = 13;
    else
        SignalSema(semaidRegFunc);
    return rc;
}

/* sceMcGetInfo: command 1.  Each of the three optional out-pointers
 * turns into a "please fetch this" flag in the payload and is also
 * parked in a module global, because the reply is scattered by the RPC
 * end function mceGetInfoApdx rather than by this function.  The flags
 * are written with real branches, not a materialised comparison. */

int sceMcGetInfo(int port, int slot, int *type, int *free, int *format)
{
    int rc;

    if (PollSema(semaidRegFunc) < 0)
        return -200;
    if (mcClientID[9] == 0) {
        SignalSema(semaidRegFunc);
        return -100;
    }
    sifParamOrd[1] = port;
    sifParamOrd[2] = slot;
    sifParamOrd[7] = (int)sifParamNext;
    if (type != 0)
        sifParamOrd[5] = 1;
    else
        sifParamOrd[5] = 0;
    if (free != 0)
        sifParamOrd[4] = 1;
    else
        sifParamOrd[4] = 0;
    if (format != 0)
        sifParamOrd[3] = 1;
    else
        sifParamOrd[3] = 0;
    typeAddr = type;
    freeAddr = free;
    formAddr = format;
    sceSifWriteBackDCache(sifParamNext, 192);
    rc = sceSifCallRpc(mcClientID, 1, 1, sifParamOrd, 48, &retval, 4,
                       (void *)mceGetInfoApdx, sifParamNext);
    if (rc == 0)
        mcRunCmdNo = 1;
    else
        SignalSema(semaidRegFunc);
    return rc;
}

/* MATCHES, 90 words.  The last 2 were a pure adjacent swap: the original
 * sets up FlushCache's `move a0,zero` argument BEFORE the `sw s0,16(s1)`
 * payload store, gcc emits them the other way round.  No source shape
 * reaches it -- both orders of `q[63] = 0` against `p[4] = (int)q` were
 * swept (the order below is the better one, 3 words the other way), as
 * were a `mode` local for FlushCache's constant, LAUNDER_V(q),
 * LAUNDER(p), LAUNDER2(p, q) and the `*(int *)((char *)p + 16)` store
 * spelling; all leave it at 2.
 *
 * Closed with `--swap-adjacent sceMcRename:53`.  NOTE THE INDEX: the
 * differing words are at 56/57 of the FINAL disassembly, and a previous
 * sweep of sites 54..58 -- read off that disassembly -- found nothing.
 * --swap-adjacent counts instructions in the POST-PROCESSED ASM, before
 * gas steals `sb $0,63($16)` into the jal's delay slot, so the site is
 * three lower than the diff index.  Dump the indices with
 * fix_cc_asm.function_at()/RE_INSN rather than guessing them from the
 * diff.
 *
 * sceMcRename: command 14 -- but the in-flight command code recorded is
 * 19, not 14.  Two names: the path goes in the usual 1024-byte field,
 * the new name into the 32-byte tail of buffFileInfo, whose base is
 * reached backwards from the strncpy destination the same way the
 * pathname commands reach their header. */

extern char buffFileInfo[64];       /* 0x00996F80 */
extern void FlushCache(int mode);

int sceMcRename(int port, int slot, const char *name, const char *newname)
{
    int *p;
    char *q;
    int rc;

    if (PollSema(semaidRegFunc) < 0)
        return -200;
    if (mcClientID[9] == 0) {
        SignalSema(semaidRegFunc);
        return -100;
    }
    if (name == 0 || *name == 0 || newname == 0) {
        SignalSema(semaidRegFunc);
        return -210;
    }
    p = sifParamFname;
    p[0] = port;
    p[1] = slot;
    p[2] = 16;
    strncpy((char *)p + 20, name, 1023);
    q = buffFileInfo + 32;
    ((char *)p)[1043] = 0;
    strncpy(q, newname, 32);
    q -= 32;
    q[63] = 0;
    p[4] = (int)q;
    FlushCache(0);
    rc = sceSifCallRpc(mcClientID, 14, 1, p, 1044, &retval, 4, 0, 0);
    if (rc == 0)
        mcRunCmdNo = 19;
    else
        SignalSema(semaidRegFunc);
    return rc;
}

/* ------------------------------------------------------------------
 * The two RPC end functions.  Both run on the SIF interrupt after the
 * IOP's reply DMA has landed, so both read the shared blocks through
 * the uncached-accelerated alias.
 * ------------------------------------------------------------------ */

extern unsigned int strlen(const char *s);
extern void *memcpy(void *dst, const void *src, int n);

/* mceStorePwd: copy the current directory the IOP just filled in into
 * the caller's buffer, clamped to 1023 characters.  strlen is called
 * twice -- the clamp test and the length -- which is why the compare
 * result feeds a branch-likely rather than a cmov, and the source
 * pointer is re-formed for the memcpy instead of being held. */
void mceStorePwd(char *dst)
{
    char *p;
    int len;

    if (dst == 0)
        return;
    p = (char *)((int)currentDir | 0x20000000);
    if (strlen(p) < 1024)
        len = strlen(p);
    else
        len = 1023;
    memcpy(dst, (char *)((int)currentDir | 0x20000000), len);
    dst[len] = 0;
}

/* mceIntrReadFixAlign: sceMcRead's DMA can only land on aligned words,
 * so the head and tail bytes are staged in the scratch block and copied
 * into the caller's buffer here.  Each loop re-loads its count from the
 * block every iteration -- the bound lives in the loop header, not in a
 * local above it. */
void mceIntrReadFixAlign(void *arg)
{
    int *p;
    unsigned char *d;
    int i;

    p = (int *)((int)arg | 0x20000000);
    if (p[0] != 0) {
        d = (unsigned char *)p[2];
        for (i = 0; i < p[0]; i++) {
            unsigned char *s = (unsigned char *)p + 16;
            *d++ = s[i];
        }
    }
    if (p[1] != 0) {
        d = (unsigned char *)p[3];
        for (i = 0; i < p[1]; i++) {
            unsigned char *s = (unsigned char *)p + 80;
            *d++ = s[i];
        }
    }
}

/* ------------------------------------------------------------------
 * sceMcWrite: command 6.
 *
 * The IOP can only DMA from a 16-byte-aligned address, so a write whose
 * buffer does not start on a boundary is split: the leading bytes up to
 * the next boundary are staged inline in the payload at +32, and the
 * DMA covers the aligned remainder.  A write of 16 bytes or fewer is
 * staged whole.  The staging copy names the payload array at each use,
 * so its address is re-formed inside the loop the way the original
 * does, and the loop counter is compared UNSIGNED (`sltu`).
 * ------------------------------------------------------------------ */

int sceMcWrite(int fd, const void *buf, int size)
{
    unsigned int i;
    int rc;

    if (PollSema(semaidRegFunc) < 0)
        return -200;
    if (mcClientID[9] == 0) {
        SignalSema(semaidRegFunc);
        return -100;
    }
    sifParamOrd[0] = fd;
    if (size < 17) {
        sifParamOrd[5] = size;
        sifParamOrd[6] = 0;
        sifParamOrd[3] = 0;
    } else {
        int head = ((((int)buf - 1) & 0xfffffff0) - ((int)buf - 16));

        sifParamOrd[6] = (int)buf + head;
        sifParamOrd[3] = size - head;
        sifParamOrd[5] = head;
    }
    for (i = 0; i < (unsigned int)sifParamOrd[5]; i++)
        ((char *)sifParamOrd)[i + 32] = ((const char *)buf)[i];
    FlushCache(0);
    rc = sceSifCallRpc(mcClientID, 6, 1, sifParamOrd, 48, &retval, 4, 0, 0);
    if (rc == 0)
        mcRunCmdNo = 6;
    else
        SignalSema(semaidRegFunc);
    return rc;
}
