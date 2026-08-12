/* Intrusive doubly-linked task queue (free-list head doubles as list node) */

typedef struct XGL_TASK {
    struct XGL_TASK *field0; /* free-list link (node) / free-list head (queue) */
    struct XGL_TASK *field4; /* active-list next (node) / first (queue) */
    struct XGL_TASK *field8; /* active-list prev (node) / last (queue) */
    void (*fieldC)(void);    /* task entry point (node) / iterator scratch (queue) */
} XGL_TASK;

void *xglTaskRemove(XGL_TASK *node);

/* Rebind a task's entry point to xglTaskRemove so the next Execute pass
 * removes it from the active list instead of running it */
void *xglTaskWaitRemove(XGL_TASK *node)
{
    node->fieldC = (void (*)(void))xglTaskRemove;
    return 0;
}
