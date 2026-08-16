/* Char* - character status/equip-menu logic */

typedef struct {
    unsigned short field_0;   /* 0x00 */
    unsigned short field_2;   /* 0x02 */
    char pad04[0x30];
    short field_34;   /* 0x34 */
    short field_36;   /* 0x36 */
} CHARREC;

/* Returns the character's live status record -- the recovery below writes
   through it, which is where the original's second live pointer comes from. */
extern CHARREC *func_A191C0(int nNo);
extern CHARREC *func_00A11108(int nNo, void *p1, void *p2);

/* Fully heal every playable character's HP/EP to their maximums */
void CharactorAllRecovery(void)
{
    int i;
    char buf1[16];
    char buf2[16];
    CHARREC *pDst;
    CHARREC *pSrc;

    for (i = 1; i < 0xC; i++) {
        pDst = func_A191C0(i);
        pSrc = func_00A11108(i, buf1, buf2);
        pDst->field_34 = pSrc->field_0;
        pDst->field_36 = pSrc->field_2;
    }
}

/* --- Weapon/gun/accessory sub-list construction for the equip menu --- */
typedef struct {
    char pad00[2];
    char pad02[0xA];
    unsigned short hC;   /* 0x0C */
    unsigned short hE;   /* 0x0E */
    int f10;              /* 0x10 */
    unsigned char b14;    /* 0x14 */
    unsigned char b15;    /* 0x15 */
    char pad16[6];
    int f1C;               /* 0x1C */
} SPITEM;

extern short D_0036C1E0[];
extern char D_0036C20F[];
extern char D_0036C219[];
extern char *CharList;
extern void MenuSortSet(int nType, int nCol, int nKey);
extern void MenuListMake(int nType, int nArg);
extern SPITEM *MenuListGet(int nIdx);
extern int WindowSPItemChange(SPITEM *p);
extern void WindowSPSetSelect(SPITEM *p, char *pMsg);

/* Build the weapon sub-list for the character equip menu */
void CharListMake_Wpn(void)
{
    SPITEM *p = (SPITEM *)(CharList + 0x504);

    MenuSortSet(0, 4, D_0036C1E0[0]);
    MenuListMake(0, 0);
    p->hC = 0xE4;
    p->b15 = 7;
    p->hE = 0xAE;
    p->b14 = 1;
    ((unsigned char *)p)[1] = 7;
    p->f10 = 0;
    p->f1C = (int)MenuListGet(0);
    WindowSPItemChange(p);
    WindowSPSetSelect(p, D_0036C20F);
}

/* --- Stat (para) sub-list construction for the equip/status menu --- */
typedef struct {
    int f0;               /* 0x00 - display name id */
    char pad4[4];
    unsigned char f8;      /* 0x08 - "can level up" flag */
    char pad9[3];
} PARAENT;

typedef struct {
    char pad00[0x12];
    unsigned short h12;    /* 0x12 - currently equipped gun id */
    char pad14[0x21];      /* 0x14 .. 0x35 */
    signed char b35;       /* 0x35 - accessory slot */
    char pad36[0x19];      /* 0x36 .. 0x4F */
    signed char b4F;       /* 0x4F - accessory sort key / message row */
    char pad50[0x10];      /* 0x50 .. 0x60 */
    short h60;             /* 0x60 - selected character index */
} MENUWORK;

extern MENUWORK MenuWork;
extern char D_004DADB0[];
extern char D_004DAE50[];
extern int MenuParaNameGet2(int nType);
extern int MenuParaUpCheck(int nBase, int nType);

/* Build the stat sub-list for the character equip/status menu */
void CharListMake_Para(void)
{
    int i;
    int nType;
    PARAENT *p = (PARAENT *)(CharList + 0x3374);
    SPITEM *p2 = (SPITEM *)(CharList + 0x504);

    i = 0;
    while (i < 8) {
        p->f0 = MenuParaNameGet2(i);
        nType = i;
        i++;
        if (MenuParaUpCheck(MenuWork.h60, nType) == 0) {
            p->f8 = 0;
        } else {
            p->f8 = 1;
        }
        p++;
    }
    p->f0 = (int)D_004DADB0;
    p[1].f0 = 0;

    p2->b14 = 1;
    p2->b15 = 8;
    ((unsigned char *)p2)[1] = 3;
    p2->f10 = (int)D_004DAE50;
    p2->hC = 0x69;
    p2->hE = 0xC6;
    p2->f1C = (int)(CharList + 0x3374);
    WindowSPItemChange(p2);
    WindowSPSetSelect(p2, D_0036C219);
}

/* --- Gun and accessory sub-list construction --- */

/* One entry of the sort table MenuSortAddrGet hands back: a 4-byte stride
   over item ids. The two callers below disagree on the id's signedness, so
   each spells its own view of the entry. */
typedef struct {
    unsigned short nId;    /* 0x00 */
    short nUnk02;          /* 0x02 */
} SORTENT;

typedef struct {
    short nId;             /* 0x00 */
    short nUnk02;          /* 0x02 */
} SORTENTS;

/* A menu list row: text id, box id, and the "greyed out" flag the two
   builders below fill in. */
typedef struct {
    int nText;             /* 0x00 */
    int nBox;              /* 0x04 */
    unsigned char nGray;   /* 0x08 */
} SUBROW;

typedef struct {
    char pad00[0x0C];
    short h0C;             /* 0x0C - the gun id this record equips */
} GUNREC;

extern void *MenuSortAddrGet(int nType);
extern int MenuSortCheck(int nType);
extern GUNREC *func_A1A428(int nId);
extern int MenuAccessoryEquipCheck(int nChr, int nId, int nSlot);
extern char D_0036C214[];
extern char D_0036C200[][5];

/* Build the gun sub-list, greying out every row that is not the gun the
   character already has equipped */
void CharListMake_Gun(void)
{
    SPITEM *p;
    SORTENT *pSort;
    SUBROW *pRow;
    int i;
    int nRows;

    pSort = (SORTENT *)MenuSortAddrGet(0);
    pRow = (SUBROW *)MenuListGet(0);
    p = (SPITEM *)(CharList + 0x504);
    MenuSortSet(0, 8, MenuWork.h60);
    MenuListMake(0, 0);
    nRows = MenuSortCheck(0);
    for (i = 0; i < nRows; i++) {
        if (func_A1A428(pSort[i].nId)->h0C != MenuWork.h12) {
            pRow[i].nGray = 1;
        } else {
            pRow[i].nGray = 0;
        }
    }
    p->hC = 0xE4;
    p->b15 = 7;
    p->hE = 0xAE;
    p->b14 = 1;
    ((unsigned char *)p)[1] = 7;
    p->f10 = 0;
    p->f1C = (int)MenuListGet(0);
    WindowSPItemChange(p);
    WindowSPSetSelect(p, D_0036C214);
}

/* Build the accessory sub-list, greying out every row the character cannot
   equip into the selected slot */
void CharListMake_Acc(void)
{
    SPITEM *p;
    SORTENTS *pSort;
    SUBROW *pRow;
    int i;
    int nRows;

    pSort = (SORTENTS *)MenuSortAddrGet(0);
    pRow = (SUBROW *)MenuListGet(0);
    p = (SPITEM *)(CharList + 0x504);
    MenuSortSet(0, 0x10, MenuWork.b4F);
    MenuListMake(0, 0);
    nRows = MenuSortCheck(0);
    for (i = 0; i < nRows; i++) {
        if (MenuAccessoryEquipCheck(MenuWork.h60, pSort[i].nId, MenuWork.b35)) {
            pRow[i].nGray = 0;
        } else {
            pRow[i].nGray = 1;
        }
    }
    p->hC = 0xE4;
    p->hE = 0x7E;
    p->b14 = 1;
    p->b15 = 5;
    ((unsigned char *)p)[1] = 7;
    p->f10 = 0;
    p->f1C = (int)MenuListGet(0);
    WindowSPItemChange(p);
    WindowSPSetSelect(p, D_0036C200[MenuWork.b4F]);
}
