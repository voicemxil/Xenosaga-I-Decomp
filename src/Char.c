/* Char* - character status/equip-menu logic */

typedef struct {
    short field_0;    /* 0x00 */
    short field_2;    /* 0x02 */
    char pad04[0x30];
    short field_34;   /* 0x34 */
    short field_36;   /* 0x36 */
} CHARREC;

extern void func_A191C0(int nNo);
extern CHARREC *func_00A11108(int nNo, void *p1, void *p2);

/* TODO: not matching - REGISTER class near-miss. The original keeps the
   func_00A11108 return pointer in TWO places: the fresh v0 (used for both
   lhu loads) and a copied-out s0 (used for both sh stores). Every natural
   C shape (single pointer var, split src/dst vars, self-assignment,
   volatile) either collapses to one register (v0 only, 4 instructions
   short) or forces a stack spill (volatile). Not reachable from source. */
/* Fully heal every playable character's HP/EP to their maximums */
void CharactorAllRecovery(void)
{
    int i;
    char buf1[16];
    char buf2[16];
    CHARREC *p;
    short hp;
    short ep;

    for (i = 1; i < 0xC; i++) {
        func_A191C0(i);
        p = func_00A11108(i, buf1, buf2);
        hp = p->field_0;
        ep = p->field_2;
        p->field_34 = hp;
        p->field_36 = ep;
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
    char pad[0x60];
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
