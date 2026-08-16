#include "matching.h"

/* Battle scene effect/scheduler functions (sef* family, other than sefIs*) */

/* sefPrintVector/sefPrintMatrix: debug stubs, compiled out entirely */
void sefPrintVector(void) { }
void sefPrintMatrix(void) { }

extern int _sefLoadEftQue;

void sefSetLoadQue(int v) { _sefLoadEftQue = v; }
int *sefGetLoadQue(void) { return &_sefLoadEftQue; }

extern int _battleData[];
void *sefGetBattleData(void) { return _battleData; }

extern int _scheduler[];
void *sefGetScheduler(void) { return _scheduler; }

extern int _nowScheduler;
int sefGetNowScheduler(void) { return _nowScheduler; }

/* sefGetMatrixScale: tail call, args pass through unchanged */
extern float MMathCalcLength(void *v);
float sefGetMatrixScale(void *v) { return MMathCalcLength(v); }

extern void sefFreeSchedulerCf(void *p);
void sefDeleteEffect2(void *p) { sefFreeSchedulerCf(p); }
void sefDeleteEffectCf(void *p) { sefFreeSchedulerCf(p); }

/* sefGetSizeOffset: index into a static table (offset.2, rodata) */
extern int offset_2[] __asm__("offset.2");
int sefGetSizeOffset(int idx) { return offset_2[idx]; }

extern int sefCreateEffectCf2(int a, int b, int c, int d);
int sefCreateEffectCf(int a, int b, int c) { return sefCreateEffectCf2(a, b, c, -1); }

extern void sefCreateScheduler2(int a, int b, int c, int d, int e, int f);
/* Remaps a 5-arg call onto a 6-arg one: fixed 0 for arg5, caller's arg5
 * forwarded into arg6 */
void sefCreateScheduler(int a, int b, int c, int d, int e) {
    sefCreateScheduler2(a, b, c, d, 0, e);
}

extern int _battlePrm[];
extern void sresLoadBattleData(void *p);
void sefSetupEffect(void) { sresLoadBattleData(_battlePrm); }

typedef struct { char pad[0xA8C]; int flags; } SEF_SCHED_OBJ;
void sefRewindEffectCf(SEF_SCHED_OBJ *p) {
    if (p != 0) {
        p->flags |= 1;
    }
}

extern short D_0079411C[];
void *sefGetDmgNull(void) {
    return (D_0079411C[0] >= 2) ? (void *)0x20F : (void *)0x210;
}

extern short _hitFlag;

extern void srsFileLoadCf(void *p);
int sefLoadEffectCf(void *p) { srsFileLoadCf(p); return 0; }

extern void srsMemoryLoadCf(void *p);
int sefLoadMemoryEffectCf(void *p) { srsMemoryLoadCf(p); return 0; }

extern char _loadEsdData[];
/* Bounds-checked table lookup: out-of-range index returns NULL */
void *sefGetEffectName(int idx) {
    if ((unsigned int)idx >= 0xC00) return 0;
    return _loadEsdData + idx * 16;
}

typedef struct { char pad[0xA8C]; int flags; } SEF_SCHED2;
extern int _nowScheduler;
/* a0 is dead: only a1's high bit is examined */
void sefCheckFinish(int a0, int a1) {
    extern SEF_SCHED2 *_nowSchedulerPtr __asm__("_nowScheduler");
    if (a1 & 0x4000) {
        _nowSchedulerPtr->flags |= 0x40;
    }
}

extern int _lineData[];
void *sefGetLineAdr(int idx) {
    if (idx < 0) return 0;
    return (char *)_lineData + idx * 65 * 32;
}

extern int srsLeaveCdRead(void);
int sefCheckLoad(void) {
    if (_sefLoadEftQue != 0) {
        return 1;
    }
    return 0 < srsLeaveCdRead();
}

typedef struct { float x, y; } CUBEPOS;
extern void sefGetCubePosBtm(void *p);
void sefGetCubePosTop(CUBEPOS *p) {
    sefGetCubePosBtm(p);
    p->y = -p->y;
}

extern void scExecEffect(void);
extern void sdvExecAlters(void);
extern short _hitSignal;
extern short _seSignal;
void sefExecEffect(void) {
    scExecEffect();
    sdvExecAlters();
    _hitFlag = 0;
    _hitSignal = 0;
    _seSignal = 0;
}

extern void svFileLoadScript(int a, int b);
void sefExecLoadQue(void) {
    int v0 = _sefLoadEftQue;
    if (v0 > 0) {
        svFileLoadScript(0, v0);
    }
}

extern int _ptAlloc[];
void *sefGetParentLine(int a, int b) {
    if (a >= 0 && b >= 0) {
        return (char *)_ptAlloc + b * 0x280;
    }
    return 0;
}

extern void srsAnalyzeEftNo(void *p, int *a, int *b);
extern void sefSearchMapperIndex2(void *p, int a, int b);
void sefSearchMapperIndex(void *p) {
    int a, b;
    srsAnalyzeEftNo(p, &a, &b);
    sefSearchMapperIndex2(p, a, b);
}

extern int sefRandSeed;
extern float D_004D82D8;
/* Classic LCG: seed = seed*0x41C64E6D + 12345; keep bits 30..16 as a 0..0x7FFF
 * draw, scale by 1/32767. */
float sefRandf(void) {
    sefRandSeed = sefRandSeed * 0x41C64E6D + 12345;
    return (float)(int)(((unsigned int)sefRandSeed >> 16) & 0x7FFF) * D_004D82D8;
}

extern int sefCnvEtEffectNo(int a);
extern void svFileLoadScript(int a, int b);
void sefLoadEffect(int a0) {
    if (a0 > 0) {
        int v0 = sefCnvEtEffectNo(a0);
        svFileLoadScript(0, v0);
    }
}

extern void sefExecEffect(void);
void sefProgressEffect(int a0) {
    register int cnt __asm__("$16");
    if (a0 > 0) {
        cnt = a0;
        do {
            sefExecEffect();
        } while (--cnt != 0);
    }
}

typedef struct { char pad[0x10]; int nFlags; } SEF_GLS;
extern SEF_GLS GameLoopState;
extern short _initialize;
extern void svDrawScheduler2D(void);
void sefDrawEffect2D(void) {
    if (_initialize != 0 && (GameLoopState.nFlags & 0x04000000) == 0) {
        svDrawScheduler2D();
    }
}

extern void svDrawScheduler3D(void);
void sefDrawEffect3D(void) {
    if (_initialize != 0 && (GameLoopState.nFlags & 0x04000000) == 0) {
        svDrawScheduler3D();
    }
}

/* Load 8 clip-plane quadwords into vf12..vf19; left resident for the
 * VU0 macro-mode code in sefClipViewVolumeA to consume. Pure inline asm
 * -- no C expression models "load into a bank of persistent VU0 regs
 * and return without storing". */
void sefInitClipViewVolume(void *p)
{
    __asm__ __volatile__(".set noreorder\n"
        "lqc2 $vf12, 0x0(%0)\n"
        "lqc2 $vf13, 0x10(%0)\n"
        "lqc2 $vf14, 0x20(%0)\n"
        "lqc2 $vf15, 0x30(%0)\n"
        "lqc2 $vf16, 0x40(%0)\n"
        "lqc2 $vf17, 0x50(%0)\n"
        "lqc2 $vf18, 0x60(%0)\n"
        "lqc2 $vf19, 0x70(%0)\n"
        ".set reorder" : : "r"(p));
}

extern void sefClipViewVolumeA(void *p);
void sefClipViewVolume(void *a, void *b) {
    sefInitClipViewVolume((char *)b + 1264);
    sefClipViewVolumeA(a);
}

/* Copy 3 position/orientation quadwords from pA and one quadword from
 * pB into pDst -- register-to-register lq/sq via t0-t3, inline asm. */
void sefMergeMatrixPos(void *pDst, void *pA, void *pB)
{
    __asm__ __volatile__(".set noreorder\n"
        "lq $8, 0x0(%1)\n"
        "lq $9, 0x10(%1)\n"
        "lq $10, 0x20(%1)\n"
        "lq $11, 0x0(%2)\n"
        "sq $8, 0x0(%0)\n"
        "sq $9, 0x10(%0)\n"
        "sq $10, 0x20(%0)\n"
        "sq $11, 0x30(%0)\n"
        ".set reorder" : : "r"(pDst), "r"(pA), "r"(pB) : "$8", "$9", "$10", "$11", "memory");
}

/* Scale from degrees to radians via the VU0 x-lane broadcast multiply
 * -- void, unlike MMathDeg2RadVector in src/game/MMath.c (same "mfc1
 * into a pinned $f8 constant, qmtc2.ni" idiom applied to this exact
 * literal, but this one doesn't hand pDst back to the caller). */
typedef struct { float x, y, z; } SEF_VEC3;
void sefDeg2RadVector(SEF_VEC3 *pDst, SEF_VEC3 *pSrc)
{
    /* Split across two asm statements (with the $f8 constant's own
     * initializer sandwiched between them) so the lqc2 lands before the
     * literal's lwc1 -- the original schedules the vector load first
     * and only then materializes the degrees-to-radians constant. */
    __asm__ __volatile__(".set noreorder\n"
        "lqc2 $vf2, 0x0(%0)\n"
        ".set reorder" : : "r"(pSrc));

    {
        register float k __asm__("$f8") = 0.01745329238f;
        __asm__ __volatile__(".set noreorder\n"
            "mfc1 $t0, %1\n qmtc2.ni $t0, $vf1\n"
            "vmulx.xyz $vf2, $vf2, $vf1x\n"
            "sqc2 $vf2, 0x0(%0)\n"
            ".set reorder" : : "r"(pDst), "f"(k));
    }
}

/* Transform a direction vector by the current (already-resident) view
 * basis in vf3/vf4/vf7/vf8: out = row7*x + row8*y + row3*z + row4*w,
 * after negating the source z. Persistent-register VU0 macro code --
 * the basis is set up by an earlier call and simply still live here. */
void sefGetDirVector(void *pDst, void *pSrc)
{
    __asm__ __volatile__(".set noreorder\n"
        "lqc2 $vf1, 0x0(%1)\n"
        "vsub.z $vf1z, $vf0z, $vf1z\n"
        "vmulax.xyzw $ACC, $vf7, $vf1x\n"
        "vmadday.xyzw $ACC, $vf8, $vf1y\n"
        "vmaddaz.xyzw $ACC, $vf3, $vf1z\n"
        "vmaddw.xyzw $vf1, $vf4, $vf1w\n"
        "sqc2 $vf1, 0x0(%0)\n"
        ".set reorder" : : "r"(pDst), "r"(pSrc) : "memory");
}

/* Sign-extend a packed short[3] vector (unaligned ldl/ldr doubleword
 * load) into a quadword of s32 lanes via the pcgth/pextlh MMI idiom,
 * then store. No C expression reproduces the ldl/ldr macro shape here
 * (unlike the aligned lq/sq case) -- inline asm with $9/$10. */
void sefLerpIVectorA(void *pSrc, void *pDst)
{
    __asm__ __volatile__(".set noreorder\n"
        "ldl $9, 0x7(%0)\n"
        "ldr $9, 0x0(%0)\n"
        "pcgth $10, $0, $9\n"
        "pextlh $10, $10, $9\n"
        "sq $10, 0x0(%1)\n"
        ".set reorder" : : "r"(pSrc), "r"(pDst) : "$9", "$10", "memory");
}

/* out = pAdd + (float)pSrc * fScale, on the VU0 x-lane; fScale enters
 * in $f12 and the "r"(fScale) operand forces the compiler to emit the
 * same mfc1 v0,$f12 the original used to stage it into a GPR before
 * qmtc2. */
void sefScaleIVectorAdd(void *pDst, void *pSrc, void *pAdd, float fScale)
{
    __asm__ __volatile__(".set noreorder\n"
        "lqc2 $vf1, 0x0(%1)\n"
        "lqc2 $vf3, 0x0(%2)\n"
        "vitof0.xyzw $vf1, $vf1\n"
        "qmtc2 %3, $vf2\n"
        "vmulx.xyz $vf1, $vf1, $vf2x\n"
        "vadd.xyz $vf3, $vf1, $vf3\n"
        "sqc2 $vf3, 0x0(%0)\n"
        ".set reorder" : : "r"(pDst), "r"(pSrc), "r"(pAdd), "r"(fScale) : "memory");
}

/* Lerp between two adjacent packed short[3] keyframe records (10 bytes
 * apart) by fScale and store the result as a float vector. Same
 * unaligned ldl/ldr + pcgth/pextlh sign-extend idiom as sefLerpVectorA,
 * done twice; dsrl skips the leading 16-bit time field of each record.
 * Pure VU0 macro-mode hardware asm. */
void sefLerpVectorB(void *pSrc, void *pDst, float fScale)
{
    __asm__ __volatile__(".set noreorder\n"
        "addiu $2, %0, 10\n"
        "mfc1 $9, %2\n"
        "qmtc2 $9, $vf3\n"
        "ldl $8, 0x7(%0)\n"
        "ldr $8, 0x0(%0)\n"
        "ldl $9, 0x7($2)\n"
        "ldr $9, 0x0($2)\n"
        "dsrl $8, $8, 0x10\n"
        "dsrl $9, $9, 0x10\n"
        "pcgth $10, $0, $8\n"
        "pcgth $11, $0, $9\n"
        "pextlh $10, $10, $8\n"
        "pextlh $11, $11, $9\n"
        "qmtc2 $10, $vf1\n"
        "qmtc2 $11, $vf2\n"
        "vitof0.xyzw $vf1, $vf1\n"
        "vitof0.xyzw $vf2, $vf2\n"
        "vsub.xyz $vf2, $vf2, $vf1\n"
        "vmulx.xyz $vf2, $vf2, $vf3x\n"
        "vadd.xyz $vf2, $vf2, $vf1\n"
        "sqc2 $vf2, 0x0(%1)\n"
        ".set reorder" : : "r"(pSrc), "r"(pDst), "f"(fScale)
        : "$2", "$8", "$9", "$10", "$11", "memory");
}

/* Integer-result twin of sefLerpVectorB: no dsrl (the records here start
 * at the value, not a time field) and a vftoi0 on the way out. Pure VU0
 * macro-mode hardware asm. */
void sefLerpIVectorB(void *pSrc, void *pDst, float fScale)
{
    __asm__ __volatile__(".set noreorder\n"
        "addiu $2, %0, 10\n"
        "ldl $8, 0x7(%0)\n"
        "ldr $8, 0x0(%0)\n"
        "ldl $9, 0x7($2)\n"
        "ldr $9, 0x0($2)\n"
        "mfc1 $10, %2\n"
        "qmtc2 $10, $vf3\n"
        "pcgth $10, $0, $8\n"
        "pcgth $11, $0, $9\n"
        "pextlh $10, $10, $8\n"
        "pextlh $11, $11, $9\n"
        "qmtc2 $10, $vf1\n"
        "qmtc2 $11, $vf2\n"
        "vitof0.xyzw $vf1, $vf1\n"
        "vitof0.xyzw $vf2, $vf2\n"
        "vsub.xyz $vf2, $vf2, $vf1\n"
        "vmulx.xyz $vf2, $vf2, $vf3x\n"
        "vadd.xyz $vf2, $vf2, $vf1\n"
        "vftoi0.xyzw $vf2, $vf2\n"
        "sqc2 $vf2, 0x0(%1)\n"
        ".set reorder" : : "r"(pSrc), "r"(pDst), "f"(fScale)
        : "$2", "$8", "$9", "$10", "$11", "memory");
}

/* --- sefExecLineData: progress one "line" scheduler object one frame --- */
typedef struct {
    char pad000[0x1C0];
    unsigned short f1C0;
    char pad1C2[0x1D0 - 0x1C2];
    void *f1D0;
    void *f1D4;
    void *f1D8;
    void *f1DC;
    char pad1E0[0x1E8 - 0x1E0];
    void *f1E8;
    void *f1EC;
    char pad1F0[0x1F4 - 0x1F0];
    void *f1F4;
    char pad1F8[0x5F8 - 0x1F8];
    float f5F8;
    float f5FC;
    char pad600[0x802 - 0x600];
    short f802;
    short f804;
    unsigned short f806;
    char pad808[0x80A - 0x808];
    unsigned short f80A;
    unsigned short f80C;
    char pad80E[0x812 - 0x80E];
    unsigned short nUsed;   /* 0x812 -- slot-in-use flag */
    unsigned short f814;
    short f816;
    char pad818[0x820 - 0x818];
} SEF_LINE;

extern float _gravity;
extern float _height;
extern float _colision;
/* Sign-extend a packed short[3] source (same ldl/ldr+pcgth/pextlh idiom
 * as sefLerpIVectorA) into a float vector via vitof0, skipping the
 * first 16 bits of the unaligned doubleword with dsrl -- inline asm. */
void sefLerpVectorA(void *pSrc, void *pDst)
{
    __asm__ __volatile__(".set noreorder\n"
        "ldl $9, 0x7(%0)\n"
        "ldr $9, 0x0(%0)\n"
        "dsrl $9, $9, 0x10\n"
        "vmove.w $vf2w, $vf0w\n"
        "pcgth $10, $0, $9\n"
        "pextlh $10, $10, $9\n"
        "qmtc2 $10, $vf1\n"
        "vitof0.xyz $vf2, $vf1\n"
        "sqc2 $vf2, 0x0(%1)\n"
        ".set reorder" : : "r"(pSrc), "r"(pDst) : "$9", "$10", "memory");
}

/* out = pA - pB (a stack-temp quadword), then forward to
 * sefGetDirMatrix(pObj, &out); pObj passes through untouched. */
typedef struct { float x, y, z, w; } SEF_VEC4;
extern void sefGetDirMatrix(void *pObj, void *pDir);
void sefGetVecMatrix(void *pObj, SEF_VEC4 *pA, SEF_VEC4 *pB)
{
    SEF_VEC4 diff;
    __asm__ __volatile__(".set noreorder\n"
        "lqc2 $vf1, 0x0(%0)\n"
        "lqc2 $vf2, 0x0(%1)\n"
        "vsub.xyz $vf1, $vf1, $vf2\n"
        "sqc2 $vf1, 0x0(%2)\n"
        ".set reorder" : : "r"(pA), "r"(pB), "r"(&diff) : "memory");
    sefGetDirMatrix(pObj, &diff);
}

extern void sefLerpVectorSC(void *pTable, void *pState)
{
    if (sefLerpVector(pTable, pState) != 0) {
        float k = 0.1f;
        char *pDst = (char *)pState + 16;

        __asm__ __volatile__(".set noreorder\n"
            "lqc2 $vf1, 0x0(%0)\n"
            "mfc1 $t0, %1\n"
            "qmtc2 $t0, $vf2\n"
            "vmulx.xyz $vf1, $vf1, $vf2x\n"
            "sqc2 $vf1, 0x0(%0)\n"
            ".set reorder" : : "r"(pDst), "f"(k) : "$8", "memory");
    }
}
