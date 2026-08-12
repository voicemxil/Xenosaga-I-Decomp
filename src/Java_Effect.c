/* Native bindings for the script VM's xeno.Effect class */

typedef union {
    int i;
    unsigned int u;
    float f;
    short h;
    unsigned char c;
    void *p;
} JVAL;

typedef struct {
    char pad00[0x10];
    int nOffset;
} JFIELD;

typedef struct {
    char pad00[0x18];
    int nInstance;
} JCLASS;

typedef struct {
    int field_0;
    unsigned int nLength;
    char *pData;
} JARRAY;

typedef struct {
    int nClass;                 /* 0x00 */
    float z;                    /* 0x04 */
    float y;                    /* 0x08 */
    float x;                    /* 0x0C */
    float w;                    /* 0x10 */
} JVECTOR4F;

typedef struct {
    char pad000[0x80];
    float aTranslate[4];        /* 0x080 */
    float aRotate[4];           /* 0x090 */
    float aScale[4];            /* 0x0A0 */
    char pad0B0[0x60C];
    int nCaster;                /* 0x6BC */
    int nTargetChr;             /* 0x6C0 */
    int nCasterUnit;            /* 0x6C4 */
    int nTargetUnit;            /* 0x6C8 */
    char pad6CC[0x3C0];
    int nFlags;                 /* 0xA8C */
    char padA90[0xA];
    unsigned char nDisp;        /* 0xA9A */
} EFFECT;

typedef struct {
    char pad00[0xE8];
    EFFECT *pEffect;            /* 0xE8 */
} UNITWORK;

int classJava_xeno_Effect;
int classJava_xeno_Chr;
int classJava_xeno_Unit;
int classJava_xeno_util_Vector4f;
float D_004D83BC;
float D_004D83C0;

extern char D_004DC130[];
extern char D_004DC138[];
extern char D_004DC140[];
extern char D_004DC148[];
extern char D_004DC150[];
extern char D_004DC158[];
extern char D_004DC160[];
extern char D_004DC168[];
extern char D_004DC170[];

int loadConstString(char *, int);
JFIELD *lookupClassField(int, int, int);
void FX_call(int, int, char *, unsigned int);
void sefRewindEffectCf(EFFECT *);
void sefClearEffectCf(EFFECT *);

#define EFFECT_FIELD(name) \
    (lookupClassField(classJava_xeno_Effect, loadConstString(name, -1), 0)->nOffset)

#define EFFECT_PEER(obj) (*(EFFECT **)((obj) + EFFECT_FIELD(D_004DC140)))

/* Fire the effect described by the script object's id and name */
void Java_xeno_Effect_call__I(void *pEnv, JVAL *pArgs, JVAL *pRet)
{
    char *obj = (char *)pArgs[0].p;
    int nId;
    JARRAY *pArray;

    if (obj != 0) {
        nId = *(int *)(obj + EFFECT_FIELD(D_004DC130));
        pArray = *(JARRAY **)(obj + EFFECT_FIELD(D_004DC138));
        FX_call(pArgs[1].i, nId, pArray->pData, pArray->nLength);
    }
}

/* Show or hide the effect, rewinding it when it is shown */
void Java_xeno_Effect_disp__Z(void *pEnv, JVAL *pArgs, JVAL *pRet)
{
    char *obj = (char *)pArgs[0].p;
    EFFECT *pEff;

    if (obj != 0) {
        pEff = EFFECT_PEER(obj);
        if (pEff != 0) {
            if (pArgs[1].c != 0) {
                sefRewindEffectCf(pEff);
                pEff->nDisp = 1;
            } else {
                pEff->nDisp = 0;
            }
        }
    }
}

/* Set the effect's XYZ scale */
void Java_xeno_Effect_setScale__FFF(void *pEnv, JVAL *pArgs, JVAL *pRet)
{
    char *obj = (char *)pArgs[0].p;
    EFFECT *pEff;

    if (obj != 0) {
        pEff = EFFECT_PEER(obj);
        if (pEff != 0) {
            pEff->aScale[0] = pArgs[1].f;
            pEff->aScale[1] = pArgs[2].f;
            pEff->aScale[2] = pArgs[3].f;
        }
    }
}

/* Return the effect's scale as a Vector4f */
void Java_xeno_Effect_getScale__(void *pEnv, JVAL *pArgs, JVAL *pRet)
{
    static JVECTOR4F scale;
    char *obj = (char *)pArgs[0].p;
    EFFECT *pEff;
    JVECTOR4F *p;

    if (obj != 0) {
        pEff = EFFECT_PEER(obj);
        p = &scale;
        p->nClass = ((JCLASS *)classJava_xeno_util_Vector4f)->nInstance;
        p->x = pEff->aScale[0];
        p->y = pEff->aScale[1];
        p->z = pEff->aScale[2];
        p->w = pEff->aScale[3];
        pRet->p = p;
    }
}

/* Copy the effect's position into the script object's fields */
void Java_xeno_Effect_getTranslate__(void *pEnv, JVAL *pArgs, JVAL *pRet)
{
    char *obj = (char *)pArgs[0].p;
    float *pT;

    if (obj != 0) {
        pT = EFFECT_PEER(obj)->aTranslate;
        *(float *)(obj + EFFECT_FIELD(D_004DC148)) = pT[0];
        *(float *)(obj + EFFECT_FIELD(D_004DC150)) = pT[1];
        *(float *)(obj + EFFECT_FIELD(D_004DC158)) = pT[2];
    }
}

/* Copy the script object's fields into the effect's position */
void Java_xeno_Effect_setTranslate__(void *pEnv, JVAL *pArgs, JVAL *pRet)
{
    char *obj = (char *)pArgs[0].p;
    float *pT;

    if (obj != 0) {
        pT = EFFECT_PEER(obj)->aTranslate;
        pT[0] = *(float *)(obj + EFFECT_FIELD(D_004DC148));
        pT[1] = *(float *)(obj + EFFECT_FIELD(D_004DC150));
        pT[2] = *(float *)(obj + EFFECT_FIELD(D_004DC158));
    }
}

/* Copy the effect's rotation out, converting radians to degrees */
void Java_xeno_Effect_getRotate__(void *pEnv, JVAL *pArgs, JVAL *pRet)
{
    char *obj = (char *)pArgs[0].p;

    if (obj != 0) {
        float fPi = D_004D83BC;
        float *pR = EFFECT_PEER(obj)->aRotate;

        *(float *)(obj + EFFECT_FIELD(D_004DC160)) = pR[0] / fPi * 180.0f;
        *(float *)(obj + EFFECT_FIELD(D_004DC168)) = pR[1] / fPi * 180.0f;
        *(float *)(obj + EFFECT_FIELD(D_004DC170)) = pR[2] / fPi * 180.0f;
    }
}

/* Copy the script object's rotation in, converting degrees to radians */
void Java_xeno_Effect_setRotate__(void *pEnv, JVAL *pArgs, JVAL *pRet)
{
    char *obj = (char *)pArgs[0].p;

    if (obj != 0) {
        float *pR = EFFECT_PEER(obj)->aRotate;

        pR[0] = *(float *)(obj + EFFECT_FIELD(D_004DC160)) / 180.0f * 3.1415927f;
        pR[1] = *(float *)(obj + EFFECT_FIELD(D_004DC168)) / 180.0f * 3.1415927f;
        pR[2] = *(float *)(obj + EFFECT_FIELD(D_004DC170)) / 180.0f * 3.1415927f;
    }
}

/* Attach a character as the effect's caster */
void Java_xeno_Effect_setCaster__Lxeno_Chr_(void *pEnv, JVAL *pArgs, JVAL *pRet)
{
    char *obj = (char *)pArgs[0].p;
    EFFECT *pEff;

    if (obj != 0) {
        pEff = EFFECT_PEER(obj);
        obj = (char *)pArgs[1].p;
        pEff->nCaster = *(int *)(obj + lookupClassField(classJava_xeno_Chr,
            loadConstString(D_004DC140, -1), 0)->nOffset);
    }
}

/* Attach a character as the effect's target */
void Java_xeno_Effect_setTarget__Lxeno_Chr_(void *pEnv, JVAL *pArgs, JVAL *pRet)
{
    char *obj = (char *)pArgs[0].p;
    EFFECT *pEff;

    if (obj != 0) {
        pEff = EFFECT_PEER(obj);
        obj = (char *)pArgs[1].p;
        pEff->nTargetChr = *(int *)(obj + lookupClassField(classJava_xeno_Chr,
            loadConstString(D_004DC140, -1), 0)->nOffset);
    }
}

/* Attach a unit as the effect's caster and link it back */
void Java_xeno_Effect_setCaster__Lxeno_Unit_(void *pEnv, JVAL *pArgs, JVAL *pRet)
{
    char *obj = (char *)pArgs[0].p;
    EFFECT *pEff;
    UNITWORK *pUnit;

    if (obj != 0) {
        pEff = EFFECT_PEER(obj);
        if (pEff != 0) {
            obj = (char *)pArgs[1].p;
            pUnit = *(UNITWORK **)(obj + lookupClassField(classJava_xeno_Unit,
                loadConstString(D_004DC140, -1), 0)->nOffset);
            if (pUnit != 0) {
                pEff->nCasterUnit = (int)pUnit;
                pUnit->pEffect = pEff;
            }
        }
    }
}

/* Attach a unit as the effect's target */
void Java_xeno_Effect_setTarget__Lxeno_Unit_(void *pEnv, JVAL *pArgs, JVAL *pRet)
{
    char *obj = (char *)pArgs[0].p;
    EFFECT *pEff;
    UNITWORK *pUnit;

    if (obj != 0) {
        pEff = EFFECT_PEER(obj);
        if (pEff != 0) {
            obj = (char *)pArgs[1].p;
            pUnit = *(UNITWORK **)(obj + lookupClassField(classJava_xeno_Unit,
                loadConstString(D_004DC140, -1), 0)->nOffset);
            if (pUnit != 0) {
                pEff->nTargetUnit = (int)pUnit;
            }
        }
    }
}

/* Nudge the effect's position by an XYZ offset */
void Java_xeno_Effect_setTransOffset__FFF(void *pEnv, JVAL *pArgs, JVAL *pRet)
{
    char *obj = (char *)pArgs[0].p;
    EFFECT *pEff;

    if (obj != 0) {
        pEff = EFFECT_PEER(obj);
        if (pEff != 0) {
            pEff->aTranslate[0] += pArgs[1].f;
            pEff->aTranslate[1] += pArgs[2].f;
            pEff->aTranslate[2] += pArgs[3].f;
        }
    }
}

/* Read back the effect's force-loop flag */
void Java_xeno_Effect_getForceLoop__(void *pEnv, JVAL *pArgs, JVAL *pRet)
{
    char *obj = (char *)pArgs[0].p;
    EFFECT *pEff;

    if (obj != 0) {
        pEff = EFFECT_PEER(obj);
        if (pEff != 0) {
            pRet->c = *(unsigned char *)&pEff->nFlags & 0x20;
        }
    }
}

/* Set or clear the effect's force-loop flag */
void Java_xeno_Effect_setForceLoop__Z(void *pEnv, JVAL *pArgs, JVAL *pRet)
{
    char *obj = (char *)pArgs[0].p;
    EFFECT *pEff;

    if (obj != 0) {
        pEff = EFFECT_PEER(obj);
        if (pEff != 0) {
            if (pArgs[1].c == 1) {
                pEff->nFlags |= 0x20;
            } else {
                pEff->nFlags &= ~0x20;
            }
        }
    }
}

/* Read back the effect's clipping flag */
void Java_xeno_Effect_getClip__(void *pEnv, JVAL *pArgs, JVAL *pRet)
{
    char *obj = (char *)pArgs[0].p;
    EFFECT *pEff;

    if (obj != 0) {
        pEff = EFFECT_PEER(obj);
        if (pEff != 0) {
            pRet->c = *(unsigned char *)&pEff->nFlags & 0x10;
        }
    }
}

/* Set or clear the effect's clipping flag */
void Java_xeno_Effect_setClip__Z(void *pEnv, JVAL *pArgs, JVAL *pRet)
{
    char *obj = (char *)pArgs[0].p;
    EFFECT *pEff;

    if (obj != 0) {
        pEff = EFFECT_PEER(obj);
        if (pEff != 0) {
            if (pArgs[1].c == 1) {
                pEff->nFlags |= 0x10;
            } else {
                pEff->nFlags &= ~0x10;
            }
        }
    }
}

/* Tear the effect down */
void Java_xeno_Effect_clearEffect__(void *pEnv, JVAL *pArgs, JVAL *pRet)
{
    char *obj = (char *)pArgs[0].p;
    EFFECT *pEff;

    if (obj != 0) {
        pEff = EFFECT_PEER(obj);
        if (pEff != 0) {
            sefClearEffectCf(pEff);
        }
    }
}

/* Set or clear the effect's motion flag */
void Java_xeno_Effect_setMotion__Z(void *pEnv, JVAL *pArgs, JVAL *pRet)
{
    char *obj = (char *)pArgs[0].p;
    EFFECT *pEff;

    if (obj != 0) {
        pEff = EFFECT_PEER(obj);
        if (pEff != 0) {
            if (pArgs[1].c == 1) {
                pEff->nFlags |= 0x80;
            } else {
                pEff->nFlags &= ~0x80;
            }
        }
    }
}

/* Set or clear the effect's no-attach flag */
void Java_xeno_Effect_noAttach__Z(void *pEnv, JVAL *pArgs, JVAL *pRet)
{
    char *obj = (char *)pArgs[0].p;
    EFFECT *pEff;

    if (obj != 0) {
        pEff = EFFECT_PEER(obj);
        if (pEff != 0) {
            if (pArgs[1].c == 1) {
                pEff->nFlags |= 0x400;
            } else {
                pEff->nFlags &= ~0x400;
            }
        }
    }
}
