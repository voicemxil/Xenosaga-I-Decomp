/* PS2 SDK sceSif (EE<->IOP SIF) low-level register/table accessors.
 *
 * These reach a fixed low-memory SIF control block. The block is named
 * in config/symbol_addrs.txt (`soft_reg` at 0x00991A00 for the eight
 * software registers, `_data_table_009918D8` for the command-buffer
 * table), so referencing it by name gets the linker's ordinary %hi/%lo
 * relocation and the original's lui+addiu -- where a numeric
 * `(int *)0x00991A00` cast gets lui+ori. That is the whole reason these
 * five used to be inline-asm bodies; see CONTRIBUTING.md, "Raw
 * addresses".
 */

#include "matching.h"

extern int soft_reg[8];
extern int _data_table_009918D8[16];

int sceSifGetSreg(int idx)
{
    return soft_reg[idx];
}

/* Two steering constructs here, both for instruction placement only:
 *   - PIN+LAUNDER keeps the array base in v0. Left alone gcc picks v1,
 *     because v0 is the return register and the return value is live;
 *     the original lets the base occupy v0 and then overwrites it.
 *   - PASSTHRU emits the `move v0,a1` (return value) BEFORE the store,
 *     which is what leaves the store, not the move, in the jr delay
 *     slot. Written as a plain `return val;` the two swap places. */
int sceSifSetSreg(int idx, int val)
{
    PIN(int *base, "$2");
    int r;

    base = soft_reg;
    LAUNDER(base);
    PASSTHRU(r, val);
    base[idx] = val;
    return r;
}

/* The SIF command-buffer table: entries 3/4 are the system command
 * buffer and its size, 5/6 the user one. Both setters return the
 * previous buffer.
 *
 * The two stores in each are written size-then-buffer to fake, under
 * -fno-schedule-insns, the schedule the first scheduling pass gives for
 * free -- exactly like sceSifAddCmdHandler below. If sceSif.c gets
 * FILE_CFLAGS_OVERRIDE = "-O2 -G0", swap BOTH pairs back to
 * buffer-then-size at the same time as AddCmdHandler's; those three
 * swaps together take this file to 23 match / 0 not under the new flag
 * (measured 2026-08-15). */
void *sceSifSetSysCmdBuffer(void *buf, int size)
{
    void *old = (void *)_data_table_009918D8[3];

    _data_table_009918D8[3] = (int)buf;
    _data_table_009918D8[4] = size;
    return old;
}

void *sceSifSetCmdBuffer(void *buf, int size)
{
    void *old = (void *)_data_table_009918D8[5];

    _data_table_009918D8[5] = (int)buf;
    _data_table_009918D8[6] = size;
    return old;
}

void *sceSifGetDataTable(void)
{
    return _data_table_009918D8;
}

/* Thin dispatch wrappers around the underscore-prefixed internal SIF
 * loader/heap primitives (same house pattern as sceFs.c). */

extern int sceSifFreeSysMemory(int addr);
/* (path, section name, destination, mode) -- the section name is what
 * sceSifLoadElf below hard-codes to "all". */
extern int _sceSifLoadElfPart(const char *path, const char *sec, void *dest,
                              int mode);
extern int _sceSifLoadModuleBuffer(int nAddr, int arglen, const void *args,
                                   int *result);
extern int _sceSifLoadModule(const char *path, int arglen, const void *args,
                             int *result, int mode);

int sceSifFreeIopHeap(int addr)
{
    return sceSifFreeSysMemory(addr);
}

int sceSifLoadElfPart(const char *path, const char *sec, void *dest)
{
    return _sceSifLoadElfPart(path, sec, dest, 1);
}

int sceSifLoadModuleBuffer(int a0, int a1, int a2)
{
    int local;

    return _sceSifLoadModuleBuffer(a0, a1, (const void *)a2, &local);
}

int sceSifLoadStartModule(int a0, int a1, int a2, int a3)
{
    return _sceSifLoadModule((const char *)a0, a1, (const void *)a2,
                             (int *)a3, 0);
}

int sceSifLoadStartModuleBuffer(int a0, int a1, int a2, int a3)
{
    return _sceSifLoadModuleBuffer(a0, a1, (const void *)a2, (int *)a3);
}

int sceSifLoadModule(int a0, int a1, int a2)
{
    int local;

    return _sceSifLoadModule((const char *)a0, a1, (const void *)a2,
                             &local, 0);
}

/* sceSifExitCmd: tears down the SIF cmd layer -- disables the SIF0 DMA
 * channel, removes its handler, and clears the init-check flag.
 * `sif0_handleid` and `_cmd_init_check` are named fixed addresses from
 * config/symbol_addrs.txt.
 *
 * The v0/v1 address-temp difference that had this parked as assembly
 * was not an allocator tie-break: DisableDmac() and RemoveDmacHandler()
 * return the previous state / handler id, so with their true `int`
 * return type the call result occupies v0 and both address temps move
 * to v1 by themselves -- the same wrong-prototype cause as
 * sceCdCallback in sceCd.c. No PIN or LAUNDER needed. */

extern int DisableDmac(int channel);
extern int RemoveDmacHandler(int channel, int handlerid);
extern int sif0_handleid;
extern int _cmd_init_check;

void sceSifExitCmd(void)
{
    DisableDmac(5);
    RemoveDmacHandler(5, sif0_handleid);
    _cmd_init_check = 0;
}

/* sceSifIsAliveIop: bit 0x10000 of SIF register 4 is the "IOP is up"
 * flag.
 *
 * The phrasing is load-bearing. Written as a boolean expression --
 * `return (x & 0x10000) != 0;`, `!!(...)`, or any cast of it -- gcc
 * folds the single-bit test into sra+andi (2 words) and the function is
 * one word short. Written as an explicit branch it keeps the mask in a
 * register (`lui`+`and`) and materialises the boolean with sltu, which
 * is what the original does. `x & 0x10000; return x > 0;` also matches;
 * the branch form is kept because it is the clearer source. */

int sceSifIsAliveIop(void)
{
    if (sceSifGetReg(4) & 0x10000)
        return 1;
    return 0;
}

/* sceSifLoadElf: load every section of an ELF from the IOP-side loader.
 * The "fixed unnamed 0x4D4BE8 constant" that had this parked as
 * assembly is just a string literal: asm/data/cod/2BE280.rodata.s
 * labels 0x004D4BE8 as `.asciz "all"`, the section-name argument
 * _sceSifLoadElfPart takes (compare sceSifLoadElfPart above, which
 * passes the caller's own section name through). */

int sceSifLoadElf(const char *path, void *dest)
{
    return _sceSifLoadElfPart(path, "all", dest, 1);
}

/* sceSifRemoveCmdHandler / sceSifAddCmdHandler: pick one of two SIF
 * command-table pointers out of the fixed low-memory _data_table block
 * (base 0x9918D8, named `_data_table_009918D8` in
 * config/symbol_addrs.txt, same block sceSifSetSysCmdBuffer/
 * sceSifSetCmdBuffer index into above) by the sign of the handler
 * index, then zero (Remove) or store (Add) the selected slot. */

void sceSifRemoveCmdHandler(int idx)
{
    int off = idx << 3;
    PIN(int base, "$4");

    if (idx < 0)
        base = _data_table_009918D8[3];
    else
        base = _data_table_009918D8[5];
    off = off + base;
    *(int *)off = 0;
}

/* NOTE for the flag flip: the two stores below are written in reverse
 * field order on purpose, to fake -- under -fno-schedule-insns -- the
 * schedule the first scheduling pass produces for free.  If sceSif.c
 * ever gets FILE_CFLAGS_OVERRIDE = "-O2 -G0" (see sceCd.c's header for
 * the same request), swap them back to slot[0] then slot[1]: that one
 * change is the ONLY thing standing between this file and 23 match / 0
 * not at "-O2 -G0" (measured 2026-08-15) TOGETHER WITH the two stores
 * in sceSifSetSysCmdBuffer/sceSifSetCmdBuffer above, and the flag is what
 * sceSifSendCmd, sceSifRpcLoop, sceSifAllocIopHeap, sceSifAllocSysMemory,
 * sceSifFreeSysMemory, sceSifSearchModuleByName and
 * sceSifSearchModuleByAddress are all waiting on. */
void sceSifAddCmdHandler(int idx, void (*handler)(void), void *arg)
{
    int off = idx << 3;
    PIN(int base, "$4");
    int *slot;

    if (idx < 0)
        base = _data_table_009918D8[3];
    else
        base = _data_table_009918D8[5];
    off = off + base;
    slot = (int *)off;
    slot[0] = (int)handler;
    slot[1] = (int)arg;
}

/* sceSifSyncIop: reads sceSifGetReg(4) (an IOP sync/status register),
 * and if bit 0x40000 is set, re-inits the debug tty and always returns
 * 1; otherwise returns 0. A plain C if/return body gets GCC's own
 * synthesized epilogue, which already matches (see sceSifExitCmd's
 * comment for why that works once the function is a straight
 * fall-off-the-end/return shape rather than a shared-epilogue branch),
 * so this compiles as real C. */

extern int sceSifGetReg(int a0);
extern void sceResetttyinit(void);

int sceSifSyncIop(void)
{
    if (sceSifGetReg(4) & 0x40000) {
        sceResetttyinit();
        return 1;
    }
    return 0;
}

/* sceSifExitRpc: tears down the SIF cmd layer via sceSifExitCmd, then
 * clears the RPC init-check flag `_sceSifInitCheck`, a named fixed
 * address from config/symbol_addrs.txt -- so this compiles as real C
 * (same reasoning as sceSifExitCmd/sceSifSyncIop above). */

void sceSifExitCmd(void);
extern int _sceSifInitCheck;

void sceSifExitRpc(void)
{
    sceSifExitCmd();
    _sceSifInitCheck = 0;
}

/* sceSifUnloadModule: version-checks the loadfile linkage (_lf_bind /
 * _lf_version), then dispatches an "unload" opcode through the same
 * SIF-RPC boilerplate as the scePad RPC family in scePad.c (fixed
 * low-memory payload block, named `_senddata` in
 * config/symbol_addrs.txt, base 0x994840, and client-data block, named
 * `cd_00994A40`, base 0x994A40), returning the reply on success or a
 * fixed error code otherwise. */

extern int _lf_bind(int a0);
extern int _lf_version(void);
struct SifRpcClientData;
extern int sceSifCallRpc(struct SifRpcClientData *pCd, unsigned int nFno, int nMode,
                         void *pSend, int nSSize, void *pRecv, int nRSize,
                         void *pEndFunc, void *pEndArg);
extern struct SifRpcClientData cd_00994A40;

/* The loadfile RPC send/receive block at 0x994840. Its first word is the
 * scalar argument (module id, address) or reply; sceSifSearchModuleByName
 * fills the 252-byte name buffer that follows. */
typedef struct SifLfSendData {
    int  arg;           /*   0 */
    int  pad;           /*   4 */
    char name[252];     /*   8..259 */
    char sec[252];      /* 260..511  second name buffer, used only by
                         *           _sceSifLoadElfPart */
} SifLfSendData;

extern SifLfSendData _senddata;

int sceSifUnloadModule(int modId)
{
    PIN(int id, "$17") = modId;

    if (_lf_bind(id) < 0)
        return (int)0xffff0000;
    if (_lf_version() != 0)
        return (int)0xfffefffc;
    _senddata.arg = id;
    {
        void *pCd = &cd_00994A40;

        LAUNDER_V(pCd);
        if (sceSifCallRpc(pCd, 8, 0, &_senddata, 4, &_senddata, 4, 0, 0) < 0)
            return (int)0xfffeffff;
    }
    return _senddata.arg;
}

/* ------------------------------------------------------------------
 * SIF RPC server side: the queue/server-data linked lists.
 *
 * Two intrusive singly-linked lists, both walked under DIntr/EIntr:
 *   - all registered queues hang off _data_table_00993280[10]
 *     (0x00993280 + 40) through SifRpcDataQueue.next;
 *   - each queue's registered servers hang off its own `link` field
 *     through SifRpcServerData.link, and its *pending requests* hang
 *     off `start` through SifRpcServerData.next.
 * Field offsets are from the original code; the names follow the
 * public PS2 SDK's sceSifRpc structures. `pad` covers the fields this
 * translation unit never touches.
 * ------------------------------------------------------------------ */

typedef struct SifRpcServerData SifRpcServerData;
typedef struct SifRpcDataQueue SifRpcDataQueue;

struct SifRpcServerData {
    int   sid;                  /*  0 */
    void *func;                 /*  4 */
    void *buff;                 /*  8 */
    int   size;                 /* 12 */
    void *cfunc;                /* 16 */
    void *cbuff;                /* 20 */
    int   pad[8];               /* 24..55 */
    SifRpcServerData *link;     /* 56  next server registered on `base` */
    SifRpcServerData *next;     /* 60  next pending request */
    SifRpcDataQueue  *base;     /* 64 */
};

struct SifRpcDataQueue {
    int   thread_id;            /*  0 */
    int   active;               /*  4 */
    SifRpcServerData *link;     /*  8  registered servers */
    SifRpcServerData *start;    /* 12  pending request list head */
    SifRpcServerData *end;      /* 16 */
    SifRpcDataQueue  *next;     /* 20 */
};

extern int DIntr(void);
extern int EIntr(void);
extern void *_data_table_00993280[16];
extern int _bind_check;
extern char _lfversion[4];
void *memset(void *dst, int c, int n);

/* sceSifLoadFileReset: drop the loadfile RPC binding so the next call
 * re-binds. */
int sceSifLoadFileReset(void)
{
    _bind_check = -1;
    memset(&_lfversion, 0, 4);
    return 0;
}

/* sceSifGetNextRequest: pop the queue's next pending request, with
 * interrupts off, and leave `active` set only while one is out.
 *
 * Reading `next` into a local BEFORE storing to the queue is
 * load-bearing: it is what gcc hoists into the branch-likely delay slot
 * (`bnezl` + `lw v1,60(s1)`).  Written as `q->start = p->next;` last,
 * the `li v0,1` goes in the slot instead and two words differ. */
void *sceSifGetNextRequest(SifRpcDataQueue *q)
{
    SifRpcServerData *p;
    SifRpcServerData *next;

    DIntr();
    p = q->start;
    if (p == 0) {
        q->active = 0;
    } else {
        next = p->next;
        q->active = 1;
        q->start = next;
    }
    EIntr();
    return p;
}

/* sceSifRemoveRpcQueue: unlink a queue from the global queue list and
 * return the node that preceded it (0 if it was not found). */
SifRpcDataQueue *sceSifRemoveRpcQueue(SifRpcDataQueue *q)
{
    SifRpcDataQueue *p;

    DIntr();
    p = (SifRpcDataQueue *)_data_table_00993280[10];
    if (p == q) {
        _data_table_00993280[10] = p->next;
    } else {
        while (p != 0) {
            if (p->next == q) {
                p->next = q->next;
                break;
            }
            p = p->next;
        }
    }
    EIntr();
    return p;
}

/* sceSifRemoveRpc: same unlink, one level down -- take a server out of
 * its queue's registration list. */
SifRpcServerData *sceSifRemoveRpc(SifRpcServerData *sd, SifRpcDataQueue *q)
{
    SifRpcServerData *p;

    DIntr();
    p = q->link;
    if (p == sd) {
        q->link = p->link;
    } else {
        while (p != 0) {
            if (p->link == sd) {
                p->link = sd->link;
                break;
            }
            p = p->link;
        }
    }
    EIntr();
    return p;
}

/* ------------------------------------------------------------------
 * SIF RPC transport: packet pool and header structures.
 *
 * The RPC layer keeps three pools inside one control block: `pkt_table`
 * (client request packets, scanned for a free slot), `rdata_table`
 * (receive-data packets, handed out round-robin through `rdata_index`)
 * and `fpkt_table` (directly indexed by request id). Every packet is 64
 * bytes; `rec_id` bit 0 is the in-use flag.
 * ------------------------------------------------------------------ */

typedef struct SifRpcPacket SifRpcPacket;

struct SifRpcPacket {
    int           pad0[4];      /*  0..15 */
    unsigned int  rec_id;       /* 16  bit 0 = allocated */
    SifRpcPacket *pkt_addr;     /* 20 */
    int           pid;          /* 24 */
    void         *client;       /* 28  binding client data */
    unsigned int  sid;          /* 32  server id / function number */
    int           ssize;        /* 36 */
    void         *recv;         /* 40 */
    int           rsize;        /* 44 */
    int           async;        /* 48 */
    void         *serve;        /* 52 */
    int           pad1[2];      /* 56..63 */
};

typedef struct SifRpcHeader {
    SifRpcPacket *pkt_addr;     /*  0 */
    int           rpc_id;       /*  4 */
    int           sema_id;      /*  8 */
    unsigned int  mode;         /* 12 */
} SifRpcHeader;

typedef struct SifRpcData {
    int           pid;              /*  0 */
    SifRpcPacket *pkt_table;        /*  4 */
    int           pkt_table_len;    /*  8 */
    int           pad0;             /* 12 */
    void         *client_table;     /* 16 */
    SifRpcPacket *rdata_table;      /* 20 */
    int           rdata_table_len;  /* 24 */
    SifRpcPacket *fpkt_table;       /* 28 */
    int           fpkt_table_len;   /* 32 */
    int           rdata_index;      /* 36 */
} SifRpcData;

/* _sceRpcFreePacket: drop the in-use bit and the request id. */
void _sceRpcFreePacket(SifRpcPacket *pkt)
{
    pkt->pid = 0;
    pkt->rec_id &= ~1;
}

/* _sceRpcGetPacket: linear-scan the client packet table for a slot with
 * the in-use bit clear, stamp it with its table index and a fresh
 * request id, and return it. Request id 0 is skipped -- `id` is the
 * post-increment result, so a wrapped counter reaching 0 takes a second
 * bump. */
SifRpcPacket *_sceRpcGetPacket(SifRpcData *rd)
{
    SifRpcPacket *pkt;
    int i;
    int n;
    int id;

    DIntr();
    pkt = rd->pkt_table;
    for (i = 0; i < rd->pkt_table_len; i++, pkt++) {
        if (!(pkt->rec_id & 1)) {
            pkt->rec_id = (i << 16) | 5;
            n = ++rd->pid;
            if (n == 1) {
                rd->pid++;
                id = 1;
            } else {
                id = n;
            }
            pkt->pkt_addr = pkt;
            pkt->pid = id;
            EIntr();
            return pkt;
        }
    }
    EIntr();
    return 0;
}

/* _sceRpcGetFPacket: the round-robin receive-data pool. */
SifRpcPacket *_sceRpcGetFPacket(SifRpcData *rd)
{
    int i = rd->rdata_index % rd->rdata_table_len;
    SifRpcPacket *pkt = rd->rdata_table + i;

    rd->rdata_index = i + 1;
    return pkt;
}

/* _sceRpcGetFPacket2: index the directly-addressed pool when the id is
 * in range, otherwise fall back to the round-robin one. */
SifRpcPacket *_sceRpcGetFPacket2(SifRpcData *rd, int rid)
{
    if (rid < 0 || rid >= rd->fpkt_table_len)
        return _sceRpcGetFPacket(rd);
    return rd->fpkt_table + rid;
}

/* sceSifCheckStatRpc: a request is complete once its packet still
 * carries our request id and the SIF side has set the done bit. */
int sceSifCheckStatRpc(SifRpcHeader *hdr)
{
    SifRpcPacket *pkt = hdr->pkt_addr;

    if (pkt == 0)
        goto err;
    if (hdr->rpc_id != pkt->pid)
        goto err;
    if (pkt->rec_id & 1)
        goto ok;
err:
    return 0;
ok:
    return 1;
}

/* sceSifSendCmd / sceSifSendCmdIntr share one internal entry point that
 * takes an extra leading "from interrupt" argument; the public entry
 * passes 0. Seven int arguments fit in registers on this ABI ($a0-$a3,
 * $t0-$t3), so the shift is pure register moves. */
extern unsigned int _sceSifSendCmd(unsigned int fid, int intr, void *pkt,
                                   int pktsize, void *src, void *dest,
                                   int size);

unsigned int sceSifSendCmd(unsigned int fid, void *pkt, int pktsize,
                           void *src, void *dest, int size)
{
    return _sceSifSendCmd(fid, 0, pkt, pktsize, src, dest, size);
}

/* sceSifSetRpcQueue: zero a queue descriptor, give it its server
 * thread, and append it to the global queue list. */
void sceSifSetRpcQueue(SifRpcDataQueue *q, int thread_id)
{
    SifRpcDataQueue *head;
    SifRpcDataQueue *p;

    DIntr();
    head = (SifRpcDataQueue *)_data_table_00993280[10];
    q->thread_id = thread_id;
    q->active = 0;
    q->link = 0;
    q->start = 0;
    q->end = 0;
    q->next = 0;
    if (head == 0) {
        _data_table_00993280[10] = q;
    } else {
        p = head;
        while (p->next != 0)
            p = p->next;
        p->next = q;
    }
    EIntr();
}

/* sceSifRegisterRpc: fill in a server descriptor and append it to its
 * queue's server list. */
void sceSifRegisterRpc(SifRpcServerData *sd, int sid, void *func, void *buff,
                       void *cfunc, void *cbuff, SifRpcDataQueue *q)
{
    SifRpcServerData *head;
    SifRpcServerData *p;

    DIntr();
    sd->next = 0;
    sd->link = 0;
    sd->sid = sid;
    head = q->link;
    sd->func = func;
    sd->buff = buff;
    sd->cfunc = cfunc;
    sd->cbuff = cbuff;
    sd->base = q;
    if (head == 0) {
        q->link = sd;
    } else {
        p = head;
        while (p->link != 0)
            p = p->link;
        p->link = sd;
    }
    EIntr();
}

/* sceSifRpcLoop: the server thread body -- drain the queue, then sleep
 * until the SIF interrupt handler wakes us with more. Never returns, so
 * there is no epilogue. */
extern void sceSifExecRequest(SifRpcServerData *sd);
extern void SleepThread(void);

void sceSifRpcLoop(SifRpcDataQueue *q)
{
    SifRpcServerData *sd;

    for (;;) {
        while ((sd = (SifRpcServerData *)sceSifGetNextRequest(q)) != 0)
            sceSifExecRequest(sd);
        SleepThread();
    }
}

/* ------------------------------------------------------------------
 * IOP heap / system memory RPC. One client-data block (`cd_00994680`),
 * one send buffer (`sdata`) and one reply word (`rdata_009946C0`), all
 * named fixed addresses. `_bind` holds the bind state: negative means
 * the RPC has never been bound and every entry point fails closed.
 * ------------------------------------------------------------------ */

typedef struct SifRpcClientData {
    SifRpcHeader hdr;               /*  0..15 */
    unsigned int command;           /* 16 */
    void        *buff;              /* 20 */
    void        *cbuff;             /* 24 */
    void       (*func)(void *);     /* 28 */
    void        *para;              /* 32 */
    SifRpcServerData *serve;        /* 36 */
} SifRpcClientData;

extern int sceSifBindRpc(SifRpcClientData *cd, unsigned int sid, int mode);
extern int _bind;
extern SifRpcClientData cd_00994680;
extern int rdata_009946C0;
extern int sdata[3];

/* sceSifInitIopHeap: bind the heap RPC, spinning on a fixed-count delay
 * loop until the IOP side answers.
 *
 * MATCHES, 34 words.  Everything -- the rotated while-loop, its four
 * R5900 erratum pad nops, the `li v1,-1` preheader constant, the bgezl --
 * came out of the C.  The last 3 words were the classic v0/v1 allocator
 * tie-break on the final `_bind = 0; return 0;`: the original puts the
 * %hi address temp in v0 and overwrites it with the return value, gcc
 * puts it in v1 and sets v0 first.  Swept without success: `return
 * _bind;`, `r = 0; _bind = r; return r;` with and without LAUNDER, a
 * volatile store, PIN($2) with and without PASSTHRU (which costs a real
 * addiu because it defeats %hi/%lo folding).
 *
 * Closed with two fixer sites that must be read together:
 *     --swap-regs sceSifInitIopHeap:2-3:25,27
 *     --swap-adjacent sceSifInitIopHeap:26!
 * gcc emits `lui $3,%hi(_bind)` / `move $2,$0` / `sw $0,%lo(_bind)($3)`.
 * The ranged --swap-regs renames only the two %hi lines (25 and 27, gcc's
 * own asm indexing) to $2, and the FORCED --swap-adjacent then exchanges
 * the `move` and the `sw`, giving the original's `lui $2` / `sw ...($2)` /
 * `move $2,$0`.  The `!` is required because after the rename the pair
 * looks dependent to swap_ok; the FINAL code is correct (the store uses
 * the lui result, the move then overwrites it with the return value) and
 * tools/audit_swaps.py confirms the emitted instructions are a pure
 * reorder of gcc's.  This "rename then force-swap" pairing is the general
 * recipe when the diff is registers AND order in the same two words. */
int sceSifInitIopHeap(void)
{
    int i;

    for (;;) {
        if (sceSifBindRpc(&cd_00994680, 0x80000003, 0) < 0)
            return -1;
        if (cd_00994680.serve != 0)
            break;
        for (i = 0x100000; i != -1; i--)
            ;
    }
    _bind = 0;
    return 0;
}

int sceSifAllocIopHeap(int size)
{
    if (_bind < 0)
        return 0;
    sdata[0] = size;
    if (sceSifCallRpc(&cd_00994680, 1, 0, sdata, 4,
                      &rdata_009946C0, 4, 0, 0) < 0)
        return 0;
    return rdata_009946C0;
}

int sceSifAllocSysMemory(int size, int mode, void *addr)
{
    if (_bind < 0)
        return 0;
    sdata[0] = mode;
    sdata[1] = size;
    sdata[2] = (int)addr;
    if (sceSifCallRpc(&cd_00994680, 4, 0, sdata, 12,
                      &rdata_009946C0, 4, 0, 0) < 0)
        return 0;
    return rdata_009946C0;
}

int sceSifFreeSysMemory(int addr)
{
    if (_bind < 0)
        return 0;
    sdata[0] = addr;
    if (sceSifCallRpc(&cd_00994680, 2, 0, sdata, 4,
                      &rdata_009946C0, 4, 0, 0) < 0)
        return -1;
    return rdata_009946C0;
}

/* sceSifSearchModuleByAddress / ByName: the loadfile RPC's two module
 * lookups, same version-check-then-dispatch boilerplate as
 * sceSifUnloadModule above. */

extern char *strncpy(char *dst, const char *src, unsigned int n);

int sceSifSearchModuleByAddress(void *addr)
{
    /* Same $17 pin as sceSifUnloadModule, and for the same reason: the
     * argument and the &_senddata base are both live across two calls,
     * and gcc hands $16 to whichever pseudo it allocates first. The
     * original gives $16 to the base and $17 to the argument; every
     * natural spelling (int parameter, base hoisted into a local before
     * or after the calls) gives the opposite. sceSifSearchModuleByName
     * below needs no pin -- it reuses one register for both roles. */
    PIN(int a, "$17");
    SifLfSendData *sd;

    a = (int)addr;
    if (_lf_bind(a) < 0)
        return (int)0xffff0000;
    if (_lf_version() != 0)
        return (int)0xfffefffc;
    sd = &_senddata;
    sd->arg = a;
    if (sceSifCallRpc(&cd_00994A40, 10, 0, sd, 4, sd, 4, 0, 0) < 0)
        return (int)0xfffeffff;
    return sd->arg;
}

int sceSifSearchModuleByName(const char *name)
{
    if (_lf_bind((int)name) < 0)
        return (int)0xffff0000;
    if (_lf_version() != 0)
        return (int)0xfffefffc;
    strncpy(_senddata.name, name, 252);
    _senddata.name[251] = 0;
    if (sceSifCallRpc(&cd_00994A40, 9, 0, &_senddata, 512,
                      &_senddata, 4, 0, 0) < 0)
        return (int)0xfffeffff;
    return _senddata.arg;
}

/* sceSifGetIopAddr: loadfile RPC opcode 3 -- ask the IOP for the address
 * of an exported symbol and store it into *dest at the caller's width.
 * `width` is 0/1/2 for byte/halfword/word; anything else is rejected
 * before the RPC, which is why the trailing `else` below is unreachable
 * in practice but still emitted (the original tests it too). */
int sceSifGetIopAddr(int nAddr, void *pDest, int width)
{
    /* $s3/$s2 for the two saved arguments: gcc's global-alloc ranks the
     * destination pointer above the address and hands them out the other
     * way round, which then also pushes the _senddata base off $s1. */
    PIN(int addr, "$19");
    PIN(void *dest, "$18");

    addr = nAddr;
    dest = pDest;
    if (_lf_bind(addr) < 0)
        return (int)0xffff0000;
    if ((unsigned int)width >= 3)
        return (int)0xfffefffe;

    _senddata.arg = addr;
    _senddata.pad = width;
    if (sceSifCallRpc(&cd_00994A40, 3, 0, &_senddata, 32,
                      &_senddata, 32, 0, 0) < 0)
        return (int)0xfffeffff;

    if (width == 0)
        *(unsigned char *)dest = (unsigned char)_senddata.arg;
    else if (width == 1)
        *(unsigned short *)dest = (unsigned short)_senddata.arg;
    else if (width == 2)
        *(int *)dest = _senddata.arg;
    else
        return (int)0xfffefffe;

    return 0;
}

/* _sceSifLoadElfPart: the loadfile RPC behind sceSifLoadElfPart and
 * sceSifLoadElf.  Both 252-byte name buffers of the send block are filled
 * (ELF path, then section name), the RPC function number is the caller's
 * `mode`, and the 16-byte reply's first two words are copied out to the
 * caller's result pair.  A zero entry point is reported as 0xfffefffd. */
int _sceSifLoadElfPart(const char *path, const char *sec, void *dest, int mode)
{
    PIN(const char *p, "$16");
    int *result;

    p = path;
    if (_lf_bind((int)p) < 0)
        return (int)0xffff0000;
    if (_lf_version() != 0)
        return (int)0xfffefffc;

    strncpy(_senddata.name, p, 252);
    _senddata.name[251] = 0;
    strncpy(_senddata.sec, sec, 252);
    _senddata.sec[251] = 0;

    if (sceSifCallRpc(&cd_00994A40, mode, 0, &_senddata, 512,
                      &_senddata, 16, 0, 0) < 0)
        return (int)0xfffeffff;

    if (_senddata.arg == 0)
        return (int)0xfffefffd;

    result = (int *)dest;
    result[0] = _senddata.arg;
    result[1] = _senddata.pad;
    return 0;
}

/* sceSifLoadIopHeap: copy a module path into the load-into-heap RPC block
 * and fire opcode 3 on the heap client.  The name is copied by hand (not
 * strncpy) with a 252-byte cap, and the request size sent is the copied
 * length plus the 4-byte address word plus the terminator. */
typedef struct SifLihData {
    int  addr;          /*   0 */
    char name[252];     /*   4..255 */
} SifLihData;

extern SifLihData _lih_data;

int sceSifLoadIopHeap(const char *name, int addr)
{
    int i;

    if (_bind < 0)
        return 0;

    for (i = 0; i < 252; i++)
        if ((_lih_data.name[i] = name[i]) == 0)
            break;
    if (i == 252)
    {
        i = 251;
        _lih_data.name[251] = 0;
    }
    _lih_data.addr = addr;
    _lih_data.name[251] = 0;

    if (sceSifCallRpc(&cd_00994680, 3, 0, &_lih_data, i + 5,
                      &rdata_009946C0, 4, 0, 0) < 0)
        return -1;
    return rdata_009946C0;
}

extern void *memcpy(void *dst, const void *src, unsigned int n);

/* sceSifStopModule: loadfile RPC opcode 7.  The caller's argument block is
 * copied into the send block's second 252-byte buffer, clamped to 252
 * bytes; the clamped branch passes the constant 252 so gcc expands that
 * copy inline (the aligned ld/sd and unaligned ldl/ldr variants both come
 * from that one constant-size memcpy).  The reply's second word is handed
 * back through `result` and its first word is the return value. */
int sceSifStopModule(int nModId, int nArgLen, const void *args, int *result)
{
    PIN(int arglen, "$17");
    PIN(int modId, "$19");
    int r, n;

    arglen = nArgLen;
    modId = nModId;
    if (_lf_bind(modId) < 0)
        return (int)0xffff0000;
    if (_lf_version() != 0)
        return (int)0xfffefffc;

    _senddata.arg = modId;
    if (args != 0)
    {
        if (arglen >= 253)
        {
            memcpy(_senddata.sec, args, 252);
            _senddata.pad = 252;
        }
        else
        {
            memcpy(_senddata.sec, args, arglen);
            _senddata.pad = arglen;
        }
    }
    else
    {
        _senddata.pad = 0;
    }

    if (sceSifCallRpc(&cd_00994A40, 7, 0, &_senddata, 512,
                      &_senddata, 8, 0, 0) < 0)
        return (int)0xfffeffff;

    n = _senddata.pad;
    r = _senddata.arg;
    *result = n;
    return r;
}

/* _sceSifLoadModule: the loadfile RPC behind sceSifLoadModule and
 * sceSifLoadStartModule.  Same shape as sceSifStopModule, except the send
 * block also carries the module path and the argument LENGTH lands in the
 * block's first word rather than its second. */
int _sceSifLoadModule(const char *pPath, int arglen, const void *args,
                      int *result, int mode)
{
    /* Same $17 pin as _sceSifLoadElfPart: without it gcc emits the path
     * copy first and the whole prologue comes out rotated by one. */
    PIN(const char *path, "$17");
    int r, n;

    path = pPath;
    if (_lf_bind((int)path) < 0)
        return (int)0xffff0000;
    if (_lf_version() != 0)
        return (int)0xfffefffc;

    strncpy(_senddata.name, path, 252);
    _senddata.name[251] = 0;

    if (args != 0)
    {
        if (arglen >= 253)
        {
            memcpy(_senddata.sec, args, 252);
            _senddata.arg = 252;
        }
        else
        {
            memcpy(_senddata.sec, args, arglen);
            _senddata.arg = arglen;
        }
    }
    else
    {
        _senddata.sec[0] = 0;
        _senddata.arg = 0;
    }

    if (sceSifCallRpc(&cd_00994A40, mode, 0, &_senddata, 512,
                      &_senddata, 8, 0, 0) < 0)
        return (int)0xfffeffff;

    n = _senddata.pad;
    r = _senddata.arg;
    *result = n;
    return r;
}

/* _sceSifLoadModuleBuffer: loadfile RPC opcode 6.  Instruction for
 * instruction the same function as sceSifStopModule above with a
 * different opcode -- the send block's first word is the buffer address
 * instead of a module id. */
int _sceSifLoadModuleBuffer(int nAddr, int nArgLen, const void *args,
                            int *result)
{
    PIN(int arglen, "$17");
    PIN(int addr, "$19");
    int r, n;

    arglen = nArgLen;
    addr = nAddr;
    if (_lf_bind(addr) < 0)
        return (int)0xffff0000;
    if (_lf_version() != 0)
        return (int)0xfffefffc;

    _senddata.arg = addr;
    if (args != 0)
    {
        if (arglen >= 253)
        {
            memcpy(_senddata.sec, args, 252);
            _senddata.pad = 252;
        }
        else
        {
            memcpy(_senddata.sec, args, arglen);
            _senddata.pad = arglen;
        }
    }
    else
    {
        _senddata.pad = 0;
    }

    if (sceSifCallRpc(&cd_00994A40, 6, 0, &_senddata, 512,
                      &_senddata, 8, 0, 0) < 0)
        return (int)0xfffeffff;

    n = _senddata.pad;
    r = _senddata.arg;
    *result = n;
    return r;
}


/* ------------------------------------------------------------------
 * The three built-in SIF system command handlers and the RPC server-side
 * lookup.  Each handler takes the received packet and the handler
 * argument installed with it, which for these is the SIF data table.
 * ------------------------------------------------------------------ */

/* A system command packet: the four-word SIF command header followed by
 * this command's two payload words. */
typedef struct SifCmdSRegPacket {
    int hdr[4];     /*  0..15 */
    int index;      /* 16 */
    int value;      /* 20 */
} SifCmdSRegPacket;

/* SIF_CMD_CHANGE_SADDR: remember the IOP's new command-buffer address. */
void _change_addr(SifCmdSRegPacket *pkt, int *tbl)
{
    tbl[2] = pkt->index;
}

/* SIF_CMD_SET_SREG: write one of the eight shared software registers. */
void _set_sreg(SifCmdSRegPacket *pkt, int *tbl)
{
    int *sreg = (int *)tbl[7];

    sreg[pkt->index] = pkt->value;
}

/* The from-interrupt entry of sceSifSendCmd. */
unsigned int isceSifSendCmd(unsigned int fid, void *pkt, int pktsize,
                            void *src, void *dest, int size)
{
    return _sceSifSendCmd(fid, 1, pkt, pktsize, src, dest, size);
}

/* _search_svdata: find the server registered for `sid` by walking every
 * queue in the global list and every server on each queue. */
SifRpcServerData *_search_svdata(int sid, void **tbl)
{
    SifRpcDataQueue *q;
    SifRpcServerData *sd;

    for (q = (SifRpcDataQueue *)tbl[10]; q != 0; q = q->next) {
        for (sd = q->link; sd != 0; sd = sd->link) {
            if (sd->sid == sid)
                return sd;
        }
    }
    return 0;
}

/* The loadfile RPC's version handshake.  `_lfversion` is the four
 * version bytes the IOP module reported at bind time; they are accepted
 * if they equal this build's own libinfo version (the 12-byte offset
 * into __ps2_klibinfo__) or the wildcard the caller may have installed.
 * Non-zero means "incompatible". */
extern char __ps2_klibinfo__[];
extern void *_lfwildcard;
extern int memcmp(const void *a, const void *b, unsigned int n);

int _lf_version(void)
{
    const char *mine = __ps2_klibinfo__ + 12;
    int bad = 0;

    if (memcmp(_lfversion, mine, 4) != 0)
        if (memcmp(_lfversion, _lfwildcard, 4) != 0)
            bad = (memcmp(mine, _lfwildcard, 4) != 0);

    return bad;
}

/* _lf_bind: bind the loadfile RPC server once, retrying while the IOP has
 * not published it yet, then read back its four version bytes.  The
 * argument is ignored -- every caller passes something, but the original
 * never reads it.  `_bind_check` negative means "not bound yet". */
int _lf_bind(int unused)
{
    int i;

    if (_bind_check < 0) {
        for (;;) {
            if (sceSifBindRpc(&cd_00994A40, 0x80000006, 0) < 0)
                return -1;
            if (cd_00994A40.serve != 0) {
                _bind_check = 0;
                if (sceSifCallRpc(&cd_00994A40, 255, 0, 0, 0,
                                  &_senddata, 4, 0, 0) < 0)
                    return (int)0xfffeffff;
                memcpy(_lfversion, &_senddata, 4);
                return 0;
            }
            for (i = 0x100000; i != -1; i--)
                ;
        }
    }
    return 0;
}

/* sceSifBindRpc: allocate a request packet, point it at the client data
 * and the wanted server id, and send the bind command.  Bit 0 of `mode`
 * selects the non-blocking form: the blocking one creates a semaphore
 * and waits on it for the IOP's reply. */
struct SemaParam {
    int          currentCount;
    int          maxCount;
    int          initCount;
    int          numWaitThreads;
    unsigned int attr;
    unsigned int option;
};

extern int CreateSema(struct SemaParam *param);
extern int DeleteSema(int id);
extern int WaitSema(int id);

int sceSifBindRpc(SifRpcClientData *cd, unsigned int sid, int mode)
{
    SifRpcPacket *pkt;

    cd->command = 0;
    cd->serve = 0;
    pkt = _sceRpcGetPacket((SifRpcData *)_data_table_00993280);
    if (pkt == 0)
        return -1;

    cd->hdr.pkt_addr = pkt;
    cd->hdr.rpc_id = pkt->pid;
    pkt->sid = sid;
    pkt->pkt_addr = pkt;
    pkt->client = cd;

    if ((mode & 1) == 0) {
        struct SemaParam sema;

        sema.maxCount = 1;
        sema.initCount = 0;
        cd->hdr.sema_id = CreateSema(&sema);
        if (cd->hdr.sema_id < 0) {
            _sceRpcFreePacket(pkt);
            return -3;
        }
        if (sceSifSendCmd(0x80000009, pkt, 64, 0, 0, 0) == 0) {
            _sceRpcFreePacket(pkt);
            DeleteSema(cd->hdr.sema_id);
            return -2;
        }
        WaitSema(cd->hdr.sema_id);
        DeleteSema(cd->hdr.sema_id);
        return 0;
    }

    cd->hdr.sema_id = -1;
    if (sceSifSendCmd(0x80000009, pkt, 64, 0, 0, 0) == 0) {
        _sceRpcFreePacket(pkt);
        return -2;
    }
    return 0;
}

/* MATCHES, 123 words.  The source follows the SDK's original packet setup:
 * populate the client and call packet, copy the bound server pointer, then
 * select cache write-back and blocking behavior.  This compiler rebuild
 * hoists the server-pointer load/store across the otherwise exact setup and
 * rotates five independent arguments before the blocking send.  The strict
 * --retime-rpc-call-setup pass validates both complete windows before
 * restoring the retail schedule; it adds or removes no instructions.
 *
 * sceSifCallRpc: issue one RPC on an already-bound client.  Bit 1 of
 * `mode` suppresses the data-cache write-back of the two buffers (when
 * they are the same buffer, one flush of the larger size covers both);
 * bit 0 selects the non-blocking form, where `endfunc` is called from the
 * reply handler instead of this thread waiting on a semaphore. */
extern void sceSifWriteBackDCache(void *ptr, int size);

int sceSifCallRpc(SifRpcClientData *cd, unsigned int fno, int mode,
                  void *send, int ssize, void *recv, int rsize,
                  void *endfunc, void *efarg)
{
    SifRpcPacket *pkt;
    struct SemaParam sema;

    pkt = _sceRpcGetPacket((SifRpcData *)_data_table_00993280);
    if (pkt == 0)
        return -1;

    cd->para = efarg;
    cd->hdr.pkt_addr = pkt;
    cd->hdr.rpc_id = pkt->pid;
    cd->func = (void (*)(void *))endfunc;
    pkt->sid = fno;
    pkt->ssize = ssize;
    pkt->recv = recv;
    pkt->rsize = rsize;
    pkt->pkt_addr = pkt;
    pkt->client = cd;
    pkt->serve = cd->serve;

    if ((mode & 2) == 0) {
        if (send == recv) {
            sceSifWriteBackDCache(send, (ssize < rsize) ? rsize : ssize);
        } else {
            if (ssize > 0)
                sceSifWriteBackDCache(send, ssize);
            if (rsize > 0)
                sceSifWriteBackDCache(recv, rsize);
        }
    }

    if (mode & 1) {
        if (endfunc == 0)
            pkt->async = 0;
        else
            pkt->async = 1;
        cd->hdr.sema_id = -1;
        if (sceSifSendCmd(0x8000000a, pkt, 64, send, cd->buff, ssize) != 0)
            return 0;
        _sceRpcFreePacket(pkt);
        return -2;
    }

    sema.maxCount = 1;
    sema.initCount = 0;
    cd->hdr.sema_id = CreateSema(&sema);
    if (cd->hdr.sema_id < 0) {
        _sceRpcFreePacket(pkt);
        return -3;
    }
    pkt->async = 1;
    if (sceSifSendCmd(0x8000000a, pkt, 64, send, cd->buff, ssize) == 0) {
        DeleteSema(cd->hdr.sema_id);
        _sceRpcFreePacket(pkt);
        return -2;
    }
    WaitSema(cd->hdr.sema_id);
    DeleteSema(cd->hdr.sema_id);
    return 0;
}

/* sceSifGetOtherData: SIF command 0x8000000c -- ask the IOP to DMA `size`
 * bytes from its address `src` into EE memory at `dest`.  Same
 * request-packet plumbing as sceSifBindRpc; bit 0 of `mode` selects the
 * non-blocking form.  The request's three payload words share the packet
 * slots the RPC path uses for the function number and buffer sizes, hence
 * the casts. */
int sceSifGetOtherData(SifRpcClientData *cd, void *src, void *dest,
                       int size, int mode)
{
    SifRpcPacket *pkt;

    pkt = _sceRpcGetPacket((SifRpcData *)_data_table_00993280);
    if (pkt == 0)
        return -1;

    cd->hdr.pkt_addr = pkt;
    cd->hdr.rpc_id = pkt->pid;
    pkt->sid = (unsigned int)src;
    pkt->ssize = (int)dest;
    pkt->recv = (void *)size;
    pkt->pkt_addr = pkt;
    pkt->client = cd;

    if ((mode & 1) == 0) {
        struct SemaParam sema;

        sema.maxCount = 1;
        sema.initCount = 0;
        cd->hdr.sema_id = CreateSema(&sema);
        if (cd->hdr.sema_id < 0) {
            _sceRpcFreePacket(pkt);
            return -3;
        }
        if (sceSifSendCmd(0x8000000c, pkt, 64, 0, 0, 0) == 0) {
            _sceRpcFreePacket(pkt);
            DeleteSema(cd->hdr.sema_id);
            return -2;
        }
        WaitSema(cd->hdr.sema_id);
        DeleteSema(cd->hdr.sema_id);
        return 0;
    }

    cd->hdr.sema_id = -1;
    if (sceSifSendCmd(0x8000000c, pkt, 64, 0, 0, 0) == 0) {
        _sceRpcFreePacket(pkt);
        return -2;
    }
    return 0;
}

/* _request_rdata: the server side's reply path.  A receive-data packet is
 * taken from the round-robin pool, stamped with the requester's packet
 * address and client data and the "receive data" opcode, and sent back
 * with the request's own source/destination/size triple.
 *
 * TODO: near-miss, 15 of 24 words, SCHEDULING only -- the sibling call
 * (`j isceSifSendCmd` after the epilogue) and all 24 instructions are
 * right, but our build sinks the `pkt->pkt_addr = req->pkt_addr` load and
 * store to the very end where the original issues them first and lets the
 * 0x8000000c materialisation and the argument loads fill in around them.
 * Swept without success: the three stores in both source orders, and
 * reading both copied fields into locals before any store (the
 * GameResourceAlloc lever) -- the post-reload scheduler produces the same
 * order from all three shapes. */
void _request_rdata(SifRpcPacket *req, SifRpcData *rd)
{
    SifRpcPacket *pkt = _sceRpcGetFPacket(rd);

    pkt->pkt_addr = req->pkt_addr;
    pkt->client = req->client;
    pkt->sid = 0x8000000c;
    isceSifSendCmd(0x80000008, pkt, 64, (void *)req->sid,
                   (void *)req->ssize, (int)req->recv);
}
