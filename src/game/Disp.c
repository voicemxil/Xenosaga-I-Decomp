/*
 * Disp: field-overlay drawing helpers -- enemy markers, the collision
 * debug overlays and the "pillar" ring primitive the encounter effects
 * draw around an actor.
 */

#include "common.h"

/* The four enemy-marker models, loaded by the field resource pass. */
extern void *AdrsEnemyExclamation;
extern void *AdrsEnemyQuestion;
extern void *AdrsEnemySphere;

typedef struct {
    void *pUnk00;
    void *pExclamation;
    void *pQuestion;
    void *pSphere;
} DISPMARKTBL;

/* The enemy-awareness marker draw. The shipped build has the body
 * compiled out; what survives is the marker table being built on the
 * stack and copied into the (now unused) working copy. */
void Disp_EnemyMark(void)
{
    char aWork[0xE0];
    DISPMARKTBL aMark;
    DISPMARKTBL aInit;

    aInit.pUnk00 = NULL;
    aInit.pExclamation = AdrsEnemyExclamation;
    aInit.pQuestion = AdrsEnemyQuestion;
    aInit.pSphere = AdrsEnemySphere;
    aMark = aInit;
}

/* A point in the field renderer's primitives.  The parameter form is a
 * bare 4-float struct (4-byte aligned, as it sits inside packed actor
 * work areas); the local working copies are the quadword-aligned union,
 * which is why the loads off pCenter are ldl/ldr pairs while the
 * local-to-local copies are plain ld/sd. */
typedef int DISP_TI __attribute__((mode(TI)));

typedef struct {
    float x;
    float y;
    float z;
    float w;
} DISPXYZW;

typedef union {
    DISPXYZW v;
    DISP_TI q;
} DISPVEC;

extern float xglSin(float fRad);
extern float xglCos(float fRad);
extern float D_004D7F88;
extern float D_004D7F8C;

/* Walk the seventeen 22.5-degree steps around a vertical cylinder at
 * pCenter, building the four corners of each wall quad.  The quad
 * submission itself is compiled out of the shipped build; the corner
 * maths is all that is left. */
/* The two adjacent step-angle symbols intentionally stay distinct.  That
 * source shape makes GCC reuse f20 for each constant and product, matching
 * retail's lone callee-saved FPR.  GCC still canonicalizes the two sources of
 * each commutative multiply in the opposite encoding, corrected at the two
 * audited --swap-fp-operands sites without changing the operation. */
void DispPillar(DISPXYZW *pCenter, float *pSize)
{
    DISPVEC a;
    DISPVEC b;
    DISPVEC c;
    DISPVEC d;
    float index_float;
    float next_float;
    float angle0;
    float angle1;
    int i;

    pCenter->w = 1.0f;
    for (i = 0; i < 17; i++) {
        a.v = *pCenter;
        b.v = *pCenter;
        index_float = (float)i;
        angle0 = D_004D7F88;
        angle0 = index_float * angle0;
        a.v.x += xglSin(angle0) * pSize[0];
        angle1 = D_004D7F8C;
        a.v.z += xglCos(angle0) * pSize[0];
        next_float = (float)(i + 1);
        angle1 = next_float * angle1;
        b.v.x += xglSin(angle1) * pSize[0];
        b.v.z += xglCos(angle1) * pSize[0];
        c = a;
        d = b;
        c.v.y += pSize[1];
        d.v.y += pSize[1];
    }
}
