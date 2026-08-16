/* PS2 SDK libpad (scePad*) client side.
 *
 * All eleven functions in this file are ordinary C.  The fixed low-memory objects they touch are all
 * named in config/symbol_addrs.txt, so they are declared by name rather
 * than cast from a numeric address (CONTRIBUTING.md, "Raw addresses"):
 *
 *     ReqStateStr  0x004AB9C8   request-code -> string table
 *     PadStateStr  0x004AB9A8   pad-state-code -> string table
 *     isInit       0x004AB9A0   "pad module initialised" flag
 *     padsif       0x009969C0   SIF-RPC client data block
 *     buffer_00996C00 0x00996C00 128-byte RPC send/receive payload
 *     PadInfo      0x00996A10   per-port/slot state, 28 bytes each,
 *                               indexed port*112 + slot*28
 *
 * ---------------------------------------------------------------------
 * BUILD FLAG.  This translation unit was built WITHOUT
 * `-fno-schedule-insns`, so configure.py carries
 * FILE_CFLAGS_OVERRIDE["scePad.c"] = "-O2 -G0" and all ELEVEN functions
 * below are plain C.  The tell is the R5900 dual multiplier: the
 * original pairs the `port*112` and `slot*28` index multiplies as
 * `mult1`/`mult` on the two independent units, which only the first
 * scheduling pass emits.  With the flag left on, seven of the eleven are
 * 4-40 words off.
 *
 * The override must stay per file -- sceMpeg.c and sceVif1Pk.c both
 * regress at "-O2 -G0", so SDK_CFLAGS itself must not change.
 * ---------------------------------------------------------------------
 */

extern char *ReqStateStr[4];
extern char *PadStateStr[8];
extern int   isInit;
extern int   padsif;
extern int   buffer_00996C00[32];

/* Per-(port,slot) open/state bookkeeping the RPC wrappers guard on. */
typedef struct scePadSlotInfo {
    unsigned char *dma;         /* +0: the port's 256-byte DMA buffer */
    int reserved[3];
    int open;                   /* nonzero once the port/slot is opened */
    int stat20;
    int stat24;
} scePadSlotInfo;               /* 28 bytes */

extern scePadSlotInfo PadInfo[2][4];
unsigned char *scePadGetDmaStr(int port, int slot);
void *memcpy(void *dst, const void *src, int n);

char *strcpy(char *dst, const char *src);
extern int sceSifCallRpc(void *pCd, unsigned int fno, int mode, void *send,
                         int ssize, void *recv, int rsize, void *ef, void *ea);

/* Copy the name of a pad request code into `str` ("" when out of range).
 * The out-of-range arm is GCC's own inlining of strcpy() of a one-byte
 * string constant, which is why it loads a byte from .rodata instead of
 * storing zero; the in-range arm is a real tail call to strcpy. */
void scePadReqIntToStr(int req, char *str)
{
    if ((unsigned int)req < 4)
        strcpy(str, ReqStateStr[req]);
    else
        strcpy(str, "");
}

/* Copy the name of a pad state code into `str` ("" when out of range). */
void scePadStateIntToStr(int state, char *str)
{
    if ((unsigned int)state < 8)
        strcpy(str, PadStateStr[state]);
    else
        strcpy(str, "");
}

/* RPC opcode 12: ask the IOP pad module how many ports it supports. */
int scePadGetPortMax(void)
{
    buffer_00996C00[0] = 12;
    if (sceSifCallRpc(&padsif, 1, 0, buffer_00996C00, 128,
                      buffer_00996C00, 128, 0, 0) < 0)
        return 0;
    return buffer_00996C00[3];
}

/* RPC opcode 15: shut the pad module down, clearing `isInit` when the
 * IOP side confirms with a reply of exactly 1. */
int scePadEnd(void)
{
    int r;

    buffer_00996C00[0] = 15;
    if (sceSifCallRpc(&padsif, 1, 0, buffer_00996C00, 128,
                      buffer_00996C00, 128, 0, 0) < 0)
        return 0;
    r = buffer_00996C00[3];
    if (r == 1)
        isInit = 0;
    return r;
}


/* Was hand-transcribed asm while this TU was built with
 * -fno-schedule-insns; with -O2 -G0 the plain C matches. */
int scePadGetSlotMax(int port)
{
    buffer_00996C00[0] = 13;
    buffer_00996C00[1] = port;
    if (sceSifCallRpc(&padsif, 1, 0, buffer_00996C00, 128,
                      buffer_00996C00, 128, 0, 0) < 0)
        return 0;
    return buffer_00996C00[3];
}

/* Was hand-transcribed asm while this TU was built with
 * -fno-schedule-insns; with -O2 -G0 the plain C matches. */
int scePadInit2(int mode)
{
    int i;

    for (i = 0; i < 4; i++) {
        PadInfo[0][i].open = 0;
        PadInfo[0][i].stat24 = 0;
        PadInfo[0][i].stat20 = 0;
        PadInfo[1][i].open = 0;
        PadInfo[1][i].stat24 = 0;
        PadInfo[1][i].stat20 = 0;
    }
    buffer_00996C00[0] = 16;
    buffer_00996C00[4] = 0;
    if (sceSifCallRpc(&padsif, 1, 0, buffer_00996C00, 128,
                      buffer_00996C00, 128, 0, 0) < 0)
        return 0;
    return buffer_00996C00[3];
}

/* Was hand-transcribed asm while this TU was built with
 * -fno-schedule-insns; with -O2 -G0 the plain C matches. */
int scePadPortClose(int port, int slot, int mode)
{
    if (PadInfo[port][slot].open == 0)
        return 0;
    buffer_00996C00[0] = 14;
    buffer_00996C00[1] = port;
    buffer_00996C00[2] = slot;
    buffer_00996C00[4] = 1;
    if (sceSifCallRpc(&padsif, 1, 0, buffer_00996C00, 128,
                      buffer_00996C00, 128, 0, 0) < 0)
        return 0;
    PadInfo[port][slot].open = 0;
    return buffer_00996C00[3];
}

/* Was hand-transcribed asm while this TU was built with
 * -fno-schedule-insns; with -O2 -G0 the plain C matches. */
int scePadGetReqState(int port, int slot)
{
    if (PadInfo[port][slot].open == 0)
        return 0;
    return scePadGetDmaStr(port, slot)[113];
}

/* Was hand-transcribed asm while this TU was built with
 * -fno-schedule-insns; with -O2 -G0 the plain C matches. */
int scePadGetState(int port, int slot)
{
    unsigned char *p;

    if (PadInfo[port][slot].open == 0)
        return 99;
    p = scePadGetDmaStr(port, slot);
    if (p[112] == 6 && p[113] == 2)
        return 5;
    return p[112];
}

/* Was hand-transcribed asm while this TU was built with
 * -fno-schedule-insns; with -O2 -G0 the plain C matches. */
int scePadRead(int port, int slot, void *buf)
{
    int *p;

    if (PadInfo[port][slot].open == 0)
        return 0;
    p = (int *)scePadGetDmaStr(port, slot);
    memcpy(buf, p, p[24]);
    return p[24];
}

/* Was hand-transcribed asm while this TU was built with
 * -fno-schedule-insns; with -O2 -G0 the plain C matches. */
int scePadGetButtonMask(int port, int slot)
{
    unsigned char *p;

    if (PadInfo[port][slot].open == 0)
        return 0;
    p = scePadGetDmaStr(port, slot);
    if (p[114] != 1)
        return 0;
    if (p[100] < 2)
        return 0;
    if (p[102] < 2)
        return 0;
    return (long)p[121] + ((long)p[122] << 8) + ((long)p[123] << 16)
         + ((long)p[124] << 24);
}

/* scePadGetDmaStr: hand back the port/slot's DMA buffer after flushing
 * the 256 bytes the IOP writes into it out of the data cache.  The
 * buffer is double-banked: two 128-byte halves, and a pair of sequence
 * counters at +88 and +216 (word 22 of each half) says which half holds
 * the newest sample.  `(a < b) << 7` is the whole selection -- writing
 * it as a shifted comparison rather than a ternary is what produces the
 * original's slt + sll 7 + addu with no branch. */
extern void SyncDCache(void *start, void *end);

unsigned char *scePadGetDmaStr(int port, int slot)
{
    unsigned char *p;
    int sel;

    p = PadInfo[port][slot].dma;
    SyncDCache(p, p + 256);
    sel = *(int *)(p + 88) < *(int *)(p + 216);
    return p + (sel << 7);
}

/* Sample sequence number of the newest frame in the port's buffer. */
int scePadGetFrameCount(int port, int slot)
{
    int *p;

    if (PadInfo[port][slot].open == 0)
        return 0;
    p = (int *)scePadGetDmaStr(port, slot);
    return p[22];
}

/* Post a new request code (byte +113 of the DMA buffer) for the IOP
 * side to pick up; 1 on success, 0 if the port is not open. */
int scePadSetReqState(int port, int slot, int state)
{
    if (PadInfo[port][slot].open == 0)
        return 0;
    scePadGetDmaStr(port, slot)[113] = state;
    return 1;
}

/* Pressure-sensitive button mode is just a button-info mask: all twelve
 * bits on to enter, zero to leave, and "are all twelve on?" to test. */
extern int scePadSetButtonInfo(int port, int slot, int mask);
extern int scePadGetButtonMask(int port, int slot);

int scePadEnterPressMode(int port, int slot)
{
    if (PadInfo[port][slot].open == 0)
        return 0;
    return scePadSetButtonInfo(port, slot, 0xFFF);
}

int scePadExitPressMode(int port, int slot)
{
    if (PadInfo[port][slot].open == 0)
        return 0;
    return scePadSetButtonInfo(port, slot, 0);
}

int scePadInfoPressMode(int port, int slot)
{
    if (PadInfo[port][slot].open == 0)
        return 0;
    return scePadGetButtonMask(port, slot) == 0x3FFFF;
}

/* RPC opcode 18: version of the IOP-side pad module. */
int scePadGetModVersion(void)
{
    buffer_00996C00[0] = 18;
    if (sceSifCallRpc(&padsif, 1, 0, buffer_00996C00, 128,
                      buffer_00996C00, 128, 0, 0) < 0)
        return 0;
    return buffer_00996C00[3];
}

/* RPC opcode 20: set the controller-disconnect warning level.  The
 * argument is stored into the payload BEFORE the opcode -- reversing
 * the two statements reverses the two stores. */
int scePadSetWarningLevel(int level)
{
    buffer_00996C00[1] = level;
    buffer_00996C00[0] = 20;
    if (sceSifCallRpc(&padsif, 1, 0, buffer_00996C00, 128,
                      buffer_00996C00, 128, 0, 0) < 0)
        return 0;
    return buffer_00996C00[2];
}
