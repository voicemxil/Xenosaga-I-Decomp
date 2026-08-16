/* xglTask callbacks that are not part of a larger subsystem: the
 * item-get banner the event script raises, and the per-frame VIF packet
 * the map-change wipe pushes. */

#include "common.h"

/* The window createItemGetWin hands back; nState reaches 2 when the
 * banner has finished playing. */
typedef struct {
    u16 nState;                     /* 0x00 */
} ITEMWIN;

struct MAPUNIT;

/* The authored map-object record, inline in a map unit at +0x1A0 */
typedef struct {
    u8 pad000[0x5];                 /* 0x1A0 */
    s8 nUnk005;                     /* 0x1A5 */
    u8 pad006[0x2A];                /* 0x1A6 */
    short nMethod;                  /* 0x1D0 */
} TASKMAPUNITSET;

typedef struct MAPUNIT {
    u8 pad000[0xA2];                /* 0x000 */
    u8 nUnk0A2;                     /* 0x0A2 */
    u8 pad0A3[0xFD];                /* 0x0A3 */
    TASKMAPUNITSET set;             /* 0x1A0 */
    u8 pad1D2[0x12E];               /* 0x1D2 */
} MAPUNIT;

/* Shared by both item-get tasks: the script-raised one has no owning
 * map unit and no table entry, so it only fills in nItemName. */
typedef struct {
    u8 pad000[0x18];                /* 0x00 */
    int nItemName;                  /* 0x18 */
    MAPUNIT *pOwner;                /* 0x1C */
    ITEMWIN *pWin;                  /* 0x20 */
    s8 nKind;                       /* 0x24 */
    u8 pad025[0x1];                 /* 0x25 */
    short nItemId;                  /* 0x26 */
    s8 nCount;                      /* 0x28 */
    s8 nUnk029;                     /* 0x29 */
    u8 pad02A[0x2];                 /* 0x2A */
    int nMoney;                     /* 0x2C */
    s8 nCreated;                    /* 0x30 */
} EVTITEMGET;

/* One row of the treasure-chest contents table */
typedef struct {
    u8 nKind;                       /* 0x00 */
    u8 nItem;                       /* 0x01 */
    u8 nCount;                      /* 0x02 */
    u8 nUnk03;                      /* 0x03 */
    int nMoney;                     /* 0x04 */
} ITEMBOXENTRY;

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

extern ITEMBOXENTRY ItemBoxTbl[];
extern char *strno[];
extern MAPUNIT MapUnit[];
extern char D_004DB780[];

void *memset(void *, int, int);
int sprintf(char *, const char *, ...);
ITEMWIN *createItemGetWin(char *);
void xglFlagsSet1(int, int);
int GetItemName(int, int);
void dataMoneyBoxInc(int);
void dataBoxInc(int, int);
void CallMethod_I(char *, int);
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

/* TODO: near-miss (17/205 words) - everything is structurally right and
 * the residual is register naming in the ItemBoxTbl read: the original
 * puts the table base in $a3, the index shift in $a2 and the byte temp
 * in $v0, gcc rotates that to $a2/$v0/$a3.  Two findings did land: the
 * original computes the entry address TWICE (one base for the four byte
 * fields, another for the money word) which only appears if the reads
 * are written as ItemBoxTbl[n].field rather than through a cached
 * pointer; and the count-down credit loop is a guarded do-while
 * (blez guard, bnez back-edge), not `while (n > 0)`.  Swept: all 120
 * orderings of the five table reads, cached-pointer forms for the byte
 * and money halves, int vs short index, and moving the nCreated store.
 * Treasure-chest item-get: look the chest contents up in ItemBoxTbl,
 * build the banner text (money, single item, or a spelled-out count),
 * credit the inventory, then tell the owning map unit it is open. */
void taskItemGet(EVTITEMGET *pTask)
{
    char szMsg[64];
    TASKMAPUNITSET *pSet;
    short nItemId;
    int nNum;
    int nItem;

    GameLoopState.nUnk01C = 0;
    if (pTask->nCreated == 0) {
        memset(szMsg, 0, 64);
        nItemId = pTask->nItemId;
        pTask->nKind = ItemBoxTbl[nItemId].nKind;
        nItem = ItemBoxTbl[nItemId].nItem;
        pTask->nCount = ItemBoxTbl[nItemId].nCount;
        pTask->nUnk029 = ItemBoxTbl[nItemId].nUnk03;
        pTask->nMoney = ItemBoxTbl[nItemId].nMoney;
        xglFlagsSet1(0x79EC7 + nItemId, 1);
        pTask->nCreated = 1;
        if (pTask->nMoney != 0) {
            sprintf(szMsg, "Obtained %d G.", pTask->nMoney);
            dataMoneyBoxInc(pTask->nMoney);
        } else {
            nNum = pTask->nCount;
            if (nNum == 1) {
                int nName;

                nName = GetItemName(pTask->nKind, nItem);
                pTask->nItemName = nName;
                if (pTask->nKind == 10) {
                    sprintf(szMsg, "Obtained \x0c" "2\x9b\xbe%s\x0c\x80\x80\x80.",
                            nName);
                } else {
                    sprintf(szMsg, "Obtained %s.", nName);
                }
                dataBoxInc(pTask->nKind, nItem);
            } else {
                int nName;
                int nCount;

                nCount = nNum;
                if (nCount >= 100) {
                    nCount = 99;
                }
                nName = GetItemName(pTask->nKind, nItem);
                pTask->nItemName = nName;
                if (pTask->nCount < 10) {
                    if (pTask->nKind == 10) {
                        sprintf(szMsg,
                                "Obtained %s \x0c" "2\x9b\xbe%s\x0c\x80\x80\x80.",
                                strno[nCount], nName);
                    } else {
                        sprintf(szMsg, "Obtained %s %s.", strno[nCount], nName);
                    }
                } else {
                    if (pTask->nKind == 10) {
                        sprintf(szMsg,
                                "Obtained %s%s \x0c" "2\x9b\xbe%s\x0c\x80\x80\x80",
                                strno[nCount / 10], strno[nCount % 10], nName);
                    } else {
                        sprintf(szMsg, "Obtained %s%s %s", strno[nCount / 10],
                                strno[nCount % 10], nName);
                    }
                }
                if (nCount > 0) {
                    do {
                        nCount--;
                        dataBoxInc(pTask->nKind, nItem);
                    } while (nCount != 0);
                }
            }
        }
        pTask->pWin = createItemGetWin(szMsg);
        GameLoopState.nFlags |= 0x20000;
    }
    if (pTask->pWin->nState == 2) {
        pSet = &pTask->pOwner->set;
        GameLoopState.nFlags &= ~0x20000;
        if (pSet->nMethod != -1) {
            CallMethod_I(D_004DB780, pSet->nMethod);
        }
        if (pSet->nUnk005 != -1) {
            MapUnit[pSet->nUnk005].nUnk0A2 = 5;
        }
        xglTaskWaitRemove(pTask);
    }
}
