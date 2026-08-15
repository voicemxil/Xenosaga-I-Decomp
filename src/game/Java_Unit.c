/* Native bindings for the script VM's xeno.Unit class */

typedef union {
    int i;
    unsigned int u;
    short h;
    unsigned short uh;
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
    float *pData;           /* 0x08 */
} JARRAY;

typedef struct {
    int nClass;             /* 0x00 */
    float fX;               /* 0x04 */
    float fY;               /* 0x08 */
    float fZ;               /* 0x0C */
    float fW;               /* 0x10 */
} JVECTOR4F;

typedef struct {
    int nFlags;             /* 0x000 */
    char pad004[0x8];       /* 0x004 */
    void *pTaskFunc;        /* 0x00C */
    char pad010[0x20];      /* 0x010 */
    float aScale[3];        /* 0x030 */
    float fScaleW;          /* 0x03C */
    char pad040[0x60];      /* 0x040 */
    unsigned char nSerial;  /* 0x0A0 */
    unsigned char nSignal;  /* 0x0A1 */
    char pad0A2[0x4];       /* 0x0A2 */
    short nMonitorPrio;     /* 0x0A6 */
    char pad0A8[0x50];      /* 0x0A8 */
    int nJoint;             /* 0x0F8 */
    void *pParent;          /* 0x0FC */
    char pad100[0x18];      /* 0x100 */
    short nMtnMask;         /* 0x118 */
    char pad11A[0x86];      /* 0x11A */
    int aArgs[4];           /* 0x1A0 */
    char pad1B0[0x8];       /* 0x1B0 */
    void *pTaskArg;         /* 0x1B8 */
    char pad1BC[0x4];       /* 0x1BC */
    int nTaskState;         /* 0x1C0 */
    char pad1C4[0x70];      /* 0x1C4 */
    int nShadowA;           /* 0x234 */
    int nShadowB;           /* 0x238 */
    float fShadowClipScale; /* 0x23C */
    char pad240[0x90];      /* 0x240 */
    short aShadowMapId[8];  /* 0x2D0 */
    int nShadowMapCount;    /* 0x2E0 */
    int nRenderCommand;     /* 0x2E4 */
    int nFilter;            /* 0x2E8 */
    int nSortOffset;        /* 0x2EC */
    float aFilterParam[4];  /* 0x2F0 */
} UNITWORK;

typedef struct {
    int field_000;          /* 0x000 */
    int nState;             /* 0x004 */
    char pad008[0x1C];      /* 0x008 */
    void (*pFunc)(void);    /* 0x024 */
    void (*pFunc2)(void);   /* 0x028 */
    char pad02C[0x214];     /* 0x02C */
    float aPivot[3];        /* 0x240 */
    char pad24C[0x4];       /* 0x24C */
    float aAxis[4];         /* 0x250 */
} UNITSEQ;

int classJava_xeno_Unit;
int classJava_xeno_Chr;
JCLASS *classJava_xeno_util_Vector4f;

extern UNITSEQ unitSequence[];
extern char D_004DC1D0[];
extern char D_004DC1D8[];
extern char D_004DC1E0[];
extern char D_004DC1E8[];
extern char D_004DC1F0[];
extern char D_004DC1F8[];
extern char D_004DC200[];

int loadConstString(char *pName, int nLength);
JFIELD *lookupClassField(int nClass, int nName, int nFlags);
int JNI_isInstanceOf(void *pObject, int nClass);
void UNIT_moveXZ(int nMode, void *pEnv, JVAL *pArgs, JVAL *pRet);
void UNIT_motion(int nMode, void *pEnv, JVAL *pArgs, JVAL *pRet);
void UNIT_rotX(int nMode, void *pEnv, JVAL *pArgs, JVAL *pRet);
void UNIT_rotY(int nMode, void *pEnv, JVAL *pArgs, JVAL *pRet);
void UNIT_rotZ(int nMode, void *pEnv, JVAL *pArgs, JVAL *pRet);
void UNIT_sclX(int nMode, void *pEnv, JVAL *pArgs, JVAL *pRet);
void UNIT_sclY(int nMode, void *pEnv, JVAL *pArgs, JVAL *pRet);
void UNIT_sclZ(int nMode, void *pEnv, JVAL *pArgs, JVAL *pRet);
void MDL_setVisible(void *pModel, int nPart, int nVisible);
void MAP_callUnitGroup(int nGroup, void *pFunc);
void unit_suspend(void *pUnit)
{
    *(int *)pUnit |= 0x10;
}
void unit_resume(void *pUnit)
{
    *(int *)pUnit &= ~0x10;
}
void copyArgs_00303EC8(void *pDst, void *pSrc, int nSize);
int ACT_jointGetAccessories(void *pParent, int nJoint);
void UNIT_setUpdate(void *pObject, void *pParent);
void tyaElevatorTask(UNITWORK *pUnit);
void SEQ_transCNSUnitChr(void);
void SEQ_rotYCNSUnitChr(void);

/* Resolve the native work block a script Unit object points at */
#define UNIT_WORK(pObj) (*(UNITWORK **)((char *)(pObj) + \
    lookupClassField(classJava_xeno_Unit, loadConstString(D_004DC1D0, -1), 0)->nOffset))

/* Resolve the native peer a script Chr object points at, by the shared field name */
#define CHR_PEER_OF(pObj) (*(void **)((char *)(pObj) + \
    lookupClassField(classJava_xeno_Chr, loadConstString(D_004DC1D0, -1), 0)->nOffset))

/* Byte offset of a named field within a xeno.Unit script object */
#define UNIT_FIELD(name) \
    (lookupClassField(classJava_xeno_Unit, loadConstString(name, -1), 0)->nOffset)

/* Constrain the unit's Y rotation to track another actor */
void Java_xeno_Unit_rotYCNS__Ljava_lang_Object_IFFF(void *pEnv, JVAL *pArgs, JVAL *pRet)
{
    void *pObj = pArgs[0].p;
    UNITWORK *pUnit;
    UNITSEQ *pSeq;
    char *p;

    pUnit = UNIT_WORK(pObj);
    UNIT_setUpdate(pObj, pUnit);
    pSeq = &unitSequence[pUnit->nSerial];
    p = (char *)pSeq + 0xB8;
    pSeq->pFunc2 = SEQ_rotYCNSUnitChr;
    pSeq->nState |= 4;
    *(void **)(p + 0x00) = CHR_PEER_OF(pArgs[1].p);
    *(int *)(p + 0x14) = pArgs[2].i;
    *(float *)(p + 0x30) = pArgs[3].f;
    *(float *)(p + 0x34) = pArgs[4].f;
    *(float *)(p + 0x38) = pArgs[5].f;
}

/* Constrain the unit's position to track another actor */
void Java_xeno_Unit_transCNS__Ljava_lang_Object_IFFF(void *pEnv, JVAL *pArgs, JVAL *pRet)
{
    void *pObj = pArgs[0].p;
    UNITWORK *pUnit;
    UNITSEQ *pSeq;
    char *p;

    pUnit = UNIT_WORK(pObj);
    UNIT_setUpdate(pObj, pUnit);
    pSeq = &unitSequence[pUnit->nSerial];
    p = (char *)pSeq + 0x38;
    pSeq->pFunc = SEQ_transCNSUnitChr;
    pSeq->nState |= 1;
    *(void **)(p + 0x00) = CHR_PEER_OF(pArgs[1].p);
    *(int *)(p + 0x40) = pArgs[2].i;
    *(float *)(p + 0x10) = pArgs[3].f;
    *(float *)(p + 0x14) = pArgs[4].f;
    *(float *)(p + 0x18) = pArgs[5].f;
}

/* Copy the unit's rotation back into the script object (radians to degrees) */
void Java_xeno_Unit_getRotate__(void *pEnv, JVAL *pArgs, JVAL *pRet)
{
    char *pObj = (char *)pArgs[0].p;
    float *pRot = (float *)((char *)UNIT_WORK(pObj) + 0x20);

    *(float *)(pObj + UNIT_FIELD(D_004DC1D8)) = pRot[0] / 3.1415927f * 180.0f;
    *(float *)(pObj + UNIT_FIELD(D_004DC1E0)) = pRot[1] / 3.1415927f * 180.0f;
    *(float *)(pObj + UNIT_FIELD(D_004DC1E8)) = pRot[2] / 3.1415927f * 180.0f;
}

/* Read the unit's pending signal number back into the script */
void Java_xeno_Unit_getSignal__(void *pEnv, JVAL *pArgs, JVAL *pRet)
{
    void *pObj;

    pObj = pArgs[0].p;
    if (JNI_isInstanceOf(pObj, classJava_xeno_Unit) == 0) {
        pRet->i = 0;
    } else {
        pRet->i = UNIT_WORK(pObj)->nSignal;
    }
}

/* Copy the unit's position back into the script object */
void Java_xeno_Unit_getTranslate__(void *pEnv, JVAL *pArgs, JVAL *pRet)
{
    char *pObj = (char *)pArgs[0].p;
    float *pPos = (float *)((char *)UNIT_WORK(pObj) + 0x10);

    *(float *)(pObj + UNIT_FIELD(D_004DC1F0)) = pPos[0];
    *(float *)(pObj + UNIT_FIELD(D_004DC1F8)) = pPos[1];
    *(float *)(pObj + UNIT_FIELD(D_004DC200)) = pPos[2];
}

/* Script hook with no native side effect */
void Java_xeno_Unit_invalidate__(void *pEnv, JVAL *pArgs, JVAL *pRet)
{
}

/* Move the unit to an absolute XZ position */
void Java_xeno_Unit_move__FFFZ(void *pEnv, JVAL *pArgs, JVAL *pRet)
{
    UNIT_moveXZ(1, pEnv, pArgs, pRet);
}

/* Move the unit to a named XZ waypoint */
void Java_xeno_Unit_move__IFFZ(void *pEnv, JVAL *pArgs, JVAL *pRet)
{
    UNIT_moveXZ(0, pEnv, pArgs, pRet);
}

/* Script hook with no native side effect */
void Java_xeno_Unit_move__ILjava_lang_Object_Z(void *pEnv, JVAL *pArgs, JVAL *pRet)
{
}

/* Script hook with no native side effect */
void Java_xeno_Unit_move__Ljava_lang_Object_FZ(void *pEnv, JVAL *pArgs, JVAL *pRet)
{
}

/* Play a motion on the unit */
void Java_xeno_Unit_mtn__IIFZ(void *pEnv, JVAL *pArgs, JVAL *pRet)
{
    UNIT_motion(0, pEnv, pArgs, pRet);
}

/* Play a motion on the unit with an explicit blend setup */
void Java_xeno_Unit_mtn__IIIIIFZ(void *pEnv, JVAL *pArgs, JVAL *pRet)
{
    UNIT_motion(1, pEnv, pArgs, pRet);
}

/* Rotate the unit about X by an absolute angle */
void Java_xeno_Unit_rotX__FFZ(void *pEnv, JVAL *pArgs, JVAL *pRet)
{
    UNIT_rotX(1, pEnv, pArgs, pRet);
}

/* Rotate the unit about X towards a named angle */
void Java_xeno_Unit_rotX__IFZ(void *pEnv, JVAL *pArgs, JVAL *pRet)
{
    UNIT_rotX(0, pEnv, pArgs, pRet);
}

/* Script hook with no native side effect */
void Java_xeno_Unit_rotX__ILjava_lang_Object_Z(void *pEnv, JVAL *pArgs, JVAL *pRet)
{
}

/* Script hook with no native side effect */
void Java_xeno_Unit_rotX__Ljava_lang_Object_FZ(void *pEnv, JVAL *pArgs, JVAL *pRet)
{
}

/* Rotate the unit about Y by an absolute angle */
void Java_xeno_Unit_rotY__FFZ(void *pEnv, JVAL *pArgs, JVAL *pRet)
{
    UNIT_rotY(1, pEnv, pArgs, pRet);
}

/* Rotate the unit about Y towards a named angle */
void Java_xeno_Unit_rotY__IFZ(void *pEnv, JVAL *pArgs, JVAL *pRet)
{
    UNIT_rotY(0, pEnv, pArgs, pRet);
}

/* Script hook with no native side effect */
void Java_xeno_Unit_rotY__ILjava_lang_Object_Z(void *pEnv, JVAL *pArgs, JVAL *pRet)
{
}

/* Script hook with no native side effect */
void Java_xeno_Unit_rotY__Ljava_lang_Object_FZ(void *pEnv, JVAL *pArgs, JVAL *pRet)
{
}

/* Rotate the unit about Z by an absolute angle */
void Java_xeno_Unit_rotZ__FFZ(void *pEnv, JVAL *pArgs, JVAL *pRet)
{
    UNIT_rotZ(1, pEnv, pArgs, pRet);
}

/* Rotate the unit about Z towards a named angle */
void Java_xeno_Unit_rotZ__IFZ(void *pEnv, JVAL *pArgs, JVAL *pRet)
{
    UNIT_rotZ(0, pEnv, pArgs, pRet);
}

/* Script hook with no native side effect */
void Java_xeno_Unit_rotZ__ILjava_lang_Object_Z(void *pEnv, JVAL *pArgs, JVAL *pRet)
{
}

/* Script hook with no native side effect */
void Java_xeno_Unit_rotZ__Ljava_lang_Object_FZ(void *pEnv, JVAL *pArgs, JVAL *pRet)
{
}

/* Scale the unit along X by an absolute factor */
void Java_xeno_Unit_sclX__FFZ(void *pEnv, JVAL *pArgs, JVAL *pRet)
{
    UNIT_sclX(1, pEnv, pArgs, pRet);
}

/* Scale the unit along X towards a named factor */
void Java_xeno_Unit_sclX__IFZ(void *pEnv, JVAL *pArgs, JVAL *pRet)
{
    UNIT_sclX(0, pEnv, pArgs, pRet);
}

/* Scale the unit along Y by an absolute factor */
void Java_xeno_Unit_sclY__FFZ(void *pEnv, JVAL *pArgs, JVAL *pRet)
{
    UNIT_sclY(1, pEnv, pArgs, pRet);
}

/* Scale the unit along Y towards a named factor */
void Java_xeno_Unit_sclY__IFZ(void *pEnv, JVAL *pArgs, JVAL *pRet)
{
    UNIT_sclY(0, pEnv, pArgs, pRet);
}

/* Scale the unit along Z by an absolute factor */
void Java_xeno_Unit_sclZ__FFZ(void *pEnv, JVAL *pArgs, JVAL *pRet)
{
    UNIT_sclZ(1, pEnv, pArgs, pRet);
}

/* Scale the unit along Z towards a named factor */
void Java_xeno_Unit_sclZ__IFZ(void *pEnv, JVAL *pArgs, JVAL *pRet)
{
    UNIT_sclZ(0, pEnv, pArgs, pRet);
}

/* Turn the unit's collision response on or off */
void Java_xeno_Unit_setCollision__Z(void *pEnv, JVAL *pArgs, JVAL *pRet)
{
    void *pObj;
    UNITWORK *pUnit;

    pObj = pArgs[0].p;
    pUnit = UNIT_WORK(pObj);
    if (pArgs[1].b != 0) {
        pUnit->nFlags |= 0x100;
    } else {
        pUnit->nFlags &= ~0x100;
    }
}

/* Copy the script object's rotation into the unit (degrees to radians) */
void Java_xeno_Unit_setRotate__(void *pEnv, JVAL *pArgs, JVAL *pRet)
{
    char *pObj = (char *)pArgs[0].p;
    float *pRot = (float *)((char *)UNIT_WORK(pObj) + 0x20);

    pRot[0] = *(float *)(pObj + UNIT_FIELD(D_004DC1D8)) / 180.0f * 3.1415927f;
    pRot[1] = *(float *)(pObj + UNIT_FIELD(D_004DC1E0)) / 180.0f * 3.1415927f;
    pRot[2] = *(float *)(pObj + UNIT_FIELD(D_004DC1E8)) / 180.0f * 3.1415927f;
}

/* Copy the script object's position into the unit */
void Java_xeno_Unit_setTranslate__(void *pEnv, JVAL *pArgs, JVAL *pRet)
{
    char *pObj = (char *)pArgs[0].p;
    float *pPos = (float *)((char *)UNIT_WORK(pObj) + 0x10);

    pPos[0] = *(float *)(pObj + UNIT_FIELD(D_004DC1F0));
    pPos[1] = *(float *)(pObj + UNIT_FIELD(D_004DC1F8));
    pPos[2] = *(float *)(pObj + UNIT_FIELD(D_004DC200));
}

/* Show or hide one part of the unit's model */
void Java_xeno_Unit_setVisible__IZ(void *pEnv, JVAL *pArgs, JVAL *pRet)
{
    void *pObj;
    UNITWORK *pUnit;

    pObj = pArgs[0].p;
    pUnit = UNIT_WORK(pObj);
    MDL_setVisible((char *)pUnit + 0x240, pArgs[1].i, pArgs[2].b);
}

/* Show or hide the whole unit */
void Java_xeno_Unit_setVisible__Z(void *pEnv, JVAL *pArgs, JVAL *pRet)
{
    void *pObj;
    UNITWORK *pUnit;

    pObj = pArgs[0].p;
    pUnit = UNIT_WORK(pObj);
    if (pArgs[1].b != 0) {
        pUnit->nFlags &= ~0x4;
    } else {
        pUnit->nFlags |= 0x4;
    }
}

/* Post a signal number to the unit */
void Java_xeno_Unit_signal__I(void *pEnv, JVAL *pArgs, JVAL *pRet)
{
    void *pObj;

    pObj = pArgs[0].p;
    if (JNI_isInstanceOf(pObj, classJava_xeno_Unit) == 0) {
        pRet->i = 0;
    } else {
        UNIT_WORK(pObj)->nSignal = pArgs[1].b;
    }
}

/* Script hook with no native side effect */
void Java_xeno_Unit_validate__(void *pEnv, JVAL *pArgs, JVAL *pRet)
{
}

/* Attach the unit to a parent object, optionally through an accessory joint */
void Java_xeno_Unit_setParent__Ljava_lang_Object_I(void *pEnv, JVAL *pArgs, JVAL *pRet)
{
    void *pObj;
    void *pParentObj;
    void *pParent;
    UNITWORK *pUnit;
    int nJoint;

    nJoint = pArgs[2].i;
    pParentObj = pArgs[1].p;
    pObj = pArgs[0].p;
    pUnit = UNIT_WORK(pObj);
    pParent = *(void **)((char *)pParentObj +
        lookupClassField(classJava_xeno_Chr, loadConstString(D_004DC1D0, -1), 0)->nOffset);
    pUnit->pParent = pParent;
    if ((nJoint & 0x8000) != 0) {
        pUnit->nJoint = ACT_jointGetAccessories(pParent, nJoint & 0x7FFF) | 0x8000;
    } else {
        pUnit->nJoint = nJoint;
    }
    UNIT_setUpdate(pObj, pUnit);
}

/* Store one script argument word into the unit's argument block */
void Java_xeno_Unit_setArgs__III(void *pEnv, JVAL *pArgs, JVAL *pRet)
{
    int nValue;
    int nSize;
    void *pObj;
    int nOffset;
    UNITWORK *pUnit;

    nValue = pArgs[2].i;
    nOffset = pArgs[1].i;
    pObj = pArgs[0].p;
    nSize = pArgs[3].i;
    pUnit = UNIT_WORK(pObj);
    if (nSize >= 1 && nSize <= 4) {
        copyArgs_00303EC8((char *)pUnit + nOffset + 0x1A0, &nValue, nSize);
    }
}

/* Read one script argument word back out of the unit's argument block */
void Java_xeno_Unit_getArgs__II(void *pEnv, JVAL *pArgs, JVAL *pRet)
{
    int nValue;
    void *pObj;
    UNITWORK *pUnit;
    int nOffset;
    int nSize;

    nSize = pArgs[2].i;
    pObj = pArgs[0].p;
    nOffset = pArgs[1].i;
    pUnit = UNIT_WORK(pObj);
    if (nSize >= 1 && nSize <= 4) {
        copyArgs_00303EC8(&nValue, (char *)pUnit + nOffset + 0x1A0, nSize);
        pRet->i = nValue;
    }
}

/* Copy a script array into the unit's argument block */
void Java_xeno_Unit_setArgs__ILjava_lang_Object_I(void *pEnv, JVAL *pArgs, JVAL *pRet)
{
    int nSize;
    void *pObj;
    JARRAY *pArray;
    UNITWORK *pUnit;

    pObj = pArgs[0].p;
    pArray = (JARRAY *)pArgs[2].p;
    nSize = pArgs[3].i;
    pUnit = UNIT_WORK(pObj);
    if (nSize > 0) {
        copyArgs_00303EC8((char *)pUnit + 0x1A0, (char *)pArray + 4, nSize);
    }
}

/* Store four script argument words into one argument-block slot */
/* TODO: near-miss - the original keeps the slot base in $v1 and shuffles it
   through $a1/$a2/$s0 while restoring callee-saved registers; gcc folds the
   0x1A0 displacement into the base here, so 3 words differ. */
void Java_xeno_Unit_setArgs__IIIII(void *pEnv, JVAL *pArgs, JVAL *pRet)
{
    void *pObj;
    UNITWORK *pUnit;
    UNITWORK *pSlot;
    int nSlot;

    pObj = pArgs[0].p;
    nSlot = pArgs[1].i << 4;
    pUnit = UNIT_WORK(pObj);
    pSlot = (UNITWORK *)((char *)pUnit + nSlot);
    pSlot->aArgs[0] = pArgs[2].i;
    pSlot->aArgs[1] = pArgs[3].i;
    pSlot->aArgs[2] = pArgs[4].i;
    pSlot->aArgs[3] = pArgs[5].i;
}

/* Read the unit's scale back into the script as a Vector4f */
void Java_xeno_Unit_getScale__(void *pEnv, JVAL *pArgs, JVAL *pRet)
{
    static JVECTOR4F scale;
    void *pObj;
    UNITWORK *pUnit;

    pObj = pArgs[0].p;
    pUnit = UNIT_WORK(pObj);
    scale.nClass = classJava_xeno_util_Vector4f->nStatic;
    scale.fZ = pUnit->aScale[0];
    scale.fY = pUnit->aScale[1];
    scale.fX = pUnit->aScale[2];
    scale.fW = pUnit->fScaleW;
    pRet->p = &scale;
}

/* Set the unit's scale directly */
void Java_xeno_Unit_setScale__FFF(void *pEnv, JVAL *pArgs, JVAL *pRet)
{
    void *pObj;
    UNITWORK *pUnit;

    pObj = pArgs[0].p;
    pUnit = UNIT_WORK(pObj);
    pUnit->aScale[0] = pArgs[1].f;
    pUnit->aScale[1] = pArgs[2].f;
    pUnit->aScale[2] = pArgs[3].f;
}

/* Read the unit's serial number back into the script */
void Java_xeno_Unit_getSerial__(void *pEnv, JVAL *pArgs, JVAL *pRet)
{
    void *pObj;
    UNITWORK *pUnit;

    pObj = pArgs[0].p;
    pUnit = UNIT_WORK(pObj);
    pRet->i = pUnit->nSerial;
}

/* Set the unit's motion bone mask */
void Java_xeno_Unit_mtnSetMask__I(void *pEnv, JVAL *pArgs, JVAL *pRet)
{
    void *pObj;
    UNITWORK *pUnit;
    int nMask;

    pObj = pArgs[0].p;
    nMask = pArgs[1].i;
    pUnit = UNIT_WORK(pObj);
    pUnit->nMtnMask = nMask;
}

/* Read the unit's sequence state back into the script */
void Java_xeno_Unit_getState__(void *pEnv, JVAL *pArgs, JVAL *pRet)
{
    void *pObj;
    UNITWORK *pUnit;

    pObj = pArgs[0].p;
    pUnit = UNIT_WORK(pObj);
    pRet->i = unitSequence[pUnit->nSerial].nState;
}

/* Read the unit's pivot point into a caller-supplied Vector4f */
void Java_xeno_Unit_getPivot__Lxeno_util_Vector4f_(void *pEnv, JVAL *pArgs, JVAL *pRet)
{
    JVECTOR4F *pVec;
    void *pObj;
    UNITWORK *pUnit;
    UNITSEQ *pSeq;

    pVec = (JVECTOR4F *)pArgs[1].p;
    pObj = pArgs[0].p;
    if (pVec != 0) {
        pUnit = UNIT_WORK(pObj);
    pSeq = &unitSequence[pUnit->nSerial];
        pVec->fZ = pSeq->aPivot[0];
        pVec->fY = pSeq->aPivot[1];
        pVec->fX = pSeq->aPivot[2];
    }
}

/* Set the unit's pivot point */
void Java_xeno_Unit_setPivot__FFF(void *pEnv, JVAL *pArgs, JVAL *pRet)
{
    void *pObj;
    UNITWORK *pUnit;
    UNITSEQ *pSeq;

    pObj = pArgs[0].p;
    pUnit = UNIT_WORK(pObj);
    pSeq = &unitSequence[pUnit->nSerial];
    pSeq->aPivot[0] = pArgs[1].f;
    pSeq->aPivot[1] = pArgs[2].f;
    pSeq->aPivot[2] = pArgs[3].f;
}

/* Read the unit's rotation axis into a caller-supplied Vector4f */
void Java_xeno_Unit_getAxis__Lxeno_util_Vector4f_(void *pEnv, JVAL *pArgs, JVAL *pRet)
{
    JVECTOR4F *pVec;
    void *pObj;
    UNITWORK *pUnit;
    UNITSEQ *pSeq;

    pVec = (JVECTOR4F *)pArgs[1].p;
    pObj = pArgs[0].p;
    if (pVec != 0) {
        pUnit = UNIT_WORK(pObj);
    pSeq = &unitSequence[pUnit->nSerial];
        pVec->fZ = pSeq->aAxis[0];
        pVec->fY = pSeq->aAxis[1];
        pVec->fX = pSeq->aAxis[2];
        pVec->fW = 1.0f;
    }
}

/* Set the unit's rotation axis */
void Java_xeno_Unit_setAxis__FFFF(void *pEnv, JVAL *pArgs, JVAL *pRet)
{
    void *pObj;
    UNITWORK *pUnit;
    UNITSEQ *pSeq;

    pObj = pArgs[0].p;
    pUnit = UNIT_WORK(pObj);
    pSeq = &unitSequence[pUnit->nSerial];
    pSeq->aAxis[0] = pArgs[1].f;
    pSeq->aAxis[1] = pArgs[2].f;
    pSeq->aAxis[2] = pArgs[3].f;
    pSeq->aAxis[3] = pArgs[4].f;
}

/* Suspend every unit in a group */
void Java_xeno_Unit_suspend__I(void *pEnv, JVAL *pArgs, JVAL *pRet)
{
    MAP_callUnitGroup(pArgs[0].i, unit_suspend);
}

/* Resume every unit in a group */
void Java_xeno_Unit_resume__I(void *pEnv, JVAL *pArgs, JVAL *pRet)
{
    MAP_callUnitGroup(pArgs[0].i, unit_resume);
}

/* Hook the unit up to the elevator update task */
void Java_xeno_Unit_initElevatorFunc__(void *pEnv, JVAL *pArgs, JVAL *pRet)
{
    void *pObj;
    UNITWORK *pUnit;
    void *pArg;

    pObj = pArgs[0].p;
    pUnit = UNIT_WORK(pObj);
    pArg = (char *)pObj +
        lookupClassField(classJava_xeno_Unit, loadConstString(D_004DC1F8, -1), 0)->nOffset;
    pUnit->pTaskFunc = (void *)tyaElevatorTask;
    pUnit->nTaskState = 0;
    pUnit->pTaskArg = pArg;
    tyaElevatorTask(pUnit);
}

/* Turn the unit's map shadow on or off */
void Java_xeno_Unit_map_shadow__I(void *pEnv, JVAL *pArgs, JVAL *pRet)
{
    void *pObj;
    UNITWORK *pUnit;

    pObj = pArgs[0].p;
    pUnit = UNIT_WORK(pObj);
    if (pArgs[1].i != 0) {
        pUnit->nFlags |= 0x20;
    } else {
        pUnit->nFlags &= ~0x20;
    }
}

/* Select the render command list used to draw the unit */
void Java_xeno_Unit_renderCommand__I(void *pEnv, JVAL *pArgs, JVAL *pRet)
{
    void *pObj;
    UNITWORK *pUnit;

    pObj = pArgs[0].p;
    pUnit = UNIT_WORK(pObj);
    pUnit->nRenderCommand = pArgs[1].i;
}

/* Select the unit's post-effect filter mode */
void Java_xeno_Unit_setFilter__I(void *pEnv, JVAL *pArgs, JVAL *pRet)
{
    void *pObj;
    UNITWORK *pUnit;
    int nMode;

    pObj = pArgs[0].p;
    pUnit = UNIT_WORK(pObj);
    nMode = pArgs[1].i;
    pUnit->nFilter = 0;
    switch (nMode) {
    case 2:
        pUnit->nFilter = 1;
        pRet->i = 4;
        break;
    case 3:
        pUnit->nFilter = 2;
        pRet->i = 4;
        break;
    }
}

/* Copy the unit's post-effect filter parameters from a float array */
void Java_xeno_Unit_setFilterParam__aF(void *pEnv, JVAL *pArgs, JVAL *pRet)
{
    void *pObj;
    UNITWORK *pUnit;
    JARRAY *pArray;
    float *pDst;
    unsigned int i;

    pObj = pArgs[0].p;
    pUnit = UNIT_WORK(pObj);
    pArray = (JARRAY *)pArgs[1].p;
    pDst = pUnit->aFilterParam;
    for (i = 0; i < pArray->nLength; i++) {
        pDst[i] = pArray->pData[i];
    }
}

/* Set the unit's shadow parameters */
void Java_xeno_Unit_setShadow__II(void *pEnv, JVAL *pArgs, JVAL *pRet)
{
    UNITWORK *pUnit;

    pUnit = UNIT_WORK(pArgs[0].p);
    pUnit->nShadowA = pArgs[1].i;
    pUnit->nShadowB = pArgs[2].i;
}

/* Set the scale applied to the unit's shadow clip volume */
void Java_xeno_Unit_shadow_clip_scale__F(void *pEnv, JVAL *pArgs, JVAL *pRet)
{
    UNITWORK *pUnit;

    pUnit = UNIT_WORK(pArgs[0].p);
    pUnit->fShadowClipScale = pArgs[1].f;
}

/* Add one shadow map id to the unit's list */
void Java_xeno_Unit_shadow_map_id__I(void *pEnv, JVAL *pArgs, JVAL *pRet)
{
    UNITWORK *pUnit;

    pUnit = UNIT_WORK(pArgs[0].p);
    if (pUnit->nShadowMapCount < 8) {
        pUnit->aShadowMapId[pUnit->nShadowMapCount++] = pArgs[1].uh;
    }
}

/* Drop every shadow map id from the unit's list */
void Java_xeno_Unit_shadow_map_reset__(void *pEnv, JVAL *pArgs, JVAL *pRet)
{
    UNITWORK *pUnit;

    pUnit = UNIT_WORK(pArgs[0].p);
    pUnit->nShadowMapCount = 0;
}

/* Bias the unit's depth-sort key */
/* TODO: near-miss - the original converts with cvt.w.s (the R5900 FPU has no
   trunc.w.s); this ee-gcc build emits trunc.w.s for the (int) cast, so the
   conversion word differs.  Everything else matches. */
void Java_xeno_Unit_setSortOffset__F(void *pEnv, JVAL *pArgs, JVAL *pRet)
{
    UNITWORK *pUnit;

    pUnit = UNIT_WORK(pArgs[0].p);
    pUnit->nSortOffset = pArgs[1].f;
}

/* Turn view-frustum clipping of the unit on or off */
void Java_xeno_Unit_setClip__I(void *pEnv, JVAL *pArgs, JVAL *pRet)
{
    UNITWORK *pUnit;

    pUnit = UNIT_WORK(pArgs[0].p);
    if (pArgs[1].i != 0) {
        pUnit->nFlags |= 0x40;
    } else {
        pUnit->nFlags &= ~0x40;
    }
}

/* Raise or clear the unit's monitor priority */
void Java_xeno_Unit_setMonitorPrio__I(void *pEnv, JVAL *pArgs, JVAL *pRet)
{
    UNITWORK *pUnit;

    pUnit = UNIT_WORK(pArgs[0].p);
    if (pArgs[1].i != 0) {
        pUnit->nMonitorPrio = 1;
    } else {
        pUnit->nMonitorPrio = 0;
    }
}
