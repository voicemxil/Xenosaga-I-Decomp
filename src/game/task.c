/* xglTask callbacks that are not part of a larger subsystem: the
 * item-get banner the event script raises, and the per-frame VIF packet
 * the map-change wipe pushes. */

#include "common.h"

/* The window createItemGetWin hands back; nState reaches 2 when the
 * banner has finished playing. */
typedef struct {
    u16 nState;                     /* 0x00 */
} ITEMWIN;

typedef struct {
    u8 pad000[0x18];                /* 0x00 */
    int nItemName;                  /* 0x18 */
    u8 pad01C[0x4];                 /* 0x1C */
    ITEMWIN *pWin;                  /* 0x20 */
    s8 nKind;                       /* 0x24 */
    u8 pad025[0xB];                 /* 0x25 */
    s8 nCreated;                    /* 0x30 */
} EVTITEMGET;

typedef struct {
    u8 pad000[0x10];                /* 0x00 */
    int nFlags;                     /* 0x10 */
    u8 pad014[0x8];                 /* 0x14 */
    int nUnk01C;                    /* 0x1C */
} TASKGAMELOOP;

/* Framebuffer/render state block */
typedef struct {
    u8 pad000[0x14];                /* 0x00 */
    u16 nUnk14;                     /* 0x14 */
    u8 pad016[0x1A];                /* 0x16 */
    int nUnk30;                     /* 0x30 */
} TASKRENDER;

/* The map-change wipe's own counter block */
typedef struct {
    u8 pad000[0x10];                /* 0x00 */
    u8 nWipe;                       /* 0x10 */
} MAPCHANGETSK;

extern TASKGAMELOOP GameLoopState;
extern TASKRENDER sRender;
extern MAPCHANGETSK tsk;
/* The wipe's VIF/GIF packet template (a static local in the original;
 * symbol_addrs.txt carries a C-nameable alias for it). */
extern long long TestPrim_00362790[];

void sceVif1PkCnt(void *, unsigned int);
void sceVif1PkAddDataN(void *, unsigned int *, unsigned int);
void nmlModelSendBackBufferSignal(void);

void *memset(void *, int, int);
int sprintf(char *, const char *, ...);
ITEMWIN *createItemGetWin(char *);
void xglTaskWaitRemove(void *);

/* "You obtained <item>" banner: build the message once, then wait for
 * the window to finish and drop the task. */
void taskEvtItemGet(EVTITEMGET *pTask)
{
    char szMsg[64];

    GameLoopState.nUnk01C = 0;
    if (pTask->nCreated == 0) {
        memset(szMsg, 0, 64);
        pTask->nCreated = 1;
        if (pTask->nKind == 10) {
            sprintf(szMsg, "Obtained \x0c" "2\x9b\xbe%s\x0c\x80\x80\x80.",
                    pTask->nItemName);
        } else {
            sprintf(szMsg, "Obtained %s.", pTask->nItemName);
        }
        pTask->pWin = createItemGetWin(szMsg);
    }
    if (pTask->pWin->nState == 2) {
        GameLoopState.nFlags &= ~0x20000;
        xglTaskWaitRemove(pTask);
    }
}

/* Map-change wipe: patch the current framebuffer id and the wipe step
 * into the packet template, push it, and count the wipe down. */
void taskMapChange(void *pPacket)
{
    TestPrim_00362790[4] = ((long long)tsk.nWipe << 32) | 100;
    TestPrim_00362790[10] = (0x24120000 | (sRender.nUnk14 << 5)) | 0x640000000LL;
    sceVif1PkCnt(pPacket, 0);
    sceVif1PkAddDataN(pPacket, (unsigned int *)TestPrim_00362790, 44);
    nmlModelSendBackBufferSignal();
    tsk.nWipe += 252;
    if (tsk.nWipe == 0) {
        sRender.nUnk30 = 0;
    }
}
