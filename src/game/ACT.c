/* Actor layer - creation, matrix/joint plumbing, motion update and draw passes */

#include "matching.h"

#include "game/actor.h"

typedef struct {
    int f00;                        /* 0x00 */
    int f04;                        /* 0x04 */
    int f08;                        /* 0x08 */
    int f0C;                        /* 0x0C */
    int f10;                        /* 0x10 */
    int f14;                        /* 0x14 */
    int f18;                        /* 0x18 */
    int f1C;                        /* 0x1C */
    int f20;                        /* 0x20 */
    int f24;                        /* 0x24 */
    int f28;                        /* 0x28 */
    int f2C;                        /* 0x2C */
    int f30;                        /* 0x30 */
    u8 pad34[0x22C];                /* 0x34 */
} ACT_SEQUENCE;

int actMatrixHeap;

extern RSRC matrixHeap;
extern u8 actMatrix[];
extern u8 matrixHeapBlock[];
extern ACTOR actor[];
extern ACT_SEQUENCE actSequence[];
extern void nmlModelDirectSend(int, void *, int);

void memset();
void RSRC_inactiveSource(int, ACTOR *);
void *RSRC_alloc(int, int, ACTOR *);
int FCV2_checkData(int);
int PACK_getEntry(int, int);
int ANM_getEntry(void *, int);
void MDL_create(void *, void *);
void MDL_setVisible(void *, unsigned int, int);
void MDL_setGroupVisible(void *, unsigned int, int);
int JNT_getMoveElement(void *);
int *JNT_getAccessories(void *);
int JNT_getRootElement(void *);
void EXM_StepShakeWind(void);
void xglStudioFlushActiveCamera(void);
void xglMatrixStackUnit(void);
void DB_reset(int, int);
void LOOK_target_doit(void);
void SEQ_motion(ACTOR *);
void *PLAY_getCurrent(void);
void UnduParamInit(void *);
float UnduGet(float, float);
void UnduGet2(void *, float, float);
void nmlModelSetToumei(int);
void nmlModelSetZwrite(int);
void nmlModelSetStencil(int);
void nmlModelSetFilter(int, float, float);
void nmlModelSetTransparency(float);
void nmlModelSetReflTransparency(float);

void ACT_info_00306090(void);
void ACT_resetMatrix(ACTOR *);
void ACT_resetParent();
void ACT_modelDraw(ACTOR *);
int ACT_setParent(ACTOR *, int, ACTOR *, int, int);
void ACT_setHumanHand(ACTOR *, int);
void ACT_setVisible(ACTOR *, unsigned int, int);
void ACT_modelDrawSub(ACTOR *);
void ACT_updateMotionSub(ACTOR *, int);
void ACT_updateMotion(ACTOR *);
void ACT_updateMotionCore(ACTOR *, int);
void ACT_updateSequence(ACTOR *);
void ACT_updateNPC(ACTOR *);
void ACT_dispose2(ACTOR *, int);
ACTOR *ACT_create(int, int);
int ACT_jointGetAccessories(ACTOR *, unsigned int);

/* Reserve the matrix buffers an actor needs for its joint hierarchy */
int ACT_allocMatrix(ACTOR *a, int nCount)
{
    void *pMatrix;
    char *pWork;
    int nSize;
    int nBase;
    int nExtra;

    if (nCount < 0) {
        if (a->pJoint != 0) {
            nCount = ((JOINT *)a->pJoint)->nMatrixNum;
        } else {
            nCount = 1;
        }
    }
    if (actMatrixHeap == 0) {
        return -1;
    }
    if (nCount <= 0) {
        return nCount;
    }
    nSize = nCount * 64;
    nBase = nCount * 128;
    nExtra = (a->nMatrixType == 1) ? nSize : 0;
    pMatrix = RSRC_alloc(actMatrixHeap, nBase + nExtra, a);
    if (pMatrix != 0) {
        a->pMatrixWork = (char *)pMatrix + nSize;
        a->pMatrix = pMatrix;
        if (a->nMatrixType == 1) {
            pWork = (char *)pMatrix + nBase;
            a->pMatrixEx = pWork;
            a->pMatrixEx2 = pWork + nCount * 32;
            memset(pWork, 0, nSize);
        }
        ACT_resetMatrix(a);
    }
    return nCount;
}

/* Allocate a raw block of matrices from the actor matrix heap */
void *ACT_allocBlock(void *pOwner, int nCount)
{
    return RSRC_alloc(actMatrixHeap, nCount * 64, pOwner);
}

/* Build the actor matrix heap over its static backing store */
void ACT_matrixInit(void)
{
    matrixHeap.nSize = 0xC0000;
    matrixHeap.nItemCount = 0;
    matrixHeap.nItemCapacity = 0x40;
    actMatrixHeap = (int)&matrixHeap;
    matrixHeap.pBase = (u_int)actMatrix;
    matrixHeap.pItems = (RSRCITEM *)matrixHeapBlock;
    matrixHeap.pCurrent = (u_int)actMatrix;
    memset(matrixHeapBlock, 0, 0x400);
}

/* Release an actor, unlinking it from its parent */
void ACT_dispose(ACTOR *a)
{
    ACT_dispose2(a, 0);
}

/* Release an actor, optionally keeping its resource pointers */
void ACT_dispose2(ACTOR *a, int nKeep)
{
    int *p;
    int i;

    RSRC_inactiveSource(actMatrixHeap, a);
    if (a->pParent != 0) {
        ACT_resetParent(a, a->pParent);
    }
    a->nAlive = 0;
    a->pUpdate = 0;
    a->nUnk008 = 0;
    a->nFlags = 0;
    if (nKeep == 0) {
        p = &a->pPack[7];
        for (i = 0xA; i >= 0; i--) {
            *p = 0;
            p--;
        }
    }
}

/* Draw every live actor for this frame */
void ACT_draw(void)
{
    ACTOR *a;
    int i;

    EXM_StepShakeWind();
    ACT_info_00306090();
    a = actor;
    for (i = 0x3F; i >= 0; i--, a++) {
        if (a->nAlive != 0) {
            ACT_modelDraw(a);
        }
    }
}

/* Run the per-frame update callback of every live actor */
void ACT_update(void)
{
    ACTOR *a;
    int i;

    xglStudioFlushActiveCamera();
    DB_reset(8, 8);
    a = actor;
    for (i = 0x3F; i >= 0; i--, a++) {
        if (a->nAlive != 0 && a->pUpdate != 0) {
            a->pUpdate(a);
        }
    }
    LOOK_target_doit();
}

/* Resolve (and cache) the move-element id of an actor's joint tree */
int ACT_jointGetMoveElementID(ACTOR *a)
{
    int nId = a->nMoveElementID;

    if (nId < 0) {
        if (a->pJoint != 0) {
            nId = JNT_getMoveElement(a->pJoint);
            a->nMoveElementID = nId;
        } else {
            a->nMoveElementID = 0;
            nId = 0;
        }
    }
    return nId;
}

/* Look up an accessory bone id, caching the joint accessory table */
int ACT_jointGetAccessories(ACTOR *a, unsigned int nJoint)
{
    /* The local is read back out of the field AFTER the if, not assigned
       inside it. Assigning the local inside makes gcc store from the local's
       register instead of the call result, and lets it thread the second
       (redundant) null test away, which loses both the store's operand and
       the delay-slot pick. */
    int nResult = -1;
    int *pAcc;

    if (a->pAccessories == 0) {
        a->pAccessories = JNT_getAccessories(a->pJoint);
    }
    pAcc = a->pAccessories;
    if (pAcc != 0) {
        if (nJoint < 0x29) {
            nResult = pAcc[nJoint + 8];
        }
    }
    return nResult;
}

/* Detach a held object from a character's hand */
void ACT_resetArms(ACTOR *a, ACTOR *b, int nMode)
{
    if (nMode == 0x108) {
        ACT_resetParent(a, b, nMode);
        ACT_setHumanHand(b, 0);
    }
}

/* Attach an object to an accessory bone or to a character's hand */
int ACT_setArms(ACTOR *a, ACTOR *b, int nMode, int nFlags)
{
    int nJoint;

    /* TODO: near-miss - single-instruction diff: emits bnel where the
       original has plain bne for the "nMode == 0x108" check; every other
       instruction (incl. register alloc and store/arg order) is byte-
       identical. gcc 2.96 chooses bnel here because the delay-slot store
       (move v0,s0, staging the return value) is dead on the not-taken path
       (a later "move v0,s0" at the merge point overwrites it), so it's safe
       to annul. 10 source variants tried this session (else-if, split ifs,
       early-return before the check, switch on nMode, register-pinned
       compare temp at $2, asm memory barrier, redundant nResult local) -
       all either reproduce this exact bnel or, when they do get plain bne
       (via an early "if (nMode != 0x108) return" form), break register
       allocation ($2->$3) and call-argument evaluation order elsewhere.
       Looks like a genuine gcc branch-likely cost heuristic tied to CFG
       shape, not reachable from C. Do not re-attempt without a new idiom. */
    if ((nMode & 0xFF00) == 0) {
        nJoint = ACT_jointGetAccessories(b, nMode);
        if (nJoint > 0) {
            a = (ACTOR *)ACT_setParent(a, 0, b, nJoint, nFlags);
        } else {
            a = 0;
        }
    } else if (nMode == 0x108) {
        a = (ACTOR *)ACT_setParent(a, 2, b, 0x42, nFlags | 2);
        ACT_setHumanHand(b, 0x8007);
    }
    return (int)a;
}

/* Parent a face model to a body and hide the body's own 'FACE' group */
int ACT_setFace(ACTOR *a, ACTOR *b, int nBone)
{
    int nResult = ACT_setParent(a, 2, b, 0x30, nBone);

    if (nResult != 0) {
        ACT_setVisible(b, 0x46414345, 0);
    }
    return nResult;
}

/* Fetch the user-data pointer of the actor's animation player */
int ACT_animGetUserData(ACTOR *a)
{
    return a->pUserData;
}

/* Test whether the actor's current animation data is valid */
int ACT_animCheckData(ACTOR *a)
{
    return FCV2_checkData(a->pAnimData);
}

/* Advance the actor's motion and that of all of its children */
void ACT_updateMotionCore(ACTOR *a, int nPause)
{
    /* TODO: near-miss, 3 of 54 words (was 17). Pinning the actor pointer to
       $s1 is what carries the whole register assignment: without it gcc puts
       the actor in $s2 and the loop counter in $s1, the mirror image of the
       original, and every s-register reference differs. Do NOT also pin the
       counter or pp -- pinning the counter to $s2 (which is where it lands
       anyway) costs an extra instruction (55 words, 35 diffs), and pinning
       pp to $v0 or $s0 wrecks the frame layout (49 diffs).
       The child base is taken inside each arm rather than once above the
       flag test: hoisting it makes the pseudo live across the test, so gcc
       gives it $a0 instead of reusing the $v0 the flag word just freed. */
    PIN(ACTOR *p, "$17");
    ACTOR **pp;
    int i;

    p = a;
    if (p->pParent != 0) {
        return;
    }
    ACT_updateMotionSub(p, nPause);
    if (p->nChildNum > 0) {
        if ((p->nFlags & 0x1000) == 0) {
            pp = p->pChild;
            for (i = 0; i < p->nChildNum; i++) {
                ACT_updateMotionSub(pp[i], nPause);
            }
        } else {
            pp = p->pChild;
            for (i = 0; i < p->nChildNum; i++) {
                pp[i]->nFlags |= 0x1000;
            }
        }
    }
}

/* Advance the actor's motion */
void ACT_updateMotion(ACTOR *a)
{
    ACT_updateMotionCore(a, 0);
}

/* Re-evaluate the actor's motion without advancing time */
void ACT_updateMotionPause(ACTOR *a)
{
    ACT_updateMotionCore(a, 1);
}

/* Draw an actor and, unless suppressed, all of its children */
void ACT_modelDraw(ACTOR *a)
{
    ACTOR **pp;
    int i;

    if (a->pParent == 0) {
        ACT_modelDrawSub(a);
        if ((a->nFlags & 0x1008) == 0) {
            if (a->nChildNum > 0) {
                pp = a->pChild;
                for (i = 0; i < a->nChildNum; i++) {
                    ACT_modelDrawSub(pp[i]);
                }
            }
        }
    }
}

/* Create the actor's model, binding it to the joint tree when there is one */
void ACT_setModelWrapper(ACTOR *a)
{
    void *pModel = a->model;
    void *pRes = a->pModelRes;
    void *pJoint = a->pJoint;
    void *pTexture = a->pTexture;
    int nRoot;

    if (pRes != 0) {
        if (pJoint == 0) {
            MDL_create(pModel, pRes);
            return;
        }
        MDL_create(pModel, pRes);
        nRoot = JNT_getRootElement(pJoint);
        nRoot += (((JOINT *)pJoint)->nElementNum +
                  ((JOINT *)pJoint)->nExElementNum) << 6;
        *(void **)((char *)pModel + 0x54) = pTexture;
        a->nFlags |= 0x4000;
        *(int *)((char *)pModel + 0x58) = nRoot;
    }
}

/* Motion resource table initialiser (nothing to do) */
void ACT_initMTNResource(void)
{
}

/* Actor resource system initialiser (nothing to do) */
void ACT_resourceInit(void)
{
}

/* Look up an animation entry by packed pack/entry id */
int ACT_animGetData(ACTOR *a, unsigned int nId)
{
    nId &= 0x7FF;
    return PACK_getEntry(a->pPack[nId >> 8], nId & 0xFF);
}

/* Look up the animation entry the actor is currently playing */
int ACT_animGetCurrent(ACTOR *a)
{
    ANM *p = (ANM *)a->anim;

    return ANM_getEntry(p, a->pPack[(p->nFlags >> 8) & 7]);
}

/* Bind an extra motion pack to one actor, or to every actor */
void ACT_initExMotion(ACTOR *a, int nIndex, int nData)
{
    ACTOR *p;
    int i;

    if (a == 0) {
        p = actor;
        for (i = 0; i < 0x40; i++) {
            p->pPack[nIndex] = nData;
            p++;
        }
    } else {
        a->pPack[nIndex] = nData;
    }
}

/* Show or hide a model part, or a whole part group */
void ACT_setVisible(ACTOR *a, unsigned int nId, int nFlag)
{
    if (nId >= 0x80) {
        MDL_setGroupVisible(a->model, nId, nFlag);
    } else {
        MDL_setVisible(a->model, nId, nFlag);
    }
}

/* Default actor update: follow the undulating ground, then run the motion */
void ACT_updateDefault(ACTOR *a)
{
    if (a->nFlags & 0x40) {
        *(float *)((char *)a + 0x14) =
            UnduGet(*(float *)((char *)a + 0x10), *(float *)((char *)a + 0x18));
    }
    ACT_updateMotion(a);
}

/* Actor update used by the RECT-random test actors */
void ACT_updateRECTRand(ACTOR *a)
{
    ACT_updateMotion(a);
}

/* Clear the script sequence slot belonging to an actor */
void ACT_initSequenceAt(ACTOR *a)
{
    ACT_SEQUENCE *p = &actSequence[a->nSerial];

    p->f00 = 0;
    p->f04 = 0;
    p->f0C = 0;
    p->f14 = 0;
    p->f18 = 0;
    p->f1C = 0;
    p->f20 = 0;
    p->f24 = 0;
    p->f28 = 0;
    p->f2C = 0;
    p->f30 = 0;
}

/* Detach the script VM object from one actor, or reset every actor */
void ACT_initVMObject(ACTOR *a)
{
    int i;

    if (a == 0) {
        a = actor;
        for (i = 0; i < 0x40; i++) {
            a->nFlags = 0x20;
            a->nShadowType = 1;
            a->nShadowSize = 0x50;
            a->nVMObject = 0;
        }
    } else {
        a->nVMObject = 0;
    }
}

/* Create a script-driven character actor */
ACTOR *ACT_createChr(int nId, int nArg)
{
    ACTOR *a = ACT_create(nId, nArg);
    ACT_SEQUENCE *p;

    /* Store order here is not deducible: all 22 stores are independent, and
       both an exhaustive sweep of the five flag/counter stores (120) and of
       the six around pUpdate/fScale (720), plus a 22-statement hill climb,
       bottom out at 4 diffs -- two independent transpositions of stores that
       gcc issues in the wrong order. nVMObject-before-nSignal is the one
       ordering the search did find (5 -> 4); the remaining two are
       reordering flags on ACT.c, audited as pure reorders:
         --rotate-seq ACT_createChr:18:5,ACT_createChr:19:-4
             exchanges the nFlags and nSignal stores, which sit four
             instructions apart with the actSequence index arithmetic
             interleaved, so neither a swap nor one rotation expresses it.
         --rotate ACT_createChr:24:2
             swaps the pUpdate and fScale[3] stores. --swap-adjacent
             refuses this site because both insns are memory ops; a
             two-instruction rotate is the same transposition without
             that guard. */
    if (a != 0) {
        p = &actSequence[a->nSerial];
        a->nShadowSize = 0x50;
        a->nShadowType = 1;
        a->nVMObject = 0;
        a->nSignal = 0;
        a->nFlags = 0x20;
        a->nUnk088 = 0;
        a->pUpdate = ACT_updateSequence;
        a->fScale[0] = 1.0f;
        a->fScale[1] = 1.0f;
        a->fScale[2] = 1.0f;
        a->fScale[3] = 1.0f;
        p->f00 = 0;
        p->f04 = 0;
        p->f0C = 0;
        p->f14 = 0;
        p->f18 = 0;
        p->f1C = 0;
        p->f20 = 0;
        p->f24 = 0;
        p->f28 = 0;
        p->f2C = 0;
        p->f30 = 0;
    }
    return a;
}

/* Create an NPC actor driven by the undulating-ground walker */
ACTOR *ACT_createNPC(int nId, int nArg)
{
    ACTOR *a = ACT_create(nId, nArg);
    ACT_SEQUENCE *p;

    if (a != 0) {
        UnduParamInit(&a->nUndu);
        p = &actSequence[a->nSerial];
        a->nSignal = 0;
        a->nVMObject = 0;
        a->pUpdate = ACT_updateNPC;
        a->nUnk088 = 0;
        p->f00 = 0;
        p->f04 = 0;
        p->f0C = 0;
        p->f14 = 0;
        p->f18 = 0;
        p->f1C = 0;
        p->f20 = 0;
        p->f24 = 0;
        p->f28 = 0;
        p->f2C = 0;
        p->f30 = 0;
    }
    return a;
}

/* Actor update for map-pack (scenery) actors */
void ACT_updateMPack(ACTOR *a)
{
    float *p = PLAY_getCurrent();

    xglMatrixStackUnit();
    SEQ_motion(a);
    ACT_updateMotion(a);
    *(float *)((char *)a + 0x6F4) = p[0x44 / 4];
    a->nUndu = 0;
    UnduGet2(&a->nUndu, *(float *)((char *)a->pMatrix + 0x30),
             *(float *)((char *)a->pMatrix + 0x38));
}

/* Reset the whole actor table at the start of a scene */
void ACT_initScene(void)
{
    ACTOR *a = actor;
    int i;

    for (i = 0x3F; i >= 0; i--) {
        a->nFlags = 0x20;
        a->nShadowType = 1;
        a->nShadowSize = 0x50;
        a->nUnk084 = 0;
        a->nAlive = 0;
        a->pUpdate = 0;
        a->nUnk008 = 0;
        a->fUnk01C = 1.0f;
        a->nUnk03C = 0;
        a->nUnk04C = 0;
        UnduParamInit(&a->nUndu);
        a++;
    }
}

/* Translate a script hand code into the internal hand/bone mask */
void ACT_setHand(ACTOR *a, int nHand)
{
    int nType;

    if ((a->nAlive & 0xF000) == 0) {
        nType = nHand & 0xF0;
        nHand &= 0xF;
        switch (nType) {
        case 0x00:
            nHand |= nHand << 8;
            break;
        case 0x10:
            nHand |= 0x8000;
            break;
        case 0x20:
            nHand = (nHand << 8) | 0x80;
            break;
        }
        ACT_setHumanHand(a, nHand);
    }
}

/* Draw filter for the Guno transparency effect */
void ACT_filterGuno(ACTOR *a)
{
    nmlModelSetToumei(1);
    nmlModelSetZwrite(1);
    nmlModelSetStencil(1);
    nmlModelSetFilter(1, *(float *)((char *)a + 0x9CC), *(float *)((char *)a + 0x9C8));
    nmlModelSetTransparency(*(float *)((char *)a + 0x9C0));
    nmlModelSetReflTransparency(*(float *)((char *)a + 0x9C4));
}

/* Draw filter for the stealth transparency effect */
void ACT_filterStealth(ACTOR *a)
{
    nmlModelSetToumei(1);
    nmlModelSetZwrite(1);
    nmlModelSetStencil(1);
    nmlModelSetFilter(2, *(float *)((char *)a + 0x9CC), *(float *)((char *)a + 0x9C8));
    nmlModelSetTransparency(*(float *)((char *)a + 0x9C0));
    nmlModelSetReflTransparency(*(float *)((char *)a + 0x9C4));
}

/* Shadow renderer initialiser (nothing to do) */
void ACT_DrawShadowInit(void)
{
}

extern u8 D_00478F80[];

/* End the shadow render batch */
void ACT_DrawShadowEnd(void)
{
    nmlModelDirectSend(1, D_00478F80, 3);
}
