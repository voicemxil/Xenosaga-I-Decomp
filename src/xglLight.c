/* Directional/parallel light rig used by the xgl engine's shading pipeline */

typedef int TI __attribute__((mode(TI)));

void xglVectorNormal(void *pDest, void *pSource);
extern unsigned int Vu0CallLightCalcMatrix[];

/* TODO: near-miss (2/4 words differ, SCHEDULING) - 2.96 schedules the sq
 * into the jr delay slot here; the original keeps sq before jr with a
 * genuine nop in the delay slot. Every natural single-statement form
 * gets the same reschedule. */
/* Copy a light's ambient color quadword verbatim */
void xglLightIntensityAmbient(void *pLight, void *pSrc)
{
    *(TI *)pLight = *(TI *)pSrc;
}

/* TODO: near-miss (6/9 words, REGISTER class) - original keeps the
 * computed address in $v0/$v1, this in-place-advances $a0; every
 * natural form tried reuses $a0. */
/* Set one of a light's up-to-3 parallel colors (no-op past slot 2) */
void xglLightIntensityParallel(void *pLight, unsigned int nNo, void *pSrc)
{
    char *pBase = (char *)pLight + nNo * 0x20;
    char *pDst = pBase + 0x10;

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
