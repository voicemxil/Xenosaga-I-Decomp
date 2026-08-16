/* Model-system subsystem re-initializers (thin trampolines to CONSTRUCT_*) */

void CONSTRUCT_ALPHA_GROUP(void);
void CONSTRUCT_PARENT_BUF(void);
void CONSTRUCT_MAP_HANDLE(void *handle);
void CONSTRUCT_FADE_CONTROL(void *ctrl);
void CONSTRUCT_MODELSYSTEM(void);

void INIT_ALPHA_GROUP(void)
{
    CONSTRUCT_ALPHA_GROUP();
}

void INIT_PARENT_BUF(void)
{
    CONSTRUCT_PARENT_BUF();
}

void INIT_MAP_HANDLE(void *handle)
{
    CONSTRUCT_MAP_HANDLE(handle);
}

void INIT_FADE_CONTROL(void *ctrl)
{
    CONSTRUCT_FADE_CONTROL(ctrl);
}

void INIT_MODELSYSTEM(void)
{
    CONSTRUCT_MODELSYSTEM();
}

typedef struct {
    int field_00;
    int field_04;
    int field_08;
    int field_0C;
    int field_10;
    int field_14;
    int field_18;
    int field_1C;
    int field_20;
} INIT_BACK_BUFFER_DATA;

extern INIT_BACK_BUFFER_DATA s_inBackBuffer;

void INIT_BACK_BUFFER(void)
{
    s_inBackBuffer.field_10 = -1;
    s_inBackBuffer.field_14 = 0;
    s_inBackBuffer.field_00 = 0;
    s_inBackBuffer.field_04 = 0;
    s_inBackBuffer.field_08 = 0;
    s_inBackBuffer.field_0C = 0;
    s_inBackBuffer.field_18 = 0;
    s_inBackBuffer.field_1C = 0;
    s_inBackBuffer.field_20 = 0;
}

/* ------------------------------------------------------------------ */
/* Map-unit initialisers.                                             */
/*                                                                    */
/* Every field unit (MAPUNIT) is built by a per-kind Init function:    */
/* it composes the unit's local matrix from the authored position and  */
/* rotation, publishes the matrix pointer, then binds the per-frame    */
/* update handler and the resource slots for that kind of unit.        */
/* ------------------------------------------------------------------ */

#include "common.h"

struct MAPUNIT;

/* The authored "set" record the map data supplies for a unit; it lives
 * inline in the unit at +0x1A0.  Only the fields the initialisers touch
 * are named. */
typedef struct MAPUNITSET {
    u8 pad000[0x2];                 /* 0x1A0 */
    u16 nNo;                        /* 0x1A2 */
    u8 pad004[0x29];                /* 0x1A4 */
    s8 nSerial;                     /* 0x1CD */
    u8 pad02E[0x16];                /* 0x1CE */
    int nUnk044;                    /* 0x1E4 */
} MAPUNITSET;

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
    float *pMatrix;                 /* 0x080 */
    u8 pad084[0x1C];                /* 0x084 */
    u8 nIndex;                      /* 0x0A0 */
    u8 nUnk0A1;                     /* 0x0A1 */
    u8 nUnk0A2;                     /* 0x0A2 */
    u8 pad0A3[0x1];                 /* 0x0A3 */
    u16 nAlive;                     /* 0x0A4 */
    short nUnk0A6;                  /* 0x0A6 */
    short nUnk0A8;                  /* 0x0A8 */
    u8 pad0AA[0x16];                /* 0x0AA */
    float fSubPos[4];               /* 0x0C0 */
    int nUnk0D0;                    /* 0x0D0 */
    int nUnk0D4;                    /* 0x0D4 */
    int nUnk0D8[2];                 /* 0x0D8 */
    int pModel[2];                  /* 0x0E0 */
    int nEffect[3];                 /* 0x0E8 */
    u8 pad0F4[0x8];                 /* 0x0F4 */
    int nUnk0FC;                    /* 0x0FC */
    u8 pad100[0xA0];                /* 0x100 */
    MAPUNITSET set;                 /* 0x1A0 */
    u8 pad1E8[0x38];                /* 0x1E8 */
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

extern int uwares_tbl[];
extern int xtxres_tbl[];
extern char printflg;

int printf(const char *, ...);
void xglMatrixStackUnit(void);
void xglMatrixStackTrans(float *);
void xglMatrixStackRotX(float);
void xglMatrixStackRotY(float);
void xglMatrixStackRotZ(float);
void xglMatrixStackSave(float *);
int xglFlagsGet1(int);
void sefDeleteEffectCf(int);

void MAP_updateUnitSymbol(MAPUNIT *);
void MAP_updateUnitSaveSymbol(MAPUNIT *);
void MAP_updateUnitShopSymbol(MAPUNIT *);
void MAP_updateUnitItemSymbol(MAPUNIT *);
void MAP_updateUnitSpecialSymbol(MAPUNIT *);
void SetItemSymbolRsrc(MAPUNIT *);

/* Event symbol (the "!" marker over an event trigger) */
void InitEvsSymbol(MAPUNIT *pUnit)
{
    float *pMatrix;
    MAPUNITSET *pSet;

    pMatrix = pUnit->fMatrix;
    pSet = &pUnit->set;
    xglMatrixStackUnit();
    xglMatrixStackTrans(pUnit->fPos);
    xglMatrixStackRotX(pUnit->fRot[0]);
    xglMatrixStackRotY(pUnit->fRot[1]);
    xglMatrixStackRotZ(pUnit->fRot[2]);
    xglMatrixStackSave(pMatrix);
    pUnit->pMatrix = pMatrix;
    if (pSet->nSerial == -1) {
        pSet->nSerial = 0;
    }
    pSet->nUnk044 = 0;
    pUnit->pUpdate = MAP_updateUnitSymbol;
    pUnit->nFlags |= 0x10000;
    if (printflg) {
        printf("Create Shop Symbol %f %f %f\n", pUnit->fPos[0], pUnit->fPos[1],
               pUnit->fPos[2]);
    }
}

/* Return-point symbol - identical to the event symbol apart from the
 * kind tag the map data carries */
void InitRetSymbol(MAPUNIT *pUnit)
{
    float *pMatrix;
    MAPUNITSET *pSet;

    pMatrix = pUnit->fMatrix;
    pSet = &pUnit->set;
    xglMatrixStackUnit();
    xglMatrixStackTrans(pUnit->fPos);
    xglMatrixStackRotX(pUnit->fRot[0]);
    xglMatrixStackRotY(pUnit->fRot[1]);
    xglMatrixStackRotZ(pUnit->fRot[2]);
    xglMatrixStackSave(pMatrix);
    pUnit->pMatrix = pMatrix;
    if (pSet->nSerial == -1) {
        pSet->nSerial = 0;
    }
    pSet->nUnk044 = 0;
    pUnit->pUpdate = MAP_updateUnitSymbol;
    pUnit->nFlags |= 0x10000;
    if (printflg) {
        printf("Create Shop Symbol %f %f %f\n", pUnit->fPos[0], pUnit->fPos[1],
               pUnit->fPos[2]);
    }
}

/* Shop symbol: binds shop model/texture slot 6 */
void InitShopSymbol(MAPUNIT *pUnit)
{
    float *pMatrix;
    MAPUNITSET *pSet;
    int nModel;
    int nTexture;

    nModel = uwares_tbl[6];
    nTexture = xtxres_tbl[6];
    pMatrix = pUnit->fMatrix;
    pSet = &pUnit->set;
    pUnit->pModel[0] = nModel;
    pUnit->pModel[1] = nTexture;
    xglMatrixStackUnit();
    xglMatrixStackTrans(pUnit->fPos);
    xglMatrixStackRotX(pUnit->fRot[0]);
    xglMatrixStackRotY(pUnit->fRot[1]);
    xglMatrixStackRotZ(pUnit->fRot[2]);
    xglMatrixStackSave(pMatrix);
    pUnit->pMatrix = pMatrix;
    if (pSet->nSerial == -1) {
        pSet->nSerial = 0;
    }
    pSet->nUnk044 = 0;
    pUnit->pUpdate = MAP_updateUnitShopSymbol;
    pUnit->nFlags |= 0x10000;
    pUnit->nFlags |= 0x40000000;
    if (printflg) {
        printf("Create Shop Symbol %f %f %f\n", pUnit->fPos[0], pUnit->fPos[1],
               pUnit->fPos[2]);
    }
}

/* Save point: binds save-point model/texture slot 5 */
void InitSaveSymbol(MAPUNIT *pUnit)
{
    float *pMatrix;
    MAPUNITSET *pSet;
    int nModel;
    int nTexture;

    nModel = uwares_tbl[5];
    nTexture = xtxres_tbl[5];
    pMatrix = pUnit->fMatrix;
    pSet = &pUnit->set;
    pUnit->pModel[0] = nModel;
    pUnit->pModel[1] = nTexture;
    xglMatrixStackUnit();
    xglMatrixStackTrans(pUnit->fPos);
    xglMatrixStackRotX(pUnit->fRot[0]);
    xglMatrixStackRotY(pUnit->fRot[1]);
    xglMatrixStackRotZ(pUnit->fRot[2]);
    xglMatrixStackSave(pMatrix);
    pUnit->pMatrix = pMatrix;
    if (pSet->nSerial == -1) {
        pSet->nSerial = 0;
    }
    pSet->nUnk044 = 0;
    pUnit->pUpdate = MAP_updateUnitSaveSymbol;
    pUnit->nFlags |= 0x10000;
    pUnit->nFlags |= 0x20000000;
    if (printflg) {
        printf("Create Save Symbol %f %f %f\n", pUnit->fPos[0], pUnit->fPos[1],
               pUnit->fPos[2]);
    }
}

/* Special (scripted) symbol - always drawn, always resource-bound */
void InitSpecialSymbol(MAPUNIT *pUnit)
{
    float *pMatrix;
    MAPUNITSET *pSet;

    pMatrix = pUnit->fMatrix;
    pSet = &pUnit->set;
    xglMatrixStackUnit();
    xglMatrixStackTrans(pUnit->fPos);
    xglMatrixStackRotX(pUnit->fRot[0]);
    xglMatrixStackRotY(pUnit->fRot[1]);
    xglMatrixStackRotZ(pUnit->fRot[2]);
    xglMatrixStackSave(pMatrix);
    pUnit->pMatrix = pMatrix;
    if (pSet->nSerial == -1) {
        pSet->nSerial = 0;
    }
    pUnit->pUpdate = MAP_updateUnitSpecialSymbol;
    pUnit->nFlags |= 0x10004;
    pUnit->nFlags |= 0x10000000;
    SetItemSymbolRsrc(pUnit);
}

/* Item symbol: a collectable marker.  Serial numbers in the special
 * range get the scripted handler; anything already collected (its world
 * flag set) is killed off along with its effects. */
void InitItemSymbol(MAPUNIT *pUnit)
{
    float *pMatrix;
    MAPUNITSET *pSet;
    int *pEffect;
    int i;

    pMatrix = pUnit->fMatrix;
    pSet = &pUnit->set;
    xglMatrixStackUnit();
    xglMatrixStackTrans(pUnit->fPos);
    xglMatrixStackRotX(pUnit->fRot[0]);
    xglMatrixStackRotY(pUnit->fRot[1]);
    xglMatrixStackRotZ(pUnit->fRot[2]);
    xglMatrixStackSave(pMatrix);
    pUnit->pMatrix = pMatrix;
    if (pSet->nSerial == -1) {
        pSet->nSerial = 0;
    }
    pUnit->pUpdate = MAP_updateUnitItemSymbol;
    pUnit->nFlags |= 0x10000;
    pUnit->nFlags |= 0x10000000;
    if ((unsigned int)(pUnit->nAlive - 0x7033) < 5) {
        pUnit->pUpdate = MAP_updateUnitSpecialSymbol;
        return;
    }
    if (pSet->nNo >= 602) {
        pUnit->nAlive = -1;
        pUnit->pUpdate = 0;
        pUnit->nUnk008 = 0;
        printf("ItemSymbol serial error!! %d\n", (short)pSet->nNo);
        return;
    }
    if (xglFlagsGet1((short)pSet->nNo + 0x79EC7) == 1) {
        pUnit->nAlive = -1;
        pUnit->pUpdate = 0;
        pEffect = pUnit->nEffect;
        for (i = 0; i < 3; i++) {
            if (*pEffect != 0) {
                sefDeleteEffectCf(*pEffect);
            }
            pEffect++;
        }
        return;
    }
    SetItemSymbolRsrc(pUnit);
}
