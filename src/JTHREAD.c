/* Script VM thread list management */

typedef struct JTHREAD
{
    char            pad00[8];
    struct JTHREAD *pNext; /* 0x08 */
    char            pad0C[4];
    int             nId;   /* 0x10 */
} JTHREAD;

typedef struct JSYM
{
    int nKey; /* 0x00 */
    int nVal; /* 0x04 - zero terminates the table */
} JSYM;

JTHREAD *jthreadTop;

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
