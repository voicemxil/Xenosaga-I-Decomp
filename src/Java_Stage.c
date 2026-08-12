/* Native bindings for the script VM's xeno.Stage class */

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
    int field_0;
    unsigned int nLength;
    char *pData;
} JARRAY;

typedef struct {
    int field_0;
    JARRAY *pArray;
} JSTRING;

typedef struct {
    int nClass;                 /* 0x00 */
    char pad04[0x9];
    unsigned char n0D;          /* 0x0D */
    char pad0E[0x6];
    void *pStage;               /* 0x14 */
    void *pMethod;              /* 0x18 */
    char pad1C[0x8];
    int nFlags;                 /* 0x24 */
    char pad28[0x14];
    short field_3C;             /* 0x3C */
    unsigned short field_3E;    /* 0x3E */
} JTHREAD;

typedef struct {
    float aColor[3];            /* 0x00 */
    float fTime;                /* 0x0C */
} FADEPARAM __attribute__((aligned(8)));

typedef struct {
    long long pad000[5];
    int pad028;
    int n02C;                   /* 0x02C */
    int n030;                   /* 0x030 */
    char pad034[0x24];
    int n058;                   /* 0x058 */
    int n05C;                   /* 0x05C */
    char pad060[0x20];
    float aColor[3];            /* 0x080 */
    char pad08C[0x4];
    FADEPARAM fade;             /* 0x090 */
    FADEPARAM fadePrev;         /* 0x0A0 */
} GAMELOOPSTATE;

typedef struct {
    char pad00[0x40];
    int nParts;                 /* 0x40 */
} NMLMODEL;

typedef struct {
    int field_0;
    NMLMODEL *pModel;           /* 0x04 */
} STAGEWORK;

int classJava_xeno_Stage;
int TYPE_Void;


extern char D_004DC208[];
extern GAMELOOPSTATE GameLoopState;
extern STAGEWORK *D_003386D4[];
extern int D_00338730[];
extern int D_00338734[];
extern char JTHREAD_defaultStage[];

int loadConstString(char *, int);
JFIELD *lookupClassField(int, int, int);
int JNI_isInstanceOf(int, int);
void *findMethod(int, int, int);
JTHREAD *JTHREAD_get(int);
void SCRIPT_load_DBG(char *);
void SCRIPT_exec(void);
void SCRIPT_fade(int);
void EnemySound_StopAll(int);
void nmlModelSetMapLastEntry(NMLMODEL *, int);
void nmlModelSetMapLastInit(void);
void nmlModelSetBackBuffer(int, int, int, int);
void nmlModelSetBackBufferClear(void);
void nmlModelSetEffectWrite(int);
void nmlModelSetFadeInInterrupt(int, float, float, float);
void nmlModelSetFadeInCancel(int);
void nmlModelSetFadeOutCancel(int);
void nmlModelSetPartsVisible(NMLMODEL *, int, int);
void nmlModelInitPartsVisible(NMLMODEL *, int);
void GameBgDrawType1Entry(char *);
void GameBgDrawType2Entry(char *);
void xglRenderClearColor(unsigned int);

/* Bind a script method to the stage thread and start it */
/* TODO: not matching - four loads in the findMethod argument block are
   scheduled the other way round and the mode test comes out bne, not bnel */
void Java_xeno_Stage_start__ILjava_lang_Object_(JTHREAD *pEnv, JVAL *pArgs, JVAL *pRet)
{
    int nClass = classJava_xeno_Stage;
    char *obj = (char *)pArgs[0].p;
    JSTRING *pName;
    JARRAY *pArr;
    JTHREAD *pThread;
    void *pMethod;
    int nStage;

    if (JNI_isInstanceOf((int)obj, nClass) == 0) {
        pRet->i = 0;
        return;
    }
    nStage = *(int *)(obj + lookupClassField(nClass, loadConstString(D_004DC208, -1), 0)->nOffset);
    if (pArgs[1].i == 0) {
        return;
    }
    if (pArgs[1].i != 1) {
        return;
    }
    *(int *)(nStage + 0xC) = 0;
    pName = (JSTRING *)pArgs[2].p;
    pArr = pName->pArray;
    nClass = *(int *)*(int *)obj;
    pMethod = findMethod(nClass, loadConstString(pArr->pData, pArr->nLength), TYPE_Void);
    pThread = JTHREAD_get(nStage);
    if (pThread != 0) {
        pThread->pStage = JTHREAD_defaultStage;
        pThread->pMethod = pMethod;
        pThread->nFlags |= 0x10;
    }
    if (pThread == pEnv) {
        pThread->nFlags |= 0x21;
    }
}

/* Clear the stage thread's running flag */
void Java_xeno_Stage_stop__(void *pEnv, JVAL *pArgs, JVAL *pRet)
{
    char *obj = (char *)pArgs[0].p;
    JTHREAD *pThread;

    if (JNI_isInstanceOf((int)obj, classJava_xeno_Stage) == 0) {
        pRet->i = 0;
        return;
    }
    pThread = JTHREAD_get(*(int *)(obj + lookupClassField(classJava_xeno_Stage,
        loadConstString(D_004DC208, -1), 0)->nOffset));
    if (pThread != 0) {
        pThread->nFlags &= ~0x10;
    }
}

/* Load and run a script file on the calling thread */
void Java_xeno_Stage_play__Ljava_lang_String_(JTHREAD *pEnv, JVAL *pArgs, JVAL *pRet)
{
    JSTRING *pName = (JSTRING *)pArgs[1].p;

    pEnv->n0D = 7;
    pEnv->nFlags |= 5;
    pEnv->field_3C = pEnv->field_3E;
    SCRIPT_load_DBG(pName->pArray->pData);
    SCRIPT_exec();
}

/* Mark a map part as the last one drawn */
void Java_xeno_Stage_setPartsLast__I(void *pEnv, JVAL *pArgs, JVAL *pRet)
{
    STAGEWORK *pWork = D_003386D4[0];

    if (pWork != 0) {
        NMLMODEL *pModel = pWork->pModel;

        if (pModel != 0) {
            nmlModelSetMapLastEntry(pModel, pArgs[0].i);
        }
    }
}

/* Reset the map's last-drawn part list */
void Java_xeno_Stage_setPartsLastReset__(void *pEnv, JVAL *pArgs, JVAL *pRet)
{
    nmlModelSetMapLastInit();
}

/* Set the stage's ambient RGB tint */
void Java_xeno_Stage_setColor__FFF(void *pEnv, JVAL *pArgs, JVAL *pRet)
{
    GameLoopState.aColor[0] = pArgs[0].f;
    GameLoopState.aColor[1] = pArgs[1].f;
    GameLoopState.aColor[2] = pArgs[2].f;
}

/* Start a screen fade, clamping the colour to 0..1 */
void Java_xeno_Stage_setFade__IIFFF(void *pEnv, JVAL *pArgs, JVAL *pRet)
{
    float *pCol;
    int i;

    GameLoopState.n058 = pArgs[0].i;
    GameLoopState.fade.fTime = (float)pArgs[1].i;
    GameLoopState.fade.aColor[0] = pArgs[2].f;
    GameLoopState.fade.aColor[1] = pArgs[3].f;
    GameLoopState.fade.aColor[2] = pArgs[4].f;
    EnemySound_StopAll(1);
    pCol = GameLoopState.fade.aColor;
    for (i = 2; i >= 0; i--) {
        if (1.0f < *pCol) {
            *pCol = 1.0f;
        }
        if (*pCol < 0.0f) {
            *pCol = 0.0f;
        }
        pCol++;
    }
    if (GameLoopState.fade.fTime < 1.0f) {
        GameLoopState.fade.fTime = 1.0f;
    }
    GameLoopState.n05C = GameLoopState.n058;
    GameLoopState.fadePrev = GameLoopState.fade;
    SCRIPT_fade(pArgs[1].i);
}

/* Interrupt the model fade-in and start a screen fade */
void Java_xeno_Stage_setEventFade__IFFFIFFF(void *pEnv, JVAL *pArgs, JVAL *pRet)
{
    nmlModelSetFadeInInterrupt(pArgs[0].i, pArgs[1].f, pArgs[2].f, pArgs[3].f);
    GameLoopState.n058 = 0;
    GameLoopState.fade.fTime = (float)pArgs[4].i;
    GameLoopState.fade.aColor[0] = pArgs[5].f;
    GameLoopState.fade.aColor[1] = pArgs[6].f;
    GameLoopState.fade.aColor[2] = pArgs[7].f;
    SCRIPT_fade(pArgs[4].i);
}

/* Select the back buffer used for frame rendering */
void Java_xeno_Stage_setFrameRender__II(void *pEnv, JVAL *pArgs, JVAL *pRet)
{
    nmlModelSetBackBuffer(pArgs[0].i, pArgs[1].i, -1, 0);
}

/* Cancel the model fade in, out, or both */
void Java_xeno_Stage_setFadeCancel__I(void *pEnv, JVAL *pArgs, JVAL *pRet)
{
    switch (pArgs[0].i) {
    case 0:
        nmlModelSetFadeInCancel(0x3C);
        nmlModelSetFadeOutCancel(0x3C);
        break;
    case 1:
        nmlModelSetFadeOutCancel(0x3C);
        break;
    case 2:
        nmlModelSetFadeInCancel(0x3C);
        break;
    }
}

/* Show or hide one map part, or reset them all */
/* TODO: not matching - gcc turns the setPartsVisible call into a sibling
   call here; the original keeps the jal and branches to the epilogue */
void Java_xeno_Stage_setVisible__IZ(void *pEnv, JVAL *pArgs, JVAL *pRet)
{
    STAGEWORK *pWork = D_003386D4[0];

    if (pWork != 0) {
        NMLMODEL *pModel = pWork->pModel;

        if (pModel != 0) {
            int nParts = pArgs[0].i;

            if (nParts >= 0) {
                if (nParts < pModel->nParts) {
                    nmlModelSetPartsVisible(pModel, nParts, pArgs[1].c);
                }
            } else {
                nmlModelInitPartsVisible(pModel, pArgs[1].c);
            }
        }
    }
}

/* Enter, leave, or switch the pre-rendered background mode */
void Java_xeno_Stage_setCFBG__II_F(void *pEnv, JVAL *pArgs, JVAL *pRet)
{
    JARRAY *pArray = (JARRAY *)pArgs[2].p;

    switch (pArgs[0].i) {
    case 0:
        GameLoopState.n02C = 0;
        GameLoopState.n030 = 0;
        break;
    case 1:
        GameBgDrawType1Entry(pArray->pData);
        break;
    case 2:
        GameBgDrawType2Entry(pArray->pData);
        break;
    }
}

/* Toggle effect writes into the model pass */
void Java_xeno_Stage_setEffectRender__I(void *pEnv, JVAL *pArgs, JVAL *pRet)
{
    nmlModelSetEffectWrite(pArgs[0].i);
}

/* Set the render command flag */
void Java_xeno_Stage_renderCommand__I(void *pEnv, JVAL *pArgs, JVAL *pRet)
{
    D_00338730[0] = pArgs[0].i;
}

/* Pack an RGB triple into the render clear colour */
void Java_xeno_Stage_setBgColor__FFF(void *pEnv, JVAL *pArgs, JVAL *pRet)
{
    xglRenderClearColor((((unsigned int)(pArgs[2].f * 255.0f) & 0xFF) << 16)
                      + (((unsigned int)(pArgs[1].f * 255.0f) & 0xFF) << 8)
                      + ((unsigned int)(pArgs[0].f * 255.0f) & 0xFF));
}

/* Set the background clip flag */
void Java_xeno_Stage_setBgClip__I(void *pEnv, JVAL *pArgs, JVAL *pRet)
{
    D_00338734[0] = pArgs[0].i;
}

/* Request a back buffer clear */
void Java_xeno_Stage_clrBackBuffer__(void *pEnv, JVAL *pArgs, JVAL *pRet)
{
    nmlModelSetBackBufferClear();
}
