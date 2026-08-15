/* Native bindings for the script VM's xeno.Camera class */

typedef union {
    int i;
    unsigned int u;
    short h;
    float f;
    void *p;
    unsigned char b;
} JVAL;

typedef struct {
    char pad000[0x18];      /* 0x00 */
    int nStatic;            /* 0x18 */
} JCLASS;

typedef struct {
    char pad00[0x10];       /* 0x00 */
    int nOffset;            /* 0x10 */
} JFIELD;

typedef struct {
    int nClass;             /* 0x00 */
    unsigned int nLength;   /* 0x04 */
    void *pData;            /* 0x08 */
} JARRAY;

typedef struct {
    int nOn;                /* 0x000 */
    int nMode;              /* 0x004 */
    char pad008[0x18];      /* 0x008 */
    float fClipNear;        /* 0x020 */
    float fClipFar;         /* 0x024 */
    char pad028[0x6C];      /* 0x028 */
    float fFov;             /* 0x094 */
    char pad098[0x8];       /* 0x098 */
    float aRotate[3];       /* 0x0A0 */
    char pad0AC[0x24];      /* 0x0AC */
    float aTranslate[3];    /* 0x0D0 */
    char pad0DC[0x514];     /* 0x0DC */
} XGLCAMERA;

typedef struct {
    float fX;               /* 0x000 */
    float fY;               /* 0x004 */
    float fZ;               /* 0x008 */
    int pad00C;             /* 0x00C */
    int nBind;              /* 0x010 */
    int nTarget;            /* 0x014 */
    char pad018[0x488];     /* 0x018 */
} CAMCNS;

typedef struct {
    char pad000[0x6];       /* 0x000 */
    short nStart;           /* 0x006 */
    short nEnd;             /* 0x008 */
    char pad00A[0x496];     /* 0x00A */
} CAMSPL;

typedef union {
    CAMCNS cns;
    CAMSPL spl;
} CAMCHAN;

typedef struct {
    int nClass;             /* 0x0000 */
    int nIndex;             /* 0x0004 */
    int pad0008;            /* 0x0008 */
    int nTransMode;         /* 0x000C */
    int nViewMode;          /* 0x0010 */
    int nRollMode;          /* 0x0014 */
    int nFovMode;           /* 0x0018 */
    int nTransBind;         /* 0x001C */
    int nViewBind;          /* 0x0020 */
    int nRollBind;          /* 0x0024 */
    int nFovBind;           /* 0x0028 */
    int pad002C;            /* 0x002C */
    CAMCHAN trans;          /* 0x0030 */
    CAMCHAN view;           /* 0x04D0 */
    CAMCHAN roll;           /* 0x0970 */
    CAMCHAN fov;            /* 0x0E10 */
    int nAttach;            /* 0x12B0 */
    float fAspect;          /* 0x12B4 */
    char pad12B8[0x8];      /* 0x12B8 */
} TCAMERA;

typedef struct {
    char pad000[0xC0];      /* 0x000 */
    unsigned char nCameraMode;  /* 0x0C0 */
    unsigned char nCameraFlags; /* 0x0C1 */
    char pad0C2[0x6];       /* 0x0C2 */
    short nCameraNo;        /* 0x0C8 */
    char pad0CA[0x6];       /* 0x0CA */
} GAMELOOPSTATE;

typedef struct {
    float fX;               /* 0x00 */
    float fY;               /* 0x04 */
    float fZ;               /* 0x08 */
    float fW;               /* 0x0C */
} CFANGLE;

/* One 128-bit angle record.  The EE compiler uses its native quadword mode
 * for copies of this aligned four-float value, matching the original lq/sq
 * aggregate copy without inline assembly. */
typedef int CFANGLE_QUAD __attribute__((mode(TI)));
typedef union {
    CFANGLE angle;
    CFANGLE_QUAD quad;
} CFANGLE_VALUE;

typedef struct {
    unsigned char nOffsetFlag;  /* 0x00 */
    unsigned char nHokanFlag;   /* 0x01 */
    char pad002[0x6];       /* 0x02 */
    float fAnglePitch;      /* 0x08 */
    char pad00C[0x4];       /* 0x0C */
    float aOffset[3];       /* 0x10 */
    float fOffsetRoll;      /* 0x1C */
    int nOffsetA;           /* 0x20 */
    float fOffsetB;         /* 0x24 */
    int nOffsetC;           /* 0x28 */
    float fOffsetD;         /* 0x2C */
    CFANGLE angle;          /* 0x30 */
    float aLock[3];         /* 0x40 */
    int nLock;              /* 0x4C */
    float aHokan[2];        /* 0x50 */
    char pad058[0x8];       /* 0x58 */
    float aFog[8];          /* 0x60 */
    char pad080[0x20];      /* 0x80 */
} CFCAMERA;

JCLASS *classJava_xeno_Camera;
int classJava_xeno_Chr;
int classJava_xeno_Unit;

extern TCAMERA tcamera[];
extern CFCAMERA CfCameraDefine[];
extern GAMELOOPSTATE GameLoopState;
extern char D_004DC178[];

XGLCAMERA *xglStudioGetCamera2(int nCamera);
void camera_change(int nCamera);
void CAMERA_rotateSPL(int nMode, void *pEnv, JVAL *pArgs, JVAL *pRet);
void CAMERA_transSPL(int nMode, void *pEnv, JVAL *pArgs, JVAL *pRet);
void CAMERA_viewSPL(int nMode, void *pEnv, JVAL *pArgs, JVAL *pRet);
void TCAMERA_setRotate(XGLCAMERA *pCamera, float fX, float fY, float fZ);
void TCAMERA_setRoll(XGLCAMERA *pCamera, float fRoll);
void TCAMERA_setTranslate(XGLCAMERA *pCamera, float fX, float fY, float fZ);
void TCAMERA_setFov(XGLCAMERA *pCamera, float fFov);
void TCAMERA_setView(XGLCAMERA *pCamera, float fX, float fY, float fZ);
void SPL_init(void *pSpl, int nTime, void *pData, unsigned int nCount, int nStride);
void GameCameraChangeID(int nId, int nA, int nB);
int JNI_isInstanceOf(void *pObject, int nClass);
int loadConstString(char *pName, int nLength);
JFIELD *lookupClassField(int nClass, int nName, int nFlags);
float I2F(int nValue);

/* Read the camera's X rotation back into the script, in degrees */
void Java_xeno_Camera_getRotateX__(void *pEnv, JVAL *pArgs, JVAL *pRet)
{
    TCAMERA *pObj;
    XGLCAMERA *pCamera;

    pObj = (TCAMERA *)pArgs[0].p;
    pCamera = xglStudioGetCamera2(pObj->nIndex);
    pRet->f = pCamera->aRotate[0] / 3.1415927f * 180.0f;
}

/* Read the camera's Y rotation back into the script, in degrees */
void Java_xeno_Camera_getRotateY__(void *pEnv, JVAL *pArgs, JVAL *pRet)
{
    TCAMERA *pObj;
    XGLCAMERA *pCamera;

    pObj = (TCAMERA *)pArgs[0].p;
    pCamera = xglStudioGetCamera2(pObj->nIndex);
    pRet->f = pCamera->aRotate[1] / 3.1415927f * 180.0f;
}

/* Read the camera's Z rotation back into the script, in degrees */
void Java_xeno_Camera_getRotateZ__(void *pEnv, JVAL *pArgs, JVAL *pRet)
{
    TCAMERA *pObj;
    XGLCAMERA *pCamera;

    pObj = (TCAMERA *)pArgs[0].p;
    pCamera = xglStudioGetCamera2(pObj->nIndex);
    pRet->f = pCamera->aRotate[2] / 3.1415927f * 180.0f;
}

/* Read the camera's X position back into the script */
void Java_xeno_Camera_getTranslateX__(void *pEnv, JVAL *pArgs, JVAL *pRet)
{
    TCAMERA *pObj;
    XGLCAMERA *pCamera;

    pObj = (TCAMERA *)pArgs[0].p;
    pCamera = xglStudioGetCamera2(pObj->nIndex);
    pRet->f = pCamera->aTranslate[0];
}

/* Read the camera's Y position back into the script */
void Java_xeno_Camera_getTranslateY__(void *pEnv, JVAL *pArgs, JVAL *pRet)
{
    TCAMERA *pObj;
    XGLCAMERA *pCamera;

    pObj = (TCAMERA *)pArgs[0].p;
    pCamera = xglStudioGetCamera2(pObj->nIndex);
    pRet->f = pCamera->aTranslate[1];
}

/* Read the camera's Z position back into the script */
void Java_xeno_Camera_getTranslateZ__(void *pEnv, JVAL *pArgs, JVAL *pRet)
{
    TCAMERA *pObj;
    XGLCAMERA *pCamera;

    pObj = (TCAMERA *)pArgs[0].p;
    pCamera = xglStudioGetCamera2(pObj->nIndex);
    pRet->f = pCamera->aTranslate[2];
}

/* Drive the camera's field of view from a spline */
void Java_xeno_Camera_fovSPL__aFI(void *pEnv, JVAL *pArgs, JVAL *pRet)
{
    TCAMERA *pObj;
    JARRAY *pArray;
    int nTime;

    pObj = (TCAMERA *)pArgs[0].p;
    pArray = (JARRAY *)pArgs[1].p;
    nTime = pArgs[2].i;
    xglStudioGetCamera2(pObj->nIndex);
    pObj->nFovMode = 0x12;
    pObj->nFovBind = 0;
    SPL_init(&pObj->fov, nTime, pArray->pData, pArray->nLength / 2, 1);
    pObj->fov.spl.nStart = -1;
}

/* Make this camera the active one */
void Java_xeno_Camera_change__(void *pEnv, JVAL *pArgs, JVAL *pRet)
{
    TCAMERA *pObj;

    pObj = (TCAMERA *)pArgs[0].p;
    camera_change(pObj->nIndex);
}

/* Drive the camera's rotation from a spline */
void Java_xeno_Camera_rotateSPL__aFI(void *pEnv, JVAL *pArgs, JVAL *pRet)
{
    CAMERA_rotateSPL(0, pEnv, pArgs, pRet);
}

/* Drive the camera's rotation from a spline with an explicit key range */
void Java_xeno_Camera_rotateSPL__aFIII(void *pEnv, JVAL *pArgs, JVAL *pRet)
{
    CAMERA_rotateSPL(1, pEnv, pArgs, pRet);
}

/* Switch this camera on or off */
void Java_xeno_Camera_setActive__Z(void *pEnv, JVAL *pArgs, JVAL *pRet)
{
    TCAMERA *pObj;
    XGLCAMERA *pCamera;

    pObj = (TCAMERA *)pArgs[0].p;
    pCamera = xglStudioGetCamera2(pObj->nIndex);
    pCamera->nOn = pArgs[1].b;
}

/* Bind a script Camera object to one of the engine's camera slots */
void Java_xeno_Camera_create__I(void *pEnv, JVAL *pArgs, JVAL *pRet)
{
    TCAMERA *pObj;
    int nIndex;

    nIndex = pArgs[0].i;
    pObj = &tcamera[nIndex];
    pObj->fAspect = 1.2f;
    pObj->nClass = classJava_xeno_Camera->nStatic;
    pObj->nIndex = nIndex;
    xglStudioGetCamera2(nIndex);
    pRet->p = pObj;
}

/* Constrain the camera position to an actor with an XYZ offset */
/* TODO: near-miss - the original schedules the third argument's `lwc1` ahead of the
   first two and interleaves the `ld` epilogue with the closing float stores; the
   argument-order and declaration-order sweeps all keep the loads in source order. */
void Java_xeno_Camera_transCNS__Ljava_lang_Object_FFF(void *pEnv, JVAL *pArgs, JVAL *pRet)
{
    float fX;
    float fY;
    float fZ;
    float aPos[3];
    TCAMERA *pObj;
    void *pTarget;
    int nOfs;

    pObj = (TCAMERA *)pArgs[0].p;
    /* The Z component is read first -- that ordering is what puts the
       three argument loads in the original's $f2/$f0/$f1 order; reading
       them straight into aPos[] in index order costs three words. */
    fZ = pArgs[4].f;
    fX = pArgs[2].f;
    fY = pArgs[3].f;
    aPos[0] = fX;
    aPos[1] = fY;
    aPos[2] = fZ;
    pTarget = pArgs[1].p;
    xglStudioGetCamera2(pObj->nIndex);
    pObj->nTransMode = 0x14;
    /* A target that is neither a Chr nor a Unit leaves nTarget alone but
       still binds and moves the camera -- the original falls through to
       the stores below rather than returning. */
    if (JNI_isInstanceOf(pTarget, classJava_xeno_Chr) == 1) {
        nOfs = lookupClassField(classJava_xeno_Chr,
            loadConstString(D_004DC178, -1), 0)->nOffset;
        pObj->trans.cns.nTarget = *(int *)((char *)pTarget + nOfs) + 0x10;
    } else if (JNI_isInstanceOf(pTarget, classJava_xeno_Unit) == 1) {
        nOfs = lookupClassField(classJava_xeno_Unit,
            loadConstString(D_004DC178, -1), 0)->nOffset;
        pObj->trans.cns.nTarget = *(int *)((char *)pTarget + nOfs) + 0x10;
    }
    pObj->trans.cns.nBind = 1;
    pObj->trans.cns.fX = aPos[0];
    pObj->trans.cns.fY = aPos[1];
    pObj->trans.cns.fZ = aPos[2];
}

/* Drive the camera's position from a spline */
void Java_xeno_Camera_transSPL__aFI(void *pEnv, JVAL *pArgs, JVAL *pRet)
{
    CAMERA_transSPL(0, pEnv, pArgs, pRet);
}

/* Drive the camera's position from a spline with an explicit key range */
void Java_xeno_Camera_transSPL__aFIII(void *pEnv, JVAL *pArgs, JVAL *pRet)
{
    CAMERA_transSPL(1, pEnv, pArgs, pRet);
}

/* Drive the camera's look-at point from a spline */
void Java_xeno_Camera_viewSPL__aFI(void *pEnv, JVAL *pArgs, JVAL *pRet)
{
    CAMERA_viewSPL(0, pEnv, pArgs, pRet);
}

/* Drive the camera's look-at point from a spline with an explicit key range */
void Java_xeno_Camera_viewSPL__aFIII(void *pEnv, JVAL *pArgs, JVAL *pRet)
{
    CAMERA_viewSPL(1, pEnv, pArgs, pRet);
}

/* Drive the camera's roll from a spline */
void Java_xeno_Camera_rollSPL__aFI(void *pEnv, JVAL *pArgs, JVAL *pRet)
{
    TCAMERA *pObj;
    JARRAY *pArray;
    int nTime;

    pObj = (TCAMERA *)pArgs[0].p;
    pArray = (JARRAY *)pArgs[1].p;
    nTime = pArgs[2].i;
    xglStudioGetCamera2(pObj->nIndex);
    pObj->nRollMode = 0x12;
    pObj->nRollBind = 0;
    SPL_init(&pObj->roll, nTime, pArray->pData, pArray->nLength / 2, 1);
    pObj->roll.spl.nStart = -1;
}

/* Set the camera's rotation directly */
void Java_xeno_Camera_setRotate__FFF(void *pEnv, JVAL *pArgs, JVAL *pRet)
{
    TCAMERA *pObj;
    XGLCAMERA *pCamera;

    pObj = (TCAMERA *)pArgs[0].p;
    pCamera = xglStudioGetCamera2(pObj->nIndex);
    pObj->nViewMode = 0x10;
    TCAMERA_setRotate(pCamera, pArgs[1].f, pArgs[2].f, pArgs[3].f);
}

/* Set the camera's roll directly */
void Java_xeno_Camera_setRoll__F(void *pEnv, JVAL *pArgs, JVAL *pRet)
{
    TCAMERA *pObj;
    XGLCAMERA *pCamera;

    pObj = (TCAMERA *)pArgs[0].p;
    pCamera = xglStudioGetCamera2(pObj->nIndex);
    pObj->nRollMode = 0x10;
    TCAMERA_setRoll(pCamera, pArgs[1].f);
}

/* Set the camera's position directly */
void Java_xeno_Camera_setTranslate__FFF(void *pEnv, JVAL *pArgs, JVAL *pRet)
{
    TCAMERA *pObj;
    XGLCAMERA *pCamera;

    pObj = (TCAMERA *)pArgs[0].p;
    pCamera = xglStudioGetCamera2(pObj->nIndex);
    pObj->nTransMode = 0x10;
    TCAMERA_setTranslate(pCamera, pArgs[1].f, pArgs[2].f, pArgs[3].f);
}

/* Set the camera's field of view directly */
void Java_xeno_Camera_setFov__F(void *pEnv, JVAL *pArgs, JVAL *pRet)
{
    TCAMERA *pObj;
    XGLCAMERA *pCamera;

    pObj = (TCAMERA *)pArgs[0].p;
    pCamera = xglStudioGetCamera2(pObj->nIndex);
    pObj->nFovMode = 0x10;
    TCAMERA_setFov(pCamera, pArgs[1].f);
}

/* Read the camera's field of view back into the script, in degrees */
void Java_xeno_Camera_getFov__(void *pEnv, JVAL *pArgs, JVAL *pRet)
{
    TCAMERA *pObj;
    XGLCAMERA *pCamera;

    pObj = (TCAMERA *)pArgs[0].p;
    pCamera = xglStudioGetCamera2(pObj->nIndex);
    pRet->f = pCamera->fFov / 3.1415927f * 180.0f;
}

/* Set the camera's look-at point directly */
void Java_xeno_Camera_setView__FFF(void *pEnv, JVAL *pArgs, JVAL *pRet)
{
    TCAMERA *pObj;
    XGLCAMERA *pCamera;

    pObj = (TCAMERA *)pArgs[0].p;
    pCamera = xglStudioGetCamera2(pObj->nIndex);
    pObj->nViewMode = 0x10;
    TCAMERA_setView(pCamera, pArgs[1].f, pArgs[2].f, pArgs[3].f);
}

/* Attach the camera to a target object for one of its follow modes */
void Java_xeno_Camera_start__ILjava_lang_Object_(void *pEnv, JVAL *pArgs, JVAL *pRet)
{
    TCAMERA *pObj;
    XGLCAMERA *pCamera;
    void *pTarget;
    int nMode;

    pObj = (TCAMERA *)pArgs[0].p;
    nMode = pArgs[1].i;
    pTarget = pArgs[2].p;
    pCamera = xglStudioGetCamera2(pObj->nIndex);
    pCamera->nMode = nMode;
    if (nMode == 3) {
        if (pTarget != 0) {
            if (JNI_isInstanceOf(pTarget, classJava_xeno_Chr) != 0) {
                pObj->nAttach = *(int *)((char *)pTarget +
                    lookupClassField(classJava_xeno_Chr,
                        loadConstString(D_004DC178, -1), 0)->nOffset);
            }
        } else {
            pObj->nAttach = 0;
        }
    }
}

/* Set the orbit angles of one or all chase-camera definitions */
/* TODO: near-miss (140 built vs 142 original words).  Everything down to
   the copy loop matches; the two missing words are both in that loop and
   are the same TI-mode wall Java_xeno_Camera_resetFog__I hits:
    - gcc hoists `lq v0,0(sp)` (the loop-invariant `value.quad`) out of the
      loop.  The original re-loads it every iteration through a dedicated
      `move a0,sp` address register.  This is not type-based aliasing --
      pDef is provably based on CfCameraDefine and `value` is a distinct
      local, so gcc's base-decl analysis licenses the motion regardless of
      the union member read.
    - gcc folds the 0x30 displacement into the `sq`, where the original
      keeps `addiu v1,a2,48` and stores at offset 0.
   Everything else, including the six angle-wrap loops, is byte-exact; the
   register-role differences in the diff are knock-on from the two words.
*/
/* TODO: near-match (LENGTH, 142 original / 140 built). The recovered native
 * quadword representation now emits the original lq/sq aggregate copy, but
 * GCC hoists the source load ahead of the loop bookkeeping instead of keeping
 * the original stack-pointer copy and load/store schedule. */
void Java_xeno_Camera_setCFAngle__IFFFF(void *pEnv, JVAL *pArgs, JVAL *pRet)
{
    int nIndex;
    int nFirst;
    int nLast;
    CFCAMERA *pDef;
    CFANGLE_VALUE value;
    float fTwoPi;

    nIndex = pArgs[1].i;
    if (nIndex < 0) {
        nFirst = 0;
        nLast = 0x21;
    } else {
        nFirst = nIndex;
        nLast = nFirst + 1;
    }
    pDef = &CfCameraDefine[nFirst];
    value.angle.fX = pArgs[2].f / 180.0f * 3.1415927f;
    value.angle.fY = pArgs[3].f / 180.0f * 3.1415927f;
    value.angle.fZ = pArgs[4].f / 180.0f * 3.1415927f;
    value.angle.fW = pArgs[5].f;
    fTwoPi = 6.2831855f;
    while (value.angle.fX < 0.0f) {
        value.angle.fX += fTwoPi;
    }
    while (value.angle.fX > fTwoPi) {
        value.angle.fX -= fTwoPi;
    }
    while (value.angle.fY < 0.0f) {
        value.angle.fY += fTwoPi;
    }
    while (value.angle.fY > fTwoPi) {
        value.angle.fY -= fTwoPi;
    }
    while (value.angle.fZ < 0.0f) {
        value.angle.fZ += fTwoPi;
    }
    while (value.angle.fZ > fTwoPi) {
        value.angle.fZ -= fTwoPi;
    }
    while (nFirst < nLast) {
        pDef->nOffsetFlag = 0;
        *(CFANGLE_QUAD *)&pDef->angle = value.quad;
        pDef->fAnglePitch = pDef->angle.fY;
        pDef++;
        nFirst++;
    }
    GameLoopState.nCameraFlags |= 0x40;
}

/* Set the interpolation weights of one or all chase-camera definitions */
void Java_xeno_Camera_setCFHokan__IFF(void *pEnv, JVAL *pArgs, JVAL *pRet)
{
    int nIndex;
    int nFirst;
    int nLast;
    CFCAMERA *pDef;
    float fA;
    float fB;

    nIndex = pArgs[1].i;
    if (nIndex < 0) {
        nFirst = 0;
        nLast = 0x21;
    } else {
        nFirst = nIndex;
        nLast = nFirst + 1;
    }
    pDef = &CfCameraDefine[nFirst];
    fA = pArgs[2].f;
    fB = pArgs[3].f;
    while (nFirst < nLast) {
        pDef->aHokan[0] = fA;
        pDef->aHokan[1] = fB;
        pDef++;
        nFirst++;
    }
    GameLoopState.nCameraFlags |= 0x40;
}

/* Set the look-at lock target of one or all chase-camera definitions */
void Java_xeno_Camera_setCFLock__IIFFF(void *pEnv, JVAL *pArgs, JVAL *pRet)
{
    int nIndex;
    int nFirst;
    int nLast;
    CFCAMERA *pDef;
    int nLock;
    float fX;
    float fY;
    float fZ;

    nIndex = pArgs[1].i;
    if (nIndex < 0) {
        nFirst = 0;
        nLast = 0x21;
    } else {
        nFirst = nIndex;
        nLast = nFirst + 1;
    }
    pDef = &CfCameraDefine[nFirst];
    nLock = pArgs[2].i;
    fX = pArgs[3].f;
    fY = pArgs[4].f;
    fZ = pArgs[5].f;
    while (nFirst < nLast) {
        pDef->nLock = nLock;
        pDef->aLock[0] = fX;
        pDef->aLock[1] = fY;
        pDef->aLock[2] = fZ;
        pDef++;
        nFirst++;
    }
    GameLoopState.nCameraFlags |= 0x40;
}

/* Set the eye offset of one or all chase-camera definitions */
void Java_xeno_Camera_setCFOffset__IFFFIFIF(void *pEnv, JVAL *pArgs, JVAL *pRet)
{
    int nIndex;
    int nFirst;
    int nLast;
    CFCAMERA *pDef;
    int nA;
    int nC;
    float aOff[3];
    float fB;
    float fD;

    nIndex = pArgs[1].i;
    if (nIndex < 0) {
        nFirst = 0;
        nLast = 0x21;
    } else {
        nFirst = nIndex;
        nLast = nFirst + 1;
    }
    pDef = &CfCameraDefine[nFirst];
    aOff[0] = pArgs[2].f;
    aOff[1] = pArgs[3].f;
    aOff[2] = pArgs[4].f;
    nA = pArgs[5].h;
    fB = pArgs[6].f;
    nC = pArgs[7].h;
    fD = pArgs[8].f;
    while (nFirst < nLast) {
        pDef->nOffsetFlag = 0;
        pDef->aOffset[0] = aOff[0];
        pDef->aOffset[1] = aOff[1];
        pDef->aOffset[2] = aOff[2];
        pDef->nOffsetA = nA;
        pDef->fOffsetB = fB;
        pDef->nOffsetC = nC;
        pDef->fOffsetD = fD;
        pDef++;
        nFirst++;
    }
    GameLoopState.nCameraFlags |= 0x40;
}

/* Change the camera's interpolation mode and rebuild the camera list */
void Java_xeno_Camera_setMode__I(void *pEnv, JVAL *pArgs, JVAL *pRet)
{
    GameLoopState.nCameraMode = pArgs[1].b;
    GameLoopState.nCameraFlags |= 0x40;
    camera_change(0);
}

/* Read the camera's interpolation mode back into the script */
void Java_xeno_Camera_getMode__(void *pEnv, JVAL *pArgs, JVAL *pRet)
{
    pRet->i = GameLoopState.nCameraMode;
}

/* Enable or disable pedestal interpolation on chase-camera definitions */
void Java_xeno_Camera_setCFPedestalHokan__II(void *pEnv, JVAL *pArgs, JVAL *pRet)
{
    int nIndex;
    int nFirst;
    int nLast;
    CFCAMERA *pDef;
    int nOn;

    nIndex = pArgs[1].i;
    if (nIndex < 0) {
        nFirst = 0;
        nLast = 0x21;
    } else {
        nFirst = nIndex;
        nLast = nFirst + 1;
    }
    pDef = &CfCameraDefine[nFirst];
    nOn = pArgs[2].i;
    while (nFirst < nLast) {
        if (nOn != 0) {
            pDef->nHokanFlag |= 1;
        } else {
            pDef->nHokanFlag &= 0xFE;
        }
        pDef++;
        nFirst++;
    }
}

/* Switch to a different camera entry by id */
void Java_xeno_Camera_changeID__III(void *pEnv, JVAL *pArgs, JVAL *pRet)
{
    GameCameraChangeID(pArgs[1].i, pArgs[2].i, pArgs[3].i);
}

/* Set the camera's near and far clip distances */
void Java_xeno_Camera_setClipRange__FF(void *pEnv, JVAL *pArgs, JVAL *pRet)
{
    TCAMERA *pObj;
    XGLCAMERA *pCamera;

    pObj = (TCAMERA *)pArgs[0].p;
    pCamera = xglStudioGetCamera2(pObj->nIndex);
    pCamera->fClipNear = pArgs[1].f;
    pCamera->fClipFar = pArgs[2].f;
}


/* TODO: not matching (27 built vs 25 original words).  The logic and the
   0x21-entry sweep are right; two things block it, both TI-mode codegen:
    - the `por $r,$0,$0` zero-materialisation wall, which the fix_cc_asm
      peephole requested in Java_Chr.c's setPointLightReset note removes
      (verified: `sq zero` does appear under a prototype of that pass);
    - with that applied the only remainder is that gcc folds the 0x60 and
      0x70 displacements into the two `sq`s, where the original keeps two
      separate address registers and stores at offset 0.  That costs the
      two loop-padding nops gcc inserts instead, which is the whole 2-word
      difference.

   Clear the eight fog parameters of one camera definition, or of every
   definition when the index is negative.  Both halves go out as TI-mode
   quadword stores. */
void Java_xeno_Camera_resetFog__I(void *pEnv, JVAL *pArgs, JVAL *pRet)
{
    typedef int T128 __attribute__((mode(TI)));
    int nIndex;
    int nFirst;
    int nLast;
    int nNum;
    CFCAMERA *pDef;

    nIndex = pArgs[1].i;
    if (nIndex < 0) {
        nFirst = 0;
        nLast = 0x21;
    } else {
        nFirst = nIndex;
        nLast = nFirst + 1;
    }
    if (nFirst < nLast) {
        pDef = &CfCameraDefine[nFirst];
        nNum = nLast - nFirst;
        do {
            *(T128 *)&pDef->aFog[0] = 0;
            *(T128 *)&pDef->aFog[4] = 0;
            nNum--;
            pDef++;
        } while (nNum != 0);
    }
}
