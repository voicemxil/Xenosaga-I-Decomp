/* Drill - the drill-vehicle sequence: state functions driven from the map
 * unit's work block at +0x1A0, plus the chase camera and the container
 * hit test that spawns the crash effects. */

#include "common.h"

/* --- the shared 0x300-byte map-unit record (MapUnit[64]) ------------- */

typedef struct MAP_UNIT_WORK {      /* MAP_UNIT + 0x1A0 */
    char  pad_00[2];                /* 0x1A0 */
    unsigned short number;          /* 0x1A2 */
    char  container;                /* 0x1A4 */
    char  pad_05[3];
    short cameraMode;               /* 0x1A8 */
    char  pad_0A[0x1A];
    float radius;                   /* 0x1C4 */
    char  pad_28[0x0B];
    unsigned char hitCount;         /* 0x1D3 */
    char  pad_34[0x18];
    void *pOther;                   /* 0x1EC */
    char  pad_50[8];
    unsigned int flags;             /* 0x1F8 */
    char  pad_5C[0x104];
} MAP_UNIT_WORK;

typedef struct MAP_UNIT {
    unsigned int status;            /* 0x000 */
    int          field_004;         /* 0x004 */
    char         pad_008[0x08];
    float        position[4];       /* 0x010 */
    char         pad_020[0x84];
    short        id;                /* 0x0A4 */
    char         pad_0A6[0xFA];
    MAP_UNIT_WORK work;             /* 0x1A0 */
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
    char  pad_00[0x10];
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
