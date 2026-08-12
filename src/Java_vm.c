/* Native bindings for the script VM's xeno.vm classes (System, Thread, Math) */

typedef union {
    int i;
    unsigned int u;
    float f;
    short h;
    unsigned char c;
    void *p;
} JVAL;

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
    char pad00[0x8];
    char *pText;                /* 0x08 */
} JNAME;

typedef struct JDESC {
    int field_00;
    JNAME *pName;               /* 0x04 */
    char pad08[0x2];
    unsigned short nFlags;      /* 0x0A */
    char pad0C[0x2E];
    unsigned char nElemSize;    /* 0x3A */
    char pad3B[0x1];
    struct JDESC *pElem;        /* 0x3C */
} JDESC;

typedef struct {
    JDESC *pDesc;
} JCLASSREF;

typedef struct {
    JCLASSREF *pClass;          /* 0x00 */
    unsigned int nLength;       /* 0x04 */
    char *pData;                /* 0x08 */
} JOBJARRAY;

typedef struct {
    char pad00[0xD];
    unsigned char n0D;          /* 0x0D */
    char pad0E[0x2];
    void *pTarget;              /* 0x10 */
    void *pStage;               /* 0x14 */
    void *pMethod;              /* 0x18 */
    char pad1C[0x8];
    int nFlags;                 /* 0x24 */
    char pad28[0x8];
    int n030;                   /* 0x30 */
    int n034;                   /* 0x34 */
    char pad38[0x4];
    short field_3C;             /* 0x3C */
    unsigned short field_3E;    /* 0x3E */
} JTHREAD;

int TYPE_Void;

extern char JTHREAD_default[];

int loadConstString(char *, int);
void *findMethod(int, int, int);
void *memcpy(void *, const void *, unsigned int);
void System_waitFor(void *, int, int, int);
void SCRIPT_methodClearSet(void *);
int JNI_createThread(int, int, int);
void JNI_initThread(JTHREAD *);
int xglLRand(void);
float atan2f(float, float);
float xglSin(float);
float xglCos(float);

/* System.arraycopy - resolve the element size from the array class, then memcpy */
void Java_xeno_vm_System_arraycopy__Ljava_lang_Object_ILjava_lang_Object_II(void *pEnv, JVAL *pArgs, JVAL *pRet)
{
    JOBJARRAY *pSrc = (JOBJARRAY *)pArgs[0].p;
    JOBJARRAY *pDst = (JOBJARRAY *)pArgs[2].p;
    int nSrcPos = pArgs[1].i;
    int nDstPos = pArgs[3].i;
    int nLen = pArgs[4].i;
    JDESC *pDesc = pDst->pClass->pDesc;
    int nSize;

    if (pDesc->pName->pText[0] == '[') {
        do {
            pDesc = pDesc->pElem;
        } while (pDesc->pName->pText[0] == '[');
    }
    nSize = 4;
    if ((pDesc->nFlags & 0x100) != 0) {
        nSize = pDesc->nElemSize;
    }
    {
        char *pS = pSrc->pData + nSrcPos * nSize;
        char *pD = pDst->pData + nDstPos * nSize;

        memcpy(pD, pS, nLen * nSize);
    }
}

/* Put the calling thread to sleep for a number of frames */
void Java_xeno_vm_System_sleep__I(JTHREAD *pEnv, JVAL *pArgs, JVAL *pRet)
{
    int nFlags = pEnv->nFlags;

    if ((nFlags & 4) == 0) {
        int nTime;

        pEnv->n0D = 1;
        pEnv->nFlags = nFlags | 4;
        pEnv->field_3C = pEnv->field_3E;
        pEnv->n030 = 0;
        nTime = pArgs[0].i;
        if (nTime < 0) {
            nTime = 0;
        }
        pEnv->n034 = nTime;
    }
}

/* Block the calling thread on another VM object */
void Java_xeno_vm_System_waitFor__Ljava_lang_Object_(void *pEnv, JVAL *pArgs, JVAL *pRet)
{
    System_waitFor(pEnv, pArgs[0].i, 0, 0);
}

/* Clear the calling thread's method-wait flag */
void Java_xeno_vm_System_methodSignal__I(void *pEnv, JVAL *pArgs, JVAL *pRet)
{
    if (pArgs[0].i != 0) {
        SCRIPT_methodClearSet(pEnv);
    }
}

/* Allocate a new script thread */
void Java_xeno_vm_Thread_create__(void *pEnv, JVAL *pArgs, JVAL *pRet)
{
    pRet->i = JNI_createThread(4, 4, 0x40);
}

/* Point a thread at an object and a named method */
/* TODO: not matching - the two argument chains are scheduled in the other
   order; the name lookup should be issued before the target deref */
void Java_xeno_vm_Thread_setTarget__Ljava_lang_Object_Ljava_lang_String_(void *pEnv, JVAL *pArgs, JVAL *pRet)
{
    JARRAY *pArr = ((JSTRING *)pArgs[2].p)->pArray;
    char *pTarget = (char *)pArgs[1].p;
    int nClass = *(int *)*(int *)pTarget;
    JTHREAD *pThread = (JTHREAD *)pArgs[0].p;
    void *pMethod;

    pMethod = findMethod(nClass,
        loadConstString(pArr->pData, pArr->nLength), TYPE_Void);
    pThread->pTarget = pTarget;
    pThread->pMethod = pMethod;
    pThread->pStage = JTHREAD_default;
}

/* Initialise a thread and mark it runnable */
void Java_xeno_vm_Thread_start__(void *pEnv, JVAL *pArgs, JVAL *pRet)
{
    JTHREAD *pThread = (JTHREAD *)pArgs[0].p;

    JNI_initThread(pThread);
    pThread->nFlags |= 0x10;
}

/* Clear a thread's runnable flag */
void Java_xeno_vm_Thread_stop__(void *pEnv, JVAL *pArgs, JVAL *pRet)
{
    JTHREAD *pThread = (JTHREAD *)pArgs[0].p;

    pThread->nFlags &= ~0x10;
}

/* Math.random - one draw from the engine PRNG */
void Java_xeno_vm_Math_random__(void *pEnv, JVAL *pArgs, JVAL *pRet)
{
    pRet->i = xglLRand();
}

/* Math.atan2 */
void Java_xeno_vm_Math_atan2__FF(void *pEnv, JVAL *pArgs, JVAL *pRet)
{
    pRet->f = atan2f(pArgs[0].f, pArgs[1].f);
}

/* Math.cos - the retail build calls the sine helper here */
void Java_xeno_vm_Math_cos__F(void *pEnv, JVAL *pArgs, JVAL *pRet)
{
    pRet->f = xglSin(pArgs[0].f);
}

/* Math.sin - the retail build calls the cosine helper here */
void Java_xeno_vm_Math_sin__F(void *pEnv, JVAL *pArgs, JVAL *pRet)
{
    pRet->f = xglCos(pArgs[0].f);
}
