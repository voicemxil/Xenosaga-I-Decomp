#include "matching.h"

/* Core geometry/math helpers */

extern long long iRandSeed;

void xglRandSeedInit(void)
{
    iRandSeed = 0x12345678;
}

float I2F(int nValue)
{
    return (float)nValue;
}

extern float sqrtf(float);

/* Matched via the compound-assignment lever (r *= x; r -= c;) keeping
 * the poly accumulator in-place in $f0, plus a fresh temp for the final
 * s*r so the mul keeps s as first operand (two-address form otherwise
 * commutes it). */
/* asin(x) via sqrt(1-x) times a cubic minimax poly; the hardware sqrt.s
 * result is checked for NaN and falls back to the software sqrtf() */
float xglAsin(float x)
{
    float s, r, t;

    s = sqrtf(1.0f - x);
    r = x * -0.01872929931f;
    r += 0.07426100224f;
    r *= x;
    r -= 0.2121143937f;
    r *= x;
    r += 1.570728779f;
    t = s * r;
    r = 1.570796371f - t;
    return r;
}

/* MATCHES (107 words) under the proposed li.s fixer extension
 * (.scratch-xgl/fix_cc_asm_lis.py, A/B via XENO_FIX_CC_ASM) -- NOT yet
 * registered because tools/fix_cc_asm.py is shared. Levers that got here:
 * compound assignments (r *= t2; r -= c;) keep the poly accumulator
 * in-place (original's single-register $f9 shape) AND stop combine from
 * distributing (r*t2 + 1.0f)*t into r*t2*t + t; the quotient reuses y's
 * register (y = x / y); the empty-then octant shape `if (0<=x) {} else
 * octant=1;` reproduces the bc1fl polarity; the y==0 bit test goes
 * through a copied int temp (mfc1/move pair). The fixer supplies: li.s
 * low-half-zero synthesis (lui $at/mtc1 for 1.0f), the hazard nop after
 * that mtc1 before an unrelated FP compute, and the noreorder barrier
 * that keeps a li.s-expanded lwc1 out of a return delay slot. */
/* atan2 via minimax polynomial approximation of atan(t) on [0,1],
 * with octant classification + argument reduction */
float xglAtan2(float y, float x)
{
    float t2, r;
    int flip;
    int octant;
    int n;

    flip = 0;
    r = 0.002866225783f;
    octant = 0;

    if (y == 0.0f) {
        if (0.0f <= x) {
            return 0.0f;
        }
        n = *(int *)&y;
        if (n < 0) {
            return -3.141592741f;
        }
        return 3.141592741f;
    }
    if (x == 0.0f) {
        if (y < 0.0f) {
            return -1.570796371f;
        }
        return 1.570796371f;
    }

    if (0.0f <= y) {
        if (0.0f <= x) {
        } else {
            octant = 1;
        }
    } else {
        if (0.0f <= x) {
            octant = 2;
        } else {
            octant = 3;
        }
    }
    y = __builtin_fabsf(y);
    x = __builtin_fabsf(x);

    if (x < y) {
        y = x / y;
        flip = 1;
    } else {
        y = y / x;
    }

    t2 = y * y;
    r *= t2;
    r -= 0.01616573706f;
    r *= t2;
    r += 0.04290961474f;
    r *= t2;
    r -= 0.07528963685f;
    r *= t2;
    r += 0.1065626368f;
    r *= t2;
    r -= 0.1420889944f;
    r *= t2;
    r += 0.1999355108f;
    r *= t2;
    r -= 0.3333314657f;
    r *= t2;
    r += 1.0f;
    r *= y;
    if (flip != 0) {
        r = 1.570796371f - r;
    }

    if (octant == 1) {
        r = 3.141592741f - r;
    }
    if (octant == 2) {
        r = -r;
    }
    if (octant == 3) {
        r = r - 3.141592741f;
    }
    return r;
}

/* Bit-copy f through a GPR (mfc1/mtc1 round-trip) before the truncate;
 * every C pun shape either collapses to a plain trunc or spills to the
 * stack, so the bounce is spelled as single-instruction asm moves with
 * the original's register assignment pinned ($3 for the bits, $f0 for
 * the staging float). The truncate must be written "cvt.w.s" -- gas
 * rejects two-operand trunc.w.s, and on the EE cvt.w.s IS the
 * truncating encoding (funct 0x24, what objdump prints as trunc.w.s). */
int F2I(float f)
{
    PIN(int bits, "$3");
    PIN(float x, "$f0");
    int r;

    __asm__("mfc1 %0, %1" : "=r"(bits) : "f"(f));
    __asm__("mtc1 %1, %0" : "=f"(x) : "r"(bits));
    __asm__("cvt.w.s %0, %0" : "=f"(x) : "0"(x));
    __asm__("mfc1 %0, %1" : "=r"(r) : "f"(x));
    return r;
}

/* LCG step over the shared 64-bit seed; 15-bit result */
unsigned short xglSRand(void)
{
    iRandSeed = iRandSeed * 0x41c64e6dULL + 0x3039;
    return (unsigned long long) iRandSeed >> 0x10 & 0x7FFF;
}

/* --- Quadword/halfword block copies (hand-written asm in the original:
 * trapping addi, unfilled delay slots, $zero-first bne) --- */

void xglMemCopy64(void *pDst, const void *pSrc, int nCount)
{
    __asm__ __volatile__(".set noreorder\n"
        "1:\n"
        "lq $2, 0x0($5)\n sq $2, 0x0($4)\n"
        "lq $2, 0x10($5)\n sq $2, 0x10($4)\n"
        "lq $2, 0x20($5)\n sq $2, 0x20($4)\n"
        "lq $2, 0x30($5)\n sq $2, 0x30($4)\n"
        "addi $6, $6, -1\n"
        "addi $4, $4, 0x40\n"
        "addi $5, $5, 0x40\n"
        "bne $0, $6, 1b\n"
        "nop\n"
        ".set reorder" : : : "$2", "$4", "$5", "$6", "memory");
}

void xglMemCopy64b(void *pDst, const void *pSrc, int nCount)
{
    __asm__ __volatile__(".set noreorder\n"
        "1:\n"
        "lq $2, 0x0($5)\n sq $2, 0x0($4)\n"
        "lq $2, -0x10($5)\n sq $2, -0x10($4)\n"
        "lq $2, -0x20($5)\n sq $2, -0x20($4)\n"
        "lq $2, -0x30($5)\n sq $2, -0x30($4)\n"
        "addi $6, $6, -1\n"
        "addi $4, $4, -0x40\n"
        "addi $5, $5, -0x40\n"
        "bne $0, $6, 1b\n"
        "nop\n"
        ".set reorder" : : : "$2", "$4", "$5", "$6", "memory");
}

void xglMemCopy16(void *pDst, const void *pSrc, int nCount)
{
    __asm__ __volatile__(".set noreorder\n"
        "1:\n"
        "lq $2, 0x0($5)\n sq $2, 0x0($4)\n"
        "addi $6, $6, -1\n"
        "addi $4, $4, 0x10\n"
        "addi $5, $5, 0x10\n"
        "bne $0, $6, 1b\n"
        "nop\n"
        ".set reorder" : : : "$2", "$4", "$5", "$6", "memory");
}

void xglMemCopy16b(void *pDst, const void *pSrc, int nCount)
{
    __asm__ __volatile__(".set noreorder\n"
        "1:\n"
        "lq $2, 0x0($5)\n sq $2, 0x0($4)\n"
        "addi $6, $6, -1\n"
        "addi $4, $4, -0x10\n"
        "addi $5, $5, -0x10\n"
        "bne $0, $6, 1b\n"
        "nop\n"
        ".set reorder" : : : "$2", "$4", "$5", "$6", "memory");
}

void xglMemCopy8(void *pDst, const void *pSrc, int nCount)
{
    __asm__ __volatile__(".set noreorder\n"
        "1:\n"
        "ld $2, 0x0($5)\n sd $2, 0x0($4)\n"
        "addi $6, $6, -1\n"
        "addi $4, $4, 0x8\n"
        "addi $5, $5, 0x8\n"
        "bne $0, $6, 1b\n"
        "nop\n"
        ".set reorder" : : : "$2", "$4", "$5", "$6", "memory");
}

void xglMemCopy8b(void *pDst, const void *pSrc, int nCount)
{
    __asm__ __volatile__(".set noreorder\n"
        "1:\n"
        "ld $2, 0x0($5)\n sd $2, 0x0($4)\n"
        "addi $6, $6, -1\n"
        "addi $4, $4, -0x8\n"
        "addi $5, $5, -0x8\n"
        "bne $0, $6, 1b\n"
        "nop\n"
        ".set reorder" : : : "$2", "$4", "$5", "$6", "memory");
}

void xglMemCopy4(void *pDst, const void *pSrc, int nCount)
{
    __asm__ __volatile__(".set noreorder\n"
        "1:\n"
        "lw $2, 0x0($5)\n sw $2, 0x0($4)\n"
        "addi $6, $6, -1\n"
        "addi $4, $4, 0x4\n"
        "addi $5, $5, 0x4\n"
        "bne $0, $6, 1b\n"
        "nop\n"
        ".set reorder" : : : "$2", "$4", "$5", "$6", "memory");
}

void xglMemCopy4b(void *pDst, const void *pSrc, int nCount)
{
    __asm__ __volatile__(".set noreorder\n"
        "1:\n"
        "lw $2, 0x0($5)\n sw $2, 0x0($4)\n"
        "addi $6, $6, -1\n"
        "addi $4, $4, -0x4\n"
        "addi $5, $5, -0x4\n"
        "bne $0, $6, 1b\n"
        "nop\n"
        ".set reorder" : : : "$2", "$4", "$5", "$6", "memory");
}

void xglMemCopy2(void *pDst, const void *pSrc, int nCount)
{
    __asm__ __volatile__(".set noreorder\n"
        "1:\n"
        "lh $2, 0x0($5)\n sh $2, 0x0($4)\n"
        "addi $6, $6, -1\n"
        "addi $4, $4, 0x2\n"
        "addi $5, $5, 0x2\n"
        "bne $0, $6, 1b\n"
        "nop\n"
        ".set reorder" : : : "$2", "$4", "$5", "$6", "memory");
}

void xglMemCopy2b(void *pDst, const void *pSrc, int nCount)
{
    __asm__ __volatile__(".set noreorder\n"
        "1:\n"
        "lh $2, 0x0($5)\n sh $2, 0x0($4)\n"
        "addi $6, $6, -1\n"
        "addi $4, $4, -0x2\n"
        "addi $5, $5, -0x2\n"
        "bne $0, $6, 1b\n"
        "nop\n"
        ".set reorder" : : : "$2", "$4", "$5", "$6", "memory");
}

/* --- VU0 random-number generator --- */

/* Seed the VU0 R register from a float */
void xglFSrand(float fSeed)
{
    __asm__ __volatile__(".set noreorder\n"
        "mfc1 $2, %0\n"
        "qmtc2 $2, $vf1\n"
        "vrinit $R, $vf1x\n"
        ".set reorder" : : "f"(fSeed) : "$2");
}

/* Draw the next VU0 random float */
float xglFRand(void)
{
    int nRet;

    __asm__ __volatile__(".set noreorder\n"
        "vrnext.x $vf1x, $R\n"
        "qmfc2 %0, $vf1\n"
        ".set reorder" : "=r"(nRet));
    return *(float *)&nRet;
}

/* --- VU0 macro-mode sine/cosine --- */

extern unsigned int Vu0CallSin[];
extern unsigned int Vu0CallCos[];

float xglSin(float fAngle)
{
    int nRet;

    __asm__ __volatile__(".set noreorder\n"
        "ctc2.i %1, $vi27\n"
        "vnop\n"
        "qmtc2 %2, $vf4\n"
        "vcallmsr $vi27\n"
        "qmfc2.i %0, $vf1\n"
        ".set reorder"
        : "=r"(nRet)
        : "r"((unsigned int)Vu0CallSin >> 3), "r"(fAngle));
    return *(float *)&nRet;
}

float xglCos(float fAngle)
{
    int nRet;

    __asm__ __volatile__(".set noreorder\n"
        "ctc2.i %1, $vi27\n"
        "vnop\n"
        "qmtc2 %2, $vf4\n"
        "vcallmsr $vi27\n"
        "qmfc2.i %0, $vf1\n"
        ".set reorder"
        : "=r"(nRet)
        : "r"((unsigned int)Vu0CallCos >> 3), "r"(fAngle));
    return *(float *)&nRet;
}

/* --- 32-bit LCG on top of the shared 64-bit seed --- */

/* Two LCG steps; returns the concatenated middle bits of both */
int xglLRand(void)
{
    int a;
    unsigned long long s;

    a = iRandSeed * 0x41C64E6D + 12345;
    s = (unsigned int)(a * 0x41C64E6D + 12345);
    iRandSeed = s;
    return (a << 16) + (s >> 16);
}

/* --- Geometry engine bring-up --- */

extern unsigned int PacketDataVu0MicroCode[];
extern void xglDmaDirectSrcChain(unsigned int nCh, unsigned int nAddr);

/* TODO: near-miss (2d) blocked on a fixer flag. Writing the seed as a
 * plain "r"(0.1f) operand reproduces the original exactly (lwc1
 * scheduled above the addiu sp prologue, compiler-emitted mfc1) EXCEPT
 * that our gas inserts a hazard nop between the mfc1 and the asm
 * block's qmtc2; the original has none. Needs
 * FILE_FIX_FLAGS["xglMath.c"] = "--omit-hazard qmtc2". The variant
 * below (mfc1 inside the noreorder block) avoids the nop but then the
 * lwc1 loses its scheduling priority and lands below the prologue.
 * UPDATE (wave 3): --omit-hazard qmtc2 is registered but does NOT help --
 * the nop is inserted by gas itself, not the fixer. gas pads a
 * reorder-mode mfc1 before ANY dependent next insn (dependence-aware for
 * insns, unconditional flush before .word/data/labels), and it tracks
 * the hazard even across .set noreorder boundaries, so no template
 * spelling hides the qmtc2 read of $2. A ".set mips4" window around the
 * compiler mfc1 kills the hoisted lwc1 (the extra volatile asm is a
 * scheduling barrier) and did not suppress the pad. Chain-lengthening
 * empty-asm passthroughs ("" : "=f"(x) : "0"(0.1f)) hoist the lwc1 above
 * the sd $31 but perturb the tail regalloc (9 diffs). Would need a fixer
 * feature that pulls a compiler-emitted mfc1 into a following #APP
 * noreorder block (out of scope for shared tools). */
/* TODO: near-miss (2 words, SCHEDULING): the original's lwc1 of the 0.1f
 * seed sits ABOVE the `addiu sp,sp,-16` prologue word; no source form can
 * hoist body code above the textual prologue. Everything else matches
 * with the mfc1 staged inside the noreorder block (that removed the gas
 * mfc1->qmtc2 hazard nop the original does not have; was 12 diffs). */
/* Seed the VU0 R register, reset the LCG seed and upload the VU0
 * microcode overlay through a source-chain DMA */
/* VU0's random unit is seeded from a float constant.  Naming it keeps
 * gcc from emitting the `li.s` pseudo-op (whose gas mips1 wrapper hides
 * the load from the scheduler-fixup passes) and emits the original's
 * single gp-relative lwc1. */
static const float fRinitSeed = 0.1f;

/* The original's second scheduler issued this lwc1 ABOVE the prologue's
 * stack adjust; gcc 2.96 will not move a load past an $sp write, so the
 * pair is transposed by --swap-adjacent xglGeometryInit:0. */
void xglGeometryInit(void)
{
    __asm__ __volatile__(".set noreorder\n"
        "mfc1 $2, %0\n"
        "qmtc2 $2, $vf1\n"
        "vrinit $R, $vf1x\n"
        ".set reorder" : : "f"(fRinitSeed) : "$2");
    iRandSeed = 0x12345678;
    xglDmaDirectSrcChain(0, (unsigned int)PacketDataVu0MicroCode);
}
