/* Native bindings for the script VM's xeno.PlayControl class */

typedef union {
    int i;
    unsigned int u;
    float f;
    short h;
    unsigned char c;
    void *p;
} JVAL;

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
    int field_0;
    JARRAY *pArray;
} JSTRING;

typedef struct {
    int nClass;                 /* 0x00 */
    int nSignal;                /* 0x04 */
    int nChart;                 /* 0x08 */
    int nCamera;                /* 0x0C */
    char pad10[0x1C];
    int nFlags;                 /* 0x2C */
    short nMode;                /* 0x30 */
    char pad32[0x2];
    float fStart;               /* 0x34 */
    float fEnd;                 /* 0x38 */
    float fSpeed;               /* 0x3C */
    int pad40;                  /* 0x40 */
    float fCur;                 /* 0x44 */
} PLAY;

int classJava_xeno_util_Window;
int TYPE_Void;
float D_004D83B8;

int loadConstString(char *, int);
void *findMethod(int, int, int);
PLAY *PLAY_getCurrent(void);
void PLAY_setup(PLAY *);
void PLAY_setTimeChart(int, int);
void PLAY_setObserver(PLAY *, int, int, char *, void *);
void *PLAY_getCallBackParams(int);
int TCH_getInfoID(int, char *, unsigned int);

/* Bind the script object to the current playback controller */
void Java_xeno_PlayControl_create__(void *pEnv, JVAL *pArgs, JVAL *pRet)
{
    int nClass = classJava_xeno_util_Window;
    PLAY *pPlay = PLAY_getCurrent();

    pPlay->nClass = ((JCLASS *)nClass)->nInstance;
    pRet->p = pPlay;
}

/* Read the controller's signal word */
void Java_xeno_PlayControl_getSignal__(void *pEnv, JVAL *pArgs, JVAL *pRet)
{
    pRet->i = ((PLAY *)pArgs[0].p)->nSignal;
}

/* Write the controller's signal word */
void Java_xeno_PlayControl_signal__I(void *pEnv, JVAL *pArgs, JVAL *pRet)
{
    ((PLAY *)pArgs[0].p)->nSignal = pArgs[1].i;
}

/* Set the playback mode and flags, then run the setup pass */
void Java_xeno_PlayControl_init__II(void *pEnv, JVAL *pArgs, JVAL *pRet)
{
    PLAY *pPlay = (PLAY *)pArgs[0].p;

    pPlay->nMode = pArgs[1].h;
    pPlay->nFlags = pArgs[2].i;
    PLAY_setup(pPlay);
}

/* Set mode, flags and the frame range, then run the setup pass */
void Java_xeno_PlayControl_init__IIIIF(void *pEnv, JVAL *pArgs, JVAL *pRet)
{
    PLAY *pPlay = (PLAY *)pArgs[0].p;

    pPlay->nMode = pArgs[1].h;
    pPlay->nFlags = pArgs[2].i;
    PLAY_setup(pPlay);
    pPlay->fStart = (float)pArgs[3].i * D_004D83B8;
    pPlay->fEnd = (float)pArgs[4].i * D_004D83B8;
    pPlay->fSpeed = pArgs[5].f * D_004D83B8;
    pPlay->fCur = pPlay->fStart;
}

/* Attach a camera to the controller */
void Java_xeno_PlayControl_loadCamera__Ljava_lang_Object_(void *pEnv, JVAL *pArgs, JVAL *pRet)
{
    ((PLAY *)pArgs[0].p)->nCamera = pArgs[1].i;
}

/* Attach a time chart to the controller */
void Java_xeno_PlayControl_loadTimeChart__Ljava_lang_Object_(void *pEnv, JVAL *pArgs, JVAL *pRet)
{
    PLAY_setTimeChart(pArgs[0].i, pArgs[1].i);
}

/* Set the controller's running flag */
void Java_xeno_PlayControl_start__(void *pEnv, JVAL *pArgs, JVAL *pRet)
{
    ((PLAY *)pArgs[0].p)->nFlags |= 1;
}

/* Clear the controller's running flag */
void Java_xeno_PlayControl_stop__(void *pEnv, JVAL *pArgs, JVAL *pRet)
{
    ((PLAY *)pArgs[0].p)->nFlags &= ~1;
}

/* Register a script callback on a numbered chart entry */
void Java_xeno_PlayControl_setObserver__IILjava_lang_Object_Ljava_lang_String_(void *pEnv, JVAL *pArgs, JVAL *pRet)
{
    JARRAY *pArr = ((JSTRING *)pArgs[4].p)->pArray;
    char *pObj = (char *)pArgs[3].p;
    int nClass = *(int *)*(int *)pObj;
    void *pMethod;

    pMethod = findMethod(nClass, loadConstString(pArr->pData, pArr->nLength), TYPE_Void);
    PLAY_setObserver((PLAY *)pArgs[0].p, pArgs[1].i, pArgs[2].i, pObj, pMethod);
}

/* Register a script callback on a named chart entry */
void Java_xeno_PlayControl_setObserver__ILjava_lang_String_Ljava_lang_Object_Ljava_lang_String_(void *pEnv, JVAL *pArgs, JVAL *pRet)
{
    JARRAY *pArr = ((JSTRING *)pArgs[4].p)->pArray;
    char *pObj = (char *)pArgs[3].p;
    int nClass = *(int *)*(int *)pObj;
    void *pMethod;
    PLAY *pPlay;
    JARRAY *pName;
    int nId;

    pMethod = findMethod(nClass, loadConstString(pArr->pData, pArr->nLength), TYPE_Void);
    pPlay = (PLAY *)pArgs[0].p;
    pName = ((JSTRING *)pArgs[2].p)->pArray;
    nId = TCH_getInfoID(pPlay->nChart, pName->pData, pName->nLength);
    if (nId >= 0) {
        PLAY_setObserver(pPlay, pArgs[1].i, nId, pObj, pMethod);
    }
}

/* Return the parameter block for the running callback */
void Java_xeno_PlayControl_getParams__(void *pEnv, JVAL *pArgs, JVAL *pRet)
{
    pRet->p = PLAY_getCallBackParams(pArgs[0].i);
}
