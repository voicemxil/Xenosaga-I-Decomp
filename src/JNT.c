/* JNT - the joint/skeleton layer of the model system.
 *
 * The whole layer works out of a single fixed work area that lives in the
 * EE scratchpad at 0x70000000: the current model, the element cursor used
 * while walking the skeleton, the root TRS rows, the matrix palette state
 * and the element table at the end of the block.  Every entry point below
 * either pokes that work area or walks the element list hanging off the
 * model's attribute block.
 */

typedef struct {
    unsigned short nSize;       /* 0x00 */
    unsigned short nType;       /* 0x02 */
} JNT_ATTRIBUTE;

typedef struct {
    unsigned int nSize;         /* 0x00 */
    JNT_ATTRIBUTE aAttribute[1];/* 0x04 */
} JNT_ATTRIBUTE_BLOCK;

typedef struct {
    unsigned short nType;       /* 0x00 */
    char pad002[0x30];          /* 0x02 */
    short nInterrupt;           /* 0x32 */
    char pad034[0xC];           /* 0x34 */
} JNT_ELEMENT;

typedef struct {
    char pad000[0x4];           /* 0x00 */
    unsigned short nVersion;    /* 0x04 */
    unsigned short nJointNum;   /* 0x06 */
    unsigned short nElementNum; /* 0x08 */
    unsigned short nSubNum;     /* 0x0A */
    char pad00C[0x4];           /* 0x0C */
    JNT_ATTRIBUTE_BLOCK block;  /* 0x10 */
} JNT_MODEL;

typedef struct {
    int nNum;                   /* 0x00 */
    void *aKey[8];              /* 0x04 */
    void *aFunc[8];             /* 0x24 */
    void *aArg[8];              /* 0x44 */
    int nFlags;                 /* 0x64 */
    int nUnk68;                 /* 0x68 */
    int nUnk6C;                 /* 0x6C */
    int nUnk70;                 /* 0x70 */
} JNT_PRODUCER;

/* Interrupt/filter list handed to JNT_setInterrupt and JNT_getFilter. */
typedef struct {
    int nNum;                   /* 0x00 */
    int aCode[8];               /* 0x04 */
    int aValue[8];              /* 0x24 */
} JNT_FILTER;

typedef struct {
    JNT_PRODUCER *pProducer;    /* 0x000 */
    char pad004[0x24];          /* 0x004 */
    JNT_MODEL *pModel;          /* 0x028 */
    int nIndex;                 /* 0x02C */
    JNT_ELEMENT *pRoot;         /* 0x030 */
    void *pHandle;              /* 0x034 */
    int nFlags;                 /* 0x038 */
    short nCurveId;             /* 0x03C */
    char pad03E[0x2];           /* 0x03E */
    float aRootTrans[4];        /* 0x040 */
    float aRootRotate[4];       /* 0x050 */
    float aRootScale[4];        /* 0x060 */
    char pad070[0x44C];         /* 0x070 */
    int nMatrixFlag;            /* 0x4BC */
    short nMatrixNum;           /* 0x4C0 */
    char pad4C2[0x2DE];         /* 0x4C2 */
    void *pMatrix;              /* 0x7A0 */
    void *pInterpMatrix;        /* 0x7A4 */
    void *pCurve;               /* 0x7A8 */
    void *pCurve2;              /* 0x7AC */
    char pad7B0[0x40];          /* 0x7B0 */
    int nHash;                  /* 0x7F0 */
    char pad7F4[0x8];           /* 0x7F4 */
    float fFrame;               /* 0x7FC */
    float fInterp;              /* 0x800 */
    char pad804[0x4];           /* 0x804 */
    void *pCntFree;             /* 0x808 */
    char *pName;                /* 0x80C */
    int nAnimFlags;             /* 0x810 */
    float fClipR;               /* 0x814 */
    JNT_ELEMENT *pElement;      /* 0x818 */
    int nInterrupt;             /* 0x81C */
    void *pUnit;                /* 0x820 */
    char pad824[0xC];           /* 0x824 */
    JNT_ELEMENT aElement[1];    /* 0x830 */
} JNT_WORK;

/* The work area is pinned to the head of the EE scratchpad. */
#define JNT ((JNT_WORK *)0x70000000)
#define JNT_FCURVE_PACK ((void *)0x70000004)

char flagSmoothHair;

void FCV2_resetPack(void *pPack, void *pCurve);
JNT_ELEMENT *JNT_getStaticVal2(JNT_WORK *pWork, JNT_ELEMENT *pElement);

/* Element count of the model currently loaded in the work area */
int JNT_hairID_002DA9C0(JNT_WORK *pWork)
{
    return pWork->pModel->nElementNum;
}

/* Set the joint work flags */
void JNT_setFlags(int nFlags)
{
    JNT->nFlags = nFlags;
}

/* Set the shadow/clip radius */
void JNT_setClipR(float fClipR)
{
    JNT->fClipR = fClipR;
}

/* Set the animation flags */
void JNT_animSetFlags(int nFlags)
{
    JNT->nAnimFlags = nFlags;
}

/* Return the joint work flags */
int JNT_getFlags(void)
{
    return JNT->nFlags;
}

/* Find the accessory attribute (type 4) in a model's attribute block */
void *JNT_getAccessories(JNT_MODEL *pModel)
{
    JNT_ATTRIBUTE_BLOCK *pBlock;
    JNT_ATTRIBUTE *pAttribute;
    unsigned int nOffset;

    nOffset = 4;
    if (pModel == 0) {
        return 0;
    }
    pBlock = &pModel->block;
    pAttribute = pBlock->aAttribute;
    do {
        if (pAttribute->nType == 4) {
            return pAttribute + 1;
        }
        if (pAttribute->nType == 0) {
            break;
        }
        nOffset += pAttribute->nSize;
        pAttribute = (JNT_ATTRIBUTE *)((char *)pAttribute + pAttribute->nSize);
    } while (nOffset < pBlock->nSize);
    return 0;
}

/* Bind a matrix palette and reset the palette write mode */
void JNT_setMatrix(void *pMatrix)
{
    JNT->pMatrix = pMatrix;
    JNT->nMatrixFlag = 0;
}

/* Bind a matrix palette with an explicit write mode */
void JNT_setMatrix2(void *pMatrix, int nFlag)
{
    JNT->pMatrix = pMatrix;
    JNT->nMatrixFlag = nFlag;
}

/* Bind the palette to interpolate towards, and the blend factor */
void JNT_setInterpMatrix(void *pMatrix, float fInterp)
{
    JNT->pInterpMatrix = pMatrix;
    JNT->fInterp = fInterp;
}

/* Bind the pair of animation curves driving the skeleton */
void JNT_setCurve(void *pCurve, void *pCurve2)
{
    JNT->pCurve = pCurve;
    JNT->pCurve2 = pCurve2;
}

/* Index of the last element of the leading run of type-1 (move) elements */
int JNT_getMoveElement(JNT_MODEL *pModel)
{
    JNT_ATTRIBUTE_BLOCK *pBlock;
    JNT_ELEMENT *pElement;
    int nIndex;
    int nElementNum;

    pBlock = &pModel->block;
    nElementNum = pModel->nElementNum;
    pElement = (JNT_ELEMENT *)((char *)pBlock + pBlock->nSize);
    nIndex = 0;
    if (nElementNum != 0) {
        do {
            if (pElement->nType != 1) {
                return nIndex - 1;
            }
            pElement++;
            nIndex++;
        } while (nIndex < nElementNum);
    }
    return 0;
}

/* First element of a model, which follows its attribute block */
JNT_ELEMENT *JNT_getRootElement(JNT_MODEL *pModel)
{
    JNT_ATTRIBUTE_BLOCK *pBlock;

    pBlock = &pModel->block;
    return (JNT_ELEMENT *)((char *)pBlock + pBlock->nSize);
}

/* Arm the curve pack for a single-curve animation */
void JNT_setFCurve(void *pCurve, int nCurveId)
{
    JNT->nFlags |= 1;
    JNT->nCurveId = nCurveId;
    JNT->aRootTrans[3] = 1.0f;
    JNT->aRootRotate[3] = 1.0f;
    JNT->aRootScale[3] = 1.0f;
    if (pCurve == 0) {
        JNT->nFlags |= 0x20;
    } else {
        FCV2_resetPack(JNT_FCURVE_PACK, pCurve);
    }
    JNT->pElement = JNT->aElement;
}

/* Arm the curve pack and bind the destination matrix palette in one go */
void JNT_setFCurve2(void *pCurve, void *pMatrix, int nCurveId)
{
    JNT_WORK *pWork;

    pWork = JNT;
    FCV2_resetPack(JNT_FCURVE_PACK, pCurve);
    pWork->pMatrix = pMatrix;
    pWork->nCurveId = nCurveId;
    pWork->nFlags |= 1;
    pWork->aRootTrans[3] = 1.0f;
    pWork->aRootRotate[3] = 1.0f;
    pWork->aRootScale[3] = 1.0f;
}

/* Reset a producer's consumer table */
void JNT_initProducer(JNT_PRODUCER *pProducer)
{
    int i;

    pProducer->nFlags = 0;
    pProducer->nNum = 0;
    pProducer->nUnk68 = 0;
    pProducer->nUnk6C = -1;
    for (i = 7; i >= 0; i--) {
        pProducer->aFunc[i] = 0;
    }
}

/* Element at an index, elements being a flat 0x40-byte array */
JNT_ELEMENT *JNT_getElement(JNT_ELEMENT *pElement, int nIndex)
{
    return &pElement[nIndex];
}

/* Step to the next element */
JNT_ELEMENT *JNT_nextElement(JNT_ELEMENT *pElement)
{
    return pElement + 1;
}

/* Re-run the static pass over the hair (sub) elements of the model */
void JNT_resetHair(void)
{
    JNT_ELEMENT *pElement;
    int nEnd;

    JNT->nIndex = JNT->pModel->nElementNum;
    nEnd = JNT->pModel->nElementNum + JNT->pModel->nSubNum;
    pElement = JNT->pRoot + JNT->pModel->nElementNum;
    while (JNT->nIndex < nEnd) {
        pElement = JNT_getStaticVal2(JNT, pElement);
    }
}

/* Number of elements the hair pass has to cover for the current model */
int JNT_hairID_00314318(JNT_WORK *pWork)
{
    int *pCntFree;
    JNT_MODEL *pModel;
    int nNum;

    pCntFree = pWork->pCntFree;
    pModel = pWork->pModel;
    nNum = pModel->nElementNum;
    if (pCntFree[0] == 0) {
        nNum += pModel->nSubNum;
    }
    if ((pCntFree[1] & 0xFFFF0000) == 0x8A0000) {
        nNum += pCntFree[1] & 0xFFFF;
    }
    return nNum;
}

/* Tag elements named by a filter list so the walk breaks on them */
int JNT_setInterrupt(JNT_WORK *pWork, JNT_FILTER *pFilter)
{
    int i;
    int nCount;
    int nCode;

    nCount = 0;
    pWork->pElement->nInterrupt = 0;
    for (i = 1; i < pFilter->nNum; i++) {
        nCode = pFilter->aCode[i];
        if (nCode != 0 && (nCode & 0x8000) == 0) {
            pWork->pElement[nCode].nInterrupt = i;
            nCount++;
        }
    }
    return nCount;
}

/* Value of the first flagged entry of a filter list */
int JNT_getFilter(JNT_WORK *pWork, JNT_FILTER *pFilter)
{
    int i;

    for (i = 1; i < pFilter->nNum; i++) {
        if (pFilter->aCode[i] & 0x8000) {
            return pFilter->aValue[i];
        }
    }
    return 0;
}

/* Enable hair smoothing */
void JNT_onSmoothHair(void)
{
    flagSmoothHair = 1;
}

/* Disable hair smoothing */
void JNT_offSmoothHair(void)
{
    flagSmoothHair = 0;
}
