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
    char *p;

    idx = nId & 0xFFFF;
    t = idx - 1;
    idx = idx << 2;
    p = (char *)MenuEtherDataBuf;
    if ((unsigned int)t >= 0x50) {
        return 0;
    }
    return p + idx - 4;
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
    char *p;

    idx = nId & 0xFFFF;
    t = idx - 1;
    idx = idx << 6;
    p = (char *)MenuTecDataBuf;
    if ((unsigned int)t >= 0x8) {
        return 0;
    }
    return p + idx - 64;
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
    int f10;
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

/* Return the base address of one technique's save-data record, or NULL if the id is out of range */
/* TODO: near-miss (LENGTH, 17 orig vs 16 built) - the +0x3c/-32 offset pair
 * folds differently than the original's separate idx*32 and -32 terms;
 * parked after 2 attempts. */
char *MenuTecSaveDataGet(int nId)
{
    char *p;
    int idx;
    unsigned int t;
    int off;

    idx = nId & 0xFFFF;
    p = (char *)PartyDataGet();
    t = idx - 1;
    off = idx * 32;
    p = p + 0x3C;
    if (t >= 8) {
        return 0;
    }
    return p + off - 32;
}

/* Ask whether a technique's tec-level is below its saved-data level cap */
/* TODO: near-miss (LENGTH) - original computes pSave+nLev into two separate
 * registers (redundant CSE-defeating computation); parked after 2 attempts. */
int MenuTecTLevLimitCheck(int nId, int nLev)
{
    unsigned char *pSave;
    unsigned char *q;

    pSave = (unsigned char *)MenuTecSaveDataGet(nId);
    q = (unsigned char *)(nLev + (int)pSave);
    __asm__("" : "+r"(pSave));
    if (nLev < 0) {
        return 0;
    }
    return pSave[nLev] < q[24];
}

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

/* TODO: near-miss (SCHEDULING, 7) - was LENGTH 21. q1 hoisted above call 1
   (lands in the jal slot as addiu s0), scoped q2 pin $17 + launder gives the
   s1 staging and the tail j. Remaining pure sched: call-1's a1/a0 addiu pair
   is swapped (adjacent, --swap-adjacent candidate at insn 15) and the
   epilogue rotates ld s2 from position 22 to 26. */
/* Dispose a model unit's three weapon slots, then tear down its two actor/resource pairs */
void MenuModelUnitDispose(char *p)
{
    register char **pWeapon __asm__("$16");
    char *q1;
    register int i __asm__("$17");

    if (p == 0) {
        return;
    }
    pWeapon = (char **)(p + 0x34);
    for (i = 2; i >= 0; i--) {
        MenuModelWeaponDispose(*pWeapon);
        pWeapon++;
    }
    q1 = p + 0x20;
    {
        char *a1v;
        char *a0v;

        a1v = p + 0x2C;
        a0v = p + 0x24;
        ActorAndResourceDispose(a0v, a1v, 1);
    }
    {
        register char *q2 __asm__("$17");

        q2 = p + 0x28;
        __asm__("" : "+r"(q2));
        ActorAndResourceDispose(q1, q2, 0);
    }
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
        while (*p++ != 0) {
        }
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
        while (*p++ != 0) {
        }
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
        while (*p++ != 0) {
        }
        nIdx--;
    }
    return p;
}

typedef struct {
    char pad0[0x20];
    void *obj;                 /* 0x20 */
} DRAWTYPEARG;

typedef struct {
    char pad0[0x124];
    void (*func)(void *);      /* 0x124 */
} DRAWTYPEOBJ;

/* Run a model's draw-type callback, if it and its owning object both exist */
void MenuModelDrawTypeMain(DRAWTYPEARG *p)
{
    DRAWTYPEOBJ *obj;

    obj = (DRAWTYPEOBJ *)p->obj;
    if (obj != 0) {
        if (obj->func != 0) {
            obj->func(obj);
        }
    }
}

extern char sRender_0[0x5C] __asm__("sRender");
extern int D_0036C278[8] __asm__("MenuCfTaiki");

/* TODO: near-miss (SCHEDULING, 2) - was LOGIC 12. Fixed by: base-launder to
 * split lo(sRender)+0x24, cont=i<8 + launder (blocks loop reversal, pins slti
 * position), pinned cont $3, v-launder before the dst store (anchors the
 * sw/addiu tail and the delay slot). Remaining: the loop-head lw/addiu-i pair
 * issues in the opposite order; every head permute flips the tail back. */
/* Push the 8 cached "taiki" (waiting) slots into the save buffer and clear the cache */
void MenuCfTaikiPush(void)
{
    char *base;
    int *src;
    int *dst;
    int i;
    int v;
    int cont;

    base = sRender_0;
    __asm__("" : "+r"(base));
    dst = D_0036C278;
    src = (int *)(base + 0x24);
    i = 0;
    do {
        v = *src;
        i++;
        cont = i < 8;
        __asm__("" : "+r"(cont));
        *src = 0;
        __asm__("" : "+r"(v));
        *dst = v;
        src++;
        dst++;
    } while (cont != 0);
}

/* TODO: near-miss (LENGTH) - the bit-14 test compiles via srl/xori/andi
 * instead of the original's andi+sltiu; parked after 2 attempts. */
/* Ask whether an ether is flagged as "already learned" (Jouto) */
int MenuEtherJoutoCheck(int nId)
{
    int idx;
    void *p;
    int t;

    idx = nId & 0xFFFF;
    if ((unsigned int)(idx - 1) >= 80) {
        return 0;
    }
    p = func_A1A488(idx);
    t = *(unsigned short *)((char *)p + 2) & 0x4000;
    __asm__("" : "+r"(t));
    return t == 0;
}

typedef struct {
    char pad0[0x42];
    short id;                  /* 0x42 */
    char pad44[2];
} MAPNAMEENT;
extern MAPNAMEENT *save_map_text_p __asm__("save_map_text");

/* TODO: near-miss (LOGIC) - same bnezl loop-back-reuses-delay-slot-load
 * shape as MenuSegmentInfoTextGet/ItemNameGet/MapNameGet; unresolved there
 * too. Parked after 2 attempts. */
/* Search the save-map-name table for a matching id, else return the table base */
void *MenuSaveMapNameGet(int nId)
{
    register MAPNAMEENT *p __asm__("$5");
    MAPNAMEENT *base;
    int i;

    p = save_map_text_p;
    base = p;
    for (i = 0; i < 37; i++) {
        if (p->id == nId) {
            return p;
        }
        p++;
    }
    return base;
}

/* TODO: near-miss (LENGTH, 17 orig vs 21 built) - constant folding of the
 * table base + shifted index differs from the original's separate
 * lui/addiu/addu sequence. Parked after 2 attempts. */
/* Count consecutive non-NULL pointer entries (after the head) in a sort chain */
int MenuSortCheck(int nIdx)
{
    int *p;
    register int cnt __asm__("$3");
    int v;

    cnt = 0;
    p = sort[nIdx];
    v = *p;
    p++;
    if (v != 0) {
        do {
            v = *p;
            p++;
            cnt++;
        } while (v != 0);
    }
    return cnt;
}

typedef struct {
    char pad0[8];
    short s08;                 /* 0x08 */
    char pad0A[4];
    int s10;                    /* 0x10 */
} ETHERINFO;

/* TODO: near-miss (LENGTH) - original keeps the first field read in a
 * callee-saved register ($s1) across no further calls; ours allocates it
 * to a caller-saved temp instead. Parked after 2 attempts. */
/* Ask whether a character's ether point value has reached the type's requirement */
int MenuEtherCharPointCheck(int nId, int nPoint)
{
    PARAOBJ *p1;
    ETHERINFO *p2;
    int pt;

    nPoint = nPoint & 0xFFFF;
    p1 = func_A19210(nId & 0xFFFF);
    pt = p1->f10;
    p2 = (ETHERINFO *)func_A1A488(nPoint);
    pt = pt < p2->s08;
    __asm__("" : "+r"(pt));
    __asm__("" : "+r"(pt));
    return pt == 0;
}

extern int func_A1A5E8(int);
extern int func_A1A598(int);
extern void *chrEquipGet(int);

/* Ask whether a character has an engine-slot item equipped matching the mask.
   (The old REGISTER near-miss here was really swapped roles: the halfword is
   read from the func_A1A5E8 record, and chrEquipGet supplies the mask.) */
int MenuEngineEquipCheck(int nId, int nMask)
{
    char *pRec;
    int equip;
    unsigned short v;

    if (nMask == 0) {
        return 0;
    }
    pRec = (char *)func_A1A5E8(nMask);
    equip = (int)chrEquipGet(nId);
    v = *(unsigned short *)(pRec + 6);
    return (v & equip) != 0;
}

/* Ask whether a character has a frame-slot item equipped matching the mask */
int MenuFrameEquipCheck(int nId, int nMask)
{
    char *pRec;
    int equip;
    unsigned short v;

    if (nMask == 0) {
        return 0;
    }
    pRec = (char *)func_A1A598(nMask);
    equip = (int)chrEquipGet(nId);
    v = *(unsigned short *)(pRec + 6);
    return (v & equip) != 0;
}

extern void *D_0036D760[];
extern void *D_0036D7D8[];
extern int MenuScenarioNoGet(void);

/* Return a character's display-name pointer, special-cased for Shion pre-scenario-108 */
/* TODO: near-miss (LOGIC) - the inner assignment compiles to a movn
 * (conditional move) instead of the original's explicit branch; every
 * if-shape tried (nested if, early-return chain) gives the same or worse
 * diff count. Parked after 2 attempts. */
void *MenuCharNameGet(int nId)
{
    void *p;
    register int sc __asm__("$2");

    p = D_0036D760[nId];
    if (nId != 5) {
        return p;
    }
    sc = MenuScenarioNoGet();
    if (sc >= 108) {
        return p;
    }
    p = D_0036D7D8[0];
    __asm__ volatile("" : "+r"(p));
    return p;
}

extern int xglFlagsGet(int, int);
extern int dataEvtBoxChk(int);

/* Ask whether Shion's MWS (mobile weapon suit) is currently unlocked */
int MenuShionMwsCheck(void)
{
    int scenario;

    scenario = MenuScenarioNoGet();
    if (scenario < 10) {
        return 1;
    }
    if (xglFlagsGet(1023, 1) != 0) {
        return 1;
    }
    return dataEvtBoxChk(10) != 0;
}

/* --- Tag/para name tables (function-local statics in the original TU) --- */
extern void *tag_name_3[] __asm__("tag_name.3");
extern void *para_name_4[] __asm__("para_name.4");
extern void *para_name2_5[] __asm__("para_name2.5");

/* Return the status-tag name string for one tag index */
void *MenuTagTextGet(int nIdx)
{
    return tag_name_3[nIdx];
}

/* Return the primary parameter-name string for one parameter index */
void *MenuParaNameGet(int nIdx)
{
    return para_name_4[nIdx];
}

/* Return the secondary parameter-name string for one parameter index */
void *MenuParaNameGet2(int nIdx)
{
    return para_name2_5[nIdx];
}

/* Ask whether a technique's wait-up slot has already been taken */
int MenuTecWaitLimitCheck(int nId, int nWait)
{
    unsigned char *pSave;

    pSave = (unsigned char *)MenuTecSaveDataGet(nId);
    if (nWait < 0) {
        return 0;
    }
    return *(unsigned char *)(nWait + (int)pSave + 16) != 0;
}

/* Ask whether a technique's speed-up slot is still unclaimed */
int MenuTecSpeedLimitCheck(int nId, int nSpeed)
{
    unsigned char *pSave;

    pSave = (unsigned char *)MenuTecSaveDataGet(nId);
    if (nSpeed < 0) {
        return 0;
    }
    return pSave[nSpeed + 8] != 1;
}

/* Fetch the point cost of a technique's next speed-up step */
int MenuTecNextSpeedPointGet(int nId, int nSpeed)
{
    char *pData;

    pData = (char *)MenuTecDataGet(nId);
    if (nSpeed < 0) {
        return 0;
    }
    return *(unsigned short *)(pData + nSpeed * 2 + 0x20);
}

/* Fetch the point cost of a technique's next wait-up step */
int MenuTecNextWaitPointGet(int nId, int nWait)
{
    char *pData;

    pData = (char *)MenuTecDataGet(nId);
    if (nWait < 0) {
        return 0;
    }
    return *(unsigned short *)(pData + nWait * 2 + 0x30);
}

/* --- Config "taiki" (waiting) slot cache --- */
extern int MenuCfTaiki[8];
extern char sRender[];

/* Pop the 8 saved "taiki" (waiting) slots back into the render work block */
void MenuCfTaikiPop(void)
{
    char *base;
    int *src;
    int *dst;
    register int i __asm__("$4");
    register int v __asm__("$3");
    int cont;

    base = sRender_0;
    __asm__("" : "+r"(base));
    src = D_0036C278;
    dst = (int *)(base + 0x24);
    i = 0;
    do {
        v = *src;
        __asm__("" : "+r"(v));
        __asm__("" : "+r"(v));
        src++;
        i++;
        cont = i < 8;
        __asm__("" : "+r"(cont));
        *dst = v;
        dst++;
    } while (cont != 0);
}

/* --- Model menu-motion resource pointer --- */
extern void *D_0036D5C8[];

/* Return the menu-motion resource pointer when the model state and flag allow it */
void *MenuModelMenuMotionGet(void)
{
    void *p;

    p = 0;
    if (MenuModelMenuMotionCheck() == 1) {
        if (MenuModelFlag & 2) {
            p = D_0036D5C8[0];
        }
    }
    return p;
}

extern void MenuModelDrawTypeSet(void *, int);

/* Apply a draw type to the actor behind one of a model's weapon slots */
void MenuModelWeaponOpen(char *pModel, int nType, int nIdx)
{
    char *pWeapon;
    char *pActor;

    if (pModel == 0) {
        return;
    }
    pWeapon = *(char **)((nIdx << 2) + (int)pModel + 0x34);
    if (pWeapon == 0) {
        return;
    }
    pActor = *(char **)(pWeapon + 0x20);
    if (pActor == 0) {
        return;
    }
    MenuModelDrawTypeSet(pActor, nType);
}

/* --- Background task plumbing ---
   MenuBgTaskMain itself lives in tskMenuBg.c (matched there); Menu.c only
   calls it from MenuBgTaskBreak. */
extern void MenuBgTaskMain(void);

/* Remap the three dual-wield right-hand weapon ids onto their left-hand pair */
int MenuRWeaponCheck2(int nId)
{
    switch (nId) {
    case 0x70:
        nId = 0x5B;
        break;
    case 0x71:
        nId = 0x5C;
        break;
    case 0x72:
        nId = 0x5E;
        break;
    }
    return nId;
}

/* --- Model resource-request table --- */
typedef struct {
    unsigned short id;         /* 0x00 */
    short pad02;
    void *pData;               /* 0x04 */
    void *pArg;                /* 0x08 */
} MODELRESENT;
typedef struct {
    int f0;                    /* 0x00 */
    int f4;                    /* 0x04 */
    MODELRESENT ent[16];       /* 0x08 */
    int fC8;                   /* 0xC8 */
} MODELRES;
extern MODELRES MenuModelResourceState;

/* Clear every model resource-request slot and the table header */
void MenuModelResourceInit(void)
{
    MODELRES *w;
    int i;

    w = &MenuModelResourceState;
    for (i = 0; i < 16; i++) {
        w->ent[i].pData = 0;
        w->ent[i].id = 0;
        w->ent[i].pArg = 0;
    }
    w->f4 = 0;
    w->fC8 = 0;
    w->f0 = 0;
}

/* Queue a model resource request into the first free slot (slot 0 is reserved) */
void MenuModelResourceRequest(void *pData, unsigned short nId, void *pArg)
{
    int i;

    for (i = 1; i < 16; i++) {
        if (MenuModelResourceState.ent[i].pData == 0) {
            MenuModelResourceState.ent[i].pData = pData;
            MenuModelResourceState.ent[i].id = nId;
            MenuModelResourceState.ent[i].pArg = pArg;
            return;
        }
    }
}

/* --- Model memory-slot table --- */
typedef struct {
    unsigned char used;        /* 0x00 */
    unsigned char id;          /* 0x01 */
    char pad02[26];
} MODELMEMENT;                 /* 0x1C bytes */
typedef struct {
    int f0;                    /* 0x00 */
    void *f4;                  /* 0x04 */
    void *f8;                  /* 0x08 */
    MODELMEMENT ent[16];       /* 0x0C */
} MODELMEM;

/* Find the free model memory slot already tagged with the given id */
MODELMEMENT *MenuModelMemoryGet(int nArg, int nId)
{
    unsigned char *q;
    int i;

    for (i = 0; i < 16; i++) {
        q = (unsigned char *)MenuModelMemoryState + i * 28;
        if (q[12] == 0) {
            if (q[13] == nId) {
                return (MODELMEMENT *)((unsigned char *)MenuModelMemoryState + i * 28 + 12);
            }
        }
    }
    return 0;
}

/* Reset the model memory table and record its heap bounds */
void MenuModelMemoryInit(void *pTop, void *pEnd)
{
    MODELMEM *w;

    w = (MODELMEM *)MenuModelMemoryState;
    memset(w, 0, 0x1CC);
    w->f4 = pTop;
    w->f8 = pEnd;
    w->f0 = -1;
}

extern int xglFontLoad(int, void *);

/* Load (or reload) the menu font, optionally with the shared load callback */
void MenuFontLoad(int nFlag, int nCallback)
{
    static int original_font_no;
    void *pFunc;

    pFunc = 0;
    if (nCallback != 0) {
        pFunc = &menuCallback;
    }
    if (nFlag != 0) {
        original_font_no = xglFontLoad(1, pFunc);
    } else {
        xglFontLoad(original_font_no, pFunc);
    }
}

extern int xglTaskInitial(int, int, int);

/* Align the model heap, reset the model state and create the model task */
int MenuModelInit(int nMemory)
{
    int nTask;

    nTask = (nMemory + 15) & ~15;
    MenuModelTask = nTask;
    MenuModelFlag = 0;
    nTask = xglTaskInitial(nTask, 32, 0);
    MenuModelResourceInit();
    MenuModelSubWindowinit();
    return nTask;
}

extern void nmlModelSetTransparency(float);
extern void nmlModelSetToumei(int);
extern void nmlModelSetZwrite(int);

typedef struct {
    char pad0[0x120];
    float alpha;               /* 0x120 */
} ALPHAOBJ;

/* Push a model's per-object alpha into the renderer when it is translucent */
void MenuModelAlphaDraw(ALPHAOBJ *p)
{
    float a;

    a = p->alpha;
    if (a < 1.0f) {
        nmlModelSetTransparency(a);
        nmlModelSetToumei(1);
        nmlModelSetZwrite(1);
    }
}

/* Count how many of a character's six technique slots hold the given technique.
   K&R-style definition on purpose: the original's callers pass the char/tec
   bytes unnarrowed (raw lb/lh), which needs an unprototyped call; the callee
   narrows both args itself (andi) exactly as the retail build does. */
int MenuTecEquipCheck(nChr, nTec)
unsigned short nChr;
unsigned short nTec;
{
    short *p;
    int nCount;
    int i;

    p = (short *)((char *)func_A191C0_2(nChr) + 0x82);
    nCount = 0;
    for (i = 5; i >= 0; i--) {
        if (*p == nTec) {
            nCount++;
        }
        p++;
    }
    return nCount;
}

/* Swap two sort keys if the first box-value is smaller (descending value sort) */
void MenuSortSubType02(int *a, int *b)
{
    int v1, v2;
    int t1, t2;

    v1 = MenuBoxChk(*a);
    v2 = MenuBoxChk(*b);
    if (v1 < v2) {
        t1 = *a;
        t2 = *b;
        *a = t2;
        *b = t1;
    }
}

/* Swap two sort keys if the second box-value is smaller (ascending value sort) */
void MenuSortSubType03(int *a, int *b)
{
    int v1, v2;
    int t1, t2;

    v1 = MenuBoxChk(*a);
    v2 = MenuBoxChk(*b);
    if (v2 < v1) {
        t1 = *a;
        t2 = *b;
        *a = t2;
        *b = t1;
    }
}

/* Cancel an in-flight model resource load and clear the pending slot */
void MenuModelResourceCancel(void *pRes)
{
    MODELRES *w;
    int *p;

    if (MenuLoadSync() != 0) {
        MenuLoadCancel();
        w = &MenuModelResourceState;
        if (w->f4 != 0) {
            *(unsigned char *)w->f4 = 0;
        }
        p = (int *)w->ent[0].pData;
        w->f4 = 0;
        if (p != 0) {
            *p = -1;
        }
        w->ent[0].pData = 0;
        w->ent[0].id = 0;
        w->ent[0].pArg = 0;
    }
}

/* Search a character's twelve set-ether slots for the given ether id */
int MenuEtherSetCheck(unsigned short nChr, unsigned short nId)
{
    short *p;
    int i;

    p = (short *)((char *)func_A191C0_2(nChr) + 0x8E);
    for (i = 0; i < 12; i++) {
        if (*p == nId) {
            return 1;
        }
        p++;
    }
    return 0;
}

extern void eMessageCpy(char *buf, const char *str);
extern void eMessageCat(const char *str);
extern int xglFontGetStringWidth(const char *s);
typedef struct {
    char b0, b1, b2;
} PASTAGVAL;
typedef struct {
    PASTAGVAL tag;
    char pad[9];
} PASTAG;
extern PASTAG D_004DAD18;

/* Measure the pixel width of a message prefixed with the shared pas tag bytes */
int MenuPasLengthGet(char *pMsg)
{
    char buf[64];
    PASTAGVAL tag;

    tag = D_004DAD18.tag;
    eMessageCpy(buf, (char *)&tag);
    eMessageCat(pMsg);
    return xglFontGetStringWidth(buf);
}

/* Remap the three left-hand weapon ids onto their dual-wield right-hand pair */
int MenuRWeaponCheck(int nId, int nType)
{
    if (nType < 36) {
        if (nType >= 32) {
            switch (nId) {
            case 0x5B:
                nId = 0x70;
                break;
            case 0x5C:
                nId = 0x71;
                break;
            case 0x5E:
                nId = 0x72;
                break;
            }
        }
    }
    return nId;
}

/* Apply a draw type to a model unit's main actor and all three weapon slots */
void MenuModelUnitOpen(char *pUnit, int nType)
{
    int i;

    MenuModelDrawTypeSet(*(void **)(pUnit + 0x20), nType);
    for (i = 0; i < 3; i++) {
        MenuModelWeaponOpen(pUnit, nType, i);
    }
}

/* Search a character's three skill slots for the given skill, returning slot+1.
   K&R definition on purpose (same lever as MenuTecEquipCheck): retail callers
   pass the char byte and skill word raw; the callee narrows. */
int MenuSkillEquipCheck(nChr, nId)
short nChr;
short nId;
{
    short *p;
    short v;
    int i;

    p = (short *)((char *)func_A191C0_2(nChr) + 0xA6);
    i = 0;
    while (i < 3) {
        v = *p;
        p++;
        if (v == nId) {
            return i + 1;
        }
        i++;
    }
    return 0;
}

extern int save_map_text __asm__("save_map_text");
extern int map_ex_text __asm__("map_ex_text");
extern char f_name_1[] __asm__("f_name.1");
extern char f_name_0[] __asm__("f_name.0_0036D6C8");

/* Load the save-map-name text blob into the aligned scratch area */
char *MenuSaveMapTextLoad(int nMemory, int nFlag)
{
    char *pText;
    int aligned;

    aligned = (nMemory + 15) & ~15;
    pText = (char *)(aligned + 0x1000);
    save_map_text = aligned;
    if (nFlag == 0) {
        xglCdReadFile(f_name_1, (void *)aligned, 0, (void *)1);
    } else {
        MenuLoadFile(f_name_1, (void *)aligned);
    }
    return pText;
}

/* Load the map extra-text blob into the aligned scratch area */
char *MenuMapExTextLoad(int nMemory, int nFlag)
{
    char *pText;
    int aligned;

    aligned = (nMemory + 15) & ~15;
    pText = (char *)(aligned + 0x2000);
    map_ex_text = aligned;
    if (nFlag == 0) {
        xglCdReadFile(f_name_0, (void *)aligned, 0, (void *)1);
    } else {
        MenuLoadFile(f_name_0, (void *)aligned);
    }
    return pText;
}

/* --- Menu work block (current tec character/selection bytes) --- */
typedef struct {
    char pad0[3];
    unsigned char state;       /* 0x03: menu-system state */
    char pad04[0x11 - 4];
    unsigned char bEnd;        /* 0x11: 0xFF once the end-print is done */
    char pad12[0x22 - 0x12];
    signed char bHdd;          /* 0x22: HDD present flag */
    char pad23[0x26 - 0x23];
    short h26;                 /* 0x26: current AGWS weapon id */
    char pad28[0x30 - 0x28];
    unsigned char b30;         /* 0x30 */
    signed char b31;           /* 0x31: read back with lb */
    signed char b32;           /* 0x32: read back with lb */
    unsigned char b33;         /* 0x33: radar display option */
    char pad34;
    unsigned char b35;         /* 0x35: HDD activate result */
    char pad36[0x40 - 0x36];
    signed char bChr;          /* 0x40 */
    char pad41[2];
    signed char b43;           /* 0x43 */
    signed char bSel;          /* 0x44 */
    char pad45;
    signed char b46;           /* 0x46: current ether character */
    char pad47[0x54 - 0x47];
    signed char b54;           /* 0x54 */
    signed char b55;           /* 0x55 */
    signed char b56;           /* 0x56 */
    char pad57[0x60 - 0x57];
    short h60;                 /* 0x60 */
    char pad62[2];
    short h64;                 /* 0x64 */
    char pad66[0x7A - 0x66];
    signed char b7A;           /* 0x7A: equip-calc stat type */
} MENUTECWORK;
extern MENUTECWORK MenuWork;
extern int MenuTecNextTLevPointGet(int nId, int nLev);

/* Ask whether the current character can raise the selected technique's level */
int MenuTecCharTLevUpCheck(void)
{
    MENUTECWORK *w;
    PARAOBJ *p;
    int nextPoint;

    w = &MenuWork;
    if (MenuTecTLevLimitCheck(w->bChr, w->bSel) != 0) {
        p = func_A19210(w->bChr);
        nextPoint = MenuTecNextTLevPointGet(w->bChr, w->bSel);
        if (p->fC >= nextPoint) {
            return 1;
        }
    }
    return 0;
}

/* Ask whether the current character can raise the selected technique's speed */
int MenuTecCharSpeedUpCheck(void)
{
    MENUTECWORK *w;
    PARAOBJ *p;
    int nextPoint;

    w = &MenuWork;
    if (MenuTecSpeedLimitCheck(w->bChr, w->bSel) != 0) {
        p = func_A19210(w->bChr);
        nextPoint = MenuTecNextSpeedPointGet(w->bChr, w->bSel);
        if (p->fC >= nextPoint) {
            return 1;
        }
    }
    return 0;
}

/* Ask whether the current character can raise the selected technique's wait */
int MenuTecCharWaitUpCheck(void)
{
    MENUTECWORK *w;
    PARAOBJ *p;
    int nextPoint;

    w = &MenuWork;
    if (MenuTecWaitLimitCheck(w->bChr, w->bSel) != 0) {
        p = func_A19210(w->bChr);
        nextPoint = MenuTecNextWaitPointGet(w->bChr, w->bSel);
        if (p->fC >= nextPoint) {
            return 1;
        }
    }
    return 0;
}

/* Fetch the point cost of a technique's next level-up step */
int MenuTecNextTLevPointGet(int nId, int nLev)
{
    char *pData;
    unsigned char *pSave;

    pData = (char *)MenuTecDataGet(nId);
    pSave = (unsigned char *)MenuTecSaveDataGet(nId);
    if (nLev < 0) {
        return 0;
    }
    return *(unsigned short *)((nLev << 1) + (int)pData + 0x10)
        + *(unsigned char *)(pData + nLev + 8) * (pSave[nLev] + 1);
}

/* --- Model draw-type callbacks --- */
typedef struct {
    char pad0[0x60];
    float alpha;               /* 0x60 */
    void (*func)(void *);      /* 0x64 */
} DRAWTYPE;
extern void ModelDrawTypeOpen01(void *);
extern void ModelDrawTypeClose01(void *);
extern void ModelDrawTypeClose02(void *);

/* Select a model's draw-type: reset/zero its alpha or install a fade callback */
void MenuModelDrawTypeSet(void *pModel, int nType)
{
    DRAWTYPE *q;

    if (pModel == 0) {
        return;
    }
    q = (DRAWTYPE *)((char *)pModel + 0xC0);
    switch (nType) {
    case 0:
        q->alpha = 1.0f;
        break;
    case 1:
        *(int *)&q->alpha = 0;
        break;
    case 2:
        q->func = ModelDrawTypeOpen01;
        break;
    case 3:
        q->func = ModelDrawTypeClose01;
        break;
    case 4:
        q->func = ModelDrawTypeClose02;
        break;
    }
}

/* --- Background camera keep/restore plumbing --- */
typedef struct {
    long long d[0xBE];         /* 0x5F0 bytes */
} MENUBGCAMERA;
typedef struct {
    char pad0[0x828];
    int f828;
} MENUBGPARAM;
extern MENUBGPARAM *MenuBgParam;
extern int MenuBgKeepCamera;
extern MENUBGCAMERA *MenuBgCameraId;
extern MENUBGCAMERA *xglStudioGetCamera2(int);

/* Mark the background param dead, run one last tick and restore the kept camera */
void MenuBgTaskBreak(void)
{
    MENUBGCAMERA *pCam;

    MenuBgParam->f828 = -1;
    MenuBgTaskMain();
    pCam = xglStudioGetCamera2(MenuBgKeepCamera);
    *pCam = *MenuBgCameraId;
}

/* Remap a weapon mount index through the fixed mount-id table */
int MenuWpnMountIdChange(int nArg, int nIdx)
{
    int tbl[8] = { 0x20, 0x18, 0x22, 0x1A, 0x23, 0x1B, 0x21, 0x19 };

    if (nIdx < 2) {
        return tbl[nIdx + 5];
    }
    return tbl[nIdx - 1];
}

/* Decode which character family an ether id belongs to */
int MenuEtherWhoCheck(int nId)
{
    unsigned int id;

    id = nId & 0xFFFF;
    if (id - 1 < 16) {
        return 3;
    }
    if (id - 17 < 10) {
        return 2;
    }
    if (id - 27 < 10) {
        return 1;
    }
    if (id - 37 < 12) {
        return 6;
    }
    if (id - 49 < 10) {
        return 7;
    }
    if (id - 59 < 13) {
        return 4;
    }
    if (id - 72 < 8) {
        return 5;
    }
    return 0;
}

extern void *func_A1A3D8(int);
extern void *func_A1A428(int);
extern int func_A197E8(int, int);

/* Ask whether a bullet id fits the given bullet-using weapon */
int MenuBulletCheck(int nId, int nBlt)
{
    void *p;
    void *q;

    if (nId == 0) {
        return 0;
    }
    p = func_A1A3D8(nId);
    if ((*(unsigned short *)((char *)p + 6) & 0x100) == 0) {
        return 0;
    }
    if (nBlt < 0) {
        return 1;
    }
    if (nBlt == 0) {
        return 0;
    }
    q = func_A1A428(nBlt);
    if (*(short *)((char *)q + 12) == nId) {
        return nBlt;
    }
    return 0;
}

/* Bullet check with menu-mode dispatch (shop/status variants) */
int MenuBulletCheck2(int nId, int nType)
{
    switch (nType) {
    case 1:
    case 6:
        return 0;
    case 3:
        return func_A197E8(3, 62) != 0;
    default:
        return MenuBulletCheck(nId, -1);
    }
}

extern void *MenuTextGet(int);

/* Swap two sort keys if the first name-string sorts higher (descending) */
void MenuSortSubType04(int *a, int *b)
{
    unsigned char *p1;
    unsigned char *p2;
    unsigned char c1, c2;
    int t1, t2;

    p1 = *(unsigned char **)((char *)MenuTextGet(*a) + 8);
    p2 = *(unsigned char **)((char *)MenuTextGet(*b) + 8);
    do {
        c2 = *p2;
        p2++;
        c1 = *p1;
        p1++;
        if (c2 < c1) {
            t1 = *a;
            t2 = *b;
            *a = t2;
            *b = t1;
            return;
        }
        if (c1 != c2) {
            return;
        }
    } while (c1 != 0);
}

/* Swap two sort keys if the first name-string sorts lower (ascending) */
void MenuSortSubType05(int *a, int *b)
{
    unsigned char *p1;
    unsigned char *p2;
    unsigned char c1, c2;
    int t1, t2;

    p1 = *(unsigned char **)((char *)MenuTextGet(*a) + 8);
    p2 = *(unsigned char **)((char *)MenuTextGet(*b) + 8);
    do {
        c2 = *p2;
        p2++;
        c1 = *p1;
        p1++;
        if (c1 < c2) {
            t1 = *a;
            t2 = *b;
            *a = t2;
            *b = t1;
            return;
        }
        if (c1 != c2) {
            return;
        }
    } while (c1 != 0);
}

extern int wpnInfoGet(int);

/* Ask whether a weapon can be equipped by the character (type and slot masks) */
int MenuWeaponEquipCheck(int nChr, int nWpn)
{
    int nEquip;
    int nInfo;
    char *p;

    if (nChr == 0) {
        return 0;
    }
    nEquip = (int)chrEquipGet(nChr);
    nInfo = wpnInfoGet(nChr);
    if (nWpn == 0) {
        return 0;
    }
    p = (char *)func_A1A3D8(nWpn);
    if ((*(short *)(p + 12) & nInfo) == 0) {
        return 0;
    }
    return (*(unsigned short *)(p + 8) & nEquip) != 0;
}

/* TODO: near-miss (LENGTH, 35 orig vs 37-39 built) - the free-slot search loop
   in the original recomputes (i << 1) + pChr + 0xA6 per iteration (no strength
   reduction), while every source variant tried (pointer walk, indexed, ofs
   temp, search-then-store while) gets its address giv reduced to a walking
   pointer. Same unreduced-giv shape as the MenuModelMemoryGet original, but
   there the constant symbol base made the unreduced form reachable; with the
   base in a register it appears unreachable from C. Parked. */
/* Equip a skill into the given slot, or the first free slot when nSlot < 0 */
void MenuSkillEquip(short nChr, short nId, int nSlot)
{
    register char *pChr __asm__("$6");
    short *q;
    short v;
    int i;

    pChr = (char *)func_A191C0_2(nChr);
    if (nSlot >= 0) {
        *(short *)((nSlot << 1) + (int)pChr + 0xA6) = nId;
        return;
    }
    q = (short *)(pChr + 0xA6);
    i = 0;
    goto test;
loop:
    i++;
    __asm__("" : "+r"(i));
    if (i >= 3) {
        return;
    }
    q = (short *)((i << 1) + (int)pChr + 0xA6);
test:
    v = *q;
    __asm__ volatile("nop");
    if (v != 0) {
        goto loop;
    }
    *q = nId;
}

/* Decode an ether id's use-type (1 heal, 2 cure, 3 revive) */
int MenuEtherUseCheck(int nId)
{
    int id;

    id = nId & 0xFFFF;
    switch (id) {
    case 1:
    case 8:
    case 37:
        return 1;
    case 4:
    case 28:
        return 2;
    case 74:
        return 3;
    }
    return 0;
}

/* Queue the menu-motion resource matching the current model memory state */
void MenuModelMenuMotionLoad(void)
{
    char *pData;
    unsigned short nId;

    pData = (char *)D_0036D5C8;
    nId = 0;
    if ((MenuModelFlag & 2) != 0) {
        return;
    }
    switch (((MODELMEM *)MenuModelMemoryState)->f0) {
    case 10:
        nId = 0x8001;
        break;
    case 11:
        nId = 0xA100;
        break;
    case 12:
        nId = 0x8001;
        break;
    case 20:
        nId = 0xA100;
        break;
    }
    if (nId == 0) {
        return;
    }
    MenuModelResourceRequest(pData, nId, 0);
}

/* --- Tec list construction --- */
typedef struct {
    int f0;                    /* 0x00 */
    int f4;                    /* 0x04: equip count */
    unsigned char b8;          /* 0x08 */
    unsigned char b9;          /* 0x09 */
    char padA[2];
} MENUTECLISTENT;
extern MWLIST *MenuListMake(int nIdx, int nType);

/* Build the technique list: per entry, count how often it is equipped.
   (The old $s0<->$s1 allocator swap is fixed by the register-pin idiom on
   the list walker; see the wave-4 notes in this file's history.) */
void MenuTecListMake00(void)
{
    int n;
    register MENUTECLISTENT *p __asm__("$16");
    int *pSort;
    MENUTECWORK *w;
    short nTec;
    int v;

    p = (MENUTECLISTENT *)MenuListGet(0);
    pSort = MenuSortAddrGet(0);
    n = MenuSortCheck(0);
    MenuListMake(0, 0);
    if (n > 0) {
        w = &MenuWork;
        do {
            nTec = *(short *)pSort;
            pSort++;
            v = MenuTecEquipCheck(w->bChr, nTec);
            n--;
            p->b8 = 0;
            p->f4 = v;
            p++;
        } while (n != 0);
    }
}

/* TODO: near-miss (LOGIC, 5 diffs) - cases 4/5 in the original load lbu and
   sign-extend via sll/sra 24 with a cross-jumped andi tail; every C form
   tried (signed-char cast, u8/int temps, explicit shifts) is combined by
   gcc into a single lb. Parked. */
/* Fetch a character's current value for one stat type */
int MenuParaPtNowGet(int nId, int nType)
{
    char *p;
    int t;

    p = (char *)func_A191C0_2(nId);
    if ((unsigned int)nType < 8) {
        switch (nType) {
        case 0:
            return *(unsigned short *)(p + 4);
        case 1:
            return *(unsigned short *)(p + 6);
        case 2:
            return *(unsigned short *)(p + 8);
        case 3:
            return *(unsigned short *)(p + 10);
        case 4:
            t = *(unsigned char *)(p + 12);
            goto sign;
        case 5:
            t = *(unsigned char *)(p + 13);
        sign:
            return (unsigned short)(signed char)t;
        case 6:
            return *(unsigned short *)(p + 0);
        case 7:
            return *(unsigned short *)(p + 2);
        }
    }
    return 0;
}

/* Matched: the inner loop is goto-shaped (single top test, backward branch)
   with cont=i<12 computed before the i++, an explicit alignment nop on the
   fall-in path, and -- the real key -- i resets PER CHARACTER: the retail
   `move s2,zero` sits in the outer jal's delay slot, so the old shared-i
   draft was semantically wrong as well as unmatched. */
/* Recompute every character's total set-ether capacity byte */
void MenuEtherCapSet(void)
{
    unsigned char *p;
    short *q;
    char *e;
    int i;
    int nChr;
    int cont;
    short v;

    for (nChr = 1; nChr < 8; nChr++) {
        i = 0;
        p = (unsigned char *)func_A191C0_2(nChr);
        p[23] = 0;
        q = (short *)(p + 142);
        __asm__ volatile("nop");
    inner:
        cont = i < 12;
        i++;
        if (cont == 0) {
            goto next;
        }
        v = *q;
        q++;
        if (v == 0) {
            goto next;
        }
        e = (char *)func_A1A488(v);
        p[23] = p[23] + *(unsigned char *)(e + 4);
        goto inner;
    next:;
    }
}

/* --- Game-state push (actor snapshot) --- */
typedef struct {
    long long d[0x14E];        /* 0xA70 bytes */
} MENUGAMEDATA;
typedef struct {
    char pad0[0x10];
    long long f10;             /* 0x10 */
    char pad18[0x250 - 0x18];
    MENUGAMEDATA aKeep[64];    /* 0x250 */
} GAMELOOPSTATE;
extern GAMELOOPSTATE GameLoopState;
extern MENUGAMEDATA actor[];
extern void GameStateSave(void);

/* Snapshot all 64 actor slots into the game-loop keep area, then save state */
void MenuGameDataPush(void)
{
    short i;

    for (i = 0; i < 64; i++) {
        GameLoopState.aKeep[i] = actor[i];
    }
    GameStateSave();
}

/* Mark a technique's wait-up slot used: bump its level and deduct the cost.
   The q/v/t register pins (dpmul idiom) resolve the old 14-rename cycle;
   the `+r` asm on t forces the fC subtraction through $a1. */
void MenuTecWaitUp(int nId, int nWait)
{
    unsigned char *pSave;
    PARAOBJ *p;
    int nMask;
    int nOfs;
    int nextPoint;
    register unsigned char *q __asm__("$6");
    register int v __asm__("$3");
    register int t __asm__("$5");

    nMask = nId & 0xFFFF;
    pSave = (unsigned char *)MenuTecSaveDataGet(nId);
    p = func_A19210(nMask);
    if (nWait < 0) {
        return;
    }
    nextPoint = MenuTecNextWaitPointGet(nId, nWait);
    nOfs = nWait + 16;
    q = pSave + nOfs;
    v = *q + 255;
    *q = v;
    *(unsigned short *)(nWait * 12 + (int)p + 78) = v & 0xFF;
    t = p->fC - nextPoint;
    __asm__ volatile("" : "+r"(t));
    p->fC = t;
}

extern int dataItmBoxDec(int);
extern int dataEvtBoxDec(int);
extern int dataWpnBoxDec(int);
extern int dataBltBoxDec(int);
extern int dataAccBoxDec(int);
extern int dataItmBoxInc(int);
extern int dataEvtBoxInc(int);
extern int dataWpnBoxInc(int);
extern int dataBltBoxInc(int);
extern int dataAccBoxInc(int);
extern void PartyTakeAgwsOn(void);

/* Decrement a box entry's count, dispatching on the type in the high word */
int MenuBoxDec(int nId)
{
    unsigned int nType;
    int nRes;

    nType = (unsigned int)nId >> 16;
    nId = nId & 0xFFFF;
    nRes = 0;
    switch (nType - 1) {
    case 0:
        dataItmBoxDec(nId);
        break;
    case 1:
        dataEvtBoxDec(nId);
        break;
    case 2:
        dataWpnBoxDec(nId);
        break;
    case 3:
        dataBltBoxDec(nId);
        break;
    case 4:
        dataAccBoxDec(nId);
        break;
    case 5:
        nRes = -1;
        break;
    }
    return nRes;
}

/* Increment a box entry's count, dispatching on the type in the high word */
int MenuBoxInc(int nId)
{
    unsigned int nType;
    int nRes;

    nType = (unsigned int)nId >> 16;
    nId = nId & 0xFFFF;
    nRes = 0;
    switch (nType - 1) {
    case 0:
        dataItmBoxInc(nId);
        break;
    case 1:
        dataEvtBoxInc(nId);
        break;
    case 2:
        dataWpnBoxInc(nId);
        break;
    case 3:
        dataBltBoxInc(nId);
        break;
    case 4:
        dataAccBoxInc(nId);
        break;
    case 5:
        nRes = -1;
        break;
    case 6:
        PartyTakeAgwsOn();
        break;
    }
    return nRes;
}

extern void ChangeTopLevel(int);

/* Matched: `two` (=2) survives into the case-0 sb (repurposed compare temp
   in $s1), the goto-shared `top:` label hand-merges the two ChangeTopLevel
   jals (gcc would not cross-jump them), and w2 pinned $4 keeps the &MenuWork
   remat addressed off the kept $s2 lui half. */
/* Drive the sub-menu system state machine (init, main tick, teardown handoff) */
int MenuSystem(void)
{
    MENUTECWORK *w;
    register MENUTECWORK *w2 __asm__("$4");
    int st;
    int two;

    w = &MenuWork;
    two = 2;
    switch (w->state) {
    case 0:
        subMenuSystemInit(MainMenuWorkEnd);
        w->state = two;
    case 2:
        subMenuSystemMain();
        w2 = &MenuWork;
        if (w2->bEnd == 0xFF) {
            w2->state = 4;
            goto top;
        }
        break;
    case 4:
    top:
        ChangeTopLevel(0);
        break;
    }
}

/* --- Save-data / pad work blocks --- */
typedef struct {
    char pad0[0x28];
    unsigned char b28;         /* 0x28 */
    char pad29[0x3C - 0x29];
    unsigned char b3C;         /* 0x3C */
    char pad3D[0x40 - 0x3D];
    unsigned char b40;         /* 0x40 */
    char pad41[0x44 - 0x41];
    unsigned char b44;         /* 0x44 */
} MENUSAVEWORK;
typedef struct {
    char pad0[0x32];
    unsigned short h32;        /* 0x32: repeat/level mirror of trig */
    unsigned short trig;       /* 0x34 */
    char pad36[0x56 - 0x36];
    unsigned char b56;         /* 0x56 */
    char pad57[0xBE - 0x57];
    unsigned char bBE;         /* 0xBE */
} MENUPADWORK;
extern MENUSAVEWORK SaveData;
extern MENUPADWORK PadData;
extern void SsdSetOutputMode(int);
extern void PartyRadarDispSet(int);
extern int PartyDataGet(void);
extern void tyaDisplaySetting(int);
extern void xglHddActivate(int);

/* TODO: near-miss (LOGIC/order) - the eight status-byte stores interleave
   across three base registers in an order the scheduler will not reproduce
   from any source order tried. Parked. */
/* Reset the menu-system status bytes and mirror the save/pad display options */
void MenuSystemInitSet(void)
{
    MENUTECWORK *w;
    MENUSAVEWORK *sv;
    volatile MENUPADWORK *pd;
    unsigned char *pData;

    SsdSetOutputMode(1);
    w = &MenuWork;
    sv = &SaveData;
    pd = &PadData;
    pd->bBE = 0;
    sv->b28 = 1;
    w->b30 = 0;
    sv->b44 = 0;
    w->b31 = 0;
    sv->b3C = 0;
    pd->b56 = 0;
    w->b32 = 0;
    PartyRadarDispSet(1);
    pData = (unsigned char *)PartyDataGet();
    w->b33 = pData[46] ^ 1;
    tyaDisplaySetting(1);
    if (w->bHdd != 0) {
        sv->b40 = 0;
        xglHddActivate(1);
        w->b35 = sv->b40;
    }
}

extern void *func_A1A4E8(int);
extern void *func_A1A548(int);

/* Ask whether a shop entry is flagged no-sale, dispatching on the type word */
int MenuShopNoSaleCheck(int nId)
{
    int id;
    int nType;
    unsigned short flags;
    int t;

    id = nId & 0xFFFF;
    nType = (unsigned int)nId >> 16;
    flags = 0;
    if (id == 0) {
        return 0;
    }
    switch (nType) {
    case 1:
        flags = *(unsigned short *)((char *)func_A1A4E8(id) + 4);
        break;
    case 3:
        flags = *(unsigned short *)((char *)func_A1A3D8(id) + 6);
        break;
    case 4:
        flags = *(unsigned short *)((char *)func_A1A428(id) + 10);
        break;
    case 5:
        flags = *(unsigned short *)((char *)func_A1A548(id) + 4);
        break;
    }
    t = flags & 0x1000;
    return t != 0;
}

/* TODO: near-miss (REGISTER 8, was LOGIC 28) - the ternary forms reproduce
   the original's movz/movn shapes; what remains is an a1/a2 + v1/a1 rename
   pair (the `v` flags word lands in $v1 instead of $v0, cascading into the
   ternary temps). Pins on nRet ($5) change length, pins on v ($2) are
   ignored. Parked. */
/* Classify how an item can be used from the menu (0 no, 1/2 use-type, 10 event) */
int MenuItemUseCheck(short nId)
{
    char *p;
    int nRet;
    unsigned short v;

    if (nId == 0) {
        return 0;
    }
    p = (char *)func_A1A4E8(nId);
    if ((*(unsigned short *)(p + 4) & 0x8000) == 0) {
        return 0;
    }
    if (nId == 36) {
        nRet = (GameLoopState.f10 & 0x20400000) ? 2 : -1;
    } else {
        v = *(unsigned short *)(p + 12);
        if ((v & 0x1E43) != 0) {
            nRet = (*(unsigned char *)(p + 8) & 0x10) ? 2 : 1;
        } else {
            nRet = 0;
            if ((v & 0x2000) != 0) {
                nRet = 10;
            }
        }
    }
    return nRet;
}

extern void xglSoundEffectNormalID(int, int);

/* TODO: unfixable from C - the original's no-move exit branches INTO the
   jal delay slot (the second clamp movn), executing it on the skip path;
   that is gcc's delay-slot filler retargeting a branch to a stolen
   instruction. Needs the fixer flags / configure.py, outside this scope. */
/* Move a selection index by the 2D pad direction, wrapping at the list bounds */
int MenuSelectMove2(int nSel, int nMax)
{
    int nMove;

    nMove = 0;
    if (nMax < 2) {
        return nSel;
    }
    switch (PadData.trig) {
    case 0x1000:
        nMove = -2;
        break;
    case 0x4000:
        nMove = 2;
        break;
    case 0x2000:
        nMove = 1;
        break;
    case 0x8000:
        nMove = -1;
        break;
    }
    if (nMove != 0) {
        nSel = nSel + nMove;
        nSel = (nSel < 0) ? nMax - 1 : nSel;
        nSel = (nMax - 1 < nSel) ? 0 : nSel;
        xglSoundEffectNormalID(3, 0);
    }
    return nSel;
}

extern int PartyFriendCheck(int);

/* TODO: near-miss (LENGTH, 48 orig vs 43 built) - the tail nType dispatch:
   the retail build keeps out-of-line case blocks (beqz/beq to distant
   bodies, default return in the middle) while ours threads/if-converts the
   chain (xori+inline case 0) whichever if/else-if/return shape is used; the
   party-max loop itself (movn clamp) matches. Parked. */
/* Find the highest current value of one stat across the party, rounded per type */
int MenuParaUpMaxGet(int nType)
{
    int i;
    unsigned int nMax;
    unsigned int v;

    nMax = 0;
    for (i = 1; i < 8; i++) {
        if (PartyFriendCheck(i) != 0) {
            v = MenuParaPtNowGet(i, nType);
            if (nMax < v) {
                nMax = v;
            }
        }
    }
    if (nType == 0) {
        nMax = (unsigned short)((unsigned short)(nMax / 10) * 10);
    } else if (nType == 1) {
        nMax = nMax & 0xFFFE;
    }
    return nMax;
}

typedef struct {
    short h0;                  /* 0x00 */
    short h2;                  /* 0x02 */
    char pad4[0x34 - 4];
    short h34;                 /* 0x34 */
    short h36;                 /* 0x36 */
} MENUEPOBJ;
extern MENUEPOBJ *func_A11108(int, int *, int *);
extern int PartyAllPartyGet(int *, int);

/* TODO: near-miss (LENGTH) - the party loop in the original is not
   exit-test-duplicated (same class as MenuEtherCapSet). Parked. */
/* Ask whether a character's (or the whole party's) EP is already full */
int MenuCharEpCheck(int nChr)
{
    int buf1[4];
    int buf2[4];
    int list[16];
    MENUEPOBJ *p;
    int *q;
    int n;
    int i;

    if (nChr == 0) {
        return 0;
    }
    if (nChr < 0) {
        n = PartyAllPartyGet(list, 0);
        i = 0;
        q = list;
loop:
        if (i < n) {
            p = func_A11108(*q, buf1, buf2);
            q++;
            if (p->h36 == p->h2) {
                i++;
                goto loop;
            }
        }
        if (i != n) {
            goto ret0;
        }
        goto ret1;
    }
    p = func_A11108(nChr, buf1, buf2);
    if (p->h36 == p->h2) {
        goto ret1;
    }
ret0:
    return 0;
ret1:
    return 1;
}

/* ================= Wave 3: mid-size Menu functions ================= */

/* Sum the WAGL (weapon agility) halfwords of an AGWS's three weapon slots */
int MenuAgwsWaglGet(int nId)
{
    short *p;
    int nSum;
    int i;
    int v;

    nSum = 0;
    if (nId == 0) {
        return 0;
    }
    p = (short *)((char *)func_A191C0_2(nId) + 94);
    for (i = 2; i >= 0; i--) {
        v = *p;
        p++;
        if (v != 0) {
            nSum += *(short *)((char *)func_A1A3D8(v) + 18);
        }
    }
    return nSum;
}

/* --- List construction driver --- */
extern void subListMake00(void *, int);
extern void subListMake01(void *, int);
extern char msg_12[] __asm__("msg.12");

/* Build one menu work list from its sort chain, then append the terminator entry */
MWLIST *MenuListMake(int nIdx, int nType)
{
    MENUTECLISTENT *p;
    MWLIST *pList;
    int *pSort;
    void (*pFunc)(void *, int);

    pList = &mw_list[nIdx];
    p = (MENUTECLISTENT *)pList;
    pSort = sort[nIdx];
    if (nType == -10) {
        pFunc = subListMake01;
    } else {
        pFunc = subListMake00;
    }
    while (*pSort != 0) {
        pFunc(p, *pSort);
        p++;
        pSort++;
    }
    p->f4 = -1;
    p->f0 = (int)msg_12;
    p->b8 = 0;
    p[1].f0 = 0;
    return pList;
}

/* --- Scenario-number flag table --- */
typedef struct {
    int nNo;                   /* 0x00: scenario number at the top flag */
    int nHi;                   /* 0x04: first (highest) flag to test */
    int nLo;                   /* 0x08: last (lowest) flag to test */
} SCENREC;
typedef struct {
    SCENREC rec[5];
} SCENTBL;
extern SCENTBL D_004C5140;

/* Scan the scenario flag ranges from high to low and return the matching number */
int MenuScenarioNoGet(void)
{
    SCENTBL tbl;
    int i;
    int nFlag;
    int nNo;

    tbl = D_004C5140;
    for (i = 0; i < 5; i++) {
        nFlag = tbl.rec[i].nHi;
        nNo = tbl.rec[i].nNo;
        while (nFlag >= tbl.rec[i].nLo) {
            if (xglFlagsGet(nFlag, 1) != 0) {
                return nNo;
            }
            nFlag--;
            nNo--;
        }
    }
    return nNo;
}

/* --- Sort compare-function table (function-local static in the original TU) --- */
extern void (*f_11[6])(int *, int *) __asm__("f.11");

/* Bubble-sort one sort chain in place with the type's compare/swap callback */
void MenuSortChange(int nRow, int nType)
{
    int *base;
    void (*pFunc)(int *, int *);
    int count;
    int last;
    int i;
    int j;
    int *p;
    int v;

    base = sort[nRow];
    pFunc = f_11[nType];
    count = 0;
    if (*base != 0) {
        p = base + 1;
        do {
            v = *p;
            p++;
            count++;
        } while (v != 0);
    }
    last = count - 1;
    if (count != 0) {
        if (last > 0) {
            for (i = 0; i < last; i++) {
                for (j = i + 1; j < count; j++) {
                    pFunc(&base[i], &base[j]);
                }
            }
        }
    }
}

/* Compute the point cost of a stat's next advance from base, rate and current value */
int MenuParaNextPointGet(int nBase, int nType, int nNum)
{
    int base;
    int rate;
    int now;
    unsigned short total;

    base = MenuParaPtBaseGet(nBase, nType);
    rate = MenuParaPtRateGet(nBase, nType);
    now = MenuParaPtNowGet(nBase, nType);
    if (nType != 0) {
        if (nType != 1) {
            total = now + nNum;
        } else {
            total = now + nNum * 2;
        }
    } else {
        total = now + nNum * 10;
    }
    if (nType != 0) {
        return base + rate * total / 99;
    }
    return base + rate * (unsigned short)(total / 10) / 99;
}

/* Mark a technique's level-up slot used: bump its level, refresh the derived
   attack halfword and deduct the point cost.
   TODO: near-miss (6 diffs, was 13) - q/pPt/v pins + a pinned mm=$3 temp fix
   most of the cycle; m still coalesces into $a0 instead of $a3 (its pin is
   ignored) and the final sh/subu order flips. Barrier variants regress. */
void MenuTecTLevUp(int nId, int nLev)
{
    int idx;
    unsigned char *pSave;
    PARAOBJ *p;
    int nextPoint;
    register unsigned char *q __asm__("$6");
    register int *pPt __asm__("$8");
    unsigned short *pSh;
    register int v __asm__("$3");
    register int m __asm__("$7");
    register int t2 __asm__("$4");
    register int mm __asm__("$3");

    idx = nId & 0xFFFF;
    pSave = (unsigned char *)MenuTecSaveDataGet(idx);
    p = func_A19210(idx);
    if (nLev < 0) {
        return;
    }
    nextPoint = MenuTecNextTLevPointGet(nId, nLev);
    pPt = &p->fC;
    pSh = (unsigned short *)(nLev * 12 + (int)pPt + 64);
    q = pSave + nLev;
    v = *q + 1;
    *q = v;
    m = v & 0xFF;
    __asm__("" : "+r"(m));
    mm = m * 3 - 3;
    *pSh = mm;
    t2 = *pPt - nextPoint;
    __asm__ volatile("" : "+r"(t2));
    *pPt = t2;
}

/* Ask whether a character's (or the whole party's) HP is already full */
int MenuCharHpCheck(int nChr)
{
    int buf1[4];
    int buf2[4];
    int list[16];
    MENUEPOBJ *p;
    int *q;
    int n;
    int i;

    if (nChr == 0) {
        return 0;
    }
    if (nChr < 0) {
        n = PartyAllPartyGet(list, 0);
        i = 0;
        q = list;
loop:
        if (i < n) {
            p = func_A11108(*q, buf1, buf2);
            q++;
            if (p->h34 == p->h0) {
                i++;
                goto loop;
            }
        }
        if (i != n) {
            goto ret0;
        }
        goto ret1;
    }
    func_A191C0_2(nChr);
    p = func_A11108(nChr, buf1, buf2);
    if (p->h34 == p->h0) {
        goto ret1;
    }
ret0:
    return 0;
ret1:
    return 1;
}

/* Move a selection index up/down from the pad, wrapping at the list bounds */
int MenuSelectMove(int nSel, int nMax, int nFlag)
{
    MENUPADWORK *pad;
    int nMove;

    nMove = 0;
    if (nMax < 2) {
        return nSel;
    }
    pad = &PadData;
    if (pad->trig == 0x1000) {
        if (nSel != 0) {
            goto up;
        }
        if (nFlag != 0) {
            goto up;
        }
        if (pad->h32 != pad->trig) {
            goto second;
        }
up:
        nMove = -1;
    }
second:
    pad = &PadData;
    if (pad->trig == 0x4000) {
        if (nSel != nMax - 1) {
            goto down;
        }
        if (nFlag != 0) {
            goto down;
        }
        if (pad->h32 != pad->trig) {
            goto move;
        }
down:
        nMove = 1;
    }
move:
    if (nMove != 0) {
        nSel = nSel + nMove;
        if (nSel < 0) {
            nSel = nMax - 1;
        }
        if (nMax - 1 < nSel) {
            nSel = 0;
        }
        xglSoundEffectNormalID(3, 0);
    }
    return nSel;
}

/* --- Party leader/attacker mask --- */
extern int MenuScenarioNo;
extern int PartyAttackerCheck(int);
extern int PartyLeaderCheck(int);

/* Classify a character's leader mask for the current scenario window:
   1 not selectable, 2 not attacker, 3 leader, 0 plain member */
int MenuCharLeaderMask(int nId)
{
    int nChr;
    int v;
    int nRet;

    nChr = nId & 0xFFFF;
    v = MenuScenarioNo;
    if ((unsigned int)(v - 1) < 34) {
        if (nChr != 3) {
            return 1;
        }
    } else if ((unsigned int)(v - 46) < 3) {
        if (nChr != 3) {
            return 1;
        }
    } else if (v == 126) {
        if (nChr != 3) {
            return 1;
        }
    } else if ((unsigned int)(v - 142) < 3) {
        if (nChr != 3) {
            return 1;
        }
    } else if ((unsigned int)(v - 159) < 2) {
        if (nChr != 3 && nChr != 1 && nChr != 5) {
            return 1;
        }
    }
    if (((unsigned int)(nChr + 0xFFFF) & 0xFFFF) >= 7) {
        return 1;
    }
    if (PartyAttackerCheck(nChr) == 0) {
        return 2;
    }
    v = PartyLeaderCheck(nChr);
    nRet = 3;
    if (v == 0) {
        nRet = 0;
    }
    return nRet;
}

/* --- Tec sort-range table --- */
typedef struct {
    short lo;
    short hi;
} TECRANGE;
typedef struct {
    TECRANGE r[7];
} TECRANGETBL;
extern TECRANGETBL D_004D92E8;

/* TODO: near-miss (LENGTH, 66 orig vs 67 built) - the original materializes
   &MenuWork into caller-saved $a1 AFTER the table block copy and then does
   `move s2,a1` entering the loop (a live-range split); ours constant-props
   the address straight into $s2 and the scheduler hoists it above the copy.
   Tried single-pointer, w2=w split, and direct-global precheck forms; the
   split move appears unreachable while the address is a foldable constant. */
/* Rebuild sort chain 0 from the current character's technique id range */
void MenuTecSortSet00(void)
{
    int *pSort;
    TECRANGETBL tbl;
    MENUTECWORK *w;
    int nId;

    pSort = MenuSortAddrGet(0);
    tbl = D_004D92E8;
    w = &MenuWork;
    nId = tbl.r[w->bChr - 1].lo;
    if (tbl.r[w->bChr - 1].hi < nId) {
        *pSort = 0;
        return;
    }
    do {
        if (func_A197E8(w->bChr, nId) != 0) {
            *pSort = 0xA0000 + (nId & 0xFFFF);
            pSort++;
        }
        nId++;
    } while (tbl.r[w->bChr - 1].hi >= nId);
    *pSort = 0;
}

/* --- Tec list flag tables --- */
extern unsigned char D_0036DC58[];
extern unsigned char D_0036DC60[];

/* TODO: near-miss (LENGTH, 68 orig vs ~70 built) - the retail build keeps the
   precheck's &MenuWork in caller-saved $v0 and copies it into $s4 at loop
   entry (move s4,v0), with matching loop-entry copies move s2,s0 (sort
   walker) and move s3,a0 (counter, n itself living in $a0). Every form tried
   (single pointer, split w/w2, direct-global precheck, repurposed-ptr like
   MenuTecListMake01) either constant-props &MenuWork straight into the
   callee-saved register or hoists it above the guard. The repurposed-ptr
   lever fails here because phase 1 must NOT cross a call (in 01 it does).
   Also: the per-iteration table addiu pair (a1/a2 from lui halves s8/s7)
   gets fully hoisted in ours. Parked. */
/* Refresh the per-entry enable byte of tec list 0 from type/trigger checks */
void MenuTecListMake00_2(void)
{
    MENUTECLISTENT *p;
    int *pSort;
    int *pS;
    MENUTECWORK *w;
    unsigned char *q;
    int n;
    int cnt;
    int nFlag;
    int v;
    unsigned char t;

    p = (MENUTECLISTENT *)MenuListGet(0);
    pSort = MenuSortAddrGet(0);
    n = MenuSortCheck(0);
    nFlag = MenuWork.b31 < 2;
    if (n > 0) {
        w = &MenuWork;
        q = (unsigned char *)p + 8;
        pS = pSort;
        cnt = n;
        do {
            v = MenuTecTypeCheck(w->bChr, *pS);
            if (v != nFlag && v != 1) {
                *q = 1;
            } else {
                t = (w->bChr != 7 ? D_0036DC58 : D_0036DC60)[w->b31];
                if (t != MenuTecTrgCheck(*pS)) {
                    *q = 1;
                } else {
                    *q = 0;
                }
            }
            cnt--;
            q += 12;
            pS++;
        } while (cnt != 0);
    }
}

/* Build tec list 0's equip counts and up-check enable bytes.
   `ptr` deliberately starts as the MenuWork pointer and is repurposed as the
   list walker at loop entry: the retail build reuses one register ($s0) for
   both roles, with the loop's MenuWork pointer split off via a real copy. */
void MenuTecListMake01(void)
{
    void *ptr;
    MENUTECWORK *w2;
    MENUTECLISTENT *p0;
    int *pSort;
    int n;
    signed char nKeepSel;
    signed char nKeep43;
    unsigned char c;

    ptr = &MenuWork;
    p0 = (MENUTECLISTENT *)MenuListGet(0);
    pSort = MenuSortAddrGet(0);
    n = MenuSortCheck(0);
    nKeep43 = ((MENUTECWORK *)ptr)->b43;
    nKeepSel = ((MENUTECWORK *)ptr)->bSel;
    MenuListMake(0, 0);
    if (n > 0) {
        w2 = (MENUTECWORK *)ptr;
        ptr = p0;
        do {
            ((MENUTECLISTENT *)ptr)->f4 = MenuTecEquipCheck(w2->bChr, *(short *)pSort);
            c = *(unsigned char *)pSort;
            pSort++;
            w2->b43 = c;
            w2->bSel = BitToTecNo((signed char)c);
            if (MenuTecCharTLevUpCheck() != 0) {
                ((MENUTECLISTENT *)ptr)->b8 = 0;
            } else if (MenuTecCharWaitUpCheck() != 0) {
                ((MENUTECLISTENT *)ptr)->b8 = 0;
            } else if (MenuTecCharSpeedUpCheck() != 0) {
                ((MENUTECLISTENT *)ptr)->b8 = 0;
            } else {
                ((MENUTECLISTENT *)ptr)->b8 = 1;
            }
            n--;
            ptr = (MENUTECLISTENT *)ptr + 1;
        } while (n != 0);
    }
    MenuWork.b43 = nKeep43;
    MenuWork.bSel = nKeepSel;
}

/* --- Item list window work --- */
typedef struct {
    char pad0[0xD];
    unsigned char b0D;         /* 0x0D */
    char pad0E[0x18 - 0x0E];
    unsigned short h18;        /* 0x18 */
    unsigned short h1A;        /* 0x1A */
    void *p1C;                 /* 0x1C */
    unsigned char b20;         /* 0x20 */
    signed char b21;           /* 0x21: read back with lb */
    char pad22[0x28 - 0x22];
    void *p28;                 /* 0x28 */
} ITEMLISTWORK;
extern ITEMLISTWORK *MenuItemList;
extern char D_004C4F50[];
extern char D_0036C200[];
extern void MenuSortSet(int, int, int);
extern void WindowSPItemChange(void *);
extern void WindowSPSetSelect(void *, void *);

/* Build item list 0 (sorted): per entry, classify menu usability into the
   enable/type bytes, then refresh the item window strings and selection */
void MenuItemListMake00(void)
{
    ITEMLISTWORK *w;
    MENUTECLISTENT *p;
    int *pSort;
    int *pS;
    int n;
    int cnt;
    int v;

    p = (MENUTECLISTENT *)MenuListGet(0);
    pSort = MenuSortAddrGet(0);
    MenuSortSet(0, 1, 0);
    MenuSortChange(0, 0);
    MenuSortChange(0, MenuWork.b31);
    MenuListMake(0, 0);
    n = MenuSortCheck(0);
    if (n > 0) {
        pS = pSort;
        cnt = n;
        do {
            v = MenuItemUseCheck(*(short *)pS);
            pS++;
            if (v != 0) {
                if (v > 0) {
                    goto arm1;
                }
                if (v != -1) {
                    goto arm1;
                }
                p->b8 = 1;
                p->b9 = 2;
                goto next;
            }
            p->b8 = 1;
            p->b9 = 1;
            goto next;
arm1:
            p->b8 = 0;
            p->b9 = 0;
next:
            cnt--;
            p++;
        } while (cnt != 0);
    }
    w = MenuItemList;
    w->b0D = 7;
    w->p1C = D_004C4F50;
    MenuItemList->b20 = 1;
    MenuItemList->b21 = 11;
    MenuItemList->h18 = 224;
    MenuItemList->h1A = MenuItemList->b21 * 24 + 6;
    MenuItemList->p28 = MenuListGet(0);
    WindowSPItemChange((char *)MenuItemList + 12);
    WindowSPSetSelect((char *)MenuItemList + 12, D_0036C200);
}

/* Rebuild item list 0 without re-sorting, then refresh the item window */
void MenuItemListMake00_1(void)
{
    ITEMLISTWORK *w;
    MENUTECLISTENT *p;
    int *pSort;
    int *pS;
    int n;
    int cnt;
    int v;

    p = (MENUTECLISTENT *)MenuListGet(0);
    pSort = MenuSortAddrGet(0);
    MenuListMake(0, 0);
    n = MenuSortCheck(0);
    if (n > 0) {
        pS = pSort;
        cnt = n;
        do {
            v = MenuItemUseCheck(*(short *)pS);
            pS++;
            switch (v) {
            case -1:
                p->b8 = 1;
                p->b9 = 2;
                break;
            case 0:
                p->b8 = 1;
                p->b9 = 1;
                break;
            case 1:
            case 2:
                p->b8 = 0;
                p->b9 = 0;
                break;
            }
            cnt--;
            p++;
        } while (cnt != 0);
    }
    w = MenuItemList;
    w->b0D = 7;
    w->p1C = D_004C4F50;
    MenuItemList->b20 = 1;
    MenuItemList->b21 = 11;
    MenuItemList->h18 = 224;
    MenuItemList->h1A = MenuItemList->b21 * 24 + 6;
    MenuItemList->p28 = MenuListGet(0);
    WindowSPItemChange((char *)MenuItemList + 12);
    WindowSPSetSelect((char *)MenuItemList + 12, D_0036C200);
}

/* --- Ether list construction --- */
extern int func_A11220(int, int);

/* Build ether list 0: name/points per entry, enable by learnable/usable state */
void MenuEtherListMake00(void)
{
    MENUTECLISTENT *p;
    int *pSort;
    int nOn;
    MENUTECWORK *w2;
    MENUEPOBJ *rec;
    int n;
    int v;

    p = (MENUTECLISTENT *)MenuListGet(0);
    pSort = MenuSortAddrGet(0);
    n = MenuSortCheck(0);
    nOn = (int)&MenuWork;
    rec = (MENUEPOBJ *)func_A191C0_2(((MENUTECWORK *)nOn)->bChr);
    MenuListMake(0, 0);
    if (n > 0) {
        w2 = (MENUTECWORK *)nOn;
        nOn = 1;
        do {
            func_A1A378(*(short *)((char *)func_A1A488(*(short *)pSort) + 6));
            p->f0 = *(int *)MenuTextGet(*(int *)pSort);
            v = func_A11220(w2->bChr, *(short *)pSort);
            p->f4 = v;
            if (v <= rec->h36) {
                p->b8 = 0;
                p->b9 = 0;
            } else {
                p->b8 = nOn;
                p->b9 = nOn;
            }
            if (MenuEtherUseCheck(*(short *)pSort) == 0) {
                p->b8 = nOn;
                p->b9 = 2;
            }
            n--;
            p++;
            pSort++;
        } while (n != 0);
    }
}

/* TODO: near-miss (SCHEDULING, 4) - nOn/n pins fixed the old 16-rename swap
   and the >= arm-swap fixed the branch-likely polarity; what remains is the
   f4-store/arg-load/increment order around the MenuEtherWhoCheck jal (ours
   hoists the arg lh above the sw and the filler steals the sw). Barriers
   regress it; parked. */
/* Build one ether sub-list: points per entry, enable by owner/ep checks */
void MenuEtherListMake02(int nIdx)
{
    MENUTECLISTENT *p2;
    int nM1;
    register int nOn __asm__("$19");
    void *ptr;
    MENUTECWORK *w2;
    PARAOBJ *rec;
    register int n __asm__("$18");
    int v;
    int f4;

    nM1 = (int)MenuListGet(nIdx);
    nOn = (int)MenuSortAddrGet(nIdx);
    n = MenuSortCheck(nIdx);
    ptr = &MenuWork;
    rec = func_A19210(((MENUTECWORK *)ptr)->bChr);
    MenuListMake(nIdx, 0);
    if (n > 0) {
        w2 = (MENUTECWORK *)ptr;
        ptr = (void *)nOn;
        p2 = (MENUTECLISTENT *)nM1;
        nM1 = -1;
        nOn = 1;
        do {
            p2->f4 = *(short *)((char *)func_A1A488(*(short *)ptr) + 8);
            v = MenuEtherWhoCheck(*(short *)ptr);
            f4 = p2->f4;
            ptr = (int *)ptr + 1;
            if (f4 == 0) {
                p2->b8 = nOn;
                p2->f4 = nM1;
            } else if (w2->bChr != v) {
                p2->b8 = nOn;
            } else if (rec->f10 >= f4 / 2) {
                p2->b8 = 0;
            } else {
                p2->b8 = nOn;
            }
            n--;
            p2++;
        } while (n != 0);
    }
}

/* --- Item list 2 (sorted key-item view) --- */
extern char D_004C4F60[];
extern char D_0036C205[];

/* Re-sort item list 2 and refresh the item window onto it */
void MenuItemListMake02(void)
{
    ITEMLISTWORK *w;

    MenuSortSet(0, 2, 0);
    MenuSortChange(0, 0);
    MenuSortChange(0, MenuWork.b32);
    MenuListMake(0, 0);
    w = MenuItemList;
    w->b0D = 7;
    w->p1C = D_004C4F60;
    MenuItemList->b20 = 2;
    MenuItemList->b21 = 11;
    MenuItemList->h18 = 468;
    MenuItemList->h1A = MenuItemList->b21 * 24 + 6;
    MenuItemList->p28 = MenuListGet(0);
    WindowSPItemChange((char *)MenuItemList + 12);
    WindowSPSetSelect((char *)MenuItemList + 12, D_0036C205);
}

/* --- Ether char record (via func_A191C0_2) --- */
typedef struct {
    char pad0[23];
    signed char bCap;          /* 0x17: total set-ether capacity */
    char pad18[0x8E - 24];
    short hEther[12];          /* 0x8E: set-ether slots */
} ETHERREC;

/* TODO: near-miss (LENGTH, 85 orig vs 87 built) - everything lines up except
   the constant 12 of the trailing nSet==12 compare: the retail build
   rematerializes `li v0,12` every outer iteration while ours loop-hoists it
   into $s8 (one extra callee-saved + frame growth, cascading renames).
   Goto-shaped outer kills ALL hoists (too far the other way); inner-loop
   shape variants (while/do-while/goto/for) don't change it. Parked. */
/* Build ether list 0's cost/name entries; enable by capacity, flag set slots */
void MenuEtherListMake01(void)
{
    MENUTECLISTENT *p;
    void *ptr;
    int *pS;
    ETHERREC *rec;
    int n;
    int i;
    int nSet;
    int cost;

    i = 0;
    p = (MENUTECLISTENT *)MenuListGet(0);
    ptr = MenuSortAddrGet(0);
    n = MenuSortCheck(0);
    rec = (ETHERREC *)func_A191C0_2(MenuWork.bChr);
    MenuListMake(0, 0);
    if (n > 0) {
        pS = (int *)ptr;
        do {
            ptr = func_A1A488(*(short *)pS);
            func_A1A378(*(short *)((char *)ptr + 6));
            p->f0 = *(int *)MenuTextGet(*pS);
            cost = *(short *)((char *)ptr + 4);
            p->f4 = cost;
            if (rec->bCap + cost < 13) {
                p->b8 = 0;
            } else {
                p->b8 = 1;
            }
            for (nSet = 0; nSet < 12; nSet++) {
                if (rec->hEther[nSet] == *(short *)pS) {
                    break;
                }
            }
            if (nSet == 12) {
                p->b9 = 0;
            } else {
                p->b9 = 1;
            }
            i++;
            p++;
            pS++;
        } while (i < n);
    }
}

/* --- Face/EP id tables (stack copies of const tables) --- */
typedef struct {
    unsigned short h[12];
} FACEEPID12;
typedef struct {
    unsigned short h[16];
} FACEEPID16;
extern FACEEPID12 D_004C51E8;
extern FACEEPID12 D_004C5200;
extern FACEEPID16 D_004C5218;
extern FACEEPID12 D_004C5238;

/* TODO: near-miss (1 diff) - a single annul-bit flip: the retail build's
   (nType-17)<12 guard is beqzl (delay `move v0,zero` stolen from the
   return-0 target, annulled) while ours emits plain beqz with the same
   fill un-annulled. Needs `--branch-likely MenuFaceEpidGet:N` in the
   fixer flags, which is outside this session's allowed configure.py
   changes; noted for the coordinator. */
/* Map a character id to its face/EP sprite id (normal and flagged variants) */
int MenuFaceEpidGet(int nType, int nFlag)
{
    FACEEPID12 tbl1;
    FACEEPID12 tbl2;
    FACEEPID16 tbl3;
    FACEEPID12 tbl4;
    int nRet;

    if (nType == 2) {
        nRet = 1025;
        if (nFlag == 0) {
            nRet = 3073;
        }
        return nRet;
    }
    if (nFlag == 0) {
        if ((unsigned int)(nType - 1) < 12) {
            tbl1 = D_004C51E8;
            return tbl1.h[nType - 1];
        }
        if ((unsigned int)(nType - 17) < 12) {
            tbl2 = D_004C5200;
            return tbl2.h[nType - 17];
        }
        return 0;
    }
    if ((unsigned int)(nType - 1) < 12) {
        tbl3 = D_004C5218;
        return tbl3.h[nType - 1];
    }
    tbl4 = D_004C5238;
    return tbl4.h[nType - 17];
}

/* --- Shop list window work --- */
typedef struct {
    char pad0[0x48];
    signed char b48;           /* 0x48 */
} SHOPWORK;
typedef struct {
    char pad0[1];
    unsigned char b01;         /* 0x01 */
    char pad02[0x0C - 2];
    unsigned short h0C;        /* 0x0C */
    unsigned short h0E;        /* 0x0E */
    void *p10;                 /* 0x10 */
    unsigned char b14;         /* 0x14 */
    unsigned char b15;         /* 0x15 */
    char pad16[0x1C - 0x16];
    void *p1C;                 /* 0x1C */
    char pad20[0x26 - 0x20];
    unsigned char b26;         /* 0x26 */
} SHOPWINSP;
extern SHOPWORK *MenuShopWork;
extern SHOPWINSP *MenuShopWinSP;
extern void MenuShopSortSet(int, int, int, int);
extern void MenuShopListColorChange(int, int);
extern void WindowSPSelect(void *, int);
extern char D_004C7858[];

/* Re-sort the shop sell list and refresh the shop window onto it */
void MenuShopListChange01(void)
{
    SHOPWINSP *w;
    SHOPWINSP *w2;
    int nSort;

    nSort = 18;
    if (MenuShopWork->b48 == 0) {
        nSort = 19;
    }
    MenuShopSortSet(0, nSort, 0, 0);
    MenuListMake(0, -10);
    MenuShopListColorChange(0, 2);
    w = MenuShopWinSP;
    w->h0C = 272;
    w->h0E = 222;
    w->b15 = 9;
    MenuShopWinSP->b14 = 1;
    w2 = MenuShopWinSP;
    w2->p10 = D_004C7858;
    w2->b01 = 7;
    MenuShopWinSP->p1C = MenuListGet(0);
    WindowSPItemChange(MenuShopWinSP);
    WindowSPSetSelect(MenuShopWinSP, &D_0036C200[nSort * 5]);
    MenuShopWinSP->b26 = 6;
    WindowSPSelect(MenuShopWinSP, 0);
}

/* --- AGWS equip list windows --- */
extern char *AgwsList;
extern int MenuAccessoryEquipCheck(int, int, int);

/* TODO: near-miss (OPERANDS, 2) - the loop-entry `addiu s0,s0,8` (list+8)
   and the &MenuWork lo-add are emitted in the opposite order; the retail
   build splits the lui/addiu pair around the +8. Entry-statement and
   store-order permutes, pointer vs direct-global forms, and a named-w2
   variant all leave the same 2-insn swap (shared preheader wall with
   _Acc/_Pilot). */
/* Build the AGWS gun list: enable entries whose bullet fits the current weapon */
void MenuAgwsListMake_Gun(void)
{
    int *pSort;
    int *pS;
    unsigned char *q;
    SHOPWINSP *win;
    MENUTECWORK *w;
    int n;
    int cnt;

    pSort = MenuSortAddrGet(0);
    q = (unsigned char *)MenuListGet(0);
    win = (SHOPWINSP *)(AgwsList + 0x130);
    MenuSortSet(0, 8, -2);
    MenuListMake(0, 0);
    n = MenuSortCheck(0);
    if (n > 0) {
        q += 8;
        pS = pSort;
        w = &MenuWork;
        cnt = n;
        do {
            if (MenuBulletCheck(w->h26, *(short *)pS) != 0) {
                *q = 0;
            } else {
                *q = 1;
            }
            pS++;
            cnt--;
            q += 12;
        } while (cnt != 0);
    }
    win->b15 = 7;
    win->h0C = 229;
    win->h0E = 174;
    win->b14 = 1;
    win->b01 = 7;
    win->p10 = 0;
    win->p1C = MenuListGet(0);
    WindowSPItemChange(win);
    WindowSPSetSelect(win, &D_0036C200[30]);
}

/* TODO: near-miss (OPERANDS, 2) - same lui/addiu-split scheduling wall as
   MenuAgwsListMake_Gun above. */
/* Build the AGWS accessory list: enable entries the current AGWS can equip */
void MenuAgwsListMake_Acc(void)
{
    int *pSort;
    int *pS;
    unsigned char *q;
    SHOPWINSP *win;
    int n;
    int cnt;

    pSort = MenuSortAddrGet(0);
    q = (unsigned char *)MenuListGet(0);
    win = (SHOPWINSP *)(AgwsList + 0x130);
    MenuSortSet(0, 16, -2);
    MenuListMake(0, 0);
    n = MenuSortCheck(0);
    if (n > 0) {
        pS = pSort;
        q += 8;
        cnt = n;
        do {
            if (MenuAccessoryEquipCheck(MenuWork.h64, *(short *)pS, MenuWork.b55) != 0) {
                *q = 0;
            } else {
                *q = 1;
            }
            pS++;
            cnt--;
            q += 12;
        } while (cnt != 0);
    }
    win->h0C = 229;
    win->h0E = 150;
    win->b14 = 1;
    win->b15 = 6;
    win->b01 = 7;
    win->p10 = 0;
    win->p1C = MenuListGet(0);
    WindowSPItemChange(win);
    WindowSPSetSelect(win, &D_0036C200[35]);
}

extern char D_004C6F38[];
extern int MenuWeaponEquipPosCheck(int, int, int, int);

/* TODO: near-miss (OPERANDS, 2) - wave-4 fixes: pS/q pins + a pSort `+r`
   barrier after MenuListMake resolve the callee-saved swap, and the
   branch-likely arm-swaps + window-tail store rotation fixed the LOGIC
   diffs; only the family's shared preheader lui/addiu-split wall remains
   (same 2 words as _Gun/_Acc). */
/* Build the AGWS pilot list: gray taken/empty pilots, flag the current one */
void MenuAgwsListMake_Pilot(void)
{
    register int *pSort __asm__("$17");
    register int *pS __asm__("$18");
    register unsigned char *q __asm__("$16");
    SHOPWINSP *win;
    MENUTECWORK *w;
    PILOTREC *rec;
    int n;
    int cnt;

    q = (unsigned char *)MenuListGet(0);
    pSort = MenuSortAddrGet(0);
    win = (SHOPWINSP *)(AgwsList + 0x130);
    MenuSortSet(0, 32, 1);
    MenuListMake(0, 0);
    __asm__ volatile("" : "+r"(pSort));
    n = MenuSortCheck(0);
    if (n > 0) {
        pS = pSort;
        w = &MenuWork;
        q += 9;
        cnt = n;
        do {
            rec = func_A191C0_2(*(short *)pS);
            pS++;
            if (rec->hPilotId == w->h64) {
                q[-1] = 0;
            } else if (rec->hPilotId != 0) {
                q[-1] = 1;
            } else {
                q[-1] = 0;
            }
            if (rec->hPilotId == w->h64) {
                q[0] = 1;
            } else {
                q[0] = 0;
            }
            cnt--;
            q += 12;
        } while (cnt != 0);
    }
    win->h0C = 208;
    win->h0E = 144;
    win->b14 = 1;
    win->b15 = 5;
    win->b01 = 3;
    win->p10 = D_004C6F38;
    win->p1C = MenuListGet(0);
    WindowSPItemChange(win);
    WindowSPSetSelect(win, &D_0036C200[40]);
}

/* TODO: near-miss (LENGTH ~60) - the retail build runs the repurposed-ptr
   pattern (&MenuWork phase 1 in $s0 via a kept $s6 lui half, list walker
   phase 2, tail rematerialized from $s6); ours reproduces the s6 half but
   the allocator hands ptr $s3 instead of $s0 and shifts every copy. */
/* Build the AGWS weapon list for the selected mount: enable by equip-pos check */
void MenuAgwsListMake_Wpn(void)
{
    int *pSort;
    void *ptr;
    MENUTECWORK *w2;
    SHOPWINSP *win;
    int n;
    int cnt;

    ptr = &MenuWork;
    pSort = MenuSortAddrGet(0);
    n = (int)MenuListGet(0);
    win = (SHOPWINSP *)(AgwsList + 0x130);
    MenuSortSet(0, 4,
        *((signed char *)ptr + ((MENUTECWORK *)ptr)->b56 + 0x10) | 0x2000);
    MenuListMake(0, 0);
    cnt = MenuSortCheck(0);
    if (cnt > 0) {
        w2 = (MENUTECWORK *)ptr;
        ptr = (unsigned char *)n + 8;
        do {
            if (MenuWeaponEquipPosCheck(w2->h64, *(short *)pSort,
                    *((signed char *)w2 + w2->b56 + 0x10),
                    w2->b54) != 0) {
                *(unsigned char *)ptr = 0;
            } else {
                *(unsigned char *)ptr = 1;
            }
            pSort++;
            cnt--;
            ptr = (unsigned char *)ptr + 12;
        } while (cnt != 0);
    }
    win->h0E = 174;
    win->b14 = 1;
    win->b01 = 7;
    win->b15 = 7;
    win->h0C = 229;
    win->p10 = 0;
    win->p1C = MenuListGet(0);
    WindowSPItemChange(win);
    WindowSPSetSelect(win,
        &D_0036C200[*((signed char *)&MenuWork + MenuWork.b56 + 0x10) * 5 - 5]);
}

/* ================= Wave 4: mid-size Menu functions ================= */

extern char *xglStudioGetCamera2_2(int) __asm__("xglStudioGetCamera2");
extern void xglCameraInit(char *);

/* --- Shop shared work (sell-mode flag / pending model break) --- */
typedef struct {
    char pad0[0x10];
    unsigned char b10;         /* 0x10: sell-mode flag */
    char pad11[0x2C - 0x11];
    int f2C;                   /* 0x2C: pending model-break flag */
} SHOPCOLWORK;
extern SHOPCOLWORK *D_004DC5F8;

/* TODO: near-miss (LENGTH, 90 orig vs 87-88 built) - the retail build keeps
   pList/pSort/w in s6/s5/s3 with real loop-entry copies into s0/s1/s4; gcc
   coalesces the copies away (pins ignored, `+r` reinforcement too; a
   liveness-forcing asm after the loop recovers one copy but rotates the
   callee-saved assignment). Same coalescer wall as MenuSkillListChange01. */
/* Build ether sub-list nIdx for the set-ether screen: temporarily set the
   selected ether's learned bit, sort/build the list, clear the bit again,
   then fill cost per entry and flag the currently-selected ether */
void MenuEtherListMake03(int nIdx)
{
    MENUTECLISTENT *pList;
    MENUTECLISTENT *p;
    int *pSort;
    int *pS;
    MENUTECWORK *w;
    MENUTECWORK *w2;
    char *p2;
    PARAOBJ *rec;
    int one;
    int nOn;
    int n;
    int cnt;
    int m;

    pList = (MENUTECLISTENT *)MenuListGet(nIdx);
    one = 1;
    pSort = MenuSortAddrGet(nIdx);
    w = &MenuWork;
    rec = func_A19210(w->b46);
    m = w->bSel - 1;
    p2 = (char *)rec + m / 8 + 40;
    *p2 |= one << (m % 8);
    MenuSortSet(nIdx, 128, w->b46);
    m = w->bSel - 1;
    *p2 &= ~(one << (m % 8));
    MenuListMake(nIdx, 0);
    n = MenuSortCheck(nIdx);
    if (n > 0) {
        w2 = w;
        nOn = 1;
        p = pList;
        pS = pSort;
        cnt = n;
        do {
            p->f4 = *(short *)((char *)func_A1A488(*(short *)pS) + 8);
            if (*(short *)pS == w2->bSel) {
                p->b8 = nOn;
            } else {
                p->b8 = 0;
            }
            pS++;
            cnt--;
            p++;
        } while (cnt != 0);
    }
}

/* --- Para (stat) level-up apply --- */

/* Apply one stat advance: bump the stat (clamped to the party max for
   HP/EP) and deduct the advance's point cost */
void MenuParaUp(int nId, int nType)
{
    char *p;
    PARAOBJ *rec;
    int nextPoint;
    register int t __asm__("$16");
    register short t2 __asm__("$16");

    p = (char *)func_A191C0_2(nId);
    rec = func_A19210(nId);
    nextPoint = MenuParaNextPointGet(nId, nType, 0);
    switch (nType) {
    case 0:
        *(unsigned short *)(p + 4) += 1;
        break;
    case 1:
        *(unsigned short *)(p + 6) += 1;
        break;
    case 2:
        *(unsigned short *)(p + 8) += 1;
        break;
    case 3:
        *(unsigned short *)(p + 10) += 1;
        break;
    case 4:
        *(unsigned char *)(p + 12) += 1;
        break;
    case 5:
        *(unsigned char *)(p + 13) += 1;
        break;
    case 6:
        t = *(unsigned short *)(p + 0) + 10;
        *(unsigned short *)(p + 0) = t;
        t2 = t;
        if (MenuParaUpMaxGet(nType) < t2) {
            *(unsigned short *)(p + 0) = MenuParaUpMaxGet(nType);
        }
        break;
    case 7:
        t = *(unsigned short *)(p + 2) + 2;
        *(unsigned short *)(p + 2) = t;
        t2 = t;
        if (MenuParaUpMaxGet(nType) < t2) {
            *(unsigned short *)(p + 2) = MenuParaUpMaxGet(nType);
        }
        break;
    }
    rec->fC -= nextPoint;
}

/* --- Tairetu (formation) file load --- */
typedef struct {
    char d[26];
} TAIRETUNAME;
extern TAIRETUNAME D_004C3220;
extern int D_004DA640;
extern int D_004DA644;
extern int D_004DA648;
extern int D_004DA64C;

/* TODO: near-miss (REGISTER 14) - the three block-address temps live in $a3
   in the retail build (with each sw stolen into the MenuLoadFile jal slot);
   ours keeps them in $v0/$v1. $7 pins collide with the filename-copy lui
   half and every barrier variant reorders the sb patch bytes. Parked. */
/* Queue the four tairetu blobs (xt0/xt1/le1/le0) into the aligned scratch
   area and return the second texture block's address */
char *MenuTairetuLoad(int nMemory)
{
    TAIRETUNAME name;
    int aligned;
    char *q;

    aligned = (nMemory + 127) & ~127;
    D_004DA640 = aligned;
    name = D_004C3220;
    name.d[22] = 'x';
    name.d[23] = 't';
    name.d[20] = '0';
    MenuLoadFile(name.d, (void *)aligned);
    q = (char *)(D_004DA640 + 0x3000);
    name.d[20] = '1';
    D_004DA648 = (int)q;
    MenuLoadFile(name.d, q);
    q = (char *)(D_004DA648 + 0x800);
    name.d[22] = 'l';
    name.d[23] = 'e';
    D_004DA64C = (int)q;
    MenuLoadFile(name.d, q);
    q = (char *)(D_004DA64C + 0x1000);
    name.d[20] = '0';
    D_004DA644 = (int)q;
    MenuLoadFile(name.d, q);
    return (char *)(D_004DA644 + 0x800);
}

/* --- Model sub-window setup --- */
extern char D_0036D5D0[];
extern void xglStudioChange(int);
extern void nmlModelSetWindow(int);

/* Bind model nId to a studio sub-window: fill its sub-window object, reset
   camera 0 onto it and hand the window to the renderer */
void MenuModelSubWindowSet(SUBWIN *p, int nId, int nType)
{
    SUBOBJ2 *q;
    char *pCam;
    int nOn;
    int v;

    v = *(int *)((char *)p + 32);
    if (nId == 0) {
        return;
    }
    if (v == 0) {
        return;
    }
    q = (SUBOBJ2 *)(D_0036D5D0 + (nId << 4) - 16);
    nOn = 1;
    ((SUBWIN *)p)->f4C = q;
    q->b1 = 0;
    q->b2 = nId;
    q->b3 = nType;
    xglStudioChange(nId);
    pCam = (char *)xglStudioGetCamera2_2(0);
    q->fC = (int)pCam;
    xglCameraInit(pCam);
    xglCameraSetWindow((int)pCam, 0, 0, 0, 0);
    *(int *)pCam = nOn;
    *(int *)(pCam + 4) = nOn;
    nmlModelUseSubWindow(nId, nType);
    if (nType == nOn) {
        nmlModelSetWindow(nId);
    }
    xglStudioChange(0);
}

/* --- Shop model window --- */
typedef struct {
    char pad0[0x10];
    signed char b10;           /* 0x10 */
    unsigned char b11;         /* 0x11 */
    char pad12;
    unsigned char b13;         /* 0x13: model display state */
    char pad14[0x20 - 0x14];
    char *pUnit;               /* 0x20 */
} SHOPMODELWORK;
extern void MenuModelUnitBreak_v() __asm__("MenuModelUnitBreak");
extern float D_004D7E60;

/* TODO: near-miss (2 diffs) - the delay-slot filler steals the case-0 lwc1
   into the switch-dispatch beqz slot where the retail build keeps the shared
   `move a0,s0` there; statement-order variants tried. Parked. */
/* Drive the shop screen's model display state (open, fade wait, idle) */
void MenuShopModelDisp(SHOPMODELWORK *w)
{
    char *pUnit;
    char *p2;

    pUnit = w->pUnit;
    if (pUnit == 0) {
        if (D_004DC5F8->f2C != 0) {
            MenuModelUnitBreak_v();
            D_004DC5F8->f2C = 0;
        }
    }
    if (w->b11 == 0) {
        return;
    }
    p2 = pUnit + 192;
    if (D_004DC5F8->f2C != 0) {
        w->b13 = 30;
        D_004DC5F8->f2C = 0;
    }
    switch (w->b13) {
    case 10:
        break;
    case 0:
        *(float *)(p2 + 20) = D_004D7E60;
        MenuModelUnitOpen((char *)w, 2);
        w->b13 = 10;
        break;
    case 30:
        MenuModelUnitOpen((char *)w, 1);
        w->b13 = 31;
        /* fall through */
    case 31:
        if (*(float *)(p2 + 96) == 0.0f) {
            w->b10 = -1;
        }
        break;
    }
}

/* --- Model pad-driven control --- */
typedef struct {
    char pad0[0x60];
    float f60;                 /* 0x60 */
    char pad64[0xD0 - 0x64];
    float pos[2];              /* 0xD0 */
    char padD8[0xE0 - 0xD8];
    float zoom[3];             /* 0xE0 */
    char padEC[0xF0 - 0xEC];
    int fF0;                   /* 0xF0 */
} MODELCTRLUNIT;
typedef struct {
    char pad0[0x20];
    MODELCTRLUNIT *pUnit;      /* 0x20 */
} MODELCTRLWORK;
typedef struct {
    char pad0[0x30];
    unsigned short h30;        /* 0x30: pad level bits */
} MODELCTRLPAD;
extern MODELCTRLPAD D_00490D90;
extern float D_004D7DA8;
extern float D_004D7DAC;
extern float D_004D7DB0;
extern float D_004D7DB4;
extern float D_004D7DB8;
extern float D_004D7DBC;
extern float D_004D7DC0;

/* TODO: near-miss (LENGTH, 66 orig vs 65 built) - the retail build
   rematerializes &D_00490D90 (addiu off the kept lui half) separately in the
   nBtn branch slot AND at the block end where ours cross-jumps them into one
   shared addiu; a reload-into-variable form gets further away. Parked. */
/* Move/zoom a menu model from the held pad bits, clamping the zoom range */
void MenuModelControl(MODELCTRLWORK *w)
{
    MODELCTRLUNIT *u;
    float *pos;
    float *zoom;
    int nPad;
    unsigned short nBtn;

    u = w->pUnit;
    pos = u->pos;
    zoom = u->zoom;
    if (u->fF0 == 0) {
        return;
    }
    nPad = D_00490D90.h30;
    if (nPad & 0x1000) {
        pos[0] -= D_004D7DA8;
    }
    if (nPad & 0x4000) {
        pos[0] += D_004D7DAC;
    }
    if (nPad & 0x8000) {
        pos[1] -= D_004D7DB0;
    }
    if (nPad & 0x2000) {
        pos[1] += D_004D7DB4;
    }
    nBtn = nPad & 1;
    if (nBtn) {
        zoom[0] -= D_004D7DB8;
        if (u->f60 <= 0.25f) {
            zoom[0] = 0.25f;
        }
    }
    if (D_00490D90.h30 & 2) {
        zoom[0] += D_004D7DBC;
        if (D_004D7DC0 <= u->f60) {
            zoom[0] = D_004D7DC0;
        }
    }
    zoom[1] = zoom[0];
    zoom[2] = zoom[0];
}

/* --- Equip-compare point delta --- */
extern unsigned char D_0036D897[];

/* Difference between the equip-preview stats and the current stats for the
   stat type selected on the equip screen */
int MenuCharEquipCalcPointGet(int nChr)
{
    int buf1[4];
    int buf2[4];
    char *p1;
    char *p2;
    register int nRet __asm__("$16");

    nRet = 0;
    if (nChr == 0) {
        return nRet;
    }
    p1 = (char *)func_A191C0_2(nChr);
    p2 = (char *)func_A11108(nChr, buf1, buf2);
    switch (MenuWork.b7A) {
    case 0:
        nRet = *(short *)(p2 + 0);
        nRet -= *(short *)(p1 + 0);
        break;
    case 1:
        nRet = *(short *)(p2 + 2);
        nRet -= *(short *)(p1 + 2);
        break;
    case 2:
        nRet = *(short *)(p2 + 4);
        nRet -= *(short *)(p1 + 4);
        nRet += buf1[D_0036D897[MenuWork.h60]];
        break;
    case 3:
        nRet = *(short *)(p2 + 6);
        nRet -= *(short *)(p1 + 6);
        nRet += buf2[0];
        break;
    case 4:
        nRet = *(short *)(p2 + 8);
        nRet -= *(short *)(p1 + 8);
        break;
    case 5:
        nRet = *(short *)(p2 + 10);
        nRet -= *(short *)(p1 + 10);
        nRet += buf2[1];
        break;
    case 6:
        nRet = *(signed char *)(p2 + 12);
        nRet -= *(signed char *)(p1 + 12);
        break;
    case 7:
        nRet = *(signed char *)(p2 + 13);
        nRet -= *(signed char *)(p1 + 13);
        break;
    }
    return nRet;
}

/* --- Item list 1 (per-category sorted views) --- */
extern signed char D_0036C1B3[];
extern char D_0036C20A[];

/* Re-sort item list nIdx by its category preset and refresh the item window */
void MenuItemListMake01(int nIdx)
{
    ITEMLISTWORK *w;

    switch (nIdx) {
    case 0:
        MenuSortSet(0, 4, -1);
        break;
    case 1:
        MenuSortSet(0, 16, -1);
        break;
    case 2:
        MenuSortSet(0, 8, -1);
        break;
    case 3:
        MenuSortSet(0, 4, -2);
        break;
    case 4:
        MenuSortSet(0, 16, -2);
        break;
    case 5:
        MenuSortSet(0, 8, -2);
        break;
    }
    MenuSortChange(0, 0);
    MenuSortChange(0, D_0036C1B3[nIdx]);
    MenuListMake(0, 0);
    w = MenuItemList;
    w->b0D = 7;
    w->p1C = D_004C4F60;
    MenuItemList->b20 = 2;
    MenuItemList->b21 = 11;
    MenuItemList->h18 = 468;
    MenuItemList->h1A = MenuItemList->b21 * 24 + 6;
    MenuItemList->p28 = MenuListGet(0);
    WindowSPItemChange((char *)MenuItemList + 12);
    WindowSPSetSelect((char *)MenuItemList + 12, &D_0036C20A[nIdx * 5]);
}

/* --- AGWS camera keep block --- */
typedef struct {
    char d[16];
} CAMVEC;
typedef struct {
    long long d[4];
} CAMKEEP;
extern CAMKEEP D_004C7190;

/* Reset studio camera 0 and load the fixed AGWS-view position/target vectors */
void MenuAgwsCameraSet(void)
{
    CAMKEEP keep;
    char *pCam;

    keep = D_004C7190;
    pCam = xglStudioGetCamera2_2(0);
    xglCameraInit(pCam);
    *(CAMVEC *)(pCam + 0xD0) = *(CAMVEC *)&keep;
    *(int *)(pCam + 4) = 1;
    *(CAMVEC *)(pCam + 0xA0) = *((CAMVEC *)&keep + 1);
}

/* --- AGWS parameter window set --- */
extern void ParaSet(void *, void *, int *, int *);
extern char MenuAgwsPara[];
extern char MenuAgwsPara2[];

/* Fill the AGWS parameter window rows; nType 0 adds the second row block,
   nType 10 re-renders it with the pilot-id halfword temporarily kept */
void MenuAgwsParaSet(int nId, int nType)
{
    int buf1[4];
    int buf2[4];
    char *pChr;
    void *rec;
    short keep;

    if (nId == 0) {
        return;
    }
    pChr = (char *)func_A191C0_2(nId);
    keep = *(short *)(pChr + 52);
    rec = func_A11108(nId, buf1, buf2);
    ParaSet(rec, MenuAgwsPara2, buf1, buf2);
    if (nType == 0) {
        ParaSet(rec, MenuAgwsPara, buf1, buf2);
    }
    if (nType == 10) {
        ParaSet(rec, MenuAgwsPara, buf1, buf2);
        *(short *)(pChr + 52) = keep;
    }
}

/* --- Shop list color refresh --- */
extern int MenuBoxMoneyGet(int, int);
extern int dataMoneyBoxChk();
extern int subMenuShopEquipCheck(int, int);

/* Recolor one shop list: gray entries the player cannot afford (buy view)
   or flag no-sale entries (sell view) */
void MenuShopListColorChange(int nIdx, int nType)
{
    int *pSort;
    char *pList;
    unsigned char *q;
    char *p2;
    int n;
    int i;
    int one;
    int v;
    int m;

    pSort = MenuSortAddrGet(nIdx);
    n = MenuSortCheck(nIdx);
    pList = (char *)MenuListGet(nIdx);
    if (D_004DC5F8->b10 == 0 || nType == 2) {
        i = 0;
        if (n > 0) {
            q = (unsigned char *)pList + 8;
            one = 1;
            do {
                v = MenuSortGet(nIdx, i);
                i++;
                m = MenuBoxMoneyGet(v, 0);
                if (dataMoneyBoxChk() < m) {
                    *q = one;
                }
                q += 12;
            } while (i < n);
        }
    } else {
        i = 0;
        while (i < n) {
            MenuBoxChk(MenuSortGet(nIdx, i));
            subMenuShopEquipCheck(MenuSortGet(nIdx, i), 0);
            v = MenuShopNoSaleCheck(pSort[i]);
            p2 = pList + i * 12;
            i++;
            if (v == 1) {
                p2[8] = v;
            }
        }
    }
}

/* --- Skill list window --- */
extern char *MenuSkillList;
extern int SkillSetLvGet(int);
extern char D_004DB640[];

/* TODO: near-miss (REGISTER ~39) - the b9 beqzl polarity now matches (wave-4
   arm-swap lever: branch-likely slot always takes the ELSE arm, so write the
   condition so the else arm is the one the original annuls). The remaining
   callee-saved rotation (s0/s1/s2/s3 -> s3/s0/s4/s2) resists the register-pin
   idiom: gcc coalesces plain pins away, and the `+r` asm reinforcement pins
   the early pair but derails the delay-slot capture scheduling (32→39 diffs
   both ways). The window tail's five li/sb-sh stores are also emitted in a
   scheduler-shuffled order unreachable from plain source order permutes. */
/* Rebuild the skill list for the current character and refresh its window */
void MenuSkillListChange01(void)
{
    void *ptr;
    void *ptr2;
    int *pSort;
    MENUTECWORK *w2;
    SHOPWINSP *win;
    int n;
    int cnt;

    pSort = MenuSortAddrGet(0);
    ptr2 = MenuListGet(0);
    ptr = &MenuWork;
    win = (SHOPWINSP *)(MenuSkillList + 12);
    MenuSortSet(0, 64, ((MENUTECWORK *)ptr)->bChr);
    MenuListMake(0, 0);
    n = MenuSortCheck(0);
    if (n > 0) {
        w2 = (MENUTECWORK *)ptr;
        ptr = ptr2;
        ptr2 = pSort;
        cnt = n;
        do {
            ((MENUTECLISTENT *)ptr)->f4 = SkillSetLvGet(*(int *)ptr2);
            if (MenuSkillEquipCheck(w2->bChr, *(int *)ptr2) != 0) {
                ((MENUTECLISTENT *)ptr)->b9 = 1;
            } else {
                ((MENUTECLISTENT *)ptr)->b9 = 0;
            }
            ptr2 = (int *)ptr2 + 1;
            cnt--;
            ptr = (MENUTECLISTENT *)ptr + 1;
        } while (cnt != 0);
    }
    win->b01 = 3;
    win->b15 = 8;
    win->h0C = 256;
    win->h0E = 198;
    win->b14 = 1;
    win->p10 = D_004DB640;
    win->p1C = MenuListGet(0);
    WindowSPItemChange(win);
    WindowSPSetSelect(win, &D_0036C200[20]);
    WindowSPSelect(win, 0);
}
