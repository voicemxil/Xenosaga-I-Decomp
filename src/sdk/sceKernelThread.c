/* Game-owned top thread bootstrap, built with the SDK compiler. */

typedef struct INIT_SEMA_PARAM {
    int currentCount;
    int maxCount;
    int initCount;
    int numWaitThreads;
    unsigned int attr;
    unsigned int option;
} INIT_SEMA_PARAM;

typedef struct INIT_THREAD_PARAM {
    int status;
    void (*entry)(void *);
    void *stack;
    int stackSize;
    void *gpReg;
    int initPriority;
    int currentPriority;
    unsigned int attr;
    unsigned int option;
    int waitType;
    int waitId;
    int wakeupCount;
} INIT_THREAD_PARAM;

extern int topId;
extern int topSema;
extern int topArg[2];
extern char stack_6[] __asm__("stack.6");
extern int _gp;
extern void __gnu_compiled_c_00201350(void *);
extern int CreateSema(INIT_SEMA_PARAM *pParam);
extern int DeleteSema(int nSema);
extern int CreateThread(INIT_THREAD_PARAM *pParam);
extern int StartThread(int nThread, void *pArg);
extern int GetThreadId(void);
extern int ChangeThreadPriority(int nThread, int nPriority);

int InitThread(void)
{
    INIT_THREAD_PARAM thread;
    INIT_SEMA_PARAM sema;
    int nThread;

    if (topId > 0)
        goto fail;

    sema.initCount = 0;
    sema.maxCount = 0xFF;
    topSema = CreateSema(&sema);
    if (topSema < 0)
        goto fail;

    thread.entry = __gnu_compiled_c_00201350;
    thread.stack = stack_6;
    thread.stackSize = 0x400;
    thread.gpReg = &_gp;
    thread.initPriority = 0;
    nThread = CreateThread(&thread);
    topId = nThread;
    if (nThread >= 0)
        goto started;

    DeleteSema(topSema);

fail:
    return -1;

started:
    topArg[0] = 0;
    topArg[1] = 0;
    StartThread(nThread, topArg);
    ChangeThreadPriority(GetThreadId(), 1);
    return topId;
}
