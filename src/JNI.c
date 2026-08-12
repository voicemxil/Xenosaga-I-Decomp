/* JNI-style bridge between the game and the embedded Java-like script VM */

typedef struct JVM
{
    char           pad00[0x24];
    int            nFlags;   /* 0x24 */
    char           pad28[0x10];
    unsigned short nSp;      /* 0x38 */
    unsigned short unk3A;    /* 0x3A */
    unsigned short nPc;      /* 0x3C */
    unsigned short nTop;     /* 0x3E */
} JVM;

typedef struct JCLASS
{
    int nId;
} JCLASS;

typedef struct JOBJ
{
    JCLASS *pClass;
} JOBJ;

void (*jthreadResetFunc)(void);

extern int VMRegister[32];

void initClassDB(void);
int xheap_init(int nHeap, int pStart, int nSize);
int xheap_push(void);
int xheap_pop(void);
int xheap_current_clear(void);
int instanceOf(int nClassId, int nTarget);

/* Initialize the class database and the VM object heap */
int JNI_initSystem(int pStart, int nSize)
{
    initClassDB();
    return xheap_init(0, pStart, nSize);
}

/* Push a new object-heap frame */
int JNI_pushFrame(void)
{
    return xheap_push();
}

/* Pop the current object-heap frame and clear it */
int JNI_popFrame(void)
{
    xheap_pop();
    return xheap_current_clear();
}

/* Test whether an object is an instance of the given class */
int JNI_isInstanceOf(JOBJ *pObj, int nClass)
{
    int nRet = 0;

    if (pObj != 0) {
        nRet = instanceOf(pObj->pClass->nId, nClass);
    }
    return nRet;
}

/* Run and clear the pending thread-reset handler, if any */
void JNI_catchException(void)
{
    if (jthreadResetFunc != 0) {
        jthreadResetFunc();
        jthreadResetFunc = 0;
    }
}

/* Thread exception hook (empty in the retail build) */
void JNI_threadException(void)
{
}

/* Reset a VM thread's program counter, stack and flags */
void JNI_initThread(JVM *pVm)
{
    /* Assignment order chosen to reproduce the original store schedule */
    pVm->nPc = 0;
    pVm->nFlags = 0;
    pVm->nTop = 0;
    pVm->nSp = 0;
}

/* Read one of the 32 VM registers */
int JNI_getRegister(int nReg)
{
    return VMRegister[nReg & 0x1F];
}
