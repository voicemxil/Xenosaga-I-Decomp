/* Menu subsystem - equip/status UI getters, model/sub-window plumbing, sort
   helpers and misc bibration/load-sync bookkeeping */

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

/* --- Model task driver --- */
extern int MenuModelTask;
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
