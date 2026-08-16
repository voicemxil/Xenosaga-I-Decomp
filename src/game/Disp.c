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

/* Walk the seventeen 22.5-degree steps around a vertical cylinder at
 * pCenter, building the four corners of each wall quad.  The quad
 * submission itself is compiled out of the shipped build; the corner
 * maths is all that is left. */
/* TODO: near-miss (49/98 words, one instruction long). Logic, loop count,
 * struct alignment and constants all verified against the original. The
 * single extra instruction is an FP callee-saved register: the original
 * keeps 0.39269908f and the i*step product in the SAME register (f20) and
 * therefore reloads the constant per angle, while 2.96 here hoists the
 * constant into f21 and saves two FP registers. Forcing the reload with a
 * static float works but then the product cannot be CSEd across the call
 * (a static could change), which costs more than it saves. */
void DispPillar(DISPXYZW *pCenter, float *pSize)
{
    DISPVEC a;
    DISPVEC b;
    DISPVEC c;
    DISPVEC d;
    int i;

    pCenter->w = 1.0f;
    for (i = 0; i < 17; i++) {
        a.v = *pCenter;
        b.v = *pCenter;
        a.v.x += xglSin(i * 0.39269908f) * pSize[0];
        a.v.z += xglCos(i * 0.39269908f) * pSize[0];
        b.v.x += xglSin((i + 1) * 0.39269908f) * pSize[0];
        b.v.z += xglCos((i + 1) * 0.39269908f) * pSize[0];
        c = a;
        d = b;
        c.v.y += pSize[1];
        d.v.y += pSize[1];
    }
}
