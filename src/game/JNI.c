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
extern void *classDB[8];

int xheap_init(int nHeap, int pStart, int nSize);
int xheap_push(void);
int xheap_pop(void);
int xheap_current_clear(void);
int instanceOf(int nClassId, int nTarget);
void JTHREAD_waitFor(JVM *pVm);
void virtualMachine(JVM *pVm, void *pMethod, void *pArgs, void *pRet);

typedef struct DataBuffer {
    unsigned char *pStart;                                    /* 0x00 */
    unsigned char *pCur;                                      /* 0x04 */
    unsigned char *pEnd;                                      /* 0x08 */
    int nSize;                                                /* 0x0C */
    unsigned int (*pGetUIntegerAt)(struct DataBuffer *, int); /* 0x10 */
} DataBuffer;

/* One entry of a class-library package: an 8-byte header whose 0x06 field
   counts the 16-byte class records that follow it inline. */
typedef struct PDBCLASS {
    char           pad00[8];
    unsigned char *pData; /* 0x08 */
    int            nSize; /* 0x0C */
} PDBCLASS;

void DataBuffer_init(DataBuffer *pBuf, unsigned char *pData, int nSize,
                     int bBigEndian);
void PDB_getEntry(int nId, void **ppEntry, int *pnCount);
char *getStrIndex(char *pStr, int nChar);
int checkClass(DataBuffer *pBuf, char *pName, void *pArg);
void loadStaticClass(int *pOut, int nClass);

/* Clear all eight class-database slots from the end toward the front. */
void initClassDB(void)
{
    void **p;
    int i;

    i = 7;
    p = classDB;
    p += 7;
    do {
        i--;
        *p = 0;
        p--;
    } while (i >= 0);
}

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

/* Allocate or fetch a class-DB slot.  nIndex < 0 means "find the first
 * free slot in classDB[0..7]"; the resulting index (or an index the
 * caller passed in) then selects the operation: an index below 256
 * STORES pClass and returns it, an index of 256 or more is a LOOKUP of
 * classDB[nIndex & 0xff].  (The store/load senses are the opposite way
 * round from what they look like -- confirmed against the original's
 * `slti v0,a0,256` / `bnez v0` pair, whose taken edge is the store.) */
/* Allocate or fetch a class-DB slot.
 *
 * nIndex < 0 asks for the first free slot in classDB[0..7]; if all eight
 * are taken nIndex stays negative and the call returns 0.  The (possibly
 * just-chosen) index then selects the operation, and the senses are the
 * opposite way round from what they look like: an index BELOW 256 stores
 * pClass, an index of 256 or more is a lookup of classDB[nIndex & 0xff].
 * Confirmed from the original's `slti v0,a0,256` / `bnez v0` pair --
 * the taken edge is the store block.
 *
 * Three shape notes, all needed for the byte match:
 *  - the search must be goto-form, not do/while or for.  Any structured
 *    loop lets gcc 2.96 strength-reduce classDB[i] to a walking pointer
 *    and hoist the %lo out of the loop; the original re-derives
 *    base + i*4 every iteration.
 *  - the "slot 0 is free" case assigns nIndex = 0 and jumps, rather than
 *    sharing the `nIndex = i` store.  gcc emits the original's separate
 *    `move a0,zero` + `b` only when the constant is written out.
 *  - the two tail arms converge on ONE `return pClass`, with the lookup
 *    arm assigning into pClass.  Two separate returns cost four words. */
void *JNI_loadClassDB(int nIndex, void *pClass)
{
    int i;

    if (nIndex < 0) {
        i = 0;
        if (classDB[0] == 0) {
            nIndex = 0;
            goto searched;
        }
next:
        i++;
        if (i >= 8)
            goto searched;
        if (classDB[i] != 0)
            goto next;
        nIndex = i;
    }
searched:
    if (nIndex < 0)
        return 0;
    if (nIndex >= 256)
        pClass = classDB[nIndex & 0xff];
    else
        classDB[nIndex] = pClass;
    return pClass;
}


/* Enter the VM on behalf of one script method call.
 *
 * A thread flagged 0x20 (freshly rearmed) has its stack and operand top
 * reset and the rearm/run bits cleared first.  A thread flagged 0x04 is
 * blocked, so its wait condition is polled once and the call is dropped
 * if it is still blocked.  Anything left holding bit 0 or bit 2 is dead
 * or blocked and never reaches the interpreter.  The interpreter call is
 * a tail call -- the original restores every callee-saved register and
 * `j`s straight into virtualMachine. */
void JNI_callMethod(JVM *pVm, void *pMethod, void *pArgs, void *pRet)
{
    int nFlags;
    int nNew;

    nFlags = pVm->nFlags;
    if ((nFlags & 0x20) != 0) {
        pVm->nTop = 0;
        pVm->nSp = 0;
        /* A separate local, not an in-place &=: the original keeps the
           new flag word in its own register and copies it back, which an
           in-place update coalesces away (a whole-function 1-word shift). */
        nNew = nFlags & ~0x23;
        pVm->nFlags = nNew;
        nFlags = nNew;
    }
    if ((nFlags & 4) != 0) {
        JTHREAD_waitFor(pVm);
        nFlags = pVm->nFlags;
        if ((nFlags & 4) != 0) {
            return;
        }
    }
    if ((nFlags & 5) != 0) {
        return;
    }
    virtualMachine(pVm, pMethod, pArgs, pRet);
}


/* FLAG REQUEST (verified). Source is exact -- 63/63 words -- and the one
   remaining difference is `i = 0` being sunk into the delay slot of the
   `nCount > 0` guard instead of issuing before it, a pure sched2 tie-break
   (moving the initialiser inside the guard, after pBase, or into a for()
   header all cost two words instead).  Verified MATCH, all 63 words, with:
       --rotate JNI_searchClasses:16:2
   Once wired, register:
       JNI_searchClasses = 0x002F0A90, 0xFC; // JNI.c

   Note the outer retry must be `for (;;) { ... if (!pName) break; }` with a
   single `return 0` after the loop.  Returning 0 from inside the loop lets
   gcc reuse the just-tested null pointer as the return value and fold the
   branch into a branch-likely, one word short.

   Scan every class record of a package for pName, retrying with each
   ':'-separated suffix of the name and counting the retries in *pnDepth.
   Returns checkClass's non-zero result on a hit, 0 when the name runs out. */
int JNI_searchClasses(int nId, char *pName, void *pArg, int *pnDepth)
{
    DataBuffer buf;
    void *pEntry;
    int nCount;
    int i;
    int nClass;
    char *p;
    char *pBase;
    int nRet;

    PDB_getEntry(nId, &pEntry, &nCount);
    *pnDepth = 0;
    for (;;) {
        i = 0;
        if (nCount > 0) {
            pBase = (char *)pEntry;
            do {
                nClass = *(unsigned short *)(pBase + 6);
                p = pBase + 8;
                if (nClass > 0) {
                    do {
                        DataBuffer_init(&buf, ((PDBCLASS *)p)->pData,
                                        ((PDBCLASS *)p)->nSize, 1);
                        nRet = checkClass(&buf, pName, pArg);
                        if (nRet != 0) {
                            return nRet;
                        }
                        nClass--;
                        p += 16;
                    } while (nClass > 0);
                }
                i++;
                pBase = p;
            } while (i < nCount);
        }
        pName = getStrIndex(pName, ':');
        if (pName == 0) {
            break;
        }
        (*pnDepth)++;
    }
    return 0;
}


/* Load every static class of a package.  Note the original never advances
   either cursor: the entry pointer is re-read from the stack each outer
   pass and the record cursor is re-derived from it, so both loops run over
   the SAME record -- reproduced as-is. */
void JNI_loadClassLibrary(int nId)
{
    DataBuffer buf;
    void *pEntry;
    int nCount;
    int nOut;
    int i;
    int nClass;
    char *p;
    int nRet;

    PDB_getEntry(nId, &pEntry, &nCount);
    i = 0;
    if (nCount > 0) {
        do {
            nClass = *(unsigned short *)((char *)pEntry + 6);
            p = (char *)pEntry + 8;
            if (nClass > 0) {
                do {
                    DataBuffer_init(&buf, ((PDBCLASS *)p)->pData,
                                    ((PDBCLASS *)p)->nSize, 1);
                    nRet = checkClass(&buf, 0, 0);
                    if (nRet != 0) {
                        loadStaticClass(&nOut, nRet);
                    }
                    nClass--;
                } while (nClass > 0);
            }
            i++;
        } while (i < nCount);
    }
}
