/* Drill - the drill-vehicle sequence: state functions driven from the map
 * unit's work block at +0x1A0, plus the chase camera and the container
 * hit test that spawns the crash effects. */

#include "common.h"
#include "game/actor.h"

/* --- the shared 0x300-byte map-unit record (MapUnit[64]) ------------- */

/* A 16-byte vector that gcc will block-copy with ld/sd: the long long
 * arm is what raises the alignment to 8 (same idiom as Hit.c). */
typedef union DRILL_VECTOR {
    float f[4];
    long long ll[2];
} DRILL_VECTOR;

struct MAP_UNIT;

/* The sef scheduler object the drill and its dust plume ride on -- same
 * layout Java_Effect.c models as EFFECT. */
typedef struct DRILL_EFFECT {
    char  pad_000[0x80];
    DRILL_VECTOR vTranslate;        /* 0x080 */
    char  pad_090[0x634];
    struct MAP_UNIT *pCasterUnit;   /* 0x6C4 */
    char  pad_6C8[0x3C4];
    int   nFlags;                   /* 0xA8C */
    char  pad_A90[0x0A];
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
    char  pad_0A[2];
    float field_0C;                 /* 0x1AC */
    float field_10;                 /* 0x1B0 */
    float fStep;                    /* 0x1B4 */
    float field_18;                 /* 0x1B8 */
    float field_1C;                 /* 0x1BC */
    float field_20;                 /* 0x1C0 */
    float radius;                   /* 0x1C4 */
    float fCamHeight;               /* 0x1C8 */
    signed char field_2C;           /* 0x1CC */
    char  pad_2D;
    signed char field_2E;           /* 0x1CE */
    char  pad_2F[3];
    char  field_32;                 /* 0x1D2 */
    signed char hitCount;           /* 0x1D3 */
    char  pad_34[8];
    int   field_3C;                 /* 0x1DC */
    int   field_40;                 /* 0x1E0 */
    char  pad_44[8];
    DRILL_EFFECT *pOther;           /* 0x1EC */
    char  pad_50[8];
    int   flags;                    /* 0x1F8 */
    int   field_5C;                 /* 0x1FC */
    char  pad_60[0x20];
} MAP_UNIT_WORK;                    /* ends at 0x220 */

typedef struct MAP_UNIT {
    unsigned int status;            /* 0x000 */
    int          field_004;         /* 0x004 */
    char         pad_008[0x08];
    DRILL_VECTOR position;          /* 0x010 */
    int          field_20;          /* 0x020 */
    float        fAngleY;           /* 0x024 */
    int          field_28;          /* 0x028 */
    char         pad_02C[0x54];
    void        *pMatrix;           /* 0x080 */
    char         pad_084[0x1C];
    unsigned char team;             /* 0x0A0 */
    char         pad_0A1;
    unsigned char state;            /* 0x0A2 */
    char         pad_0A3;
    short        id;                /* 0x0A4 */
    char         pad_0A6[2];
    short        field_A8;          /* 0x0A8 */
    short        field_AA;          /* 0x0AA */
    char         pad_0AC[0x3C];
    DRILL_EFFECT *pDust;            /* 0x0E8 */
    char         pad_0EC[0xB4];
    MAP_UNIT_WORK work;             /* 0x1A0 */
    float        fCrashAngleX;      /* 0x220 */
    float        fCrashAngleY;      /* 0x224 */
    char         pad_228[8];
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

typedef struct DRILL_CAMWORK {      /* DRILL_CAMERA + 0x090 */
    char  pad_00[0x04];
    float fUnk04;                   /* 0x094 */
    char  pad_08[0x08];
    float fAngleX;                  /* 0x0A0 */
    float fAngleY;                  /* 0x0A4 */
    float fRoll;                    /* 0x0A8 */
    char  pad_1C[0x04];
    float vUnkB0[3];                /* 0x0B0 */
    char  pad_2C[0x04];
    float vUnkC0[3];                /* 0x0C0 */
    char  pad_3C[0x04];
    float vEye[3];                  /* 0x0D0 */
    char  pad_4C[0x04];
    float vUnkE0[3];                /* 0x0E0 */
    char  pad_5C[0x04];
    float vUnkF0[3];                /* 0x0F0 */
    char  pad_6C[0x04];
} DRILL_CAMWORK;

typedef struct DRILL_CAMERA {
    int   nActive;                  /* 0x000 */
    int   nUnk04;                   /* 0x004 */
    char  pad_008[0x88];
    DRILL_CAMWORK w;                /* 0x090 */
    char  pad_100[0x4F0];
} DRILL_CAMERA;

extern float xglSin(float fAngle);
extern float xglCos(float fAngle);

/* Place the eye on a 15/20-unit arm swung by the camera's two angles. */
void DrillCalcCameraPos(DRILL_CAMERA *pCam)
{
    DRILL_CAMWORK *pAng = &pCam->w;
    float fArm;

    pCam->w.vEye[0] = 0.7f;
    pCam->w.vEye[1] = 0.5f;
    pCam->w.vEye[2] = -17.9f;
    pCam->w.vEye[1] -= xglSin(pAng->fAngleX) * 20.0f;
    fArm = xglCos(pAng->fAngleX) * 15.0f;
    pCam->w.vEye[0] += fArm * xglSin(pAng->fAngleY);
    fArm = fArm * xglCos(pAng->fAngleY);
    pCam->w.vEye[2] += fArm;
    pCam->w.vEye[0] -= 1.2f;
}

/* Snap the camera back to the preset for the unit's current camera mode. */
void DrillResetCameraPos(MAP_UNIT *pUnit, DRILL_CAMERA *pCam)
{
    pCam->nUnk04 = 0;
    if (pUnit->work.cameraMode == 0) {
        pCam->w.fAngleX = -0.20943953f;
        pCam->w.fAngleY = 1.5707964f;
        pCam->w.fRoll = 0.0f;
        DrillCalcCameraPos(pCam);
    } else if (pUnit->work.cameraMode == 1) {
        pCam->w.fAngleX = -0.20943953f;
        pCam->w.fAngleY = 0.0f;
        pCam->w.fRoll = 0.0f;
        DrillCalcCameraPos(pCam);
    } else if (pUnit->work.cameraMode == 2) {
        pCam->w.fAngleX = -1.5707964f;
        pCam->w.fAngleY = 0.0f;
        pCam->w.fRoll = 0.0f;
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
    char           pad_2C[0x36];
    unsigned char  nStickLX;        /* 0x62 */
    unsigned char  nStickLY;        /* 0x63 */
    char           pad_64[2];
    signed char    nStickRX;        /* 0x66 */
    signed char    nStickRY;        /* 0x67 */
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
extern void CallMethod(char *pName);
extern void xglMatrixStackUnit(void);
extern void xglMatrixStackRotY(float fAngle);
extern void xglMatrixStackSave(void *pMtx);

typedef struct DRILL_GLS {
    char  pad_00[4];
    ACTOR *pPlayer;                 /* 0x04 */
    char  pad_08[8];
    unsigned int nFlags;            /* 0x10 */
    char  pad_14[0xD0];
} DRILL_GLS;

extern DRILL_GLS GameLoopState;

/* Retract the rig: raise it clear, slide it back along the rail, then
 * lift it out of the shaft and hand control back to the field.
 *
 * Byte-exact.  Block-scoped fixed-register C locals recover the original
 * allocation of the three independent constants.  The remaining scheduler
 * tie is an audited right rotation of `lw pOther; li.s 0.5f`: the instruction
 * multiset and data dependencies are unchanged. */
void DrillReturnFunc(MAP_UNIT *pUnit)
{
    MAP_UNIT_WORK *w = &pUnit->work;
    DRILL_EFFECT *pEffect;
    DRILL_CAMERA *pCam;
    float fNew;

    xglFontDebugPrintf(8, 16, "DRILL RETURN");
    if (pUnit->field_A8 > 0) {
        pUnit->field_A8--;
        pUnit->fAngleY += (float)pUnit->field_A8 * 0.1f;
        pUnit->field_20 = 0;
        pUnit->field_28 = 0;
        xglMatrixStackUnit();
        xglMatrixStackRotY(pUnit->fAngleY);
        xglMatrixStackSave(pUnit->pMatrix);
        DrillHitCheck(pUnit);
        return;
    }
    if (pUnit->position.f[1] < 4.5f) {
        fNew = pUnit->position.f[1] + w->field_1C;
        pUnit->position.f[1] = fNew;
        if (fNew >= 4.5f) {
            pUnit->position.f[1] = 4.5f;
            xglSoundEffectNormalID(0x30098, 0);
        }
        return;
    }
    pUnit->position.f[1] = 4.5f;
    if (pUnit->position.f[0] > -2.9f) {
        fNew = pUnit->position.f[0] - w->field_18;
        pUnit->position.f[0] = fNew;
        if (fNew <= -2.9f) {
            pUnit->position.f[0] = -2.9f;
            xglSoundEffectStopID(0x30098, 0);
            xglSoundEffectNormalID(0x30096, 0);
        }
        pEffect = pUnit->pDust;
        sefRewindEffectCf(pEffect);
        pEffect->nDisp = 1;
        pEffect->vTranslate.f[0] = pUnit->position.f[0];
        pEffect->vTranslate.f[2] = pUnit->position.f[2];
        pEffect = w->pOther;
        sefRewindEffectCf(pEffect);
        pEffect->nDisp = 1;
        pEffect->vTranslate.f[0] = pUnit->position.f[0];
        pEffect->vTranslate.f[1] = 6.0f;
        pEffect->vTranslate.f[2] = pUnit->position.f[2];
        return;
    }
    pEffect = pUnit->pDust;
    pUnit->position.f[0] = -2.9f;
    pEffect->nDisp = 0;
    if (pUnit->position.f[2] < -12.5f) {
        register float fBack __asm__("$f3");
        register float fHeight __asm__("$f2");
        register float fHalf __asm__("$f1");

        fNew = pUnit->position.f[2] + w->field_20;
        pUnit->position.f[2] = fNew;
        if (fNew >= -12.5f) {
            pUnit->position.f[2] = -12.5f;
            xglSoundEffectStopID(0x30096, 0);
            xglSoundEffectNormalID(0x30097, 0);
        }
        pEffect = pUnit->pRider;
        sefRewindEffectCf(pEffect);
        pEffect->nDisp = 1;
        pEffect->vTranslate.f[2] = pUnit->position.f[2];
        pEffect = w->pOther;
        fBack = -4.0f;
        fHeight = 1.5f;
        pEffect->vTranslate.f[0] = fBack;
        pEffect->vTranslate.f[1] = fHeight;
        fHalf = 0.5f;
        pEffect->vTranslate.f[2] = pUnit->position.f[2] - fHalf;
        return;
    }
    pEffect = pUnit->pRider;
    pUnit->state = 1;
    pUnit->position.f[2] = -12.5f;
    pEffect->nDisp = 0;
    pEffect = w->pOther;
    pEffect->nDisp = 0;
    w->field_3C++;
    if (w->field_2C == -1) {
        xglSoundEffectStopID(0x30096, 0);
        xglSoundEffectStopID(0x30098, 0);
        CallMethod("drill_stanby");
        return;
    }
    if (w->field_2C < w->field_3C) {
        pCam = xglStudioGetActiveCamera();
        pCam->nUnk04 = 4;
        GameLoopState.nFlags &= ~0x10000;
        pUnit->state = 0;
        GameLoopState.nFlags |= 0x80000;
        w->container = 0;
        xglSoundEffectStopID(0x30096, 0);
        xglSoundEffectStopID(0x30098, 0);
        CallMethod("drill_end");
    } else {
        xglSoundEffectStopID(0x30096, 0);
        xglSoundEffectStopID(0x30098, 0);
        CallMethod("drill_stanby");
    }
}

/* Slide the drill rig sideways along its rail while the button is held;
 * running off either end drops the whole rig into the shaft. */
void DrillXMoveFunc(MAP_UNIT *pUnit)
{
    MAP_UNIT_WORK *w = &pUnit->work;
    DRILL_CAMERA *pCam;
    DRILL_EFFECT *pEffect;

    pCam = xglStudioGetActiveCamera();
    xglFontDebugPrintf(8, 16, "DRILL XMOVE");
    Vibration_Set_Strong(128, 128, 2);
    if (PadData[0].nButton & 0x80) {
        pUnit->position.f[0] += w->field_0C;
    } else if (pUnit->position.f[0] < -1.9f) {
        pUnit->position.f[0] += w->field_0C;
    } else {
        pUnit->state = 5;
        pEffect = (DRILL_EFFECT *)sefCreateEffectCf(1594, 0, 0);
        pEffect->vTranslate = pUnit->position;
        pEffect->nFlags &= ~0x10;
        pEffect->vTranslate.f[1] = 0.0f;
        pEffect = (DRILL_EFFECT *)sefCreateEffectCf(1657, 0, 0);
        pEffect->nFlags &= ~0x400;
        pEffect->pCasterUnit = pUnit;
        pUnit->fCrashAngleX = pCam->w.fAngleX;
        pUnit->fCrashAngleY = pCam->w.fAngleY;
        xglSoundEffectStopID(0x30098, 0);
        xglSoundEffectNormalID(0x30099, 0);
        return;
    }
    if (pUnit->position.f[0] > 4.3f) {
        pUnit->position.f[0] = 4.3f;
        pUnit->state = 5;
        pEffect = (DRILL_EFFECT *)sefCreateEffectCf(1594, 0, 0);
        pEffect->vTranslate = pUnit->position;
        pEffect->nFlags &= ~0x10;
        pEffect->vTranslate.f[1] = 0.0f;
        pEffect = (DRILL_EFFECT *)sefCreateEffectCf(1657, 0, 0);
        pEffect->nFlags &= ~0x400;
        pEffect->pCasterUnit = pUnit;
        pUnit->fCrashAngleX = pCam->w.fAngleX;
        pUnit->fCrashAngleY = pCam->w.fAngleY;
        xglSoundEffectStopID(0x30098, 0);
        xglSoundEffectNormalID(0x30099, 0);
    }
    pEffect = pUnit->pDust;
    sefRewindEffectCf(pEffect);
    pEffect->nDisp = 1;
    pEffect->vTranslate.f[0] = pUnit->position.f[0];
    pEffect->vTranslate.f[2] = pUnit->position.f[2];
    pEffect = w->pOther;
    sefRewindEffectCf(pEffect);
    pEffect->nDisp = 1;
    pEffect->vTranslate.f[0] = pUnit->position.f[0];
    pEffect->vTranslate.f[1] = 6.0f;
    pEffect->vTranslate.f[2] = pUnit->position.f[2];
}

/* The drill hammering down into the rock: spin the bit, shake the camera
 * and, once it is deep enough, punch through. */
void DrillCrashFunc(MAP_UNIT *pUnit)
{
    MAP_UNIT_WORK *w = &pUnit->work;
    DRILL_CAMERA *pCam;
    DRILL_EFFECT *pEffect;

    pEffect = pUnit->pDust;
    pEffect->nDisp = 0;
    pEffect = w->pOther;
    pEffect->nDisp = 0;
    pCam = xglStudioGetActiveCamera();
    xglFontDebugPrintf(8, 16, "DRILL CRASH %2d", w->hitCount);
    pUnit->field_A8++;
    pUnit->fAngleY += (float)pUnit->field_A8 * 0.1f;
    pUnit->field_20 = 0;
    pUnit->field_28 = 0;
    xglMatrixStackUnit();
    xglMatrixStackRotY(pUnit->fAngleY);
    xglMatrixStackSave(pUnit->pMatrix);
    pCam->w.vEye[0] = pUnit->position.f[0] + 4.0f;
    pCam->w.vEye[1] = pUnit->position.f[1] + 5.0f;
    pCam->w.vEye[2] = pUnit->position.f[2] - 4.0f;
    pCam->w.fAngleX = -0.7853982f;
    pCam->w.fAngleY = 2.3561945f;
    pCam->w.fRoll = 0.0f;
    if (pUnit->field_A8 >= 16) {
        pUnit->field_A8 = 16;
        pUnit->position.f[1] -= w->field_10;
        DrillHitCheck(pUnit);
        if (pUnit->position.f[1] < 0.0f) {
            if (pUnit->field_AA == 0) {
                sefCreateEffectCf(1721, (int)&pUnit->position, 0);
            }
            pUnit->field_AA++;
            pUnit->position.f[1] = 0.0f;
            if (pUnit->field_AA >= 9) {
                pCam->w.fAngleX = pUnit->fCrashAngleX;
                pCam->w.fAngleY = pUnit->fCrashAngleY;
                DrillCalcCameraPos(pCam);
                pUnit->state = 6;
            }
        }
    }
}

/* Idle on the surface: wait for the player to board, cycle the camera
 * preset, or hand off to the Java-side "drill_stop" method. */
void DrillStandbyFunc(MAP_UNIT *pUnit)
{
    MAP_UNIT_WORK *w = &pUnit->work;
    DRILL_CAMERA *pCam;

    pCam = xglStudioGetActiveCamera();
    if (w->field_32 == 2) {
        w->cameraMode = 1;
        xglFontDebugPrintf(8, 16, "DRILL STOP");
        DrillResetCameraPos(pUnit, pCam);
        return;
    }
    if (w->field_32 == 1) {
        GameLoopState.nFlags |= 0x80000;
        pCam->nUnk04 = 4;
        GameLoopState.nFlags &= ~0x10000;
        pUnit->state = 0;
        w->container = 0;
        w->field_32 = 0;
        return;
    }
    xglFontDebugPrintf(8, 16, "DRILL STANDBY");
    if (w->flags < 0) {
        return;
    }
    DrillCameraControlFunc(pUnit, pCam);
    pUnit->field_A8 = 0;
    pUnit->field_AA = 0;
    if (PadData[0].nPress & 2) {
        DrillResetCameraPos(pUnit, pCam);
        w->cameraMode++;
        w->cameraMode %= 3;
    }
    if (PadData[0].nPress & 0x80) {
        pCam->w.vUnkE0[0] = 0.0f;
        pCam->w.vUnkE0[1] = 0.0f;
        pCam->w.vUnkE0[2] = 0.0f;
        pCam->w.vUnkB0[0] = 0.0f;
        pCam->w.vUnkB0[1] = 0.0f;
        pCam->w.vUnkB0[2] = 0.0f;
        pUnit->state = 2;
        xglSoundEffectNormalID(0x30096, 0);
        return;
    }
    if (PadData[0].nPress & 0x40) {
        if ((w->flags & 1) != 0) {
            if (w->field_5C - w->field_40 < 60) {
                return;
            }
        }
        if (w->field_32 != 1) {
            w->field_32 = 2;
            CallMethod("drill_stop");
        }
    }
}

/* Right-stick camera orbit: nudge the two angles by the stick deflection,
 * clamp them, and rebuild the eye position. */
void DrillCameraControlFunc(MAP_UNIT *pUnit, DRILL_CAMERA *pCam)
{
    DRILL_CAMWORK *cw = &pCam->w;
    MAP_UNIT_WORK *w;

    xglFontDebugPrintf(8, 32, "CAMERA CONTROL");
    w = &pUnit->work;
    pCam->nUnk04 = 0;
    cw->vUnkC0[0] = 0.0f;
    cw->vUnkC0[1] = 0.0f;
    cw->vUnkC0[2] = 0.0f;
    cw->vUnkF0[0] = 0.0f;
    cw->vUnkF0[1] = 0.0f;
    cw->vUnkF0[2] = 0.0f;
    cw->vUnkB0[0] = 0.0f;
    cw->vUnkB0[1] = 0.0f;
    cw->vUnkB0[2] = 0.0f;
    cw->vUnkE0[0] = 0.0f;
    cw->vUnkE0[1] = 0.0f;
    cw->vUnkE0[2] = 0.0f;
    if (PadData[0].nStickRX < 0) {
        cw->fAngleY += (float)(PadData[0].nStickRX + PadData[0].nStickLX)
                       * 0.0005235988f;
    }
    if (PadData[0].nStickRX > 0) {
        cw->fAngleY += (float)(PadData[0].nStickRX - PadData[0].nStickLX)
                       * 0.0005235988f;
    }
    if (PadData[0].nStickRY < 0) {
        cw->fAngleX += (float)(PadData[0].nStickRY + PadData[0].nStickLY)
                       * 0.0003f;
    }
    if (PadData[0].nStickRY > 0) {
        cw->fAngleX += (float)(PadData[0].nStickRY - PadData[0].nStickLY)
                       * 0.0003f;
    }
    if (cw->fAngleX > -0.20943953f) {
        cw->fAngleX = -0.20943953f;
    }
    if (cw->fAngleX < -1.5707964f) {
        cw->fAngleX = -1.5707964f;
    }
    if (w->field_2E == 0) {
        if (cw->fAngleY < 0.0f) {
            cw->fAngleY = 0.0f;
        }
        if (cw->fAngleY > 1.5707964f) {
            cw->fAngleY = 1.5707964f;
        }
    }
    DrillCalcCameraPos(pCam);
}

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
    pUnit->position.f[0] = -2.9f;
    pUnit->position.f[1] = 4.5f;
    pUnit->position.f[2] = -12.5f;
    w->field_3C = 0;
    w->field_05 = 0;
    w->hitCount = 0;
    w->field_32 = 0;
    pCam = xglStudioGetActiveCamera();
    pCam->w.fUnk04 = w->fCamHeight;
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
        pUnit->position.f[2] -= w->fStep;
    } else {
        pUnit->state = 3;
        xglSoundEffectStopID(0x30096, 0);
        xglSoundEffectNormalID(0x30097, 0);
    }
    if (pUnit->position.f[2] < -23.3f) {
        pUnit->position.f[2] = -23.3f;
        pUnit->state = 3;
        xglSoundEffectStopID(0x30096, 0);
        xglSoundEffectNormalID(0x30097, 0);
    }
    pEffect = pUnit->pRider;
    sefRewindEffectCf(pEffect);
    pEffect->nDisp = 1;
    pEffect->vTranslate.f[2] = pUnit->position.f[2];
    pEffect = w->pOther;
    sefRewindEffectCf(pEffect);
    pEffect->nDisp = 1;
    pEffect->vTranslate.f[0] = -4.0f;
    pEffect->vTranslate.f[1] = 1.5f;
    pEffect->vTranslate.f[2] = pUnit->position.f[2] + 0.5f;
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
        pCam->w.vUnkE0[0] = 0.0f;
        pCam->w.vUnkE0[1] = 0.0f;
        pCam->w.vUnkE0[2] = 0.0f;
        pCam->w.vUnkB0[0] = 0.0f;
        pCam->w.vUnkB0[1] = 0.0f;
        pCam->w.vUnkB0[2] = 0.0f;
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

    n = HitCheckContainerPosSize(&pUnit->position, pUnit->team, w->radius);
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
        return HitCheckEnemy(&pUnit->position, w->radius);
    }
}
