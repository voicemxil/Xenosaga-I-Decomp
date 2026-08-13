/* Intrusive doubly-linked task queue (free-list head doubles as list node) */

typedef struct XGL_TASK {
    struct XGL_TASK *field0; /* free-list link (node) / free-list head (queue) */
    struct XGL_TASK *field4; /* active-list next (node) / first (queue) */
    struct XGL_TASK *field8; /* active-list prev (node) / last (queue) */
    void (*fieldC)(void);    /* task entry point (node) / iterator scratch (queue) */
} XGL_TASK;

void *xglTaskRemove(XGL_TASK *node);
extern int sceGsResetGraph(int interlace, int mode, int ffmd, int dispWidth);

/* Rebind a task's entry point to xglTaskRemove so the next Execute pass
 * removes it from the active list instead of running it */
void *xglTaskWaitRemove(XGL_TASK *node)
{
    node->fieldC = (void (*)(void))xglTaskRemove;
    return 0;
}

/* Carve `count` fixed-size (0x80-byte) task nodes out of the space right
 * after the queue header and chain them onto the queue's free list */
/* TODO: near-miss (34/37 words match; LENGTH class). The original zeros
 * v0 at function entry (implying a real, unused return value -- tried
 * `int` return with an implicit top-level `return 0`, which reproduces
 * that) and computes one dead `node + 0x80` right after the loop (an
 * unused address one past the last carved node) that every natural
 * source form here gets dead-code-eliminated. Everything else -- the
 * resetFlag call, the do-while chaining loop with its `bnez` back-edge,
 * the trailing field stores -- matches exactly. */
void xglTaskInitial(XGL_TASK *queue, int count, int resetFlag)
{
    XGL_TASK *node = (XGL_TASK *)((char *)queue + 0x10);
    int i;

    if (count == 0) {
        return;
    }
    if (resetFlag != 0) {
        sceGsResetGraph(1, 0, 0, 0);
    }
    queue->field0 = node;
    i = count - 1;
    if (i > 0) {
        do {
            XGL_TASK *next = (XGL_TASK *)((char *)node + 0x80);
            i--;
            node->field4 = next;
            node = next;
        } while (i != 0);
    }
    node->field4 = 0;
    queue->field4 = 0;
    queue->field8 = 0;
}

/* Allocate a node off queue's free list and splice it into the active
 * list immediately before pRef (or as the sole element if pRef == NULL) */
XGL_TASK *xglTaskEntryPrev(XGL_TASK *queue, void (*func)(void), XGL_TASK *pRef)
{
    XGL_TASK *node;
    XGL_TASK *prev;

    if (queue == 0 || (node = queue->field0) == 0) {
        return 0;
    }
    queue->field0 = node->field4;
    node->fieldC = func;
    node->field0 = queue;
    node->field4 = pRef;
    if (pRef == 0) {
        node->field8 = 0;
        queue->field8 = node;
        queue->field4 = node;
    } else {
        prev = pRef->field8;
        node->field8 = prev;
        if (prev == 0) {
            queue->field4 = node;
        } else {
            pRef->field8->field4 = node;
        }
        pRef->field8 = node;
    }
    return node;
}

/* Allocate a node off queue's free list and splice it into the active
 * list immediately after pRef (or as the sole element if pRef == NULL) */
XGL_TASK *xglTaskEntryNext(XGL_TASK *queue, void (*func)(void), XGL_TASK *pRef)
{
    XGL_TASK *node;
    XGL_TASK *next;

    if (queue == 0 || (node = queue->field0) == 0) {
        return 0;
    }
    queue->field0 = node->field4;
    node->fieldC = func;
    node->field0 = queue;
    node->field8 = pRef;
    if (pRef == 0) {
        node->field4 = 0;
        queue->field8 = node;
        queue->field4 = node;
    } else {
        next = pRef->field4;
        node->field4 = next;
        if (next == 0) {
            queue->field8 = node;
        } else {
            pRef->field4->field8 = node;
        }
        pRef->field4 = node;
    }
    return node;
}

/* Unlink node from its owning queue's active list and push it back onto
 * the free list; also advances the queue's iterator scratch if it was
 * pointing at the node being removed (safe removal mid-Execute) */
/* TODO: near-miss (23/25 words match). Original reloads node->field4 a
 * second time via a separate `lw` after the queue->fieldC store in the
 * "node is the iterator" branch; every source form tried here (shared
 * temp, duplicated reads, differently-typed temps, reversed condition)
 * has 2.96 either CSE the already-loaded value into a `move` instead of
 * a second `lw`, or flip the branch polarity (beql vs bnel). Everything
 * else -- the bnel merge, the two insert-before/after branch pairs, the
 * free-list push -- matches exactly. */
void *xglTaskRemove(XGL_TASK *node)
{
    XGL_TASK *queue = node->field0;
    XGL_TASK *next;

    if (node == (XGL_TASK *)queue->fieldC) {
        queue->fieldC = (void (*)(void))node->field4;
        next = node->field4;
    } else {
        next = node->field4;
    }
    if (next != 0) {
        next->field8 = node->field8;
    } else {
        queue->field8 = node->field8;
    }
    if (node->field8 != 0) {
        node->field8->field4 = node->field4;
    } else {
        queue->field4 = node->field4;
    }
    node->fieldC = 0;
    node->field4 = queue->field0;
    queue->field0 = node;
    return 0;
}

/* Run every task in queue's active list, caching the next pointer in the
 * queue's iterator scratch before each call so a callback may safely
 * remove itself (or any other node) via xglTaskRemove mid-iteration */
/* TODO: near-miss, same instruction count (22) as the original but 2.96
 * allocates registers/schedules the loop body differently: the original
 * preloads the look-ahead `next` pointer once before the loop and only
 * refreshes it in the back-edge branch's delay slot (bnel), keeping the
 * store to queue->fieldC in the func-null-check branch's delay slot;
 * every natural variant tried here instead recomputes `next` per
 * iteration or schedules the store into the jalr delay slot. */
void xglTaskExecute(XGL_TASK *queue)
{
    XGL_TASK *node;
    XGL_TASK *next;
    void (*func)(void);

    if (queue == 0) {
        return;
    }
    node = queue->field4;
    if (node == 0) {
        return;
    }
    next = node->field4;
    for (;;) {
        func = node->fieldC;
        if (func != 0) {
            queue->fieldC = (void (*)(void))next;
            ((void (*)(XGL_TASK *))func)(node);
        }
        node = (XGL_TASK *)queue->fieldC;
        if (node == 0) {
            break;
        }
        next = node->field4;
    }
}
