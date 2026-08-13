/* Menu subsystem - equip/status UI getters, model/sub-window plumbing, sort
   helpers and misc bibration/load-sync bookkeeping */

/* --- Work-end scratch buffer --- */
extern char MenuWorkEndTop[0x10000];
extern void *memset(void *, int, unsigned int);

/* Zero the menu work-end scratch buffer and return its base address */
void *MenuWorkEndGet(void)
{
    memset(MenuWorkEndTop, 0, 0x10000);
    return MenuWorkEndTop;
}

/* --- Load-sync bookkeeping --- */
extern int MenuLoadCount;

/* --- Bibration (controller rumble) state --- */
unsigned char MenuBibrationPad;
unsigned char MenuBibrationAct;
unsigned char MenuBibrationSpeed;
unsigned char MenuBibrationCount;
extern unsigned char D_00490DE0[][104];

/* --- Keep-select cursor slots --- */
typedef struct {
    char pad[5];
} KEEPENT;
extern KEEPENT MenuKeepSelect[11];

/* --- Model sub-window plumbing --- */
typedef struct {
    char pad00[0x4C];
    void *f4C;
} SUBWIN;
typedef struct {
    char b0;
    signed char b1;
} SUBOBJ;
extern void MenuModelResourceCancel(void *);
extern void MenuModelResourceCancel3(void *);
extern void MenuModelSubWindowMainSub(SUBWIN *);
extern char MenuModelSubWindow[0x30];
extern void *memset(void *, int, unsigned int);

/* --- Model weapon/ext-func plumbing --- */
typedef struct {
    char pad00[0x13];
    unsigned char b13;
    char pad14[0x40 - 0x14];
    void *p40;
    void *p44;
} EXTFUNC;
extern void ActorAndResourceDispose(void *, void *, int);

/* --- Model motion / unit teardown plumbing --- */
typedef struct {
    char pad20[0x20];
    void *p20;
} MOTSET1;
typedef struct {
    char pad6F0[0x6F0];
    int f6F0;
} MOTSET2;
extern void ACT_setMotion(void *);
typedef struct {
    char pad10[0x10];
    signed char b10;
} UNITBRK;

/* --- Model task driver --- */
extern int MenuModelTask;
extern int MenuModelFlag;
extern void MenuModelResourceMain(void);
extern void xglTaskExecute(int);

/* --- Save-data / char-index checks --- */
typedef struct {
    char pad[0xC];
} SAVEDATA;
extern SAVEDATA D_004A1A04;
typedef struct {
    unsigned char nFlag;
    char pad[8];
} CURSORKEEP;
extern CURSORKEEP D_004917F4;

/* --- Sort family --- */
extern int sort[][256];
typedef struct {
    char pad[0xC00];
} MWLIST;
extern MWLIST mw_list[];

/* --- Ether/Tec lookup helpers --- */
extern void *func_A1A488(int);
extern void *func_A1A378(int);
extern unsigned char ParaDataChangeTbl[];
extern unsigned short *ParaDataBuf;

/* --- Load-file / top-command / system2-init plumbing --- */
typedef struct {
    char pad[0x10];
} MENUCB;
extern MENUCB menuCallback;
extern int xglCdReadFile(char *name, void *addr, int ofs, void *cb);
extern unsigned short MenuTopCommandLock;
extern int MainMenuWorkEnd;
extern void endPrintInit(void);
extern void subMenuSystemInit(int);
extern void xglCdReadCancel(void);
extern int xglFlagsGet(int, int);

/* --- Segment-item menu monolith layout ---
   These layouts are recovered from MenuItemSegmentMain's affine accesses.
   They intentionally describe storage only; the behavioral reconstruction is
   kept out of the build until every state and renderer path is represented. */
typedef enum {
    MENU_ITEM_SEGMENT_INIT = 0,
    MENU_ITEM_SEGMENT_DRAW = 1,
    MENU_ITEM_SEGMENT_WAIT = 2,
    MENU_ITEM_SEGMENT_ENTER = 3,
    MENU_ITEM_SEGMENT_ACTIVE = 4,
    MENU_ITEM_SEGMENT_LEAVE = 5
} MENU_ITEM_SEGMENT_STATE;

typedef struct {
    unsigned char pulse;       /* 0x00: 0..0x78 highlight intensity */
    signed char pulseStep;     /* 0x01: signed highlight increment */
    char pad02[2];
    unsigned char flags;       /* 0x04: three flag-bank results in bits 0..2 */
    char pad05;
    unsigned char partPulse;   /* 0x06: 0..0x78 robot-part intensity */
    signed char partPulseStep; /* 0x07: signed robot-part increment */
    char mainSprite[0x28];     /* 0x08 */
    char flag1Sprite[0x28];    /* 0x30, flags & 1 */
    char flag2Sprite[0x28];    /* 0x58, flags & 2 */
    char allFlagsSprite[0x28]; /* 0x80, flags == 7 */
    char padA8[0x28];
    char number[0x8C];         /* 0xD0 */
} MENU_ITEM_SEGMENT_ENTRY;

typedef struct {
    unsigned char state;       /* 0x0000: MENU_ITEM_SEGMENT_STATE */
    unsigned char draw;        /* 0x0001: run the shared renderer */
    short slideX;              /* 0x0002 */
    short baseY;               /* 0x0004 */
    short pad0006;
    void *texture;             /* 0x0008 */
    int selected;              /* 0x000C: wrapped to 0..17 */
    char windows[0x328];       /* 0x0010: two 0x194-byte WindowDX objects */
    char headingSprites[0x50]; /* 0x0338: two 0x28-byte sprites */
    char headingMessages[0x88];/* 0x0388: two 0x44-byte messages */
    MENU_ITEM_SEGMENT_ENTRY entry[18]; /* 0x0410, stride 0x15C */
    char pad1C88[4];
    unsigned short allPartMask;      /* 0x1C8C: OR of subRoboPartsCheck results */
    unsigned short selectedPartMask; /* 0x1C8E */
    char partSprites[0x2F8];   /* 0x1C90: nineteen 0x28-byte sprites */
    char ribbonArea[0x188];    /* 0x1F88 */
    char printArea[0x130];     /* 0x2110 */
    void *printTexture;        /* 0x2240 */
} MENU_ITEM_SEGMENT_WORK;

typedef char MENU_ITEM_SEGMENT_ENTRY_size_check[
    sizeof(MENU_ITEM_SEGMENT_ENTRY) == 0x15C ? 1 : -1];
typedef char MENU_ITEM_SEGMENT_WORK_size_check[
    sizeof(MENU_ITEM_SEGMENT_WORK) == 0x2244 ? 1 : -1];

/* Return whether the menu task-work has already been torn down (always false - stub) */
void MenuWorkEndCheck(void)
{
}

/* Zero every keep-select cursor slot */
void MenuKeepSelectReset(void)
{
    KEEPENT *p;
    int i;

    p = MenuKeepSelect;
    for (i = 0xB; i >= 0; i--) {
        memset(p, 0, 5);
        p++;
    }
}

/* Kick off an async menu-overlay file load through the shared callback */
void MenuLoadFile(char *name, void *addr)
{
    xglCdReadFile(name, addr, 1, &menuCallback);
}

/* Return whether the current menu file load has finished */
int MenuLoadSync(void)
{
    return MenuLoadCount;
}

/* Reset the menu file-load progress counter */
void MenuLoadInit(void)
{
    MenuLoadCount = 0;
}

/* No-op: menu file-load teardown hook, unused in the retail build */
void MenuLoadEnd(void)
{
}

/* Cancel an in-flight menu file load if one is pending */
void MenuLoadCancel(void)
{
    if (MenuLoadSync() != 0) {
        MenuLoadCount = 0;
        xglCdReadCancel();
    }
}

/* Latch the controller rumble parameters for the next MenuBibrationMain tick */
void MenuBibrationSet(int nPad, int nAct, int nSpeed, int nCount)
{
    MenuBibrationCount = nCount;
    MenuBibrationAct = nAct;
    MenuBibrationSpeed = nSpeed;
    MenuBibrationPad = nPad;
}

/* Clear the latched rumble parameters */
void MenuBibrationInit(void)
{
    MenuBibrationSet(0, 0, 0, 0);
}

/* Drive one tick of controller rumble from the latched parameters */
void MenuBibrationMain(void)
{
    unsigned char count;
    unsigned char pad, act, speed;

    count = MenuBibrationCount;
    if (count != 0) {
        pad = MenuBibrationPad;
        act = MenuBibrationAct;
        speed = MenuBibrationSpeed;
        MenuBibrationCount = count - 1;
        D_00490DE0[pad][act] = speed;
    }
}

/* Test whether the given top-menu command slot is currently unlocked */
int MenuTopCommandCheck(int nCmd)
{
    unsigned short lock;
    int shift;
    int bit;

    lock = MenuTopCommandLock;
    shift = nCmd - 1;
    bit = 1 << shift;
    return (bit & ~lock) != 0;
}

/* No-op: retail pause-window debug hook */
void MenuPasWindow(void)
{
}

/* --- Model memory-state motion check --- */
extern int MenuModelMemoryState[];

/* Test whether the model memory state allows a menu-motion transition */
int MenuModelMenuMotionCheck(void)
{
    int state;

    state = MenuModelMemoryState[0];
    if (state < 0xA) {
        goto ret0;
    }
    if (state < 0xD) {
        goto ret1;
    }
    if (state != 0x14) {
        goto ret0;
    }
ret1:
    return 1;
ret0:
    return 0;
}

/* Run the model resource loader once, then hand the resulting task to the executor */
void MenuModelResourceCancel2(void *pRes)
{
    MenuModelResourceCancel(pRes);
    MenuModelResourceCancel3(pRes);
}

/* Dispatch the model sub-window main tick if the sub-window slot is populated */
void MenuModelSubWindowMain(SUBWIN *p)
{
    if (p->f4C != 0) {
        MenuModelSubWindowMainSub(p);
    }
}

/* Mark the model sub-window's inner object dead and run one last main tick */
void MenuModelSubWindowBreak(SUBWIN *p)
{
    SUBOBJ *pObj;

    pObj = (SUBOBJ *)p->f4C;
    if (pObj != 0) {
        pObj->b1 = -1;
        MenuModelSubWindowMainSub(p);
    }
}

/* Clear the model sub-window scratch buffer */
void MenuModelSubWindowinit(void)
{
    memset(MenuModelSubWindow, 0, 0x30);
}

/* Dispose a weapon model's actor and resource blocks if one is attached */
void MenuModelWeaponDispose(char *pWeapon)
{
    if (pWeapon != 0) {
        ActorAndResourceDispose(pWeapon + 0x20, pWeapon + 0x28, 2);
    }
}

/* Set a model's actor into its default motion and mark it as motion-driven */
void MenuModelMotionSet(MOTSET1 *p)
{
    MOTSET2 *q;

    q = (MOTSET2 *)p->p20;
    ACT_setMotion(q);
    q->f6F0 |= 8;
}

/* Cancel and dispose a model unit's resources, marking the slot dead */
void MenuModelUnitBreak(UNITBRK *p)
{
    if (p != 0) {
        MenuModelResourceCancel2(p);
        p->b10 = -1;
    }
}

/* Attach the given extension-function pointers to a model slot and clear its flag */
void MenuModelExtFuncSet(EXTFUNC *p, void *pFunc1, void *pFunc2)
{
    if (p != 0) {
        p->p44 = pFunc2;
        p->p40 = pFunc1;
        p->b13 = 0;
    }
}

/* Look up the actor bound to a model's weapon slot */
void *MenuModelWeaponActorGet(char *pModel, int nIdx)
{
    char *pSlot;
    void *pWeapon;

    pSlot = (char *)(nIdx << 2);
    if (pModel == 0) {
        return 0;
    }
    pSlot = pSlot + (long)pModel;
    pWeapon = *(void **)(pSlot + 0x34);
    if (pWeapon != 0) {
        return *(void **)((char *)pWeapon + 0x20);
    }
    return 0;
}

/* Run the model resource loader, then feed its task to the model task executor */
void MenuModelMain(void)
{
    MenuModelResourceMain();
    xglTaskExecute(MenuModelTask);
}

extern void MenuModelResourceInit(void);

/* Mark the model system dirty, re-run it, then clear the dirty flag and reinit resources/sub-window */
void MenuModelAllBreak(void)
{
    MenuModelFlag |= 1;
    MenuModelMain();
    MenuModelFlag &= ~1;
    MenuModelResourceInit();
    MenuModelSubWindowinit();
}

/* Return the fixed save-data block pointer */
SAVEDATA *MenuSaveDataGet(void)
{
    return &D_004A1A04;
}

/* Test whether an equipped item can be stripped via the steal mask flag */
int MenuEquipStealMaskCheck(void)
{
    return xglFlagsGet(0x3F9, 1) != 0;
}

/* Test whether the cursor-keep suppression flag is clear */
int MenuCursorKeepCheck(void)
{
    return D_004917F4.nFlag == 0;
}

/* Remap a party-slot id at the mid-story roster change (KOS-MOS <-> MOMO/mary swap) */
int MenuMaryIdChange(int nId)
{
    int nRet;
    int cond;

    if (nId == 0x14) {
        return 0xB;
    }
    if (nId == 0x13) {
        return 0xC;
    }
    cond = nId < 0x11;
    nRet = 3;
    if (cond) {
        nRet = nId;
    }
    return nRet;
}

/* Test whether a party-member index is one of the fixed main-character slots */
int MenuMainCharCheck(int nId)
{
    return (unsigned int)(nId - 1) < 7;
}

/* Test whether a party-member index is one of the AGWS-pilot-eligible slots */
int MenuMainAgwsCheck(int nId)
{
    unsigned int t;

    t = (unsigned int)(nId - 0x13) < 2;
    if (t) {
        return 0;
    }
    return 1;
}

/* Swap two sort keys ascending if out of order (descending compare) */
void MenuSortSubType01(int *a, int *b)
{
    unsigned int t1, t2;

    t1 = *a;
    t2 = *b;
    if (t1 < t2) {
        *a = t2;
        *b = t1;
    }
}

/* Swap two sort keys ascending if out of order (ascending compare) */
void MenuSortSubType00(int *a, int *b)
{
    unsigned int t1, t2;

    t1 = *a;
    t2 = *b;
    if (t2 < t1) {
        *a = t2;
        *b = t1;
    }
}

/* Return the base address of one sort-table row */
int *MenuSortAddrGet(int nRow)
{
    return sort[nRow];
}

/* Fetch one sort-table entry */
int MenuSortGet(int nRow, int nCol)
{
    return sort[nRow][nCol];
}

/* Combine a truncated id with a shifted high-word id (mid-story roster remap helper) */
int MenuIdChange(int a, int b)
{
    return (short)a + (b << 16);
}

/* Return the base address of one menu-work-list entry */
MWLIST *MenuListGet(int nIdx)
{
    return &mw_list[nIdx];
}

/* Fetch a technique's base attack-rate value */
unsigned short MenuParaPtRateGet(int nBase, int nType)
{
    unsigned short *p;

    p = ParaDataBuf + nBase * 8 + ParaDataChangeTbl[nType];
    return p[-8];
}

/* Fetch a technique's base attack-point value */
unsigned short MenuParaPtBaseGet(int nBase, int nType)
{
    int idx;
    unsigned short *p;

    idx = nBase * 8 + ParaDataChangeTbl[nType];
    p = ParaDataBuf + idx;
    return p[0x38];
}

/* Return zero: unused/disabled weapon-attachment attack check */
int MenuCharWeaponAttCheck(void)
{
    return 0;
}

/* --- Ether/Tec raw-data-buffer lookups --- */
extern void *MenuEtherDataBuf;
extern void *MenuTecDataBuf;

/* TODO: near-miss - allocator swaps p(v0)/t(v1) and schedules the idx<<2 shift into
   the branch-delay slot instead of ahead of the sltiu compare (same shape as the
   SsdGetMemoryBlocks/xglDmaMFIFOLeave irreducible allocator tie-break); every natural
   variant tried (decl order, early-return, array-index form, unsigned t) reproduces
   the same 5-word swap. */
/* Return the base address of one ether-data entry, or NULL if the id is out of range */
void *MenuEtherDataGet(int nId)
{
    int idx;
    int t;

    idx = nId & 0xFFFF;
    t = idx - 1;
    if ((unsigned int)t >= 0x50) {
        return 0;
    }
    idx = idx << 2;
    return (char *)MenuEtherDataBuf + idx - 4;
}

/* Look up an ether's element-type byte through the two ether-info indirections */
signed char MenuEtherTypeGet(int nId)
{
    void *p;
    void *q;
    short t;

    p = func_A1A488(nId & 0xFFFF);
    t = *(short *)((char *)p + 6);
    q = func_A1A378(t);
    return *(signed char *)((char *)q + 4);
}

/* TODO: near-miss - same p/t allocator swap and delay-slot-shift scheduling as
   MenuEtherDataGet above (identical function shape, different constants). */
/* Return the base address of one tec-data entry, or NULL if the id is out of range */
void *MenuTecDataGet(int nId)
{
    int idx;
    int t;

    idx = nId & 0xFFFF;
    t = idx - 1;
    if ((unsigned int)t >= 0x8) {
        return 0;
    }
    idx = idx << 6;
    return (char *)MenuTecDataBuf + idx - 0x40;
}

/* Test whether a technique's trigger-condition bits are set */
int MenuTecTrgCheck(int nId)
{
    unsigned char *p;

    p = func_A1A378(nId & 0xFFFF);
    return (*p & 3) != 0;
}

/* Run the secondary end-print init, then hand the work-end pointer to the sub-menu-system init */
void MenuSystem2Init(void)
{
    endPrintInit();
    subMenuSystemInit(MainMenuWorkEnd);
}

extern void subMenuSystemMain(void);
extern void endPrintExtFunc(int, int, int);
extern unsigned char D_0036C191[];

/* Run one sub-menu-system tick, then report whether the end-print sequence is not yet done */
int MenuSystem2(void)
{
    subMenuSystemMain();
    endPrintExtFunc(0, 0x64, 0);
    return D_0036C191[0] != 0xFF;
}

/* --- Box (item/event/weapon/bullet/accessory data record) plumbing --- */
extern int dataItmBoxChk(int);
extern int dataEvtBoxChk(int);
extern int dataWpnBoxChk(int);
extern int dataBltBoxChk(int);
extern int dataAccBoxChk(int);

/* TODO: near-miss - a single differing word, the jump-table base-pointer add
   (orig `addu`, ours `daddu`). Every switch variant tried (unsigned/int index,
   split local, explicit cast) reproduces the same 1-word diff; this looks like
   a compiler-version jump-table codegen artifact rather than a source shape,
   since no jtbl_-based switch anywhere in the codebase yet matches. Worth a
   permuter/register-pin pass or revisiting once another jtbl_ function lands. */
/* --- Para (stat point) up-check plumbing --- */
typedef struct {
    char pad0C[0xC];
    int fC;
} PARAOBJ;
extern int MenuParaPtNowGet(int nBase, int nType);
extern int MenuParaUpMaxGet(int nType);
extern PARAOBJ *func_A19210(int nBase);
extern int MenuParaNextPointGet(int nBase, int nType, int nArg2);

/* Test whether a stat still has room to level up: -2 out of points, -1 below the next
   breakpoint, 0 ready to advance */
int MenuParaUpCheck(int nBase, int nType)
{
    unsigned int ptNow, upMax;
    PARAOBJ *p;
    int nextPoint;

    ptNow = MenuParaPtNowGet(nBase, nType);
    upMax = MenuParaUpMaxGet(nType);
    if (ptNow >= upMax) {
        return -2;
    }
    p = func_A19210(nBase);
    nextPoint = MenuParaNextPointGet(nBase, nType, 0);
    if (p->fC < nextPoint) {
        return -1;
    }
    return 0;
}

/* --- Tec (technique) speed/save-data plumbing --- */
extern char *MenuTecSaveDataGet(int nId);
extern int MenuTecNextSpeedPointGet(int nId, int nSpeed);

/* Mark a technique's speed-up slot used and deduct its point cost */
void MenuTecSpeedUp(int nId, int nSpeed)
{
    int idx;
    char *pSave;
    PARAOBJ *p;
    int nextPoint;

    idx = nId & 0xFFFF;
    pSave = MenuTecSaveDataGet(idx);
    p = func_A19210(idx);
    if (nSpeed < 0) {
        return;
    }
    nextPoint = MenuTecNextSpeedPointGet(nId, nSpeed);
    pSave[nSpeed + 8] = 1;
    p->fC -= nextPoint;
}

/* --- Model sub-window main-tick detail plumbing --- */
typedef struct {
    char b0;
    unsigned char b1;
    unsigned char b2;
    unsigned char b3;
    short h4;
    short h6;
    short h8;
    short hA;
    int fC;
} SUBOBJ2;
extern void nmlModelUseSubWindow(int, int);
extern void xglCameraSetWindow(int, int, int, int, int);

/* Run the actual model sub-window tick: refresh the camera window and radar marker,
   or tear the sub-window down if its owning object has gone away */
void MenuModelSubWindowMainSub(SUBWIN *p)
{
    SUBOBJ2 *pObj;
    char *pActor;
    int fC;

    pObj = (SUBOBJ2 *)p->f4C;
    pActor = *(char **)((char *)p + 0x20);
    if (MenuModelFlag & 1) {
        pObj->b1 = -1;
    }
    if (pObj->b1 == 0) {
        fC = pObj->fC;
    } else if (pObj->b1 == 0xFF) {
        nmlModelUseSubWindow(pObj->b2, 0);
        p->f4C = 0;
        return;
    } else {
        fC = pObj->fC;
    }
    xglCameraSetWindow(fC, pObj->h4, pObj->h6, pObj->h8, pObj->hA);
    nmlModelUseSubWindow(pObj->b2, pObj->b3);
    if (pObj->b3 == 1) {
        *(short *)(pActor + 0x676) = pObj->b2;
        return;
    }
    *(short *)(pActor + 0x676) = 0;
}

/* TODO: near-miss - the weapon-dispose countdown loop matches exactly; only the
   trailing pair of ActorAndResourceDispose calls differs. The original routes
   the second call's two pointer args through s0/s1 (reusing the just-freed loop
   registers) before reloading them for the epilogue; every natural variant tried
   (inline exprs, named p1/p2 locals, reusing pWeapon, goto-shaped loop) instead
   allocates them straight into a0/a1 since nothing forces a callee-saved temp.
   Logic and byte COUNT differ only in this tail (0x90 orig vs 0x88 here). */
/* Dispose a model unit's three weapon slots, then tear down its two actor/resource pairs */
void MenuModelUnitDispose(char *p)
{
    char **pWeapon;
    int i;

    if (p == 0) {
        return;
    }
    pWeapon = (char **)(p + 0x34);
    for (i = 2; i >= 0; i--) {
        MenuModelWeaponDispose(*pWeapon);
        pWeapon++;
    }
    ActorAndResourceDispose(p + 0x24, p + 0x2C, 1);
    ActorAndResourceDispose(p + 0x20, p + 0x28, 0);
}

/* --- Hardcoded z-clear DMA packet plumbing --- */
typedef struct {
    long long f0;
    long long f8;
    long long f10;
    long long f18;
    long long f20;
    long long f28;
} DMABLK1;
typedef struct {
    int f0;
    int f4;
    int f8;
    int fC;
    int f10;
    int f14;
    int f18;
    int f1C;
} DMABLK2;
extern void nmlModelDirectSend(int, void *, int);

/* TODO: near-miss - same size (0xA4) and same set of stores/constants as the
   original, but gcc 2.96's scheduler interleaves the nmlModelDirectSend(1,
   0x70000000, 5) argument setup between the DMABLK1 field stores (reusing each
   just-freed register immediately), while ours computes the call args together
   right before the call. The logical store ORDER already matches; this looks
   like a pure instruction-scheduling difference, not a source-order one -
   candidate for the permuter, but the reorderable set (6 DMABLK1 stores + 8
   DMABLK2 stores + 3 call-arg loads = 17 statements) is too large to sweep by
   hand. */
/* Build and kick off a fixed z-clear GS packet via the scratchpad DMA staging area */
void MenuModelDirectSendZClear(void)
{
    DMABLK1 *p1 = (DMABLK1 *)0x70000000;
    DMABLK2 *p2 = (DMABLK2 *)0x70000030;

    p1->f28 = 0x47;
    p1->f8 = 0x5100000400001100LL;
    p1->f10 = 0x3023400000008001LL;
    p1->f18 = 0x55E;
    p1->f20 = 0x32001;
    p1->f0 = 0;

    p2->f0 = 0x6FF8;
    p2->f4 = 0x71F8;
    p2->f10 = 0x8FF8;
    p2->f14 = 0x8DF8;
    p2->f1C = 0;
    p2->f8 = 0;
    p2->fC = 0;
    p2->f18 = 0;

    nmlModelDirectSend(1, (void *)0x70000000, 5);
}

/* Test whether a box entry exists, dispatching on the record-type encoded in the high word */
int MenuBoxChk(int nId)
{
    unsigned int nType;
    int nRes;

    nType = (unsigned int)nId >> 16;
    nId = nId & 0xFFFF;
    nRes = 0;
    switch (nType - 1) {
    case 0: nRes = dataItmBoxChk(nId); break;
    case 1: nRes = dataEvtBoxChk(nId); break;
    case 2: nRes = dataWpnBoxChk(nId); break;
    case 3: nRes = dataBltBoxChk(nId); break;
    case 4: nRes = dataAccBoxChk(nId); break;
    case 5: nRes = 0; break;
    }
    return nRes;
}

extern int BitToTecNo(int);

/* Ask whether a technique's type flag (via BitToTecNo) equals 1 */
int MenuTecTypeCheck(int nId, int nType)
{
    int tecNo;
    unsigned char *pSave;
    int idx;

    idx = nId & 0xFFFF;
    tecNo = BitToTecNo(nType);
    pSave = MenuTecSaveDataGet(idx);
    return pSave[tecNo + 8] == 1;
}

typedef struct {
    char pad0[0x54];
    short hPilotId;    /* 0x54 */
} PILOTREC;

extern PILOTREC *func_A191C0_2(int id);

/* Search pilot slots 1-12 for the one whose pilot id matches nId */
int MenuAgwsPilotCheck(int nId)
{
    int i;
    PILOTREC *p;

    for (i = 1; i < 13; i++) {
        p = func_A191C0_2(i);
        if (p->hPilotId == nId) {
            return i;
        }
    }
    return 0;
}

extern unsigned char *D_0035CA5C;

/* TODO: near-miss (LENGTH) - the nested-while form compiles the outer loop
 * with an extra unconditional jump the original doesn't have (a top-test
 * layout with a loop-back that reuses the byte already loaded in the
 * previous iteration's delay slot); do-while and if-guard variants both
 * made it worse (20 orig vs 22/23/25 built). Parked after 2 attempts. */
/* Skip forward past (nIdx - 1) NUL-terminated strings in the segment info-text blob */
unsigned char *MenuSegmentInfoTextGet(int nIdx)
{
    unsigned char *p;

    p = D_0035CA5C;
    while (nIdx >= 2) {
        while (*p != 0) {
            p++;
        }
        p++;
        nIdx--;
    }
    return p;
}

/* Skip forward past (nIdx - 1) NUL-terminated strings in the segment item-name blob */
unsigned char *MenuSegmentItemNameGet(int nIdx)
{
    unsigned char *p;

    p = D_0035CA5C + 960;
    while (nIdx >= 2) {
        while (*p != 0) {
            p++;
        }
        p++;
        nIdx--;
    }
    return p;
}

/* Skip forward past (nIdx - 1) NUL-terminated strings in the segment map-name blob */
unsigned char *MenuSegmentMapNameGet(int nIdx)
{
    unsigned char *p;

    p = D_0035CA5C + 1344;
    while (nIdx >= 2) {
        while (*p != 0) {
            p++;
        }
        p++;
        nIdx--;
    }
    return p;
}
