/* Debug free-cursor and lighting/wind updates.
 *
 * The cursor is the developer fly-through marker: the pad's analog
 * stick drives a direction vector, which is rotated into world space by
 * the active camera's orientation (translation row zeroed) and added to
 * the cursor position. SELECT cycles which axes the stick feeds. */

#include "common.h"

typedef int TI __attribute__((mode(TI)));

typedef struct {
    u8 pad000[0x2];                 /* 0x00 */
    u16 nTrigger;                   /* 0x02 */
    u8 pad004[0x4];                 /* 0x04 */
    s8 nX;                          /* 0x08 */
    s8 nY;                          /* 0x09 */
} KEYDATA;

/* One entry of the debug actor table: 64 slots of 0xA70 bytes. */
typedef struct UPDACTOR {
    int nFlags;                     /* 0x00 */
    u8 pad004[0x10 - 0x04];         /* 0x04 */
    float fPos[4];                  /* 0x10 */
    u8 pad020[0x30 - 0x20];         /* 0x20 */
    int n30;                        /* 0x30 */
    int n34;                        /* 0x34 */
    int n38;                        /* 0x38 */
    u8 pad03C[0x50 - 0x3C];         /* 0x3C */
    float fRot[4];                  /* 0x50 */
    u8 pad060[0x80 - 0x60];         /* 0x60 */
    u8 nId;                         /* 0x80 */
    u8 pad081[0x86 - 0x81];         /* 0x81 */
    short nAlive;                   /* 0x86 */
    u8 pad088[0xA70 - 0x88];        /* 0x88 */
} UPDACTOR;

typedef struct {
    u8 pad000[0x4];                 /* 0x00 */
    int nMode;                      /* 0x04 */
    int nActor;                     /* 0x08 */
    u8 pad00C[0x4];                 /* 0x0C */
    float fPos[4];                  /* 0x10 */
    u8 pad020[0x30 - 0x20];         /* 0x20 */
    int nHold;                      /* 0x30 */
    UPDACTOR *pSel;                 /* 0x34 */
    u8 pad038[0x40 - 0x38];         /* 0x38 */
    float fOfs[4];                  /* 0x40 */
} CURSOR;

typedef struct {
    u8 pad000[0x28];                /* 0x00 */
    u16 nButton;                    /* 0x28 */
    u16 nPress;                     /* 0x2A */
    u8 pad02C[0x68 - 0x2C];         /* 0x2C */
} UPDPAD;

extern UPDPAD PadData[2];
extern UPDACTOR actor[64];
extern int mode_004DC5A8;

UPDACTOR *nextActor(int nIndex);
UPDACTOR *prevActor(int nIndex);
float ball2point(float *pPos, float *pBall, float fRadius);
void ACT_updateSequence(UPDACTOR *pActor);

typedef struct {
    u8 pad000[0x170];               /* 0x000 */
    float fMatrix[16];              /* 0x170 */
} UPDCAMERA;

extern KEYDATA keyData;
extern CURSOR cursor;

void xglStudioGetCamera(UPDCAMERA **, int);
void xglVectorLength(float *, const float *);
void xglVectorMulMat(void *, void *, void *);
void xglVectorNormal(void *, void *);

/* TODO: near-miss (17/130 words) - the camera-matrix copy.  The
 * original stores the four quadwords through a REGISTER base ($a1,
 * which is also the pointer it later passes to xglVectorMulMat) while
 * gcc addresses the frame slot sp-relative and interleaves the three
 * m[3][xyz]=0 stores into the copy.  Introducing an explicit
 * destination pointer does produce the register base but costs a
 * redundant `move` into $a1; PIN-ing that pointer to $a1 removes the
 * move and gets to 16 words, leaving only the scheduler's interleave --
 * not worth the steering for a function that still does not match.
 * Swept: pointer/array forms for both ends of the copy, a TI temp
 * between load and store, the zero stores before vs after the copy,
 * and pinning the source pointer as well.
 *
 * Stick-driven cursor move: SELECT cycles between XY, X-only and
 * Y-only, and the stick vector is only applied past a dead zone. */
void updateCursorMode1(void)
{
    float vec[4];
    float dir[4];
    float mtx[16];
    UPDCAMERA *pCamera;
    float fLen;
    float *pPos;
    KEYDATA *pKey;
    TI *pSrc;

    pKey = &keyData;
    xglStudioGetCamera(&pCamera, 0);
    pPos = cursor.fPos;
    if (pKey->nTrigger & 2) {
        cursor.nMode++;
        if (cursor.nMode >= 3) {
            cursor.nMode = 0;
        }
    }
    fLen = 0.0f;
    switch (cursor.nMode) {
    case 0:
        vec[2] = 0.0f;
        vec[3] = 1.0f;
        vec[0] = pKey->nX;
        vec[1] = -pKey->nY;
        break;
    case 1:
        vec[1] = 0.0f;
        vec[3] = 1.0f;
        vec[2] = 0.0f;
        vec[0] = pKey->nX;
        break;
    case 2:
        vec[0] = 0.0f;
        vec[3] = 1.0f;
        vec[2] = 0.0f;
        vec[1] = -pKey->nY;
        break;
    }
    xglVectorLength(&fLen, vec);
    if (fLen > 40.0f) {
        pSrc = (TI *)pCamera->fMatrix;
        ((TI *)mtx)[0] = pSrc[0];
        ((TI *)mtx)[1] = pSrc[1];
        ((TI *)mtx)[2] = pSrc[2];
        ((TI *)mtx)[3] = pSrc[3];
        mtx[12] = 0.0f;
        mtx[13] = 0.0f;
        mtx[14] = 0.0f;
        xglVectorMulMat(dir, mtx, vec);
        xglVectorNormal(dir, dir);
        fLen = 0.13333334f;
        pPos[0] += dir[0] * fLen;
        pPos[1] += dir[1] * fLen;
        pPos[2] += dir[2] * fLen;
    }
}

/* Actor-picking cursor mode: pad 1 steps through the 64-slot actor
 * table, the shoulder buttons nudge the held actor's Y offset and
 * heading, and pad 0's circle button grabs or drops it -- dropping with
 * nothing held re-picks by sphere test against the cursor position.
 *
 * Three scoping decisions carry the register allocation here:
 *
 *  - The &cursor address is materialised four times over (once as
 *    cursor.fPos minus 16, three times fresh), so each dispatch arm
 *    declares its OWN block-local CURSOR *p. One function-scope pointer
 *    instead becomes a multi-block pseudo that local_alloc skips.
 *  - The loop that re-picks writes through `cursor.pSel` by name rather
 *    than through p, which is what keeps p itself caller-saved and puts
 *    the loop's own copy in a callee-saved register.
 *  - Conversely, the float* into the selected actor's position IS one
 *    function-scope local shared by the copy-in and the write-back
 *    arms; splitting it per-block swaps it with the mode read for $v0. */
void updateCursorMode2(void)
{
    UPDCAMERA *pCamera;
    UPDACTOR *pAct;
    float *q;
    UPDPAD *pPad;
    float *pPos;
    int i;

    pAct = NULL;
    pPad = &PadData[1];
    xglStudioGetCamera(&pCamera, 0);
    pPos = cursor.fPos;
    if (pPad->nPress & 8) {
        CURSOR *p = &cursor;

        pAct = nextActor(p->nActor);
        if (pAct == NULL) {
            p->nActor = 0;
        } else {
            p->nActor = pAct->nId + 1;
        }
    }
    if (pPad->nPress & 2) {
        CURSOR *p = &cursor;

        pAct = prevActor(p->nActor);
        if (pAct == NULL) {
            p->nActor = 63;
        } else {
            p->nActor = pAct->nId - 1;
        }
    }
    if (pAct != NULL) {
        q = pAct->fPos;

        pPos[0] = q[0];
        pPos[1] = q[1];
        pPos[2] = q[2];
    }
    {
        CURSOR *p = &cursor;

        if (p->pSel != NULL) {
            if (p->nHold == 0) {
                float *pRot = p->pSel->fRot;
                unsigned int nBtn = pPad->nButton;

                if (nBtn & 0x1000) {
                    p->fOfs[1] += 0.03333333507180214f;
                }
                if (nBtn & 0x4000) {
                    p->fOfs[1] -= 0.03333333507180214f;
                }
                if (nBtn & 0x2000) {
                    pRot[1] += 0.02617993950843811f;
                }
                if (nBtn & 0x8000) {
                    pRot[1] -= 0.02617993950843811f;
                }
            }
        }
    }
    {
        CURSOR *p = &cursor;

        pPad = &PadData[0];
        if (p->pSel != NULL) {
            if (pPad->nPress & 0x10) {
                pAct = p->pSel;
                p->pSel = NULL;
                pAct->n30 = 0;
                pAct->n34 = 0;
                pAct->n38 = 0;
            } else if (p->nHold == 0) {
                int nMode;

                pAct = p->pSel;
                q = pAct->fPos;
                nMode = mode_004DC5A8;
                q[0] = pPos[0] + p->fOfs[0];
                q[1] = pPos[1] + p->fOfs[1];
                q[2] = pPos[2] + p->fOfs[2];
                if (nMode == 0) {
                    ACT_updateSequence(p->pSel);
                }
            }
        } else if (pPad->nPress & 0x10) {
            float fBest;

            p->fOfs[0] = 0.0f;
            p->fOfs[1] = 0.0f;
            p->fOfs[2] = 0.0f;
            pAct = actor;
            fBest = p->fOfs[0];
            for (i = 0; i < 64; i++) {
                if (pAct->nAlive != 0 && !(pAct->nFlags & 8) &&
                    fBest <= ball2point(pPos, pAct->fPos, 0.5f)) {
                    cursor.pSel = pAct;
                }
                pAct++;
            }
        }
    }
}
