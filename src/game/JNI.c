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
