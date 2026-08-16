#include "matching.h"

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
    char pad002[0xC];           /* 0x02 */
    unsigned short nMatrix;     /* 0x0E */
    char pad010[0x22];          /* 0x10 */
    short nInterrupt;           /* 0x32 */
    char pad034[0xC];           /* 0x34 */
} JNT_ELEMENT;

/* Fixed-width animated element consumed by JNT_getVal2. Every successful
 * evaluation advances by one 0x40-byte record. */
typedef struct {
    unsigned short nType;       /* 0x00: dispatch mode 0..6 */
    unsigned short nFlags;      /* 0x02 */
    unsigned int nRelative;     /* 0x04: optional auxiliary record */
    char pad008[0x6];           /* 0x08 */
    unsigned short nMatrix;     /* 0x0E */
    float aTranslate[4];        /* 0x10 */
    float aRotate[4];           /* 0x20 */
    float aPayload[4];          /* 0x30 */
} JNT_ANIM_RECORD;

/* Attribute returned by FCV2_getPackAttribute for an animated record. */
typedef struct {
    unsigned short nUnknown00;  /* 0x00 */
    unsigned short nUnknown02;  /* 0x02 */
    unsigned short nFlags;      /* 0x04 */
    unsigned short nChannels;   /* 0x06: three bits per vector group */
    unsigned short nMatrixT;    /* 0x08 */
    unsigned short nUnknown0A;  /* 0x0A */
    unsigned short nMatrixR;    /* 0x0C */
} JNT_FCV_ATTRIBUTE;

typedef struct {
    char pad000[0x4];           /* 0x00 */
    unsigned short nVersion;    /* 0x04 */
    unsigned short nJointNum;   /* 0x06 */
    unsigned short nElementNum; /* 0x08 */
    unsigned short nSubNum;     /* 0x0A */
    unsigned short nExtraNum;   /* 0x0C */
    unsigned short nHairNum;    /* 0x0E */
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
void CUR_MATRIX_Set(void *pMtx);
void CUR_MATRIX_Get(void *pMtx);
void CUR_MATRIX_Txyz4s(void *pVec);
void CUR_MATRIX_Rzyx4s(void *pAngle);
void JNT_readAttribute(void *pWork, void *pBlock);
JNT_ELEMENT *JNT_setMDLMatrix(JNT_WORK *pWork, JNT_ELEMENT *pElement);

/* A producer callback: (work area, root element, registered argument). */
typedef void (*JNT_CONSUMER)(JNT_WORK *, JNT_ELEMENT *, void *);

void xglMatrixUnit4s(void *pMtx);
void xglMatrixMul(void *pOut, void *pA, void *pB);
void MATRIX_convert4(void *pOut, void *pIn);

/* Element count of the model currently loaded in the work area */
int JNT_hairID(JNT_WORK *pWork)
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

/* 128-bit mode forces a single quadword lq/sq pair for the four-float
 * root TRS rows below (same lever as Java_Camera.c's CFANGLE_QUAD). */
typedef int JNT_QUAD __attribute__((mode(TI)));

/* FLAG REQUEST (verified, not yet wired into configure.py -- these three
 * are intentionally left UNREGISTERED below): gcc2.96 emits `j $31` with
 * the trailing `sq` filled into its delay slot (confirmed in the raw -S
 * output, not a gas reorder-pass steal), while the original keeps the sq
 * above the branch with a genuine nop in the slot -- exactly the shape
 * --hoist-return-store already fixes for xglLight.c. That alone reduces
 * every one of the three to a pure $v0<->$v1 tie-break (REGISTER, 4
 * diffs): the lq's address temp and its loaded value land in the
 * opposite register from the original in every source shape tried
 * (temp pointer var, direct value var, float* vs JNT_QUAD* typing,
 * declaration-order swaps). Added a new whole-function register-swap
 * pass, tools/fix_cc_asm.py's --swap-regs FUNC:A-B (A/B are raw MIPS
 * register numbers, $v0=2/$v1=3 here), to close that gap; verified
 * MATCH for all three with:
 *   --hoist-return-store JNT_getRootTrans,JNT_getRootRotate,JNT_getRootScale
 *   --swap-regs JNT_getRootTrans:2-3
 *   --swap-regs JNT_getRootRotate:2-3
 *   --swap-regs JNT_getRootScale:2-3
 * (four flags, space-joined, as one configure.py FILE_FIX_FLAGS["JNT.c"]
 * entry). Cannot self-apply: configure.py is off-limits to this agent.
 * Once wired, register:
 *   JNT_getRootTrans = 0x00313D28, 0x18; // JNT.c
 *   JNT_getRootRotate = 0x00313D40, 0x18; // JNT.c
 *   JNT_getRootScale = 0x00313D58, 0x18; // JNT.c
 */

/* Copy the root translation row out of the scratchpad work area */
void JNT_getRootTrans(void *pOut)
{
    float *pSrc;

    pSrc = JNT->aRootTrans;
    *(JNT_QUAD *)pOut = *(JNT_QUAD *)pSrc;
}

/* Copy the root rotation row out of the scratchpad work area */
void JNT_getRootRotate(void *pOut)
{
    float *pSrc;

    pSrc = JNT->aRootRotate;
    *(JNT_QUAD *)pOut = *(JNT_QUAD *)pSrc;
}

/* Copy the root scale row out of the scratchpad work area */
void JNT_getRootScale(void *pOut)
{
    float *pSrc;

    pSrc = JNT->aRootScale;
    *(JNT_QUAD *)pOut = *(JNT_QUAD *)pSrc;
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

/* gcc 2.96 emitted the nVersion>=2 test as `bnezl` (branch-likely);
 * the original used the plain `bnez` with the identical delay-slot
 * fill (`addiu v0,a1,16`), which is dead on the fall-through path, so
 * the annul bit carries no semantics here. Nothing at source level
 * reaches gcc's delayed-branch annul choice, so this one is steered:
 *   --branch-unlikely JNT_setModel:1
 * (site index 1 = second conditional branch, 0-based; site 0 is the
 * `pModel == 0` check, which already comes out plain).
 */

/* Bind a model and its handle, and locate the root element. Version 0/1
 * models have no attribute block so the block itself is the root; later
 * versions parse the attribute block and skip past it. */
void JNT_setModel(void *pHandle, JNT_MODEL *pModel)
{
    JNT_ATTRIBUTE_BLOCK *pBlock;
    unsigned int nSize;
    unsigned short nVersion;

    JNT->pHandle = pHandle;
    JNT->pModel = pModel;
    JNT->nHash = 0;
    if (pModel == 0) {
        JNT->pRoot = 0;
        return;
    }
    nVersion = pModel->nVersion;
    if (nVersion >= 2) {
        pBlock = &pModel->block;
        JNT_readAttribute(JNT, pBlock);
        nSize = pBlock->nSize;
        JNT->pRoot = (JNT_ELEMENT *)((char *)pBlock + nSize);
        return;
    }
    JNT->pRoot = (JNT_ELEMENT *)&pModel->block;
}

/* Apply the static (non-animated) transform for one element and advance
 * the model's element cursor. Types 1-4 and 5 both drive the current
 * matrix through the same T/R decode; type 0 (and anything above 5)
 * only advances the cursor. */
JNT_ELEMENT *JNT_getStaticVal2(JNT_WORK *pWork, JNT_ELEMENT *pElement)
{
    int nType;
    void *pTrans;
    void *pRotate;
    JNT_ELEMENT *pNext;

    nType = pElement->nType;
    pTrans = (char *)pElement + 0x10;
    pRotate = (char *)pElement + 0x20;
    pNext = pElement + 1;
    if (nType < 5) {
        if (nType == 0) {
            goto common;
        }
        goto transform;
    } else if (nType == 5) {
        CUR_MATRIX_Set((char *)pWork->pMatrix + (pElement->nMatrix << 6));
        CUR_MATRIX_Txyz4s(pTrans);
        CUR_MATRIX_Rzyx4s(pRotate);
        CUR_MATRIX_Get((char *)pWork->pMatrix + (pWork->nIndex << 6));
    }
    goto common;

transform:
    CUR_MATRIX_Set((char *)pWork->pMatrix + (pElement->nMatrix << 6));
    CUR_MATRIX_Txyz4s(pTrans);
    CUR_MATRIX_Rzyx4s(pRotate);
    CUR_MATRIX_Get((char *)pWork->pMatrix + (pWork->nIndex << 6));

common:
    pWork->nIndex++;
    return pNext;
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


/* Register (or re-register) a consumer callback on a producer.  An
   existing entry with the same key is overwritten in place; otherwise a
   new slot is appended.  Always returns 0. */
int JNT_addConsumer(JNT_PRODUCER *pProducer, void *pKey, void *pFunc,
                    void *pArg)
{
    int nNum;
    int i;
    int nIndex;

    nNum = pProducer->nNum;
    nIndex = -1;
    for (i = 0; i < nNum; i++) {
        if (pProducer->aKey[i] == pKey) {
            nIndex = i;
            break;
        }
    }
    if (nIndex < 0) {
        nIndex = nNum;
        pProducer->nNum = nNum + 1;
    }
    pProducer->aKey[nIndex] = pKey;
    pProducer->aFunc[nIndex] = pFunc;
    pProducer->aArg[nIndex] = pArg;
    return 0;
}


/* Rebuild the whole matrix palette for the model currently loaded in the
   work area: first every element and sub-element, then whatever joints are
   left over.  Both passes drive JNT_setMDLMatrix from the element cursor
   it returns, and both reset the palette counter first. */
void JNT_setModelMatrix(void)
{
    JNT_ELEMENT *pRoot;
    JNT_ELEMENT *pElement;
    JNT_MODEL *pModel;
    int nIndex;
    int nTotal;

    pRoot = JNT->pRoot;
    if (pRoot == 0) {
        return;
    }
    pElement = pRoot;
    pModel = JNT->pModel;
    JNT->nIndex = 0;
    nTotal = pModel->nElementNum + pModel->nSubNum;
    JNT->nMatrixNum = 0;
    nIndex = 0;
    if (nTotal != 0) {
        do {
            pElement = JNT_setMDLMatrix(JNT, pElement);
            nIndex = JNT->nIndex;
        } while (nIndex < nTotal);
        pModel = JNT->pModel;
    }
    nTotal = pModel->nJointNum;
    JNT->nMatrixNum = 0;
    if (nIndex < nTotal) {
        do {
            pElement = JNT_setMDLMatrix(JNT, pElement);
        } while (JNT->nIndex < nTotal);
    }
}

/* Begin one production pass: reset the per-frame cursor state, publish the
   producer/unit/frame into the work area and, unless the animation is
   suppressed, hand control to the producer's first consumer.  The consumer
   pointer is sanity-checked against the text segment and for alignment
   before it is called. */
int JNT_startProduction(JNT_PRODUCER *pProducer, void *pUnit, float fFrame)
{
    JNT_CONSUMER pFunc;
    JNT_ELEMENT *pRoot;
    void *pArg;

    JNT->nInterrupt = 0;
    JNT->nIndex = 0;
    pProducer->nUnk70 = 0;
    JNT->fFrame = fFrame;
    JNT->pUnit = pUnit;
    JNT->pProducer = pProducer;
    /* Read while the 0x70000000 base is still CSEd in one register: the
       original folds this load into the delay slot of the test below. */
    pRoot = JNT->pRoot;
    if ((JNT->nAnimFlags & 0x08000000) != 0) {
        pProducer->nFlags = 0;
        return 0;
    }
    if (pProducer->nNum > 0) {
        pFunc = (JNT_CONSUMER)pProducer->aFunc[0];
        pArg = pProducer->aArg[0];
        if ((unsigned int)((int)pFunc - 0x1F0000) <= 0x1E0FFFF
            && ((int)pFunc & 3) == 0) {
            pFunc(JNT, pRoot, pArg);
            if (pUnit != 0) {
                *(void **)((char *)pUnit + 0xA60) = JNT->pMatrix;
            }
        }
    }
    pProducer->nFlags = JNT->nFlags;
    return 0;
}


/* TODO: near-miss (100 built vs 99 original words).  Every block is
 * structurally right -- the quadword save of the root matrix, both element
 * walks, the two JNT_getStaticVal2 loops and the palette re-multiply -- and
 * the whole difference is one CSE inside the last loop.
 *
 * The original reads pWork->nIndex THREE times per iteration: once for the
 * MATRIX_convert4 source, once for the destination address, and once for
 * the increment.  The first two survive here because there are calls
 * between them, but gcc 2.96 folds the third into the second: the only
 * thing separating them is the four TI-mode quadword stores, and
 * -fstrict-aliasing says a `mode(TI)` store cannot touch an `int` load.
 *
 * The LAUNDER_V below is what restores that third read, and it is worth
 * exactly one word plus 19 instructions: with it the function is the
 * original's length (99 words) and 52 diffs, without it 100 words and 71.
 * It introduces no new differing word -- the two diff sets are nested.
 *
 * Everything else tried does NOT reach the CSE: the JVAL/JSVALUE union
 * trick that fixes this class in Java_Chr.c and JS.c (both wrapping the
 * quadword in a union so the stores take the union's alias set, and
 * reading nIndex through a union with a JNT_QUAD member); three copy
 * shapes (a JNT_QUAD* source cursor, the fully inlined form, and a char*
 * cursor with byte offsets); reading the index through a
 * `volatile int *`, in both cast-in-place and named-local form; and
 * building the whole file with -fno-strict-aliasing, which is strictly
 * worse -- it costs three other functions in JNT.c and leaves this one
 * at 51 diffs.
 *
 * WHAT IS LEFT (52 diffs, all in two clumps, indices 3-33 and 60-88) is a
 * register tie-break, not an aliasing problem: gcc puts pWork->pMatrix in
 * $a1 where the original uses $v1, which forces `move $4,$5` for the
 * xglMatrixUnit4s argument and pushes `move s3,a1` out of that call's
 * delay slot into the prologue.  Fencing before the call does not move
 * it (swept: LAUNDER/LAUNDER_V on pWork, on pSrc, a named pMat local,
 * and the fully direct copy form -- all 52).
 *
 * Also worth keeping: pWork->pModel has to be re-read after the first walk
 * (the `nTotal += pModel->nHairNum` line) or gcc parks it in a callee-saved
 * register and the whole register assignment shifts.
 *
 * Rebuild the palette from the current static pose.
 *
 * The root matrix is saved off to the work area's 0x720 slot and the live
 * one reset to identity, then the element walk is run twice: once over the
 * elements/sub-elements/extras and once over the hair range.  Only if the
 * hair range was non-empty is the palette then re-multiplied by the saved
 * root.  Returns the element cursor the walk left behind. */
JNT_ELEMENT *JNT_resetMatrix(JNT_WORK *pWork, JNT_ELEMENT *pElement)
{
    float aBase[16];
    float aMul[16];
    float aCur[16];
    JNT_MODEL *pModel;
    char *pRoot;
    char *pDst;
    JNT_QUAD *pSrc;
    int nIndex;
    int nTotal;
    int bHair;
    int i;

    pRoot = (char *)pWork + 0x720;
    pSrc = (JNT_QUAD *)pWork->pMatrix;
    ((JNT_QUAD *)pRoot)[0] = pSrc[0];
    ((JNT_QUAD *)pRoot)[1] = pSrc[1];
    ((JNT_QUAD *)pRoot)[2] = pSrc[2];
    ((JNT_QUAD *)pRoot)[3] = pSrc[3];
    xglMatrixUnit4s(pWork->pMatrix);

    pModel = pWork->pModel;
    nIndex = pWork->nIndex;
    nTotal = pModel->nElementNum + pModel->nSubNum + pModel->nExtraNum;
    while (nIndex < nTotal) {
        pElement = JNT_getStaticVal2(pWork, pElement);
        nIndex = pWork->nIndex;
    }
    pModel = pWork->pModel;
    nTotal += pModel->nHairNum;
    bHair = nIndex < nTotal;
    if (bHair) {
        do {
            pElement = JNT_getStaticVal2(pWork, pElement);
        } while (pWork->nIndex < nTotal);
    }
    MATRIX_convert4(aBase, pRoot);
    pWork->nIndex = nIndex;
    if (bHair) {
        do {
            MATRIX_convert4(aCur, (char *)pWork->pMatrix + pWork->nIndex * 64);
            xglMatrixMul(aMul, aBase, aCur);
            pDst = (char *)pWork->pMatrix + pWork->nIndex * 64;
            ((JNT_QUAD *)pDst)[0] = ((JNT_QUAD *)aMul)[0];
            ((JNT_QUAD *)pDst)[1] = ((JNT_QUAD *)aMul)[1];
            ((JNT_QUAD *)pDst)[2] = ((JNT_QUAD *)aMul)[2];
            ((JNT_QUAD *)pDst)[3] = ((JNT_QUAD *)aMul)[3];
            /* gcc CSEs this reload of nIndex with the one that built
             * pDst above: -fstrict-aliasing says the four TI stores in
             * between cannot touch an int. The original reloads. */
            LAUNDER_V(pWork);
            i = pWork->nIndex + 1;
            pWork->nIndex = i;
        } while (i < nTotal);
    }
    return pElement;
}
