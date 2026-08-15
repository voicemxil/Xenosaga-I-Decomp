#include "matching.h"

/* Directional/parallel light rig used by the xgl engine's shading pipeline */

typedef int TI __attribute__((mode(TI)));

void xglVectorNormal(void *pDest, void *pSource);
extern unsigned int Vu0CallLightCalcMatrix[];
void xglMatrixStackUnit(void);
void xglMatrixStackRotX(float);
void xglMatrixStackRotY(float);
void xglMatrixStackRotZ(float);
void xglMatrixStackSave(void *);

/* Default per-light templates (4 rows: ambient + up to 3 parallel slots) */
static unsigned int asIntensity_0[16] __asm__("asIntensity.0") = {
    0x3E800000, 0x3E800000, 0x3E800000, 0x3F800000,
    0x3F4CCCCD, 0x3F4CCCCD, 0x3F4CCCCD, 0x3F800000,
    0x3F000000, 0x3F000000, 0x3F000000, 0x3F800000,
    0x3F000000, 0x3F000000, 0x3F000000, 0x3F800000,
};
static unsigned int asDirection_1[16] __asm__("asDirection.1") = {
    0x00000000, 0x00000000, 0x00000000, 0x3F800000,
    0xBE861F28, 0x3F07FD47, 0xBCA290B6, 0x3F800000,
    0x4016CBE4, 0x3FA2FD60, 0x40490FDB, 0x3F800000,
    0x3EB8F33F, 0x4071A850, 0x3CE9C49C, 0x3F800000,
};
static unsigned int sIntensity0_2[4] __asm__("sIntensity0.2") = {
    0x00000000, 0x00000000, 0x00000000, 0x00000000,
};
static unsigned int sIntensity1_3[4] __asm__("sIntensity1.3") = {
    0x3F800000, 0x3F800000, 0x3F800000, 0x3F800000,
};

/* Copy a light's ambient color quadword verbatim */
void xglLightIntensityAmbient(void *pLight, void *pSrc)
{
    *(TI *)pLight = *(TI *)pSrc;
}

/* TODO: near-miss, 1/9 words differ (REGISTER pinning closed 2 of the 3
 * original diffs -- pBase/pDst now land in $v0/$v1 as the original does).
 * The remaining word is `addu v0,v0,a0` vs our `addu v0,a0,v0`: a
 * commutative-add operand-order canonicalization, not a register choice.
 * Matches the sceVif1Pk rd==rs canonicalization wall exactly -- swapping
 * the source addition order has no effect (gcc always canonicalizes),
 * and pinning the shift result to a third register didn't change it
 * either. Likely needs the same unidentified compiler build as the
 * sceVif1Pk near-misses; not a source-reachable fix. */
/* Set one of a light's up-to-3 parallel colors (no-op past slot 2) */
void xglLightIntensityParallel(void *pLight, unsigned int nNo, void *pSrc)
{
    PIN(char *pBase, "$2") = (char *)(nNo * 0x20 + (unsigned int)pLight);
    PIN(char *pDst, "$3") = pBase + 0x10;

    if (nNo < 3) {
        *(TI *)pDst = *(TI *)pSrc;
    }
}

/* Normalize and store one of a light's up-to-3 parallel directions
 * (the destination offset is always advanced by 0x20 even when the
 * slot is out of range; the call is simply skipped in that case) */
void xglLightDirection(void *pLight, unsigned int nNo, void *pSrc)
{
    char *pDst = (char *)pLight + nNo * 0x20 + 0x20;

    if (nNo < 3) {
        xglVectorNormal(pDst, pSrc);
    }
}

/* Build a light's rotation/direction matrices through the VU0
 * macro-mode light-matrix pipeline */
void xglLightCalcMatrix(void *pLight)
{
    __asm__ __volatile__(".set noreorder\n"
        "ctc2.i %0, $vi27\n"
        "lqc2 $vf4, 0x0(%1)\n lqc2 $vf5, 0x10(%1)\n"
        "lqc2 $vf6, 0x20(%1)\n lqc2 $vf7, 0x30(%1)\n"
        "lqc2 $vf8, 0x40(%1)\n lqc2 $vf9, 0x50(%1)\n"
        "lqc2 $vf10, 0x60(%1)\n"
        "vcallmsr $vi27\n"
        "cfc2.i $zero, $vi0\n"
        "sqc2 $vf20, 0x70(%1)\n sqc2 $vf21, 0x80(%1)\n"
        "sqc2 $vf22, 0x90(%1)\n sqc2 $vf23, 0xa0(%1)\n"
        "sqc2 $vf24, 0xb0(%1)\n sqc2 $vf25, 0xc0(%1)\n"
        "sqc2 $vf26, 0xd0(%1)\n sqc2 $vf27, 0xe0(%1)\n"
        ".set reorder" : : "r"((unsigned int)Vu0CallLightCalcMatrix >> 3), "r"(pLight));
}

/* Set light angles (rotation about Z,Y,X) and bake the resulting matrix's
 * direction row into slot nNo (no-op past slot 2) */
void xglLightAngle(void *pLight, unsigned int nNo, void *pAngles)
{
    unsigned int aMtx[16];
    float *pA = (float *)pAngles;
    void *pRow;
    void *pDst;

    if (nNo < 3) {
        xglMatrixStackUnit();
        xglMatrixStackRotZ(pA[2]);
        xglMatrixStackRotY(pA[1]);
        xglMatrixStackRotX(pA[0]);
        xglMatrixStackSave(aMtx);
        pDst = (char *)(nNo * 0x20 + (unsigned int)pLight) + 0x20;
        pRow = (char *)aMtx + 0x20;
        __asm__ __volatile__(".set noreorder\n"
            "lq $2, 0x0(%1)\n sq $2, 0x0(%0)\n"
            ".set reorder" : : "r"(pDst), "r"(pRow) : "$2", "memory");
    }
}

/* Reset all 4 light slots (1 ambient + 3 parallel) to their default
 * intensity/direction templates */
void xglLightSetDefault(void *pLight)
{
    int i;

    for (i = 0; i < 4; i++) {
        if (i == 0) {
            xglLightIntensityAmbient(pLight, asIntensity_0);
        } else {
            xglLightIntensityParallel(pLight, i - 1, &asIntensity_0[i * 4]);
            xglLightAngle(pLight, i - 1, &asDirection_1[i * 4]);
        }
    }
}

/* Initialize a light to full white ambient + 3 parallel lights all
 * pointed straight along the same default direction */
void xglLightInit(void *pLight)
{
    int i;

    xglLightIntensityAmbient(pLight, sIntensity1_3);
    for (i = 0; i < 3; i++) {
        xglLightIntensityParallel(pLight, i, sIntensity0_2);
        xglLightDirection(pLight, i, sIntensity1_3);
    }
}
