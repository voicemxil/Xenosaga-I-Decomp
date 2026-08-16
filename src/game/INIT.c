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
    short nKind;                    /* 0x1A0 */
    u16 nNo;                        /* 0x1A2 */
    u8 pad004[0x1];                 /* 0x1A4 */
    s8 nUnk005;                     /* 0x1A5 */
    s8 nContainer;                  /* 0x1A6 */
    s8 nHasContainer;               /* 0x1A7 */
    u8 pad008[0x20];                /* 0x1A8 */
    float fUnk028;                  /* 0x1C8 */
    u8 pad02C[0x1];                 /* 0x1CC */
    s8 nSerial;                     /* 0x1CD */
    s8 nUnk02E;                     /* 0x1CE */
    s8 nUnk02F;                     /* 0x1CF */
    u8 pad030[0x4];                 /* 0x1D0 */
    int nUnk034;                    /* 0x1D4 */
    int nUnk038;                    /* 0x1D8 */
    u8 pad03C[0x8];                 /* 0x1DC */
    int nUnk044;                    /* 0x1E4 */
    u8 pad048[0x4];                 /* 0x1E8 */
    int nUnk04C;                    /* 0x1EC */
} MAPUNITSET;

typedef struct MAPUNIT {
    int nFlags;                     /* 0x000 */
    void (*pUpdate)(struct MAPUNIT *);/* 0x004 */
    void (*pDraw)(struct MAPUNIT *);/* 0x008 */
    int nUnk00C;                    /* 0x00C */
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
    short nUnk0AA;                  /* 0x0AA */
    u8 pad0AC[0x4];                 /* 0x0AC */
    float fUnk0B0[4];               /* 0x0B0 */
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
    u8 pad1F0[0x8];                 /* 0x1F0 */
    int nUnk1F8;                    /* 0x1F8 */
    u8 pad1FC[0x24];                /* 0x1FC */
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

/* nml model handle: only the parts count is needed here */
typedef struct {
    u8 pad000[0x40];                /* 0x00 */
    int nPartsNum;                  /* 0x40 */
} NMLMODEL;

/* The map-parts resource the current stage loaded */
typedef struct {
    u8 pad000[0x4];                 /* 0x00 */
    NMLMODEL *pModel;               /* 0x04 */
} MAPPARTSRES;

/* The player actor as the boot path sees it */
typedef struct {
    int nFlags;                     /* 0x00 */
    u8 pad004[0xC];                 /* 0x04 */
    float fPos[4];                  /* 0x10 */
} INITACTOR;

/* The game-loop work block. It is a single very large object: the
 * per-scene reset writes a float at +0x29F44, which the original
 * reaches as a large constant offset from the same base register (gas
 * expands it to lui/addu/swc1), so the whole 172KB really is one
 * struct and not a run of separate globals. */
typedef struct {
    u8 pad000[0x4];                 /* 0x00000 */
    INITACTOR *pPlayer;             /* 0x00004 */
    u8 pad008[0x8];                 /* 0x00008 */
    int nFlags;                     /* 0x00010 */
    int nUnk14;                     /* 0x00014 */
    int nUnk18;                     /* 0x00018 */
    u8 pad01C[0x4];                 /* 0x0001C */
    int nUnk20;                     /* 0x00020 */
    u8 pad024[0x30];                /* 0x00024 */
    MAPPARTSRES *pParts;            /* 0x00054 */
    u8 pad058[0x188];               /* 0x00058 */
    int nUnk1E0;                    /* 0x001E0 */
    u8 pad1E4[0x29D60];             /* 0x001E4 */
    float fUnk29F44;                /* 0x29F44 */
} INITGAMELOOP;

/* Sound driver work area */
typedef struct {
    u8 pad000[0xA0];                /* 0x000 */
    int nUnk0A0;                    /* 0x0A0 */
    u8 aUnk0A4[0x800];              /* 0x0A4 */
} SOUNDWORK;

/* GS clear-environment block */
typedef struct {
    u8 pad000[0x20];                /* 0x00 */
    int nUnk20;                     /* 0x20 */
    int nUnk24;                     /* 0x24 */
    int nUnk28;                     /* 0x28 */
    int nUnk2C;                     /* 0x2C */
} CLEARENV;

/* One sef effect instance; only the local offset and the enable byte
 * the drill setup touches are named. */
typedef struct {
    u8 pad000[0x80];                /* 0x000 */
    float fOffset[3];               /* 0x080 */
    u8 pad08C[0xA0E];               /* 0x08C */
    u8 nUnkA9A;                    /* 0xA9A */
} SEFEFFECT;

typedef struct {
    u8 pad000[0x94];                /* 0x00 */
    float fUnk094;                  /* 0x94 */
} INITCAMERA;

/* Collision/height query parameters (see src/game/Undu.c) */
typedef struct {
    int field_00;
    int field_04;
    short field_08;
    short field_0A;
    u8 field_0C;
    u8 field_0D;
    short pad_0E;
    float field_10;
    float field_14;
    int field_18;
    int field_1C;
    long long field_20;
    long long field_28;
    long long field_30;
    long long field_38;
} UNDU_PARAM;

/* The position vector as the item-box registration copies it: the
 * original moves all 16 bytes with two aligned 64-bit ld/sd pairs, which
 * needs a type whose alignment says the vector really is 8-byte aligned
 * (it is -- MAPUNIT is 0x300 bytes and fPos sits at 0x10). */
typedef struct {
    long long nLo;                  /* 0x00 */
    long long nHi;                  /* 0x08 */
} INITVECTOR;

/* Model resource: the parts table lives at +nPartsOffset+0x40 */
typedef struct {
    u8 pad000[0x30];                /* 0x00 */
    float fPos[3];                  /* 0x30 */
} MODELPARTS;

typedef struct {
    u8 pad000[0x50];                /* 0x00 */
    int nPartsOffset;               /* 0x50 */
} MODELRES;

extern INITGAMELOOP GameLoopState;
extern MAPUNIT MapUnit[];
extern int *pDrillFlag;
extern INITACTOR *tActor;
extern CLEARENV ClearEnv;
extern int WorkEnd;
extern SOUNDWORK SoundWork;
extern char GameIdLight[];
extern int UseVMFlag;
extern int saveEffe;
extern int shopEffe;
extern int evsEffe;
extern int retEffe;
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
void MAP_updateUnitKoware(MAPUNIT *);
void MAP_updateUnitSaveSymbol(MAPUNIT *);
void MAP_updateUnitShopSymbol(MAPUNIT *);
void MAP_updateUnitItemSymbol(MAPUNIT *);
void MAP_updateUnitSpecialSymbol(MAPUNIT *);
void SetItemSymbolRsrc(MAPUNIT *);
void CreateUwamonoCommon(MAPUNIT *);
void SetHideObject(MAPUNIT *);
void GetPartsPos(MAPUNIT *);
void GetPartsSize(MAPUNIT *);
void nmlModelSetPartsVisible(NMLMODEL *, int, int);
void InitMapKoware(MAPUNIT *);
void InitMapDoor(MAPUNIT *);
void InitDrill(MAPUNIT *);
void InitItemBox(MAPUNIT *);
void InitMapTrap(MAPUNIT *);
void MAP_updateUnitDrill(MAPUNIT *);
void MAP_updateUnitItemBox(MAPUNIT *);
void MAP_drawUnitItemBox(MAPUNIT *);
void UnduParamInit(UNDU_PARAM *);
int UnduDataGetHeader(int, int);
float UnduCheck(float *, int, UNDU_PARAM *);
void DrillResetFlag(MAPUNIT *);
void SetContainer(int);
INITCAMERA *xglStudioGetActiveCamera(void);
SEFEFFECT *sefCreateEffectCf(int, int, int);
void xglStudioGetLight(void **);
void xglLightSetDefault(void *);
void xglRenderClearFrame(void);
void xglCdInitial(void);
void xglCdReset(void);
void xglCdSetCallback(int);
void GameResourceInit(int, int);
void ACT_init(void);
void ACT_DrawShadowInit(void);
void MSG_init(void);
void TWSYS_init(void);
void MapInit(void);
void MAP_initUnit(void);
void Enemy_SystemInit(void);
void InitUwamonoSys(void);
void GameCfPlayerMoveInit(void);
INITACTOR *ACT_create(int, int);
void ACT_initMotion(INITACTOR *);
void ACT_loadMotion(INITACTOR *, int, int);
void ACT_loadResource(INITACTOR *, int);
void ACT_setMotion(INITACTOR *, int);
int sceSifInitRpc(int);
int sceCdInit(int);
int sceSifRebootIop(char *);
int sceSifSyncIop(void);
void sceSifInitIopHeap(void);
void sceSifLoadFileReset(void);
int sceCdMmode(int);
void sceFsReset(void);
void xglCdSifLoadModule(char *, int);
int sceCdPOffCallback(void *, int);
void xglCdPowerOffCB(void);
void xglSoundInitial(void);
void xglPadInitial(void);
void xglMcInitial(void);
void xglTaskInitial(int, int, int);
void xglDmaInitial(void);
void xglGeometryInit(void);
void xglPacketInit(void);
void xglRenderInit(void);
void xglFontInitial(void);
void xglMovieInit(void);
void *memset(void *, int, unsigned int);
void xglMenuInitial(void);
void xglFontLoad(int, int);
void Vibration_Stop(void);
void xglPadRead(void);
void EnemySound_StopAll(int);
void UwamonoBgmFadeOut(void);
void GameCameraReset(void);
void XTK_setWindowOwner(int);
void ResetShootSys(void);
void sefInitEffectCf(void);
void *xglStudioGetLight2(void);
void Player_System_Init(INITACTOR *);
void xglRenderCopyDisp2Draw(void);

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
        pUnit->pDraw = 0;
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

/* Map "parts" unit - dispatch on the authored kind to the real builder */
void InitMapParts(MAPUNIT *pUnit)
{
    pUnit->pModel[0] = 0;
    switch (pUnit->set.nKind) {
    case 2:
        InitMapKoware(pUnit);
        return;
    case 1:
        InitMapDoor(pUnit);
        return;
    case 20:
        InitDrill(pUnit);
        return;
    default:
        printf("Illegal Parts Type %d\n", pUnit->set.nKind);
        pUnit->pUpdate = 0;
        return;
    }
}

/* Breakable ("kowaremono") scenery: bind the shared parts model, hide the
 * piece that has already been broken, and report it in debug builds */
void InitMapKoware(MAPUNIT *pUnit)
{
    MAPUNITSET *pSet;
    NMLMODEL *pModel;
    short nKind;

    pSet = &pUnit->set;
    nKind = pSet->nNo;
    pModel = GameLoopState.pParts->pModel;
    if (nKind < 0) {
        printf("Illegal Koware Kind = %d\n", nKind);
    }
    GetPartsPos(pUnit);
    GetPartsSize(pUnit);
    if (pSet->nUnk044 == -1) {
        pSet->nUnk044 = 1;
    }
    pUnit->nFlags = 0;
    pUnit->pUpdate = MAP_updateUnitKoware;
    CreateUwamonoCommon(pUnit);
    SetHideObject(pUnit);
    if ((short)pUnit->nAlive >= 0 && (short)pUnit->nAlive < pModel->nPartsNum) {
        nmlModelSetPartsVisible(pModel, (short)pUnit->nAlive, 1);
    } else {
        printf("\xb2\xf5\xa4\xec\xca\xaa\xc0\xb8\xc0\xae\xa5\xa8\xa5\xe9\xa1\xbc "
               "\xa5\xd1\xa1\xbc\xa5\xc4\xa5\xca\xa5\xf3\xa5\xd0\xa1\xbc\xa4\xac"
               "\xc9\xd4\xc0\xb5\xa4\xc7\xa4\xb9 %d\n", (short)pUnit->nAlive);
        pUnit->nAlive = -1;
        pUnit->pUpdate = 0;
    }
    if (printflg) {
        printf("Create Kowaremono id=%d kind=%d pos x=%f y=%f z=%f\n",
               (short)pUnit->nAlive, nKind, pUnit->fPos[0], pUnit->fPos[1],
               pUnit->fPos[2]);
    }
}

/* Generic authored-object ("uwamono") builder: validate the placement
 * against the terrain, then hand off to the per-kind initialiser */
void InitUwamono(MAPUNIT *pUnit)
{
    UNDU_PARAM param;
    MAPUNITSET *pSet;

    pSet = &pUnit->set;
    pUnit->nFlags &= ~4;
    if (pSet->nKind == 0) {
        printf("Illegal UWA_NULL!  id=%d serial=%d\n", (short)pUnit->nAlive,
               pUnit->nIndex);
        pUnit->nAlive = -1;
        pUnit->pUpdate = 0;
        return;
    }
    if (pSet->nUnk02E == -1) {
        pSet->nUnk02E = 0;
    }
    UnduParamInit(&param);
    param.field_18 = UnduDataGetHeader(0, 0x8000);
    if (pSet->nUnk02F != 0) {
        param.field_08 |= 0x20;
    }
    if (UnduCheck(pUnit->fPos, 0, &param) == -1000.0f) {
        if (pSet->nUnk02F == 0) {
            pSet->nUnk02E = 0;
        }
    }
    if ((short)pUnit->nAlive < 4096) {
        InitMapParts(pUnit);
        return;
    }
    switch (pSet->nKind) {
    case 4:
        InitItemBox(pUnit);
        return;
    case 5:
        InitItemSymbol(pUnit);
        return;
    case 3:
        InitMapTrap(pUnit);
        return;
    case 6:
        InitSaveSymbol(pUnit);
        return;
    case 7:
        InitShopSymbol(pUnit);
        return;
    case 8:
        InitEvsSymbol(pUnit);
        return;
    case 9:
        InitRetSymbol(pUnit);
        return;
    case 10:
        InitSpecialSymbol(pUnit);
        return;
    default:
        printf("Illegal Uwamono Type %d\n", pSet->nKind);
        pUnit->pUpdate = 0;
        return;
    }
}

/* The drill vehicle: three sef effects hung off the unit, with 1539 as
 * the fallback whenever the real effect id fails to spawn */
void InitDrill(MAPUNIT *pUnit)
{
    MAPUNITSET *pSet;
    SEFEFFECT *pEffect;
    INITCAMERA *pCamera;

    pSet = &pUnit->set;
    printf("InitDrill D1=%d D2=%d D3=%d\n", pSet->nUnk034, pSet->nUnk038,
           (short)pUnit->nAlive);
    GetPartsPos(pUnit);
    pSet->nSerial = 0;
    pUnit->nUnk0A2 = 0;
    pUnit->pUpdate = MAP_updateUnitDrill;
    pUnit->nUnk0A8 = 0;
    pUnit->nUnk0AA = 0;
    pDrillFlag = &pUnit->nUnk1F8;
    if (pSet->nHasContainer != 0) {
        DrillResetFlag(pUnit);
        SetContainer(pSet->nContainer);
    }
    pCamera = xglStudioGetActiveCamera();
    pSet->fUnk028 = pCamera->fUnk094;
    pEffect = sefCreateEffectCf(1662, 0, 0);
    if (pEffect == 0) {
        pEffect = sefCreateEffectCf(1539, 0, 0);
    }
    pUnit->nEffect[0] = (int)pEffect;
    pEffect->fOffset[0] = -3.2f;
    pEffect->fOffset[1] = 6.67f;
    pEffect->fOffset[2] = -12.5f;
    pEffect->nUnkA9A = 0;
    pEffect = sefCreateEffectCf(1661, 0, 0);
    if (pEffect == 0) {
        pEffect = sefCreateEffectCf(1539, 0, 0);
    }
    pUnit->nUnk230 = (int)pEffect;
    pEffect->fOffset[2] = -12.5f;
    pEffect->fOffset[0] = -3.03f;
    pEffect->fOffset[1] = 6.23f;
    pEffect->nUnkA9A = 0;
    pEffect = sefCreateEffectCf(1581, 0, 0);
    pSet->nUnk04C = (int)pEffect;
    pEffect->nUnkA9A = 0;
    DrillResetFlag(pUnit);
}

/* Per-scene reset: lighting, the CD streamer, every game subsystem and
 * the player actor itself */
void InitCf(void)
{
    void *light[4];

    xglStudioGetLight(light);
    xglLightSetDefault(light[0]);
    xglRenderClearFrame();
    ClearEnv.nUnk20 = 0;
    ClearEnv.nUnk24 = 0;
    ClearEnv.nUnk28 = 0;
    ClearEnv.nUnk2C = 0;
    xglCdInitial();
    xglCdReset();
    xglCdSetCallback(0);
    GameResourceInit(0x7000000, 0x6000000);
    ACT_init();
    ACT_DrawShadowInit();
    MSG_init();
    TWSYS_init();
    MapInit();
    MAP_initUnit();
    Enemy_SystemInit();
    InitUwamonoSys();
    GameCfPlayerMoveInit();
    tActor = ACT_create(0, 1);
    ACT_initMotion(tActor);
    ACT_loadMotion(tActor, 1, 1);
    ACT_loadResource(tActor, 1);
    ACT_setMotion(tActor, 2);
    GameLoopState.pPlayer = tActor;
    tActor->fPos[0] = -14.0f;
    tActor->fPos[2] = 18.0f;
    tActor->nFlags |= 0x10000;
}

/* Cold boot: bring up the IOP, load its modules, then every xgl service */
void InitializeSystem(void)
{
    sceSifInitRpc(0);
    sceCdInit(0);
    while (sceSifRebootIop("cdrom0:\\IOP\\IOPRP24D.IMG;1") == 0) {
        ;
    }
    while (sceSifSyncIop() == 0) {
        ;
    }
    sceSifInitRpc(0);
    sceSifInitIopHeap();
    sceSifLoadFileReset();
    sceCdInit(0);
    sceCdMmode(2);
    sceFsReset();
    xglCdSifLoadModule("sio2man", 0);
    xglCdSifLoadModule("mcman", 0);
    xglCdSifLoadModule("mcserv", 0);
    xglCdSifLoadModule("padman", 0);
    xglCdSifLoadModule("libsd", 0);
    xglCdSifLoadModule("ssd", 0);
    xglCdSifLoadModule("rssd", 0);
    sceCdPOffCallback(xglCdPowerOffCB, 0);
    WorkEnd = 0xA80000;
    xglSoundInitial();
    xglCdInitial();
    xglPadInitial();
    xglMcInitial();
    xglTaskInitial(0, 0, 0);
    xglDmaInitial();
    xglGeometryInit();
    xglPacketInit();
    xglRenderInit();
    xglFontInitial();
    xglMovieInit();
    xglMenuInitial();
}

/* Treasure chest: pick the chest model by serial, hang its lid transform
 * off the model's parts table, and register the box in the map-unit list */
/* TODO: near-miss (10/170 words, REGISTER class) - the two per-serial
 * model/texture temporaries and the 6/57 type constant land in
 * $a1/$a0/$v0 where the original used $a0/$a1 (and $a1 again for the
 * default arm's texture).  Swept: statement order inside and across the
 * three switch arms, an explicit goto-shared tail, direct stores in the
 * default arm, declaration order, and hoisting the scrutinee - every
 * variant is >= 9 words and several are far worse.  Everything else in
 * the function, including the 64-bit MapUnit vector copy, matches. */
void InitItemBox(MAPUNIT *pUnit)
{
    MAPUNITSET *pSet;
    MODELRES *pRes;
    MODELPARTS *pParts;
    MAPUNIT *pSlot;
    float *pMatrix;
    float *pPos;
    int *pEffect;
    int nModel;
    int nTexture;
    int i;

    pSet = &pUnit->set;
    switch ((short)pUnit->nAlive) {
    case 0x7005:
        nModel = uwares_tbl[4];
        nTexture = xtxres_tbl[4];
        pUnit->pModel[0] = nModel;
        pUnit->pModel[1] = nTexture;
        pSet->nUnk034 = 6;
        break;
    case 0x7008:
        nModel = uwares_tbl[7];
        nTexture = xtxres_tbl[7];
        pUnit->pModel[0] = nModel;
        pUnit->pModel[1] = nTexture;
        pSet->nUnk034 = 57;
        break;
    default:
        nModel = uwares_tbl[4];
        nTexture = xtxres_tbl[4];
        pUnit->pModel[0] = nModel;
        pUnit->pModel[1] = nTexture;
        break;
    }
    pRes = (MODELRES *)pUnit->pModel[0];
    pParts = (MODELPARTS *)((char *)pRes + pRes->nPartsOffset + 0x40);
    pUnit->nUnk230 = (int)pParts;
    pUnit->fSubPos[0] = pParts->fPos[0];
    pUnit->fSubPos[1] = pParts->fPos[1];
    pUnit->fSubPos[2] = pParts->fPos[2];
    pUnit->fSubPos[3] = 1.0f;
    xglMatrixStackUnit();
    xglMatrixStackTrans(pUnit->fSubPos);
    xglMatrixStackSave((float *)pUnit->nUnk230);
    if (pSet->nSerial == -1) {
        pSet->nSerial = 2;
    }
    pSet->nUnk044 = 0;
    pPos = pUnit->fPos;
    pMatrix = pUnit->fMatrix;
    pUnit->nFlags |= 4;
    pUnit->fUnk0B0[0] = 0.3f;
    pUnit->fUnk0B0[1] = 0.5f;
    pUnit->fUnk0B0[2] = 0.2f;
    xglMatrixStackUnit();
    xglMatrixStackTrans(pPos);
    xglMatrixStackRotX(pUnit->fRot[0]);
    xglMatrixStackRotY(pUnit->fRot[1]);
    xglMatrixStackRotZ(pUnit->fRot[2]);
    xglMatrixStackSave(pMatrix);
    pUnit->pMatrix = pMatrix;
    pUnit->pUpdate = MAP_updateUnitItemBox;
    pUnit->pDraw = MAP_drawUnitItemBox;
    pUnit->nFlags |= 0x10000;
    pUnit->nFlags |= 0x8000000;
    pUnit->nUnk0A1 = 0;
    pUnit->nUnk0A8 = 0;
    pUnit->nUnk0AA = 0;
    pUnit->nUnk0A2 = 0;
    if (pSet->nUnk005 != -1) {
        pSlot = &MapUnit[pSet->nUnk005];
        pSlot->nFlags |= 4;
        *(INITVECTOR *)pSlot->fPos = *(INITVECTOR *)pUnit->fPos;
    }
    if (pSet->nNo >= 602) {
        pUnit->nAlive = -1;
        pUnit->pUpdate = 0;
        pUnit->pDraw = 0;
        printf("ItemBox serial error!! %d\n", (short)pSet->nNo);
        return;
    }
    if (xglFlagsGet1((short)pSet->nNo + 0x79EC7) == 1) {
        pUnit->nUnk0A8 = 16;
        pUnit->nUnk0A2 = 3;
        pEffect = pUnit->nEffect;
        for (i = 2; i >= 0; i--) {
            if (*pEffect != 0) {
                sefDeleteEffectCf(*pEffect);
            }
            pEffect++;
        }
    }
}

/* TODO: near-miss (18/128 words).  Two residuals:
 *  - the `GameLoopState.fUnk29F44 = 3.0f` store.  The original
 *    materialises the constant at the very top of the flags block but
 *    stores it AFTER the nUnk20/nUnk14 stores; putting the assignment
 *    early reproduces the early lui/mtc1 (56 -> 18 diffs) but also
 *    moves the store early.  A --rotate of the three-line window cannot
 *    fire because fix_cc_asm wraps the `s.s` macro in .set mips1/mips3
 *    and --rotate requires contiguous instruction lines.
 *  - the 32-word descending clear.  The original addresses it as
 *    &GameLoopState then a SEPARATE `addiu +480`, gcc folds the 480
 *    into the %lo; the extra instruction also shifts the loop-head
 *    alignment pad, so the one folded addiu costs three words.
 *    Swept: pointer-plus-index, char* offset and two index-range loop
 *    forms, all much worse.
 * Everything else, including the thirteen-mask flag chain, matches.
 *
 * Per-battle/per-scene system reset: reload the font for the current
 * disc region, silence sound and vibration, rebuild the actor and map
 * layers, clear the run-time half of the game-loop flags, and reset all
 * sixteen id-lights. */
void InitCfSystem(void)
{
    char *pLight;
    int *pWork;
    int i;

    if ((GameLoopState.nUnk18 & 0xF0000000) == 0) {
        xglFontLoad(1, 0);
    } else {
        xglFontLoad(0, 0);
    }
    Vibration_Stop();
    xglPadRead();
    SoundWork.nUnk0A0 = 0;
    memset(SoundWork.aUnk0A4, 0, 2048);
    EnemySound_StopAll(1);
    UwamonoBgmFadeOut();
    ACT_init();
    MAP_initUnit();
    GameCameraReset();
    GameLoopState.fUnk29F44 = 3.0f;
    GameLoopState.nFlags &= ~0x10000;
    GameLoopState.nFlags &= ~0x20000;
    GameLoopState.nFlags &= ~0x4000;
    GameLoopState.nFlags &= ~0x8000;
    GameLoopState.nFlags &= ~0x40000;
    GameLoopState.nFlags &= ~0x400000;
    GameLoopState.nFlags &= ~0x800000;
    GameLoopState.nFlags &= ~0x20000000;
    GameLoopState.nFlags &= ~0x40000000;
    GameLoopState.nFlags &= ~0x1000;
    GameLoopState.nFlags &= ~0x2000;
    GameLoopState.nFlags &= ~0x100000;
    GameLoopState.nFlags &= ~0x1;
    GameLoopState.nUnk20 |= 0xF;
    GameLoopState.nUnk14 = GameLoopState.nFlags;
    GameLoopState.nFlags |= 0x80000;
    UseVMFlag = 0;
    XTK_setWindowOwner(0);
    ResetShootSys();
    sefInitEffectCf();
    saveEffe = 0;
    shopEffe = 0;
    evsEffe = 0;
    retEffe = 0;
    xglLightSetDefault(xglStudioGetLight2());
    Player_System_Init(GameLoopState.pPlayer);
    pLight = GameIdLight;
    for (i = 15; i >= 0; i--) {
        xglLightSetDefault(pLight);
        pLight += 240;
    }
    pWork = &GameLoopState.nUnk1E0;
    for (i = 31; i >= 0; i--) {
        *pWork = 0;
        pWork--;
    }
    xglRenderCopyDisp2Draw();
}

/* The six remaining Init* functions in this game -- InitTLB,
 * InitTLB32MB, InitTLBFunctions, InitAlarm, InitThread and InitExecPS2
 * -- are Sony libkernel, NOT game code, and cannot be built here. Two
 * independent tells: their prologues save callee-saved registers on a
 * 16-byte stride (2.9-ee) rather than 8 (2.96), and InitTLB ends in
 * `jal InitTLB32MB` + return where 2.96 turns the same C into a
 * sibling call (`j InitTLB32MB`). They need the SDK compiler, which
 * configure.py selects only for files named sce*; see the report.
 */
