/* tsk* task/screen state-machine drivers */

typedef struct XGL_TASK {
    struct XGL_TASK *field0;
    struct XGL_TASK *field4;
    struct XGL_TASK *field8;
    void (*fieldC)(void);
} XGL_TASK;

typedef struct TSK_MAIN {
    char pad000[0x10];
    unsigned char nState;                         /* 0x10 */
    char pad011[3];
    void (*pFunc)(struct TSK_MAIN *, void *);      /* 0x14 */
    void *pParam;                                  /* 0x18 */
} TSK_MAIN;

typedef struct { char pad000[3]; unsigned char nAbort; } FILE_WORK;
typedef struct { char pad000[1]; unsigned char nAbort; } SEISAN_WORK;
typedef struct { unsigned char nAbort; } MENUSHOP_WORK;

extern FILE_WORK *FileWork;
extern SEISAN_WORK *SeisanWork;
extern MENUSHOP_WORK *MenuShopWork;

extern void *xglTaskWaitRemove(XGL_TASK *node);

/* ov02 (Umn overlay) task variant: state is a full int, and the abort
 * flag lives in a fixed struct instance (not a pointer) far outside gp
 * range for this overlay's own compilation unit. */
typedef struct TSK_TASK {
    char pad000[0x10];
    int nState;                                   /* 0x10 */
    void (*pFunc)(struct TSK_TASK *, void *);      /* 0x14 */
    void *pParam;                                  /* 0x18 */
} TSK_TASK;

typedef struct {
    char pad000[1];
    unsigned char nAbort;    /* 0x1 */
    char pad002[0x88 - 2];
} UMN_WORK;

extern UMN_WORK UmnWork;

/* One-shot init (state 0, calls the handler then arms state 2 for the
 * next frame), per-frame step (state 2), or teardown (state -1, driven
 * by UmnWork's abort flag) */
void tskUmnObjectTaskMain(TSK_TASK *node)
{
    void (*func)(TSK_TASK *, void *) = node->pFunc;

    if (UmnWork.nAbort == 0xFF) {
        node->nState = -1;
    }
    switch (node->nState) {
    case 0:
        func(node, node->pParam);
        node->nState = 2;
        /* fall through */
    case 2:
        func(node, node->pParam);
        break;
    case -1:
        xglTaskWaitRemove((XGL_TASK *)node);
        __asm__ volatile("" ::: "memory");
        break;
    }
}

/* Per-frame driver for a File-screen task: state 0 runs the handler twice
 * (once to init, once to step after arming state 2); state 2 steps once;
 * any other state is a no-op wait. FileWork->nAbort == 0xFF tears the task
 * down instead. */
void *tskTskMain(TSK_MAIN *node)
{
    void (*func)(TSK_MAIN *, void *) = node->pFunc;

    if (FileWork->nAbort == 0xFF) {
        return xglTaskWaitRemove((XGL_TASK *)node);
    }
    switch (node->nState) {
    case 0:
        func(node, node->pParam);
        node->nState = 2;
        /* fall through */
    case 2:
        func(node, node->pParam);
        break;
    }
}

/* Same driver, gated on SeisanWork->nAbort */
void *tskTskMain2(TSK_MAIN *node)
{
    void (*func)(TSK_MAIN *, void *) = node->pFunc;

    if (SeisanWork->nAbort == 0xFF) {
        return xglTaskWaitRemove((XGL_TASK *)node);
    }
    switch (node->nState) {
    case 0:
        func(node, node->pParam);
        node->nState = 2;
        /* fall through */
    case 2:
        func(node, node->pParam);
        break;
    }
}

/* Same driver, gated on MenuShopWork->nAbort */
void *tskTskMain3(TSK_MAIN *node)
{
    void (*func)(TSK_MAIN *, void *) = node->pFunc;

    if (MenuShopWork->nAbort == 0xFF) {
        return xglTaskWaitRemove((XGL_TASK *)node);
    }
    switch (node->nState) {
    case 0:
        func(node, node->pParam);
        node->nState = 2;
        /* fall through */
    case 2:
        func(node, node->pParam);
        break;
    }
}
