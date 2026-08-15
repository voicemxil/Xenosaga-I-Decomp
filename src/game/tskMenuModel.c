#include "matching.h"

/* tskMenuModel - menu 3D-model viewer task drivers (unit + weapon) */

typedef union QUAD {
    float f[4];
    long long ll[2];
} QUAD;

typedef struct MMSTATE {           /* actor state block at +0xC0 */
    QUAD v00;                      /* 0xC0 */
    QUAD v10;                      /* 0xD0 */
    QUAD v20;                      /* 0xE0 */
    int nInit;                     /* 0xF0 */
    char padF4[0x100 - 0xF4];
    QUAD v40;                      /* 0x100 */
    char pad110[0x120 - 0x110];
    float fScale120;               /* 0x120: load-done when 1.0 */
} MMSTATE;

typedef struct MMACT {
    char pad00[0x10];
    QUAD vPos;                     /* 0x10 */
    char pad20[0x50 - 0x20];
    QUAD v50;                      /* 0x50 */
    QUAD v60;                      /* 0x60 */
    QUAD v70;                      /* 0x70 */
    char pad80[0xC0 - 0x80];
    MMSTATE st;                    /* 0xC0 */
} MMACT;

typedef struct MENUMODELTSK {
    char pad00[0x10];
    unsigned char nState;          /* 0x10: 0 init, 1 wait, 2 open,
                                    * 10 run, 0xFF teardown */
    unsigned char nReady;          /* 0x11 */
    char pad12;
    unsigned char b13;             /* 0x13 */
    char pad14[0x18 - 0x14];
    unsigned short hType;          /* 0x18: 7 = dual-weapon unit */
    char pad1A[0x1C - 0x1A];
    unsigned char nSlot;           /* 0x1C */
    unsigned char bMotion;         /* 0x1D */
    char pad1E[0x20 - 0x1E];
    MMACT *pAct;                   /* 0x20 */
    char pad24[0x28 - 0x24];
    int nLoadStat;                 /* 0x28: 0 idle, <0 failed, >0 loading */
    char pad2C[0x30 - 0x2C];
    struct MENUMODELTSK *pOwner;   /* 0x30: weapon: owning unit task */
    struct MENUMODELTSK *pWeapon[3]; /* 0x34 */
    void (*pFunc)(struct MENUMODELTSK *, void *); /* 0x40 */
    void *pParam;                  /* 0x44 */
    int *pDoneFlag;                /* 0x48 */
} MENUMODELTSK;

extern int MenuModelPauseFlag;

extern void MenuModelWeaponActorSet(MENUMODELTSK *owner, MENUMODELTSK *t);
extern void MenuModelWeaponOpen(MENUMODELTSK *owner, int n, int slot);
extern void MenuModelResourceCancel3(MENUMODELTSK *t);
extern void MenuModelWeaponDispose(MENUMODELTSK *t);
extern void MenuModelDrawTypeMain(MENUMODELTSK *t);
extern void xglTaskWaitRemove(MENUMODELTSK *t);
extern void CreateInit(MENUMODELTSK *t);
extern void ACT_updateMotion(MMACT *act);

/* Weapon-model task: waits for the owning unit's actor to finish
 * loading, opens the weapon model, then runs its callback per frame */
void tskMenuModelWeaponTaskMain(MENUMODELTSK *t)
{
    MMACT *act = t->pAct;
    int stat = t->nLoadStat;
    MENUMODELTSK *owner;
    MMACT *p;

    if (MenuModelPauseFlag & 1) {
        t->nState = 0xFF;
    }
    owner = t->pOwner;
    if (owner == 0) {
        t->nState = 0xFF;
    } else if (owner->hType == 7 && t->nSlot == 1) {
        if (owner->pWeapon[0] == 0) {
            t->nState = 0xFF;
        } else {
            stat = owner->pWeapon[0]->nLoadStat;
        }
    }
    switch (t->nState) {
    case 0:
        t->b13 = 0;
        t->nState = 1;
        /* fallthrough */
    case 1:
        if (stat == 0) {
            break;
        }
        if (stat < 0) {
            t->nState = 0xFF;
            break;
        }
        p = owner->pAct;
        if (p == 0) {
            break;
        }
        if (p->st.fScale120 == 1.0f) {
            t->nState = 2;
        }
        break;
    case 2:
        MenuModelWeaponActorSet(owner, t);
        MenuModelWeaponOpen(t->pOwner, 2, t->nSlot);
        t->nState = 10;
        t->nReady = 1;
        break;
    case 0xFF:
        MenuModelResourceCancel3(t);
        MenuModelWeaponDispose(t);
        if (t->pDoneFlag != 0) {
            *t->pDoneFlag = 0;
        }
        xglTaskWaitRemove(t);
        CreateInit(t);
        return;
    case 10:
    default:
        break;
    }
    if (t->pFunc != 0) {
        t->pFunc(t, t->pParam);
    }
    if (act != 0) {
        MenuModelDrawTypeMain(t);
        if (t->bMotion != 0) {
            ACT_updateMotion(act);
        }
    }
}

extern QUAD MenuModelBasePose;     /* 16-byte constant orientation */

extern void MenuModelUnitActorSet(void);
extern void MenuModelSubWindowBreak(void);
extern void MenuModelUnitDispose(MENUMODELTSK *t);
extern void MenuModelSubWindowMain(MENUMODELTSK *t);
extern void MenuModelNavelMove(MENUMODELTSK *t);
extern void ACT_modelDraw(MMACT *act);
extern void xglStudioChange(int n);
extern int *xglStudioGetCamera2(int n);
extern void *memset(void *, int, unsigned int);

/* Unit-model task: load state machine, teardown of attached weapon
 * tasks, first-frame pose init and the per-frame draw/motion drive */
void tskMenuModelTaskMain(MENUMODELTSK *t)
{
    MMACT *act = t->pAct;
    PIN(int stat, "$5");
    MMSTATE *q;

    stat = t->nLoadStat;

    if (MenuModelPauseFlag & 1) {
        t->nState = 0xFF;
    }
    switch (t->nState) {
    case 0:
        t->b13 = 0;
        t->nState = 1;
        /* fallthrough */
    case 1:
        if (stat == 0) {
            break;
        }
        if (stat < 0) {
            t->nState = 0xFF;
        } else {
            t->nState = 2;
        }
        break;
    case 2:
        MenuModelUnitActorSet();
        t->nState = 10;
        break;
    case 0xFF: {
        MENUMODELTSK **p;
        int i;
        char c;

        MenuModelSubWindowBreak();
        MenuModelResourceCancel3(t);
        MenuModelUnitDispose(t);
        if (t->pDoneFlag != 0) {
            *t->pDoneFlag = 0;
        }
        c = 0xFF;
        p = &t->pWeapon[0];
        i = 2;
        do {
            if (*p != 0) {
                (*p)->nState = c;
                tskMenuModelWeaponTaskMain(*p);
            }
            i--;
            p++;
        } while (i >= 0);
        xglTaskWaitRemove(t);
        CreateInit(t);
        return;
    }
    case 10:
    default:
        break;
    }
    if (t->pFunc != 0) {
        t->pFunc(t, t->pParam);
    }
    if (act == 0) {
        return;
    }
    q = &act->st;
    xglStudioChange(0);
    *xglStudioGetCamera2(0) = 1;
    if (q->nInit == 0) {
        QUAD pos;
        QUAD rot;

        memset(&pos, 0, 16);
        pos.f[3] = 1.0f;
        rot = MenuModelBasePose;
        act->vPos = pos;
        act->v50 = pos;
        act->v60 = rot;
        q->fScale120 = 0;
        q->v00 = pos;
        q->v10 = pos;
        q->v20 = rot;
    }
    if (q->nInit != 0) {
        MenuModelSubWindowMain(t);
        MenuModelNavelMove(t);
        MenuModelDrawTypeMain(t);
    }
    ACT_updateMotion(act);
    ACT_modelDraw(act);
    if (q->nInit == 0) {
        q->v40 = act->v70;
        q->nInit = 1;
        t->nReady = 1;
    }
}
