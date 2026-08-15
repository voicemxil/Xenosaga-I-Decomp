/* Script VM thread list management */

typedef struct JTHREAD
{
    char            pad00[8];
    struct JTHREAD *pNext;   /* 0x08 */
    unsigned char   nActive; /* 0x0C - cleared threads are skipped by cntl */
    char            pad0D[3];
    int             nId;     /* 0x10 */
    char            pad14[4];
    void           *pMethod; /* 0x18 - script method the default handler runs */
    char            pad1C[8];
    int             nFlags;  /* 0x24 */
} JTHREAD;

/* The 0x14 slot is a per-thread native callback in JTHREAD_cntl's walk and
   the script method pointer in the default handlers; both views are used
   through the same struct, so it is spelled out as a function pointer and
   cast where the other meaning is wanted. */
typedef void (*JTHREADFUNC)(JTHREAD *);
typedef void (*JTHREADHOOK)(void);

typedef struct JAVA_FIELD
{
    char pad00[0x10];
    int  nOffset; /* 0x10 */
} JAVA_FIELD;

extern void JNI_callMethod(JTHREAD *pThread, void *pMethod, int *pArgs,
                           int *pRet);
extern void *loadConstString(char *pStr, int nLength);
extern JAVA_FIELD *lookupClassField(int nClass, void *pName, int nFlags);
extern int classJava_xeno_Chr;
extern int classJava_xeno_Unit;
extern char D_004DC080[];

typedef struct JSYM
{
    int nKey; /* 0x00 */
    int nVal; /* 0x04 - zero terminates the table */
} JSYM;

JTHREAD *jthreadTop;
JTHREADHOOK jthreadHook;

/* Thread system init (empty in the retail build) */
void JTHREAD_init(void)
{
}

/* Walk the thread list (debug hook; the retail build reports nothing) */
void JTHREAD_info(void)
{
    JTHREAD *pThread = jthreadTop;

    while (pThread != 0) {
        pThread = pThread->pNext;
    }
}

/* Find a symbol table entry by key, or null if the table has no match.
   The key load sits at the top of the loop body so the compiler rotates
   it into the annulled delay slot of a branch-likely, as in the original. */
JSYM *JTHREAD_getSymbol(JSYM *pSym, int nKey)
{
    JSYM *pRet = 0;

    if (pSym->nVal != 0) {
        do {
            int nCur = pSym->nKey;

            pRet = pSym;
            if (nKey == nCur) {
                return pRet;
            }
            pSym++;
        } while (pSym->nVal != 0);
        pRet = 0;
    }
    return pRet;
}

/* Find a live thread by id, or null if none matches */
JTHREAD *JTHREAD_get(int nId)
{
    JTHREAD *pRet = 0;
    JTHREAD *pThread = jthreadTop;

    if (pThread != 0) {
        do {
            int nCur = pThread->nId;

            pRet = pThread;
            if (nCur == nId) {
                return pRet;
            }
            pThread = pThread->pNext;
        } while (pThread != 0);
        pRet = 0;
    }
    return pRet;
}


/* FLAG REQUEST (verified). Source is exact -- 35/35 words, and the only
   remaining difference is the prologue pair `sd ra,8(sp)` / `lw s0,jthreadTop`
   coming out in the other order, a pure sched2 tie-break (both are memory
   ops, so --swap-adjacent's independence check rejects the pair; --rotate
   with LEN 2 has no such check).  Verified MATCH, all 35 words, with:
       --rotate JTHREAD_cntl:2:2
   Five source shapes were swept for it (plain assign, initialiser, register
   qualifier, extra dead local, separate head local) -- all give the same
   2 diffs.  Once wired, register:
       JTHREAD_cntl = 0x003058C8, 0x8C; // JTHREAD.c

   Two shape notes worth keeping:
    - the walk must be goto-form with the null test at the top, so gcc's
      reorg annul-copies `pThread = pThread->pNext` into all three
      branch-likely delay slots and the `next:` block disappears entirely.
    - jthreadHook must be read as a global at each test, NOT hoisted into a
      local.  With a local it stays live across the loop in a callee-saved
      register; read directly, gcc's PRE re-materialises the load at each
      point and inserts the one on the empty-list edge into the delay slot
      of the top test, which is exactly the original's shape.

   Per-frame walk of the thread list: run every armed thread's native tick
   and stop as soon as one of them arms the global hook, then run it. */
void JTHREAD_cntl(void)
{
    JTHREAD *pThread;

    pThread = jthreadTop;
loop:
    if (pThread == 0)
        goto done;
    if ((pThread->nFlags & 0x10) == 0)
        goto next;
    if (pThread->nActive == 0)
        goto next;
    if (*(JTHREADFUNC *)((char *)pThread + 0x14) != 0) {
        (*(JTHREADFUNC *)((char *)pThread + 0x14))(pThread);
    }
    if (jthreadHook == 0)
        goto next;
done:
    if (jthreadHook != 0) {
        jthreadHook();
        jthreadHook = 0;
    }
    return;
next:
    pThread = pThread->pNext;
    goto loop;
}

/* Default scene-thread handler: call the bound script method with the
   thread id as its only argument, then drop the "running" flag if the
   thread asked to be stopped. */
void JTHREAD_defaultScene(JTHREAD *pThread)
{
    int nArgs[4];
    int nRet[4];
    int nId;
    int nFlags;

    nId = pThread->nId;
    if (nId != 0) {
        if (pThread->pMethod != 0) {
            nArgs[0] = nId;
            JNI_callMethod(pThread, pThread->pMethod, nArgs, nRet);
            nFlags = pThread->nFlags;
            if ((nFlags & 8) != 0) {
                pThread->nFlags = nFlags & ~0x10;
            }
        }
    }
}


/* Default character-thread handler.  The thread's 0x10 slot doubles as the
   bound script object; resolving its native peer and finding bit 1 already
   set means the actor is mid-transition, so the tick is skipped. */
void JTHREAD_defaultChr(JTHREAD *pThread)
{
    int nArgs[4];
    char *pObj;
    int *pPeer;
    int nFlags;
    int nDone;

    pObj = (char *)pThread->nId;
    pPeer = *(int **)(pObj + lookupClassField(classJava_xeno_Chr,
                                              loadConstString(D_004DC080, -1),
                                              0)->nOffset);
    if ((pPeer[0] & 2) != 0) {
        return;
    }
    if (pObj == 0) {
        return;
    }
    nFlags = pThread->nFlags;
    if ((nFlags & 0x10) == 0) {
        return;
    }
    if ((nFlags & 1) != 0) {
        pThread->nFlags = (nFlags & ~1) | 2;
    }
    nArgs[0] = (int)pObj;
    JNI_callMethod(pThread, pThread->pMethod, nArgs, 0);
    nDone = pThread->nFlags;
    if ((nDone & 8) != 0) {
        pThread->nFlags = nDone & ~0x10;
    }
}

/* Default unit-thread handler -- identical to JTHREAD_defaultChr except
   that the peer field is looked up in xeno.Unit rather than xeno.Chr. */
void JTHREAD_defaultUnit(JTHREAD *pThread)
{
    int nArgs[4];
    char *pObj;
    int *pPeer;
    int nFlags;
    int nDone;

    pObj = (char *)pThread->nId;
    pPeer = *(int **)(pObj + lookupClassField(classJava_xeno_Unit,
                                              loadConstString(D_004DC080, -1),
                                              0)->nOffset);
    if ((pPeer[0] & 2) != 0) {
        return;
    }
    if (pObj == 0) {
        return;
    }
    nFlags = pThread->nFlags;
    if ((nFlags & 0x10) == 0) {
        return;
    }
    if ((nFlags & 1) != 0) {
        pThread->nFlags = (nFlags & ~1) | 2;
    }
    nArgs[0] = (int)pObj;
    JNI_callMethod(pThread, pThread->pMethod, nArgs, 0);
    nDone = pThread->nFlags;
    if ((nDone & 8) != 0) {
        pThread->nFlags = nDone & ~0x10;
    }
}
