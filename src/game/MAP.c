#include "matching.h"

/* Map unit layer - slot allocation, resource binding, per-frame update and draw */

typedef unsigned char u8;
typedef unsigned short u16;

struct MAPUNIT;

typedef struct {
    int nUnk000;                    /* 0x000 */
    int nUnk004;                    /* 0x004 */
    u8 pad008[0x4];                 /* 0x008 */
    int nUnk00C;                    /* 0x00C */
    short nGroup;                   /* 0x010 */
    u8 pad012[0x2];                 /* 0x012 */
    int nUnk014;                    /* 0x014 */
    int nUnk018;                    /* 0x018 */
    int nUnk01C;                    /* 0x01C */
    int nUnk020;                    /* 0x020 */
    void (*pFunc[4])(struct MAPUNIT *);/* 0x024 */
    u8 pad034[0x20C];               /* 0x034 */
    int nUnk240;                    /* 0x240 */
    int nUnk244;                    /* 0x244 */
    int nUnk248;                    /* 0x248 */
    float fUnk24C;                  /* 0x24C */
    float fUnk250;                  /* 0x250 */
    int nUnk254;                    /* 0x254 */
    int nUnk258;                    /* 0x258 */
    float fUnk25C;                  /* 0x25C */
} UNITSEQ;

typedef struct {
    int nUnk000;                    /* 0x100 */
    float fUnk004;                  /* 0x104 */
    float fUnk008;                  /* 0x108 */
    u8 pad00C[0x8];                 /* 0x10C */
    short nMotion;                  /* 0x114 */
    u8 pad016[0x2];                 /* 0x116 */
    short nUnk018;                  /* 0x118 */
    u8 pad01A[0x6];                 /* 0x11A */
} MAPANM;

typedef struct MAPUNIT {
    int nFlags;                     /* 0x000 */
    void (*pUpdate)(struct MAPUNIT *);/* 0x004 */
    int nUnk008;                    /* 0x008 */
    void (*pDraw)(struct MAPUNIT *);/* 0x00C */
    float fPos[4];                  /* 0x010 */
    float fRot[3];                  /* 0x020 */
    u8 pad02C[0x4];                 /* 0x02C */
    float fScale[4];                /* 0x030 */
    float fMatrix[16];              /* 0x040 */
    u8 nUnk080;                     /* 0x080 */
    u8 pad081[0x1F];                /* 0x081 */
    u8 nIndex;                      /* 0x0A0 */
    u8 nUnk0A1;                     /* 0x0A1 */
    u8 nUnk0A2;                     /* 0x0A2 */
    u8 pad0A3[0x1];                 /* 0x0A3 */
    short nAlive;                   /* 0x0A4 */
    short nUnk0A6;                  /* 0x0A6 */
    short nUnk0A8;                  /* 0x0A8 */
    u8 pad0AA[0x16];                /* 0x0AA */
    float fSubPos[4];               /* 0x0C0 */
    int nUnk0D0;                    /* 0x0D0 */
    int nUnk0D4;                    /* 0x0D4 */
    int nUnk0D8[2];                 /* 0x0D8 */
    int pModel[2];                  /* 0x0E0 */
    int nUnk0E8[3];                 /* 0x0E8 */
    u8 pad0F4[0x8];                 /* 0x0F4 */
    int nUnk0FC;                    /* 0x0FC */
    MAPANM anm;                     /* 0x100 */
    u8 pad120[0x100];               /* 0x120 */
    float fUnk220[4];               /* 0x220 */
    int nUnk230;                    /* 0x230 */
    int nUnk234;                    /* 0x234 */
    int nUnk238;                    /* 0x238 */
    float fUnk23C;                  /* 0x23C */
    int mdl[0x28];                  /* 0x240 */
    int nUnk2E0;                    /* 0x2E0 */
    int nUnk2E4;                    /* 0x2E4 */
    int nUnk2E8;                    /* 0x2E8 */
    int nUnk2EC;                    /* 0x2EC */
    u8 pad2F0[0x10];                /* 0x2F0 */
} MAPUNIT;

typedef struct {
    int nUnk0;                      /* 0x00 */
    int nUnk4;                      /* 0x04 */
    u8 pad8[0x3C];                  /* 0x08 */
    float fUnk44;                   /* 0x44 */
} PLAYWORK;

typedef struct {
    int nUnk0;                      /* 0x00 */
    MAPUNIT *pPlayer;               /* 0x04 */
    u8 pad008[0x14];                /* 0x08 */
    int nUnk01C;                    /* 0x1C */
} MAPGAMELOOP;

extern MAPUNIT MapUnit[];
extern UNITSEQ unitSequence[];
extern MAPUNIT *D_00338684[];
extern int xtxres_tbl[];
extern int uwares_tbl[];
extern char D_004BE308[];
extern char D_004BE318[];
extern char D_004BE328[];
extern char D_004CA028[];
extern char D_004D2130[];
extern float D_004D7EB4;
extern float D_004D7EB8;
extern float D_004D7EBC;
extern float D_004D8784;
extern u8 UseTestPath;
extern u8 D_0037915B[];
extern MAPGAMELOOP GameLoopState;

extern void unit6003_update(MAPUNIT *);
extern void ACT_updateEnemy(MAPUNIT *);

int printf(const char *, ...);
void LOG(char *);
int CheckDist3D(float *, float *);
float UnduGet(float, float);
void ANM_resetDefault(MAPANM *, int);
PLAYWORK *PLAY_getCurrent(void);
void xglMatrixStackUnit(void);
void xglMatrixStackTrans(float *);
void xglMatrixStackRotX(float);
void xglMatrixStackRotY(float);
void xglMatrixStackRotZ(float);
void xglMatrixStackScale(float *);
void xglMatrixStackSave(float *);
void SEQ_motionUnit(MAPUNIT *);
void nmlModelSetTexture(int);
void nmlModelSetPlace(float *);
void nmlModelSetClip(int);
void nmlModelEntry(int);
int *RES_loadFile(int, int, int, int);
void MDL_create(int *, int);
void MAP_drawUnitAt(MAPUNIT *);
void MAP_updateUnitDefault(MAPUNIT *);
float Get_Angle(float *, float *);
extern short D_003386D0[];

void MapInit(void)
{
    UseTestPath = 0;
}

int MapGetNo(void)
{
    return D_003386D0[0];
}

/* Select the map data path for the current test-path setting */
char *MAP_getPath(void)
{
    switch (UseTestPath) {
    case 0:
    default:
        return D_004BE308;
    case 1:
        return D_004BE318;
    case 2:
        return D_004BE328;
    }
}

/* Bind the uwamono model/texture resources for a unit */
void MAP_LoadUwamonoResource(MAPUNIT *pUnit, int nId)
{
    int nIndex;
    int nTex;
    int nMdl;

    printf(D_004CA028);
    if (nId != 0x7012 && nId >= 0x1000) {
        nIndex = nId - 0x7001;
        nMdl = uwares_tbl[nIndex];
        nTex = xtxres_tbl[nIndex];
        pUnit->pModel[1] = nTex;
        pUnit->pModel[0] = nMdl;
    }
}

/* Spin a save-point symbol and report the player's distance to it */
int MAP_updateUnitSaveSymbol(MAPUNIT *pUnit)
{
    int nDist;

    pUnit->fRot[1] += D_004D7EB4;
    nDist = CheckDist3D(D_00338684[0]->fPos, pUnit->fPos);
    __asm__ volatile("" ::: "memory");
    return nDist;
}

/* Spin a shop symbol and report the player's distance to it */
int MAP_updateUnitShopSymbol(MAPUNIT *pUnit)
{
    int nDist;

    pUnit->fRot[1] += D_004D7EB8;
    nDist = CheckDist3D(D_00338684[0]->fPos, pUnit->fPos);
    __asm__ volatile("" ::: "memory");
    return nDist;
}

/* TODO: near-miss - gcc emits the gp-relative load before the field load */
/* Spin a symbol unit at the standard rate */
void MAP_updateUnitSymbol(MAPUNIT *pUnit)
{
    float f;

    f = pUnit->fRot[1];
    __asm__ volatile("" : "+f"(f));
    pUnit->fRot[1] = f + D_004D7EBC;
}

/* TODO: near-miss - fix_cc_asm adds one extra hazard nop after cvt.s.w */
/* Draw an item-box unit: body model plus its lid transform */
void MAP_drawUnitItemBox(MAPUNIT *pUnit)
{
    float *pSub;

    pSub = pUnit->fSubPos;
    xglMatrixStackUnit();
    xglMatrixStackTrans(pSub);
    xglMatrixStackRotX(pUnit->nUnk0A8 / 9.0f);
    xglMatrixStackSave((float *)pUnit->nUnk230);
    nmlModelSetTexture(pUnit->pModel[1]);
    nmlModelSetPlace(pUnit->fMatrix);
    nmlModelSetClip(1);
    nmlModelEntry(pUnit->pModel[0]);
    xglMatrixStackUnit();
    xglMatrixStackTrans(pSub);
    xglMatrixStackSave((float *)pUnit->nUnk230);
}

/* Load the model and texture resources for one map unit */
void MAP_loadUnitResource(MAPUNIT *pUnit, int nId)
{
    int *pRes;
    int pModel;
    int pTexture;

    pRes = RES_loadFile(-1, 4, nId, 0);
    if (pRes == 0) {
        pRes = RES_loadFile(-1, 4, nId & 0x7F00, 0);
    }
    if (pRes != 0) {
        pModel = pRes[0];
        pTexture = pRes[1];
    } else {
        pTexture = 0;
        pModel = 0;
    }
    if (pModel == 0) {
        LOG(D_004D2130);
    }
    pUnit->pModel[1] = pTexture;
    pUnit->nUnk0D4 = pModel;
    pUnit->pModel[0] = pModel;
    MDL_create(pUnit->mdl, pModel);
}

/* Allocate a map unit slot and give it default transform state */
MAPUNIT *MAP_createUnit(int nIndex, int nId)
{
    MAPUNIT *pUnit;
    void (*pDraw)(MAPUNIT *);
    int i;

    if (nIndex < 0) {
        for (i = 0; i < 0x40; i++) {
            if (MapUnit[i].nAlive < 0) {
                nIndex = i;
                break;
            }
        }
        if (nIndex < 0) {
            return 0;
        }
    }
    pUnit = &MapUnit[nIndex];
    pUnit->nIndex = nIndex;
    pUnit->nAlive = nId;
    pUnit->fScale[0] = 1.0f;
    pUnit->fScale[1] = 1.0f;
    pUnit->fScale[2] = 1.0f;
    pUnit->fScale[3] = 1.0f;
    pUnit->fUnk220[3] = 1.0f;
    pUnit->fUnk220[0] = 0.0f;
    pUnit->fUnk220[1] = 0.0f;
    pUnit->fUnk220[2] = 0.0f;
    pUnit->nUnk0D0 = 0;
    if (nId == 0x6025) {
        pDraw = unit6003_update;
    } else {
        pDraw = 0;
    }
    pUnit->pUpdate = 0;
    pUnit->nUnk008 = 0;
    pUnit->nUnk0A2 = 0;
    pUnit->pDraw = pDraw;
    for (i = 2; i >= 0; i--) {
        pUnit->nUnk0E8[i] = 0;
    }
    return pUnit;
}

/* Run the update handlers of every live map unit */
void MAP_updateUnit(void)
{
    MAPUNIT *pUnit;
    int i;

    pUnit = MapUnit;
    for (i = 0x3F; i >= 0; i--) {
        if (pUnit->nAlive >= 0 && (pUnit->nFlags & 0x10) == 0) {
            if (pUnit->pUpdate != 0) {
                pUnit->pUpdate(pUnit);
            }
            if (pUnit->pDraw != 0) {
                pUnit->pDraw(pUnit);
            }
        }
        pUnit++;
    }
}

/* Draw every live map unit */
void MAP_drawUnit(void)
{
    MAPUNIT *pUnit;
    int i;

    pUnit = MapUnit;
    for (i = 0x3F; i >= 0; i--) {
        if (pUnit->nAlive >= 0) {
            MAP_drawUnitAt(pUnit);
        }
        pUnit++;
    }
}

/* Snap a position to the ground height, leaving it alone off the map */
void MAP_getHeight(float *pPos)
{
    float fHeight;
    int bValid;

    fHeight = UnduGet(pPos[0], pPos[2]);
    bValid = (fHeight != -1000.0f);
    if (bValid) {
        pPos[1] = fHeight;
    }
}

/* TODO: near-miss - register allocation is shifted by one (a1/a2 vs a2/a3) */
/* Create a unit together with its sequence-table entry */
void MAP_createUnitPeer(int nIndex, int nId)
{
    MAPUNIT *pUnit;
    UNITSEQ *pSeq;

    pUnit = MAP_createUnit(nIndex, nId);
    if (pUnit != 0) {
        pSeq = &unitSequence[pUnit->nIndex];
        pUnit->anm.nUnk018 = 0x1FF;
        pUnit->anm.fUnk008 = D_004D8784;
        pUnit->nUnk0A1 = 0;
        pUnit->fScale[3] = 1.0f;
        pUnit->fScale[2] = 1.0f;
        pUnit->fScale[1] = 1.0f;
        pUnit->fScale[0] = 1.0f;
        pUnit->nUnk0D0 = 0;
        pUnit->nUnk0A8 = 0;
        if (nId != 0x6025) {
            pUnit->pUpdate = 0;
        }
        pSeq->fUnk25C = 1.0f;
        pSeq->nGroup = -1;
        pSeq->pFunc[3] = 0;
        pSeq->nUnk000 = 0;
        pSeq->nUnk004 = 0;
        pSeq->nUnk240 = 0;
        pSeq->nUnk244 = 0;
        pSeq->nUnk248 = 0;
        pSeq->fUnk24C = 1.0f;
        pSeq->fUnk250 = 1.0f;
        pSeq->nUnk254 = 0;
        pSeq->nUnk258 = 0;
        pSeq->nUnk00C = 0;
        pSeq->nUnk014 = 0;
        pSeq->nUnk018 = 0;
        pSeq->nUnk01C = 0;
        pSeq->nUnk020 = 0;
        pSeq->pFunc[0] = 0;
        pSeq->pFunc[1] = 0;
        pSeq->pFunc[2] = 0;
    }
}

/* Set a unit's motion index and reset its animation */
void MAP_setUnitMotion(MAPUNIT *pUnit, short nMotion)
{
    pUnit->anm.nMotion = nMotion;
    ANM_resetDefault(&pUnit->anm, pUnit->nUnk0D8[1]);
}

/* Per-frame update for units that need no motion handling */
void MAP_updateUnitMotion(MAPUNIT *pUnit)
{
}

/* Call a handler for every unit belonging to one sequence group */
int MAP_callUnitGroup(int nGroup, void (*pFunc)(MAPUNIT *))
{
    UNITSEQ *pSeq;
    MAPUNIT *pUnit;
    int i;

    if (pFunc != 0) {
        PIN(MAPUNIT *pU, "$2");
        PIN(UNITSEQ *pS, "$3");
        pS = unitSequence;
        pU = MapUnit;
        pUnit = pU;
        pSeq = pS;
        for (i = 0x3F; i >= 0; i--) {
            if (pSeq->nGroup == nGroup) {
                pFunc(pUnit);
            }
            pSeq++;
            pUnit++;
        }
    }
    return 0;
}

/* Build the matrix for a motion-pack unit and pull its play rate */
void MAP_updateUnitMPack(MAPUNIT *pUnit)
{
    PLAYWORK *pPlay;

    pPlay = PLAY_getCurrent();
    xglMatrixStackUnit();
    xglMatrixStackTrans(pUnit->fPos);
    xglMatrixStackRotX(pUnit->fRot[0]);
    xglMatrixStackRotY(pUnit->fRot[1]);
    xglMatrixStackRotZ(pUnit->fRot[2]);
    xglMatrixStackScale(pUnit->fScale);
    xglMatrixStackSave(pUnit->fMatrix);
    SEQ_motionUnit(pUnit);
    pUnit->anm.fUnk004 = pPlay->fUnk44;
}

/* Run a unit's sequence callbacks, then rebuild its matrix */
void MAP_updateUnitSequence(MAPUNIT *pUnit)
{
    UNITSEQ *pSeq;
    void (**ppFunc)(MAPUNIT *);
    float fHeight;
    int i;

    pSeq = &unitSequence[pUnit->nIndex];
    ppFunc = pSeq->pFunc;
    for (i = 3; i >= 0; i--) {
        if (*ppFunc != 0) {
            (*ppFunc)(pUnit);
        }
        ppFunc++;
    }
    if (pSeq->nUnk004 == 0) {
        pUnit->nFlags &= ~2;
        pSeq->pFunc[0] = 0;
        pSeq->pFunc[1] = 0;
        pSeq->nUnk000 = 0;
        pSeq->pFunc[2] = 0;
        pSeq->pFunc[3] = 0;
    }
    if (pUnit->nFlags & 0x100) {
        fHeight = UnduGet(pUnit->fPos[0], pUnit->fPos[2]);
        if (fHeight != -1000.0f) {
            pUnit->fPos[1] = fHeight;
        }
    }
    xglMatrixStackUnit();
    xglMatrixStackTrans(pUnit->fPos);
    xglMatrixStackRotX(pUnit->fRot[0]);
    xglMatrixStackRotY(pUnit->fRot[1]);
    xglMatrixStackRotZ(pUnit->fRot[2]);
    xglMatrixStackScale(pUnit->fScale);
    xglMatrixStackSave(pUnit->fMatrix);
    __asm__ volatile("" ::: "memory");
}

/* Default per-frame unit update: ground snap plus matrix build */
void MAP_updateUnitDefault(MAPUNIT *pUnit)
{
    float fHeight;

    if (pUnit->nFlags & 0x100) {
        fHeight = UnduGet(pUnit->fPos[0], pUnit->fPos[2]);
        if (fHeight != -1000.0f) {
            pUnit->fPos[1] = fHeight;
        }
    }
    xglMatrixStackUnit();
    xglMatrixStackTrans(pUnit->fPos);
    xglMatrixStackRotX(pUnit->fRot[0]);
    xglMatrixStackRotY(pUnit->fRot[1]);
    xglMatrixStackRotZ(pUnit->fRot[2]);
    xglMatrixStackScale(pUnit->fScale);
    xglMatrixStackSave(pUnit->fMatrix);
}

/* Clear the unit sequence table and reset every unit to the default handler */
void MAP_initUnitSequance(void)
{
    UNITSEQ *pSeq;
    MAPUNIT *pUnit;
    int i;

    pUnit = MapUnit;
    pSeq = unitSequence;
    for (i = 0x3F; i >= 0; i--) {
        pSeq->nUnk000 = 0;
        pSeq->nUnk004 = 0;
        pSeq->nUnk00C = 0;
        pSeq->nUnk014 = 0;
        pSeq->nUnk018 = 0;
        pSeq->nUnk01C = 0;
        pSeq->nUnk020 = 0;
        pSeq->pFunc[0] = 0;
        pSeq->pFunc[1] = 0;
        pSeq->pFunc[2] = 0;
        pSeq->pFunc[3] = 0;
        pSeq++;
    }
    for (i = 0x3F; i >= 0; i--) {
        pUnit->nUnk0D0 = 0;
        pUnit->nFlags = 0;
        pUnit->nUnk0A1 = 0;
        pUnit->nUnk0A8 = 0;
        pUnit->pUpdate = MAP_updateUnitDefault;
        pUnit++;
    }
}

/* TODO: near-match (LOGIC, 10 of 49 words) - moving the pTargetPos derivation
 * into the else branch (from unconditional) improved 12->10 diffs but gcc
 * still hoists "MapUnit+nOffset+0x10" above the branch test as a pure
 * expression; original computes it in the bnez delay slot. Register/schedule
 * tie-break, not clearly reachable from source; a (char*)pTarget+0x10 form
 * regresses to LENGTH (47 vs 49 words) so was reverted. */
/* Update an enemy marker from its linked map unit */
void MAP_updateUnitEnemy(MAPUNIT *pUnit)
{
    int nIndex;
    int nOffset;
    int nEnemy;
    MAPUNIT *pTarget;
    float *pTargetPos;

    nEnemy = pUnit->nUnk080;
    nIndex = D_0037915B[nEnemy * 0x38B0];
    nOffset = nIndex * 0x300;
    pTarget = (MAPUNIT *)((char *)MapUnit + nOffset);
    if (pTarget->nUnk0A2 == 0) {
        pUnit->nFlags |= 8;
    } else {
        float fAngle;

        pTargetPos = (float *)((char *)MapUnit + nOffset + 0x10);
        *(unsigned long long *)&pUnit->fPos[0] =
            *(unsigned long long *)&pTargetPos[0];
        *(unsigned long long *)&pUnit->fPos[2] =
            *(unsigned long long *)&pTargetPos[2];
        pUnit->nFlags &= ~8;
        pUnit->pUpdate = ACT_updateEnemy;
        GameLoopState.nUnk01C = 0;
        fAngle = Get_Angle(pUnit->fPos, GameLoopState.pPlayer->fPos);
        *(float *)((char *)pUnit + 0x54) = fAngle;
        *(float *)((char *)pUnit + 0x9E4) = fAngle;
    }
}

/* TODO: near-match (LENGTH) - 73 instructions versus 75 original; the three
 * descending clear loops and all field stores are reconstructed, but gcc
 * omits the original pre-loop nop before the second and third clear loops and
 * schedules the initial nested-work pointer ahead of the first flag store. */
/* Reset every map-unit slot and its embedded resource arrays */
void MAP_initUnit(void)
{
    MAPUNIT *pUnit = MapUnit;
    int *p;
    char *pWork;
    int i;
    int j;

    for (i = 0; i < 0x40; i++, pUnit++) {
        pUnit->nFlags = 0;
        pWork = (char *)pUnit + 8;
        pUnit->nAlive = -1;
        pUnit->pUpdate = 0;
        pUnit->nUnk008 = 0;
        pUnit->pDraw = 0;
        pUnit->nUnk0D4 = 0;
        pUnit->nUnk0D0 = 0;
        pUnit->fScale[0] = 1.0f;
        pUnit->fScale[1] = 1.0f;
        pUnit->fScale[2] = 1.0f;
        pUnit->fScale[3] = 1.0f;
        pUnit->fPos[0] = 0.0f;
        pUnit->fPos[1] = 0.0f;
        pUnit->fPos[2] = 0.0f;
        pUnit->fPos[3] = 1.0f;
        pUnit->fUnk220[0] = 0.0f;
        pUnit->fUnk220[1] = 0.0f;
        pUnit->fUnk220[2] = 0.0f;
        pUnit->fUnk220[3] = 1.0f;
        pUnit->nUnk2E4 = 0;
        pUnit->nUnk2E8 = 0;
        pUnit->nUnk234 = 0;
        pUnit->nUnk238 = 0x10;
        pUnit->nUnk2E0 = 0;
        pUnit->fUnk23C = 0.5f;
        pUnit->nUnk2EC = 0;
        pUnit->nUnk0A6 = 0;
        p = &pUnit->nUnk0D8[1];
        j = 1;
clear_d8:
        j--;
        *p = 0;
        p--;
        if (j >= 0) {
            goto clear_d8;
        }
        p = &pUnit->pModel[1];
        j = 1;
clear_model:
        j--;
        *p = 0;
        p--;
        if (j >= 0) {
            goto clear_model;
        }
        p = (int *)(pWork + 0xE8);
        j = 2;
clear_e8:
        j--;
        *p = 0;
        p--;
        if (j >= 0) {
            goto clear_e8;
        }
        pUnit->nUnk0FC = 0;
        pUnit->mdl[0] = 0;
    }
}
