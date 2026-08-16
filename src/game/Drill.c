/* Drill - the drill-vehicle sequence: state functions driven from the map
 * unit's work block at +0x1A0, plus the chase camera and the container
 * hit test that spawns the crash effects. */

#include "common.h"
#include "game/actor.h"

/* --- the shared 0x300-byte map-unit record (MapUnit[64]) ------------- */

/* The sef scheduler object the drill and its dust plume ride on -- same
 * layout Java_Effect.c models as EFFECT. */
typedef struct DRILL_EFFECT {
    char  pad_000[0x80];
    float aTranslate[4];            /* 0x080 */
    char  pad_090[0xA0A];
    unsigned char nDisp;            /* 0xA9A */
    char  pad_A9B[5];
} DRILL_EFFECT;

typedef struct MAP_UNIT_WORK {      /* MAP_UNIT + 0x1A0 */
    char  pad_00[2];                /* 0x1A0 */
    unsigned short number;          /* 0x1A2 */
    char  container;                /* 0x1A4 */
    char  field_05;                 /* 0x1A5 */
    char  pad_06[2];
    short cameraMode;               /* 0x1A8 */
    char  pad_0A[0x0A];
    float fStep;                    /* 0x1B4 */
    char  pad_18[0x0C];
    float radius;                   /* 0x1C4 */
    float fCamHeight;               /* 0x1C8 */
    char  pad_2C[6];
    char  field_32;                 /* 0x1D2 */
    unsigned char hitCount;         /* 0x1D3 */
    char  pad_34[8];
    int   field_3C;                 /* 0x1DC */
    int   field_40;                 /* 0x1E0 */
    char  pad_44[8];
    DRILL_EFFECT *pOther;           /* 0x1EC */
    char  pad_50[8];
    unsigned int flags;             /* 0x1F8 */
    char  pad_5C[0x34];
} MAP_UNIT_WORK;                    /* ends at 0x230 */

typedef struct MAP_UNIT {
    unsigned int status;            /* 0x000 */
    int          field_004;         /* 0x004 */
    char         pad_008[0x08];
    float        position[4];       /* 0x010 */
    char         pad_020[0x80];
    unsigned char team;             /* 0x0A0 */
    char         pad_0A1;
    unsigned char state;            /* 0x0A2 */
    char         pad_0A3;
    short        id;                /* 0x0A4 */
    char         pad_0A6[0xFA];
    MAP_UNIT_WORK work;             /* 0x1A0 */
    DRILL_EFFECT *pRider;           /* 0x230 */
    char         pad_234[0xCC];
} MAP_UNIT;

extern MAP_UNIT MapUnit[64];

extern int xglFlagsSet1(int nFlag, int nValue);

/* Release every container the drill sequence had picked up. */
void DrillClearContainer(void)
{
    MAP_UNIT *unit = MapUnit;
    int i;

    for (i = 0; i < 64; i++, unit++) {
        unsigned int id = (unsigned short)unit->id;

        if (id >= 0x7014 && id <= 0x7027) {
            unit->id = -1;
            unit->field_004 = 0;
        } else if (unit->status & 0x100000) {
            unit->id = -1;
            unit->field_004 = 0;
        }
    }
}

/* --- chase camera ---------------------------------------------------- */

typedef struct DRILL_CAMANGLE {     /* DRILL_CAMERA + 0x090 */
    char  pad_00[0x04];
    float fUnk04;                   /* 0x094 */
    char  pad_08[0x08];
    float fAngleX;                  /* 0x0A0 */
    float fAngleY;                  /* 0x0A4 */
    float fRoll;                    /* 0x0A8 */
    char  pad_1C[0x04];
} DRILL_CAMANGLE;

typedef struct DRILL_CAMERA {
    int   nActive;                  /* 0x000 */
    int   nUnk04;                   /* 0x004 */
    char  pad_008[0x88];
    DRILL_CAMANGLE ang;             /* 0x090 */
    float vUnkB0[3];                /* 0x0B0 */
    char  pad_0BC[0x14];
    float vEye[3];                  /* 0x0D0 */
    char  pad_0DC[0x04];
    float vUnkE0[3];                /* 0x0E0 */
    char  pad_0EC[0x504];
} DRILL_CAMERA;

extern float xglSin(float fAngle);
extern float xglCos(float fAngle);

/* Place the eye on a 15/20-unit arm swung by the camera's two angles. */
void DrillCalcCameraPos(DRILL_CAMERA *pCam)
{
    DRILL_CAMANGLE *pAng = &pCam->ang;
    float fArm;

    pCam->vEye[0] = 0.7f;
    pCam->vEye[1] = 0.5f;
    pCam->vEye[2] = -17.9f;
    pCam->vEye[1] -= xglSin(pAng->fAngleX) * 20.0f;
    fArm = xglCos(pAng->fAngleX) * 15.0f;
    pCam->vEye[0] += fArm * xglSin(pAng->fAngleY);
    fArm = fArm * xglCos(pAng->fAngleY);
    pCam->vEye[2] += fArm;
    pCam->vEye[0] -= 1.2f;
}

/* Snap the camera back to the preset for the unit's current camera mode. */
void DrillResetCameraPos(MAP_UNIT *pUnit, DRILL_CAMERA *pCam)
{
    pCam->nUnk04 = 0;
    if (pUnit->work.cameraMode == 0) {
        pCam->ang.fAngleX = -0.20943953f;
        pCam->ang.fAngleY = 1.5707964f;
        pCam->ang.fRoll = 0.0f;
        DrillCalcCameraPos(pCam);
    } else if (pUnit->work.cameraMode == 1) {
        pCam->ang.fAngleX = -0.20943953f;
        pCam->ang.fAngleY = 0.0f;
        pCam->ang.fRoll = 0.0f;
        DrillCalcCameraPos(pCam);
    } else if (pUnit->work.cameraMode == 2) {
        pCam->ang.fAngleX = -1.5707964f;
        pCam->ang.fAngleY = 0.0f;
        pCam->ang.fRoll = 0.0f;
        DrillCalcCameraPos(pCam);
    }
}

/* Clear the drill's progress flags and let go of every crate it holds. */
void DrillResetFlag(MAP_UNIT *pUnit)
{
    MAP_UNIT_WORK *w = &pUnit->work;
    MAP_UNIT *unit;
    int i;

    w->flags &= ~0x1F0;
    for (i = 501; i < 541; i++) {
        xglFlagsSet1(0x79EC7 + i, 0);
    }
    unit = MapUnit;
    for (i = 0; i < 64; i++, unit++) {
        if (unit->id == 0x7000 || unit->id == 0x7009) {
            unsigned int n = unit->work.number;

            if (n >= 501 && n <= 550) {
                unit->id = -1;
                unit->field_004 = 0;
            }
        }
    }
}

/* --- pad ------------------------------------------------------------- */

typedef struct DRILL_PADDATA {
    char           pad_00[0x28];
    unsigned short nButton;         /* 0x28 */
    unsigned short nPress;          /* 0x2A */
    char           pad_2C[0x3C];
} DRILL_PADDATA;

extern DRILL_PADDATA PadData[2];

extern void xglFontDebugPrintf(int x, int y, char *fmt, ...);
extern DRILL_CAMERA *xglStudioGetActiveCamera(void);
extern void xglSoundEffectNormalID(int nCode, int nRand);
extern void DrillCameraControlFunc(MAP_UNIT *pUnit, DRILL_CAMERA *pCam);
extern int HitCheckContainerPosSize(void *position, int excluded, float size);
extern int HitCheckEnemy(void *position, float size);
extern int sefCreateEffectCf(int a, int b, int c);
extern void xglSoundEffectStopID(int nCode, int nArg);
extern void sefRewindEffectCf(DRILL_EFFECT *pEffect);
extern void Vibration_Set_Strong(int a, int b, int c);
extern void ACT_setMotion2(ACTOR *pActor, int a, int b);

typedef struct DRILL_GLS {
    char  pad_00[4];
    ACTOR *pPlayer;                 /* 0x04 */
    char  pad_08[8];
    unsigned int nFlags;            /* 0x10 */
    char  pad_14[0xD0];
} DRILL_GLS;

extern DRILL_GLS GameLoopState;

/* Park the drill at its garage pose and, if the player was riding it,
 * put them back on their feet. */
void DrillPowerOffFunc(MAP_UNIT *pUnit)
{
    MAP_UNIT_WORK *w = &pUnit->work;
    DRILL_CAMERA *pCam;
    DRILL_EFFECT *pEffect;
    ACTOR *pPlayer;
    signed char nRide;

    pPlayer = GameLoopState.pPlayer;
    xglFontDebugPrintf(8, 16, "DRILL POWEROFF");
    pUnit->position[0] = -2.9f;
    pUnit->position[1] = 4.5f;
    pUnit->position[2] = -12.5f;
    w->field_3C = 0;
    w->field_05 = 0;
    w->hitCount = 0;
    w->field_32 = 0;
    pCam = xglStudioGetActiveCamera();
    pCam->ang.fUnk04 = w->fCamHeight;
    w->cameraMode = 0;
    xglSoundEffectStopID(0x30096, 0);
    xglSoundEffectStopID(0x30098, 0);
    nRide = w->container;
    if (nRide == 1) {
        pUnit->state = nRide;
        GameLoopState.nFlags |= 0x10000;
        ACT_setMotion2(pPlayer, 0, 9);
        DrillResetCameraPos(pUnit, pCam);
        w->field_3C = nRide;
        w->field_40 = 0;
        GameLoopState.nFlags &= ~0x80000;
        w->flags &= 0x7FFFFFFF;
    }
    pEffect = pUnit->pRider;
    pEffect->nDisp = 0;
    pEffect = w->pOther;
    pEffect->nDisp = 0;
}

/* Drive the drill down the shaft while the button is held; stop at the
 * bottom of the bore. */
void DrillZMoveFunc(MAP_UNIT *pUnit)
{
    MAP_UNIT_WORK *w = &pUnit->work;
    DRILL_EFFECT *pEffect;

    xglFontDebugPrintf(8, 16, "DRILL ZMOVE");
    Vibration_Set_Strong(128, 128, 2);
    w->cameraMode = 1;
    if (PadData[0].nButton & 0x80) {
        pUnit->position[2] -= w->fStep;
    } else {
        pUnit->state = 3;
        xglSoundEffectStopID(0x30096, 0);
        xglSoundEffectNormalID(0x30097, 0);
    }
    if (pUnit->position[2] < -23.3f) {
        pUnit->position[2] = -23.3f;
        pUnit->state = 3;
        xglSoundEffectStopID(0x30096, 0);
        xglSoundEffectNormalID(0x30097, 0);
    }
    pEffect = pUnit->pRider;
    sefRewindEffectCf(pEffect);
    pEffect->nDisp = 1;
    pEffect->aTranslate[2] = pUnit->position[2];
    pEffect = w->pOther;
    sefRewindEffectCf(pEffect);
    pEffect->nDisp = 1;
    pEffect->aTranslate[0] = -4.0f;
    pEffect->aTranslate[1] = 1.5f;
    pEffect->aTranslate[2] = pUnit->position[2] + 0.5f;
}

/* Idle at the bottom of the shaft: cycle the camera preset on SELECT and
 * hand control back to the walking state on START. */
void DrillZStopFunc(MAP_UNIT *pUnit)
{
    MAP_UNIT_WORK *w = &pUnit->work;
    DRILL_CAMERA *pCam;
    DRILL_EFFECT *pActor;

    xglFontDebugPrintf(8, 16, "DRILL ZSTANDBY");
    pCam = xglStudioGetActiveCamera();
    DrillCameraControlFunc(pUnit, pCam);
    if (PadData[0].nPress & 2) {
        DrillResetCameraPos(pUnit, pCam);
        w->cameraMode++;
        w->cameraMode %= 3;
    }
    if (PadData[0].nPress & 0x80) {
        pCam->vUnkE0[0] = 0.0f;
        pCam->vUnkE0[1] = 0.0f;
        pCam->vUnkE0[2] = 0.0f;
        pCam->vUnkB0[0] = 0.0f;
        pCam->vUnkB0[1] = 0.0f;
        pCam->vUnkB0[2] = 0.0f;
        pCam->nUnk04 = 0;
        pUnit->state = 4;
        xglSoundEffectNormalID(0x30098, 0);
    }
    pActor = pUnit->pRider;
    pActor->nDisp = 0;
    pActor = w->pOther;
    pActor->nDisp = 0;
}

/* Test the drill head against the crates; mark the one it grabbed and
 * spawn the debris effect for its size class.
 *
 * TODO near-miss (45/74 words, all inside the three id range tests).
 * The original's first test is evaluated in int domain
 * (`addiu -28692` + `sltiu 4`, no mask) while the second and third are
 * masked to 16 bits with `addiu` + `andi 0xffff` + `sltiu`.  Every
 * spelling swept -- `unsigned short` local, `short` local, `int` local,
 * direct field reads, explicit `(unsigned short)(id - c) < n` casts, and
 * an `unsigned short` temporary assigned from an int subtraction --
 * produces either all three tests unmasked (int domain) or all three
 * masked with the constant folded into the 16-bit representative
 * (`li 0x8fe8` + `addu` instead of `addiu -28696`).  gcc only emits the
 * original's unfolded `addiu` + separate `andi` when the truncation is
 * introduced after constant folding has run, which no source-level
 * spelling reproduces.  Everything up to and including the id load
 * matches byte for byte.
 */
int DrillHitCheck(MAP_UNIT *pUnit)
{
    MAP_UNIT_WORK *w = &pUnit->work;
    int n;
    unsigned short id;

    n = HitCheckContainerPosSize(pUnit->position, pUnit->team, w->radius);
    if (n != -1) {
        MAP_UNIT_WORK *cw = &MapUnit[n].work;

        cw->container = 1;
        w->hitCount++;
        id = MapUnit[n].id;
        if (id >= 0x7014 && id <= 0x7017) {
            return sefCreateEffectCf(1752, 0, 0);
        }
        if (id >= 0x7018 && id <= 0x7023) {
            return sefCreateEffectCf(1753, 0, 0);
        }
        if (id >= 0x7024 && id <= 0x7027) {
            return sefCreateEffectCf(1754, 0, 0);
        }
    } else {
        return HitCheckEnemy(pUnit->position, w->radius);
    }
}
