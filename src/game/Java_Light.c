/* Native bindings for the script VM's xeno.Light class */

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

int classJava_xeno_Light;
float D_004D83EC;

extern char D_004DC180[];
extern char D_004DC188[];

int loadConstString(char *, int);
JFIELD *lookupClassField(int, int, int);
int xglStudioGetLight2(void);
void xglLightIntensityAmbient(int, float *);
void xglLightIntensityParallel(int, int, float *);
void xglLightAngle(int, int, float *);
void xglLightDirection(int, int, float *);
void nmlModelSetGlobalPointLightCol(int, float *);
void nmlModelSetGlobalPointLightPos(int, float *);
void nmlModelSetGlobalPointLightReset(void);

#define LIGHT_FIELD(name) \
    (lookupClassField(classJava_xeno_Light, loadConstString(name, -1), 0)->nOffset)

/* Set the light's intensity colour (ambient or parallel) */
void Java_xeno_Light_setColor__FFF(void *pEnv, JVAL *pArgs, JVAL *pRet)
{
    char *obj = (char *)pArgs[0].p;
    float aCol[4];
    int nMode;
    int nLight;

    nMode = *(int *)(obj + LIGHT_FIELD(D_004DC180));
    nLight = *(int *)(obj + LIGHT_FIELD(D_004DC188));
    if (nLight == 0) {
        nLight = xglStudioGetLight2();
    }
    aCol[0] = pArgs[1].f;
    aCol[1] = pArgs[2].f;
    aCol[2] = pArgs[3].f;
    aCol[3] = 1.0f;
    if (nMode == 0) {
        xglLightIntensityAmbient(nLight, aCol);
    } else {
        xglLightIntensityParallel(nLight, nMode - 1, aCol);
    }
}

/* Aim the light using Euler angles in degrees */
/* TODO: not matching - the original assembler put a hazard nop between the
   li.s and the swc1 that consumes it; fix_cc_asm.py only models the
   load-feeds-FP-compute case, so that one nop is missing */
void Java_xeno_Light_setDirection__FFF(void *pEnv, JVAL *pArgs, JVAL *pRet)
{
    char *obj = (char *)pArgs[0].p;
    float aDir[4];
    int nMode;
    int nLight;

    nMode = *(int *)(obj + LIGHT_FIELD(D_004DC180));
    nLight = *(int *)(obj + LIGHT_FIELD(D_004DC188));
    if (nLight == 0) {
        nLight = xglStudioGetLight2();
    }
    aDir[0] = pArgs[1].f / 180.0f * D_004D83EC;
    aDir[1] = pArgs[2].f / 180.0f * D_004D83EC;
    aDir[2] = pArgs[3].f / 180.0f * D_004D83EC;
    aDir[3] = 1.0f;
    xglLightAngle(nLight, nMode - 1, aDir);
}

/* Aim one of the three parallel lights with a direction vector */
void Java_xeno_Light_setDirection2__FFF(void *pEnv, JVAL *pArgs, JVAL *pRet)
{
    char *obj = (char *)pArgs[0].p;
    float aDir[4];
    int nMode;
    int nLight;

    nMode = *(int *)(obj + LIGHT_FIELD(D_004DC180));
    nLight = *(int *)(obj + LIGHT_FIELD(D_004DC188));
    if (nLight == 0) {
        nLight = xglStudioGetLight2();
    }
    aDir[0] = pArgs[1].f;
    aDir[1] = pArgs[2].f;
    aDir[2] = pArgs[3].f;
    aDir[3] = 1.0f;
    if ((unsigned int)(nMode - 1) < 3) {
        xglLightDirection(nLight, nMode - 1, aDir);
    }
}

/* Set a global point light's colour */
void Java_xeno_Light_setGlobalPointLightCol__IFFF(void *pEnv, JVAL *pArgs, JVAL *pRet)
{
    float aCol[3];

    aCol[0] = pArgs[2].f;
    aCol[1] = pArgs[3].f;
    aCol[2] = pArgs[4].f;
    nmlModelSetGlobalPointLightCol(pArgs[1].i, aCol);
}

/* Set a global point light's position */
void Java_xeno_Light_setGlobalPointLightPos__IFFF(void *pEnv, JVAL *pArgs, JVAL *pRet)
{
    float aPos[3];

    aPos[0] = pArgs[2].f;
    aPos[1] = pArgs[3].f;
    aPos[2] = pArgs[4].f;
    nmlModelSetGlobalPointLightPos(pArgs[1].i, aPos);
}

/* Drop all global point lights */
void Java_xeno_Light_setGlobalPointLightReset__(void *pEnv, JVAL *pArgs, JVAL *pRet)
{
    nmlModelSetGlobalPointLightReset();
}
