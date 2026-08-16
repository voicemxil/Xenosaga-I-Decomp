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

typedef struct {
    u8 pad000[0x4];                 /* 0x00 */
    int nMode;                      /* 0x04 */
    u8 pad008[0x8];                 /* 0x08 */
    float fPos[4];                  /* 0x10 */
} CURSOR;

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
