/* xeno.Chr native methods - JNI bindings over the character actor work struct */

typedef struct {
    int unk0[4];                        /* 0x00 */
    int nOffset;                        /* 0x10 */
} JAVA_FIELD;

typedef struct {
    int unk0[6];                        /* 0x00 */
    int nInstance;                      /* 0x18 */
} JAVA_CLASS;

typedef struct {
    int nClass;                         /* 0x00 */
    float z;                            /* 0x04 */
    float y;                            /* 0x08 */
    float x;                            /* 0x0C */
    float w;                            /* 0x10 */
} JAVA_VECTOR4F;

typedef struct {
    float x;                            /* 0x00 */
    float y;                            /* 0x04 */
    float z;                            /* 0x08 */
    float w;                            /* 0x0C */
} CHR_LIGHT;

typedef struct CHR_ {
    int nFlags;                         /* 0x000 */
    void *pUpdate;                      /* 0x004 */
    void *pFilter;                      /* 0x008 */
    char pad00C[0x4];                   /* 0x00C */
    float fTranslate[3];                /* 0x010 */
    char pad01C[0x34];                  /* 0x01C */
    float fRotate[3];                   /* 0x050 */
    char pad05C[0x4];                   /* 0x05C */
    float fScale[4];                    /* 0x060 */
    char pad070[0x10];                  /* 0x070 */
    unsigned char nSerial;              /* 0x080 */
    unsigned char nSignal;              /* 0x081 */
    unsigned char nKind;                /* 0x082 */
    char pad083[0x5];                   /* 0x083 */
    short n088;                         /* 0x088 */
    char pad08A[0x6];                   /* 0x08A */
    unsigned char nShadowType;          /* 0x090 */
    unsigned char nShadowSize;          /* 0x091 */
    char pad092[0x42E];                 /* 0x092 */
    void *pObject;                      /* 0x4C0 */
    char pad4C4[0xC];                   /* 0x4C4 */
    unsigned short nModeFlags;          /* 0x4D0 */
    char pad4D2[0xA];                   /* 0x4D2 */
    float fTranslateY;                  /* 0x4DC */
    int nDataHeader;                    /* 0x4E0 */
    char pad4E4[0x13C];                 /* 0x4E4 */
    float fLookPoint[3];                /* 0x620 */
    float fLookEyeSpeed;                /* 0x62C */
    char pad630[0x30];                  /* 0x630 */
    float fLookEye[2];                  /* 0x660 */
    char pad668[0x4];                   /* 0x668 */
    float fLookSpeed;                   /* 0x66C */
    float fShadowClipScale;             /* 0x670 */
    short nLookMode;                    /* 0x674 */
    short pad676;                       /* 0x676 */
    int nLookEyeControl;                /* 0x678 */
    int nLookTarget;                    /* 0x67C */
    short nShadowMapId[8];              /* 0x680 */
    int nShadowMapNum;                  /* 0x690 */
    char pad694[0x5C];                  /* 0x694 */
    int nMotionFlags;                   /* 0x6F0 */
    char pad6F4[0x60];                  /* 0x6F4 */
    int nHairStop0;                     /* 0x754 */
    int nHairStop1;                     /* 0x758 */
    char pad75C[0x1A4];                 /* 0x75C */
    int nChildNum;                      /* 0x900 */
    struct CHR_ *aChild[27];            /* 0x904 */
    CHR_LIGHT light[3];                 /* 0x970 */
    int nFlags2;                        /* 0x9A0 */
    int nRenderCommand;                 /* 0x9A4 */
    int nPixelAlpha;                    /* 0x9A8 */
    int nPixelAlphaPartsNum;            /* 0x9AC */
    short alphaParts[8];                /* 0x9B0 */
    float fFilterParam[4];              /* 0x9C0 */
    char pad9D0[0x10];                  /* 0x9D0 */
    float fSortOffset;                  /* 0x9E0 */
    char pad9E4[0x10];                  /* 0x9E4 */
    int nTalkTo;                        /* 0x9F4 */
    int nTouchTo;                       /* 0x9F8 */
    char pad9FC[0x44];                  /* 0x9FC */
    unsigned char nMoveState;           /* 0xA40 */
} CHR;

int classJava_xeno_Chr;
int classJava_xeno_Unit;
int classJava_xeno_util_Vector4f;
float D_004D8414;
float D_004D8418;
float D_004D841C;

extern char D_004DC190[];
extern char D_004DC1A0[];
extern char D_004DC1A8[];
extern char D_004DC1B0[];
extern char D_004DC1B8[];
extern char D_004DC1C0[];
extern char D_004DC1C8[];
typedef struct {
    int field_00;                       /* 0x00 */
    int nFlags;                         /* 0x04 */
    int field_08;                       /* 0x08 */
    int field_0C;                       /* 0x0C */
    int field_10;                       /* 0x10 */
    int field_14;                       /* 0x14 */
    int field_18;                       /* 0x18 */
    int field_1C;                       /* 0x1C */
    int field_20;                       /* 0x20 */
    void *pFunc;                        /* 0x24 */
    int field_28;                       /* 0x28 */
    int field_2C;                       /* 0x2C */
    int field_30;                       /* 0x30 */
    char pad034[0x22C];                 /* 0x34 */
} ACTSEQ;

extern ACTSEQ actSequence[];
extern void ACT_updateSequence(void);
extern CHR *D_00338684[];
extern int D_0046F464[];

int loadConstString(char *, int);
JAVA_FIELD *lookupClassField(int, int, int);
int JNI_isInstanceOf(int, int);
int UnduDataGetHeader(int, int);
void copyArgs_002FEFB0(void *, void *, int);
void CHR_moveXZ(int, void *, int *, int *);
void CHR_rotX(int, void *, int *, int *);
void CHR_rotY(int, void *, int *, int *);
void CHR_rotZ(int, void *, int *, int *);
void CHR_sclX(int, void *, int *, int *);
void CHR_sclY(int, void *, int *, int *);
void CHR_sclZ(int, void *, int *, int *);
void CHR_motion(int, void *, int *, int *);
void ACT_setHand(CHR *, int);
void ACT_setVisible(CHR *, int, int);
void ACT_setArms(CHR *, CHR *, int, int);
void ACT_setRelation(CHR *, CHR *, int, int);
void ACT_setParent(CHR *, int, CHR *);
void ACT_resetArms(CHR *, CHR *, int);

/* Byte offset of a named field within a xeno.Chr script object */
#define CHR_FIELD(name) \
    (lookupClassField(classJava_xeno_Chr, loadConstString(name, -1), 0)->nOffset)

/* Resolve the native peer pointer stashed in the Chr object's field */
#define CHR_PEER(obj) (*(CHR **)((obj) + CHR_FIELD(D_004DC190)))

#define UNIT_PEER(obj) \
    (*(int *)((obj) + lookupClassField(classJava_xeno_Unit, loadConstString(D_004DC190, -1), 0)->nOffset))

/* Bind the calling script object to the player character actor */
void Java_xeno_Chr_getPlayer__(void *env, int *args, int *ret)
{
    char *obj = (char *)args[0];

    CHR_PEER(obj) = D_00338684[0];
}

/* Chr.move(int, float, float, boolean) */
void Java_xeno_Chr_move__IFFZ(void *env, int *args, int *ret)
{
    CHR_moveXZ(0, env, args, ret);
}

/* Chr.move(float, float, float, boolean) */
void Java_xeno_Chr_move__FFFZ(void *env, int *args, int *ret)
{
    CHR_moveXZ(1, env, args, ret);
}

/* Chr.move(int, Object, boolean) */
void Java_xeno_Chr_move__ILjava_lang_Object_Z(void *env, int *args, int *ret)
{
    CHR_moveXZ(2, env, args, ret);
}

/* Chr.move(Object, float, boolean) */
void Java_xeno_Chr_move__Ljava_lang_Object_FZ(void *env, int *args, int *ret)
{
    CHR_moveXZ(3, env, args, ret);
}

/* Chr.rotX(float, float, boolean) */
void Java_xeno_Chr_rotX__FFZ(void *env, int *args, int *ret)
{
    CHR_rotX(1, env, args, ret);
}

/* Chr.rotX(int, float, boolean) */
void Java_xeno_Chr_rotX__IFZ(void *env, int *args, int *ret)
{
    CHR_rotX(0, env, args, ret);
}

/* Chr.rotX(int, Object, boolean) */
void Java_xeno_Chr_rotX__ILjava_lang_Object_Z(void *env, int *args, int *ret)
{
    CHR_rotX(2, env, args, ret);
}

/* Chr.rotX(Object, float, boolean) */
void Java_xeno_Chr_rotX__Ljava_lang_Object_FZ(void *env, int *args, int *ret)
{
    CHR_rotX(3, env, args, ret);
}

/* Chr.rotY(float, float, boolean) */
void Java_xeno_Chr_rotY__FFZ(void *env, int *args, int *ret)
{
    CHR_rotY(1, env, args, ret);
}

/* Chr.rotY(int, float, boolean) */
void Java_xeno_Chr_rotY__IFZ(void *env, int *args, int *ret)
{
    CHR_rotY(0, env, args, ret);
}

/* Chr.rotY(int, Object, boolean) */
void Java_xeno_Chr_rotY__ILjava_lang_Object_Z(void *env, int *args, int *ret)
{
    CHR_rotY(2, env, args, ret);
}

/* Chr.rotY(Object, float, boolean) */
void Java_xeno_Chr_rotY__Ljava_lang_Object_FZ(void *env, int *args, int *ret)
{
    CHR_rotY(3, env, args, ret);
}

/* Chr.rotZ(float, float, boolean) */
void Java_xeno_Chr_rotZ__FFZ(void *env, int *args, int *ret)
{
    CHR_rotZ(1, env, args, ret);
}

/* Chr.rotZ(int, float, boolean) */
void Java_xeno_Chr_rotZ__IFZ(void *env, int *args, int *ret)
{
    CHR_rotZ(0, env, args, ret);
}

/* Chr.rotZ(int, Object, boolean) */
void Java_xeno_Chr_rotZ__ILjava_lang_Object_Z(void *env, int *args, int *ret)
{
    CHR_rotZ(2, env, args, ret);
}

/* Chr.rotZ(Object, float, boolean) */
void Java_xeno_Chr_rotZ__Ljava_lang_Object_FZ(void *env, int *args, int *ret)
{
    CHR_rotZ(3, env, args, ret);
}

/* Chr.mtn(int, int, int, int, int, float, boolean) */
void Java_xeno_Chr_mtn__IIIIIFZ(void *env, int *args, int *ret)
{
    CHR_motion(1, env, args, ret);
}

/* Chr.mtn(int, int, float, boolean) */
void Java_xeno_Chr_mtn__IIFZ(void *env, int *args, int *ret)
{
    CHR_motion(0, env, args, ret);
}

/* Raise a script signal on the character */
void Java_xeno_Chr_signal__I(void *env, int *args, int *ret)
{
    char *obj = (char *)args[0];

    if (JNI_isInstanceOf((int)obj, classJava_xeno_Chr) == 0) {
        ret[0] = 0;
        return;
    }
    CHR_PEER(obj)->nSignal = (unsigned char)args[1];
}

/* Read back the character's pending script signal */
void Java_xeno_Chr_getSignal__(void *env, int *args, int *ret)
{
    char *obj = (char *)args[0];

    if (JNI_isInstanceOf((int)obj, classJava_xeno_Chr) == 0) {
        ret[0] = 0;
        return;
    }
    ret[0] = CHR_PEER(obj)->nSignal;
}

/* Copy the script object's rotation fields into the actor (degrees to radians) */
void Java_xeno_Chr_setRotate__(void *env, int *args, int *ret)
{
    char *obj = (char *)args[0];
    float *pRot = (float *)((char *)CHR_PEER(obj) + 0x50);

    pRot[0] = *(float *)(obj + CHR_FIELD(D_004DC1B8)) / 180.0f * 3.1415927f;
    pRot[1] = *(float *)(obj + CHR_FIELD(D_004DC1C0)) / 180.0f * 3.1415927f;
    pRot[2] = *(float *)(obj + CHR_FIELD(D_004DC1C8)) / 180.0f * 3.1415927f;
}

/* Copy the script object's position fields into the actor */
void Java_xeno_Chr_setTranslate__(void *env, int *args, int *ret)
{
    char *obj = (char *)args[0];
    CHR *chr = CHR_PEER(obj);
    float *pPos = (float *)((char *)chr + 0x10);
    unsigned char nMoving;

    pPos[0] = *(float *)(obj + CHR_FIELD(D_004DC1A0));
    pPos[1] = *(float *)(obj + CHR_FIELD(D_004DC1A8));
    pPos[2] = *(float *)(obj + CHR_FIELD(D_004DC1B0));
    chr->nModeFlags |= 0x1000;
    chr->fTranslateY = pPos[1];
    nMoving = chr->nMoveState & 1;
    if (nMoving != 0) {
        chr->nMoveState = 2;
    }
}

/* Copy the actor's rotation back into the script object (radians to degrees) */
void Java_xeno_Chr_getRotate__(void *env, int *args, int *ret)
{
    char *obj = (char *)args[0];
    float *pRot = (float *)((char *)CHR_PEER(obj) + 0x50);

    *(float *)(obj + CHR_FIELD(D_004DC1B8)) = pRot[0] / 3.1415927f * 180.0f;
    *(float *)(obj + CHR_FIELD(D_004DC1C0)) = pRot[1] / 3.1415927f * 180.0f;
    *(float *)(obj + CHR_FIELD(D_004DC1C8)) = pRot[2] / 3.1415927f * 180.0f;
}

/* Copy the actor's position back into the script object */
void Java_xeno_Chr_getTranslate__(void *env, int *args, int *ret)
{
    char *obj = (char *)args[0];
    float *pPos = (float *)((char *)CHR_PEER(obj) + 0x10);

    *(float *)(obj + CHR_FIELD(D_004DC1A0)) = pPos[0];
    *(float *)(obj + CHR_FIELD(D_004DC1A8)) = pPos[1];
    *(float *)(obj + CHR_FIELD(D_004DC1B0)) = pPos[2];
}

/* Toggle the actor's hidden flag */
void Java_xeno_Chr_setVisible__Z(void *env, int *args, int *ret)
{
    char *obj = (char *)args[0];
    CHR *chr = CHR_PEER(obj);

    if (*(unsigned char *)&args[1] != 0) {
        chr->nFlags &= ~0x8;
    } else {
        chr->nFlags |= 0x8;
    }
}

/* Toggle visibility of one of the actor's sub-models */
void Java_xeno_Chr_setVisible__IZ(void *env, int *args, int *ret)
{
    char *obj = (char *)args[0];

    ACT_setVisible(CHR_PEER(obj), args[1], *(unsigned char *)&args[2]);
}

/* Toggle the actor's collision flag */
void Java_xeno_Chr_setCollision__Z(void *env, int *args, int *ret)
{
    char *obj = (char *)args[0];
    CHR *chr = CHR_PEER(obj);

    if (*(unsigned char *)&args[1] != 0) {
        chr->nFlags |= 0x40;
    } else {
        chr->nFlags &= ~0x40;
    }
}

/* Select which hand model the character is holding */
void Java_xeno_Chr_setHand__I(void *env, int *args, int *ret)
{
    char *obj = (char *)args[0];

    ACT_setHand(CHR_PEER(obj), args[1]);
}

/* Copy script arguments into the character's argument block */
void Java_xeno_Chr_setArgs__III(void *env, int *args, int *ret)
{
    int nValue = args[2];
    int nIndex = args[1];
    char *obj = (char *)args[0];
    int nCount = args[3];
    CHR *chr = CHR_PEER(obj);

    if ((unsigned int)(nCount - 1) < 4) {
        copyArgs_002FEFB0((char *)chr + nIndex + 0xC0, &nValue, nCount);
    }
}

/* Copy arguments back out of the character's argument block */
void Java_xeno_Chr_getArgs__II(void *env, int *args, int *ret)
{
    char *obj = (char *)args[0];
    int nValue;
    int nCount = args[2];
    int nIndex = args[1];
    CHR *chr = CHR_PEER(obj);

    if ((unsigned int)(nCount - 1) < 4) {
        copyArgs_002FEFB0(&nValue, (char *)chr + nIndex + 0xC0, nCount);
        ret[0] = nValue;
    }
}

/* Store an object reference pair into the character's argument block */
/* TODO: not matching - the second object field load is scheduled after the first store */
void Java_xeno_Chr_setArgs__ILjava_lang_Object_I(void *env, int *args, int *ret)
{
    char *obj = (char *)args[0];
    int *pObj = (int *)args[2];
    int nIndex = args[1];
    char *pArg = (char *)CHR_PEER(obj) + nIndex;

    *(int *)(pArg + 0xC4) = pObj[1];
    *(int *)(pArg + 0xC0) = pObj[2];
}

/* Return the character's actor serial number */
void Java_xeno_Chr_getSerial__(void *env, int *args, int *ret)
{
    char *obj = (char *)args[0];

    ret[0] = CHR_PEER(obj)->nSerial;
}

/* Return the shared state block for the character's serial */
void Java_xeno_Chr_getState__(void *env, int *args, int *ret)
{
    char *obj = (char *)args[0];
    CHR *chr = CHR_PEER(obj);

    ret[0] = *(int *)((char *)D_0046F464 + chr->nSerial * 0x260);
}

/* Chr.mtnGetRoot(int, Vector4f) - unimplemented */
void Java_xeno_Chr_mtnGetRoot__ILxeno_util_Vector4f_(void *env, int *args, int *ret)
{
    char *obj = (char *)args[0];
    CHR *chr = CHR_PEER(obj);
}

/* Set the character's XYZ scale */
void Java_xeno_Chr_setScale__FFF(void *env, int *args, int *ret)
{
    char *obj = (char *)args[0];
    CHR *chr = CHR_PEER(obj);

    chr->fScale[0] = *(float *)&args[1];
    chr->fScale[1] = *(float *)&args[2];
    chr->fScale[2] = *(float *)&args[3];
}

/* Return the character's scale as a Vector4f */
/* TODO: not matching - return store sinks and the $f0/$f1 pair alternates the other way */
void Java_xeno_Chr_getScale__(void *env, int *args, int *ret)
{
    static JAVA_VECTOR4F scale;
    char *obj = (char *)args[0];
    CHR *chr = CHR_PEER(obj);
    JAVA_VECTOR4F *p = &scale;

    p->nClass = ((JAVA_CLASS *)classJava_xeno_util_Vector4f)->nInstance;
    p->x = chr->fScale[0];
    p->y = chr->fScale[1];
    p->z = chr->fScale[2];
    p->w = chr->fScale[3];
    ret[0] = (int)p;
}

/* Chr.sclX(float, float, boolean) */
void Java_xeno_Chr_sclX__FFZ(void *env, int *args, int *ret)
{
    CHR_sclX(1, env, args, ret);
}

/* Chr.sclX(int, float, boolean) */
void Java_xeno_Chr_sclX__IFZ(void *env, int *args, int *ret)
{
    CHR_sclX(0, env, args, ret);
}

/* Chr.sclY(float, float, boolean) */
void Java_xeno_Chr_sclY__FFZ(void *env, int *args, int *ret)
{
    CHR_sclY(1, env, args, ret);
}

/* Chr.sclY(int, float, boolean) */
void Java_xeno_Chr_sclY__IFZ(void *env, int *args, int *ret)
{
    CHR_sclY(0, env, args, ret);
}

/* Chr.sclZ(float, float, boolean) */
void Java_xeno_Chr_sclZ__FFZ(void *env, int *args, int *ret)
{
    CHR_sclZ(1, env, args, ret);
}

/* Chr.sclZ(int, float, boolean) */
void Java_xeno_Chr_sclZ__IFZ(void *env, int *args, int *ret)
{
    CHR_sclZ(1, env, args, ret);
}

/* Chr.rotCNS(int, Object) - unimplemented */
void Java_xeno_Chr_rotCNS__ILjava_lang_Object_(void *env, int *args, int *ret)
{
}

/* Chr.rotCNS(int, float, float, float) - unimplemented */
void Java_xeno_Chr_rotCNS__IFFF(void *env, int *args, int *ret)
{
    char *obj = (char *)args[0];
    CHR *chr = CHR_PEER(obj);
}

/* Chr.setRotCNSParam(int, float, float, float, float, float) - unimplemented */
void Java_xeno_Chr_setRotCNSParam__IFFFFF(void *env, int *args, int *ret)
{
    char *obj = (char *)args[0];
    CHR *chr = CHR_PEER(obj);
}

/* Chr.relax(int, int) - unimplemented */
void Java_xeno_Chr_relax__II(void *env, int *args, int *ret)
{
    char *obj = (char *)args[0];
    CHR *chr = CHR_PEER(obj);
}

/* Return the actor's flag word */
void Java_xeno_Chr_getFlags__(void *env, int *args, int *ret)
{
    ret[0] = CHR_PEER((char *)args[0])->nFlags;
}

/* Overwrite the actor's flag word */
void Java_xeno_Chr_setFlags__I(void *env, int *args, int *ret)
{
    CHR_PEER((char *)args[0])->nFlags = args[1];
}

/* Toggle the actor's "fall off edges" mode bit */
void Java_xeno_Chr_setEdgeFall__I(void *env, int *args, int *ret)
{
    CHR *chr = CHR_PEER((char *)args[0]);

    if (args[1] == 0) {
        chr->nModeFlags &= 0x8;
    } else {
        chr->nModeFlags |= 0x8;
    }
}

/* Enable shadow rendering and set its type and size */
void Java_xeno_Chr_setShadow__II(void *env, int *args, int *ret)
{
    CHR *chr = CHR_PEER((char *)args[0]);

    chr->nFlags |= 0x20;
    chr->nShadowType = *(unsigned char *)&args[1];
    chr->nShadowSize = *(unsigned char *)&args[2];
}

/* Attach an undulation data header to the character */
void Java_xeno_Chr_setID__I(void *env, int *args, int *ret)
{
    CHR *chr = CHR_PEER((char *)args[0]);

    chr->nDataHeader = UnduDataGetHeader(0, args[1]);
    chr->nModeFlags |= 0x1000;
}

/* Toggle the actor's elevator mode bit */
void Java_xeno_Chr_setElevatorMode__I(void *env, int *args, int *ret)
{
    CHR *chr = CHR_PEER((char *)args[0]);

    if (args[1] == 0) {
        chr->nModeFlags &= 0x20;
    } else {
        chr->nModeFlags |= 0x20;
    }
}

/* Attach the character to a parent actor, optionally as a bone relation */
/* TODO: not matching - args[4] is not hoisted out of the taken branch */
void Java_xeno_Chr_setParent__Lxeno_Chr_III(void *env, int *args, int *ret)
{
    char *obj = (char *)args[0];
    JAVA_FIELD *pField = lookupClassField(classJava_xeno_Chr, loadConstString(D_004DC190, -1), 0);
    int nMode = args[2];
    int nBone = args[4];
    CHR *chr = *(CHR **)(obj + pField->nOffset);
    CHR *parent;
    int nType;

    obj = (char *)args[1];
    parent = *(CHR **)(obj + pField->nOffset);
    nType = args[3];
    if ((nMode & 0x8000) != 0) {
        ACT_setRelation(chr, parent, nMode, nBone);
    } else {
        ACT_setParent(chr, nType, parent);
    }
}

/* Set or clear motion flag bits */
void Java_xeno_Chr_setMotionFlags__IZ(void *env, int *args, int *ret)
{
    char *obj = (char *)args[0];
    CHR *chr = CHR_PEER(obj);
    int nMask = args[1];

    if (*(unsigned char *)&args[2] != 0) {
        chr->nMotionFlags |= nMask;
    } else {
        chr->nMotionFlags &= ~nMask;
    }
}

/* Copy the float filter parameters out of a Java array */
void Java_xeno_Chr_setFilterParam__aF(void *env, int *args, int *ret)
{
    char *obj = (char *)args[0];
    CHR *chr = CHR_PEER(obj);
    float *pSrc = (float *)((int *)args[1])[2];

    chr->fFilterParam[0] = pSrc[0];
    chr->fFilterParam[1] = pSrc[3];
    chr->fFilterParam[2] = pSrc[2];
    chr->fFilterParam[3] = pSrc[1];
}

/* Toggle the actor's clipping flag */
void Java_xeno_Chr_setClip__I(void *env, int *args, int *ret)
{
    char *obj = (char *)args[0];
    CHR *chr = CHR_PEER(obj);

    if (args[1] != 0) {
        chr->nFlags2 |= 0x200;
    } else {
        chr->nFlags2 &= ~0x200;
    }
}

/* Toggle the actor's Y symmetry flag */
void Java_xeno_Chr_setSymmetryY__I(void *env, int *args, int *ret)
{
    char *obj = (char *)args[0];
    CHR *chr = CHR_PEER(obj);

    if (args[1] != 0) {
        chr->nFlags2 |= 0x10;
    } else {
        chr->nFlags2 &= ~0x10;
    }
}

/* Set the actor's depth sort offset */
void Java_xeno_Chr_setSortOffset__F(void *env, int *args, int *ret)
{
    char *obj = (char *)args[0];

    CHR_PEER(obj)->fSortOffset = *(float *)&args[1];
}

/* Set one of the character's point light colours */
/* TODO: not matching - the original reloads args[1] per component (aliasing) instead of CSEing it */
void Java_xeno_Chr_setPointLightCol__IFFF(void *env, int *args, int *ret)
{
    char *obj = (char *)args[0];
    CHR *chr = CHR_PEER(obj);

    if ((unsigned int)args[1] < 3) {
        chr->nFlags |= 0x800;
        chr->nFlags2 |= 0x20;
        *(float *)((char *)chr + args[1] * 16 + 0x970) = *(float *)&args[2];
        *(float *)((char *)chr + args[1] * 16 + 0x974) = *(float *)&args[3];
        *(float *)((char *)chr + args[1] * 16 + 0x978) = *(float *)&args[4];
    }
}

/* Record the string id the character responds to when talked to */
void Java_xeno_Chr_talkto__Ljava_lang_String_(void *env, int *args, int *ret)
{
    char *obj = (char *)args[0];
    CHR *chr = CHR_PEER(obj);

    chr->nTalkTo = ((int *)((int *)args[1])[1])[2];
}

/* Record the string id the character responds to when touched */
void Java_xeno_Chr_touchto__Ljava_lang_String_(void *env, int *args, int *ret)
{
    char *obj = (char *)args[0];
    CHR *chr = CHR_PEER(obj);

    chr->nTouchTo = ((int *)((int *)args[1])[1])[2];
}

/* Find the actor's first child of the given kind */
/* TODO: near-miss - the original tests the tag with a branch-likely and annuls an
   epilogue `ld $s0` in its delay slot; every natural early-return/loop shape here
   emits a plain `bne` and two instructions fewer. */
void Java_xeno_Chr_childGetPeer__II(void *env, int *args, int *ret)
{
    char *obj = (char *)args[0];
    CHR *chr;
    CHR *child;
    int i;

    chr = CHR_PEER(obj);
    ret[0] = 0;
    if (args[1] == 0x1000000) {
        for (i = 0; i < chr->nChildNum; i++) {
            child = chr->aChild[i];
            if (child->nKind == 1) {
                ret[0] = (int)child;
                break;
            }
        }
    }
}

/* Adopt an existing actor work block as this script object's peer */
/* TODO: near-miss - the four stores through the actor base come out as
   pUpdate/pObject/n088/nSignal; the original order is pUpdate/n088/nSignal/pObject.
   A full 120-permutation sweep of the assignment order yields only three distinct
   schedules, none of them the original's, so this ordering is not reachable from C. */
void Java_xeno_Chr_setPeer__Ljava_lang_Object_(void *env, int *args, int *ret)
{
    CHR *chr = (CHR *)args[1];
    char *obj = (char *)args[0];
    ACTSEQ *seq;

    if (chr != 0) {
        CHR_PEER(obj) = chr;
        chr->pUpdate = ACT_updateSequence;
        chr->n088 = 0;
        chr->nSignal = 0;
        seq = &actSequence[chr->nSerial];
        chr->pObject = obj;
        seq->field_00 = 0;
        seq->nFlags = 0;
        seq->field_0C = 0;
        seq->field_14 = 0;
        seq->field_18 = 0;
        seq->field_1C = 0;
        seq->field_20 = 0;
        seq->pFunc = 0;
        seq->field_28 = 0;
        seq->field_2C = 0;
        seq->field_30 = 0;
    }
}

/* Toggle whether the character appears on the radar */
void Java_xeno_Chr_dispRadar__Z(void *env, int *args, int *ret)
{
    char *obj = (char *)args[0];
    CHR *chr = CHR_PEER(obj);

    if (*(unsigned char *)&args[1] != 0) {
        chr->nFlags &= ~0x80;
    } else {
        chr->nFlags |= 0x80;
    }
}

/* Make the character look at the active camera */
void Java_xeno_Chr_look_camera__(void *env, int *args, int *ret)
{
    char *obj = (char *)args[0];

    CHR_PEER(obj)->nLookMode = 2;
}

/* Make the character look at another character */
void Java_xeno_Chr_look_char__Ljava_lang_Object_(void *env, int *args, int *ret)
{
    char *obj = (char *)args[0];
    JAVA_FIELD *pField = lookupClassField(classJava_xeno_Chr, loadConstString(D_004DC190, -1), 0);
    char *pTarget = (char *)args[1];
    CHR *chr = *(CHR **)(obj + pField->nOffset);

    chr->nLookMode = 4;
    chr->nLookTarget = *(int *)(pTarget + pField->nOffset);
}

/* Make the character look at a unit object */
void Java_xeno_Chr_look_unit__Ljava_lang_Object_(void *env, int *args, int *ret)
{
    char *obj = (char *)args[0];
    CHR *chr = CHR_PEER(obj);

    int nUnit = UNIT_PEER((char *)args[1]);

    chr->nLookMode = 5;
    chr->nLookTarget = nUnit;
}

/* Make the character look at a world-space point */
/* TODO: not matching - the mode store schedules after the first float store */
void Java_xeno_Chr_look_point__FFF(void *env, int *args, int *ret)
{
    char *obj = (char *)args[0];
    CHR *chr = CHR_PEER(obj);

    chr->fLookPoint[0] = *(float *)&args[1];
    chr->fLookPoint[1] = *(float *)&args[2];
    chr->fLookPoint[2] = *(float *)&args[3];
    chr->nLookMode = 3;
}

/* Point the character's eyes at explicit yaw/pitch angles */
/* TODO: not matching - the eye-control store schedules before the first float store */
void Java_xeno_Chr_look_eye_set__FF(void *env, int *args, int *ret)
{
    char *obj = (char *)args[0];
    CHR *chr = CHR_PEER(obj);

    chr->fLookEye[0] = *(float *)&args[1] * D_004D841C / 180.0f;
    chr->fLookEye[1] = *(float *)&args[2] * D_004D841C / 180.0f;
    chr->nLookEyeControl = 0xE;
}

/* Select which eye bones the look-at system drives */
void Java_xeno_Chr_look_eye_control__I(void *env, int *args, int *ret)
{
    char *obj = (char *)args[0];

    CHR_PEER(obj)->nLookEyeControl = args[1];
}

/* Set the look-at head tracking speed */
void Java_xeno_Chr_look_speed__F(void *env, int *args, int *ret)
{
    char *obj = (char *)args[0];

    CHR_PEER(obj)->fLookSpeed = *(float *)&args[1];
}

/* Set the look-at eye tracking speed */
void Java_xeno_Chr_look_eye_speed__F(void *env, int *args, int *ret)
{
    char *obj = (char *)args[0];

    CHR_PEER(obj)->fLookEyeSpeed = *(float *)&args[1];
}

/* Set the render command index used to draw the character */
void Java_xeno_Chr_renderCommand__I(void *env, int *args, int *ret)
{
    char *obj = (char *)args[0];

    CHR_PEER(obj)->nRenderCommand = args[1];
}

/* Set the scale applied to the shadow clip volume */
void Java_xeno_Chr_shadow_clip_scale__F(void *env, int *args, int *ret)
{
    char *obj = (char *)args[0];

    CHR_PEER(obj)->fShadowClipScale = *(float *)&args[1];
}

/* Append a shadow map id to the character's list */
void Java_xeno_Chr_shadow_map_id__I(void *env, int *args, int *ret)
{
    char *obj = (char *)args[0];
    CHR *chr = CHR_PEER(obj);

    if (chr->nShadowMapNum < 8) {
        chr->nShadowMapId[chr->nShadowMapNum++] = *(unsigned short *)&args[1];
    }
}

/* Drop every shadow map id registered on the character */
void Java_xeno_Chr_shadow_map_reset__(void *env, int *args, int *ret)
{
    char *obj = (char *)args[0];

    CHR_PEER(obj)->nShadowMapNum = 0;
}

/* Freeze the character's hair simulation between two bone indices */
void Java_xeno_Chr_hairStop__II(void *env, int *args, int *ret)
{
    char *obj = (char *)args[0];
    CHR *chr = CHR_PEER(obj);

    chr->nHairStop0 = args[1];
    chr->nHairStop1 = args[2];
}

/* Set the character's global per-pixel alpha */
void Java_xeno_Chr_pixelAlpha__I(void *env, int *args, int *ret)
{
    char *obj = (char *)args[0];

    CHR_PEER(obj)->nPixelAlpha = args[1];
}

/* Append a per-part per-pixel alpha override */
/* TODO: not matching - the count store schedules first and $a1/$a2 are swapped */
void Java_xeno_Chr_pixelAlphaParts__II(void *env, int *args, int *ret)
{
    char *obj = (char *)args[0];
    CHR *chr = CHR_PEER(obj);
    int nNum = chr->nPixelAlphaPartsNum;

    if (nNum < 4) {
        chr->alphaParts[nNum * 2] = *(unsigned short *)&args[2];
        chr->alphaParts[nNum * 2 + 1] = *(unsigned short *)&args[1];
        chr->nPixelAlphaPartsNum++;
    }
}

/* Drop every per-part per-pixel alpha override */
void Java_xeno_Chr_pixelAlphaPartsReset__(void *env, int *args, int *ret)
{
    char *obj = (char *)args[0];

    CHR_PEER(obj)->nPixelAlphaPartsNum = 0;
}

/* Select which parts of the actor the motion update is allowed to touch */
void Java_xeno_Chr_setMotNoUpdate__I(void *env, int *args, int *ret)
{
    char *obj = (char *)args[0];
    CHR *chr = CHR_PEER(obj);

    switch (args[1]) {
    case 0:
        chr->nFlags &= ~0x400;
        chr->nFlags2 &= ~0x100;
        break;
    case 1:
        chr->nFlags |= 0x400;
        chr->nFlags2 &= ~0x100;
        break;
    case 2:
        chr->nFlags &= ~0x400;
        chr->nFlags2 |= 0x100;
        break;
    }
}

/* Attach another character as this one's right-hand weapon */
void Java_xeno_Chr_setWeaponR__Lxeno_Chr_(void *env, int *args, int *ret)
{
    char *obj = (char *)args[0];
    JAVA_FIELD *pField = lookupClassField(classJava_xeno_Chr, loadConstString(D_004DC190, -1), 0);
    CHR *chr = *(CHR **)(obj + pField->nOffset);

    obj = (char *)args[1];
    ACT_setArms(chr, *(CHR **)(obj + pField->nOffset), 0x108, 2);
}

/* Detach the character's right-hand weapon */
void Java_xeno_Chr_resetWeaponR__Lxeno_Chr_(void *env, int *args, int *ret)
{
    char *obj = (char *)args[0];
    JAVA_FIELD *pField = lookupClassField(classJava_xeno_Chr, loadConstString(D_004DC190, -1), 0);
    CHR *chr = *(CHR **)(obj + pField->nOffset);

    obj = (char *)args[1];
    ACT_resetArms(chr, *(CHR **)(obj + pField->nOffset), 0x108);
}

/* Chr.resetHand() - unimplemented */
void Java_xeno_Chr_resetHand__(void *env, int *args, int *ret)
{
}

/* Chr.resetEnv() - unimplemented */
void Java_xeno_Chr_resetEnv__(void *env, int *args, int *ret)
{
}

/* Toggle whether the character's collision shape is ignored */
void Java_xeno_Chr_ignoreShape__I(void *env, int *args, int *ret)
{
    char *obj = (char *)args[0];
    CHR *chr = CHR_PEER(obj);

    if (args[1] != 0) {
        chr->nFlags2 |= 0x400;
    } else {
        chr->nFlags2 &= ~0x400;
    }
}
