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
/* File scope, not block scope: gcc 2.9x keeps a block-scope extern's
 * name on the function obstack and frees it, so the .extern it emits
 * under -G8 later prints garbage and gas rejects the file. */
extern SEF_SCHED2 *_nowSchedulerPtr __asm__("_nowScheduler");
/* a0 is dead: only a1's high bit is examined */
void sefCheckFinish(int a0, int a1) {
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
/* sefGetCubePosBtm really takes two arguments (see its definition
 * below); this caller passes only the destination and leaves $a1 as it
 * found it, so call it through a one-argument pointer type rather than
 * declaring a conflicting prototype. */
extern void sefGetCubePosBtm(void *pDst, void *pSize);
void sefGetCubePosTop(CUBEPOS *p) {
    ((void (*)(void *)) sefGetCubePosBtm)(p);
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

extern unsigned int sefRandSeed;
extern float D_004D82D8;
/* Classic LCG: seed = seed*0x41C64E6D + 12345; keep bits 30..16 as a 0..0x7FFF
 * draw, scale by 1/32767. */
float sefRandf(void) {
    sefRandSeed = sefRandSeed * 0x41C64E6D + 12345;
    return (float)(int)(((unsigned int)sefRandSeed >> 16) & 0x7FFF) * D_004D82D8;
}

/* sefCnvEtEffectNo takes two arguments; the original passes only one
 * here, so this call site goes through a one-argument view of it. */
extern int sefCnvEtEffectNo1(int a) __asm__("sefCnvEtEffectNo");
extern void svFileLoadScript(int a, int b);
void sefLoadEffect(int a0) {
    if (a0 > 0) {
        int v0 = sefCnvEtEffectNo1(a0);
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

extern void sefLerpVectorSC(void *a, void *b);
/* Defined below with its real types; unprototyped here so the callers
 * in this file keep passing raw pointers. */
extern void sefLerpIVector();
extern void sefProgressInt(void *a, void *b);
/* c is dead: only b's null-ness is examined. The call is unconditional --
 * the null test only picks the table, it does not skip the lookup -- which
 * is what leaves the jal delay slot empty and the %hi/%lo pair split across
 * v0/a1. */
extern void sefGetPoint(void *p, void *table);
extern float _zeroPos_004CBF00[];
void sefGetPosition(void *a, void *b, int c)
{
    if (b == 0) {
        b = _zeroPos_004CBF00;
    }
    sefGetPoint(a, b);
    *(float *)((char *)a + 12) = 1.0f;
}

extern int sefExecLineGlobalData(void *a, int b);
extern int sefExecLineLocalData(void *a, int b);

int sefExecLineData(SEF_LINE *p, int mode)
{
    unsigned short mode2 = p->f806;
    short v0;
    float scale;
    void *p1D4;
    int local;
    p->f802 = -1;

    if (p->f80A != 0) {
        int val = (unsigned short)p->f804 - 1;
        p->f804 = val;
        if ((val << 16) >= 0) {
            /* The countdown did not underflow, so the clamp below is
             * skipped outright -- the original's bgez jumps past it. */
            goto no_clamp;
        }
        if (p->f80C == 0) {
            p->f804 = 0;
            return 0;
        }
        if (p->f814 == 0) {
            p->f804 = 0;
            return 0;
        }
    }
    if (p->f804 < 0) {
        p->f804 = -1;
    }
no_clamp:
    v0 = p->f816;

    scale = (float)v0 * 0.01f;
    _gravity = p->f5F8;
    _height = p->f5FC;
    _colision = scale;
    sefLerpVectorSC((char *)p->f1D0 + 8, (char *)p + 0xE0);

    sefLerpVectorSC((char *)p->f1D8 + 8, (char *)p + 0x100);

    p1D4 = p->f1F4;
    if (p1D4 != 0) {
        sefLerpVectorSC((char *)p1D4 + 8, (char *)p + 0x120);
        sefGetPosition((char *)p + 0xB0, (char *)p + 0x130, *(short *)p->f1F4);
    }
    p1D4 = p->f1D4;
    sefLerpIVector((char *)p1D4 + 4, (char *)p + 0x160);

    sefLerpIVector((char *)p->f1DC + 4, (char *)p + 0x180);

    sefProgressInt(p->f1E8, (char *)p + 0x1AC);
    sefProgressInt(p->f1EC, (char *)p + 0x1B8);

    if (p->f804 >= 0) {
        if (sefExecLineGlobalData(p, mode2) == 0) {
            return 1;
        }
    }
    local = sefExecLineLocalData(p, mode2);

    {
        register int iw __asm__("$3");
        iw = p->f806;
        p->f814 = local;
        iw = iw + 1;
        p->f806 = iw;
        if ((unsigned short)iw == 0x3FF) {
            if (p->f80A != 0) {
                p->f806 = 0x3FE;
            } else {
                register int diff __asm__("$2") = iw - p->f1C0;
                p->f806 = diff;
                if ((unsigned short)diff == 0) {
                    p->f806 = 1;
                }
            }
        }
    }
    return 1;
}

/* Raise the hit-effect signal flag */
void sefHitEffect(void)
{
    _hitFlag = 1;
}

/* Release a "local data" slot's particle-allocator handle, if idx is in
 * range. */
extern void sevFreePtAllocator(int handle);
/* The byte offset is materialised in an unsigned int first: written as
 * `base + (idx * 2 + 1536)` gcc reassociates to `(base + idx*2) + 1536`
 * so it can fold the constant into the address, clobbering a0. Widening
 * the offset to unsigned blocks that fold and leaves the original's
 * `addiu v0,v0,1536` / `addu s0,a0,v0` pair. */
void sefFreeLocalData(void *base, int idx)
{
    short *p;
    unsigned int off;

    if ((unsigned int)idx < 256) {
        off = (unsigned int)(idx * 2 + 1536);
        p = (short *)((char *)base + off);
        sevFreePtAllocator(*p);
        *p = -1;
    }
}

/* Re-init an effect Cf slot's file-load state and register its 2D-draw
 * callback into the shared render struct (owned by xgl -- sRender);
 * offsets 0x2C/0x38 only, referenced by address, not by xgl's real
 * struct type */
extern short _sefBattleMode;
extern void sefDestroyEffectCf(void);
extern void sresLoadCfMemory(void);
typedef struct {
    char pad[0x2C];
    int cb1;                 /* 0x2C */
    char pad2[0x38 - 0x2C - 4];
    void (*cb2)(void);       /* 0x38 */
} SEF_RENDER_CB;
extern SEF_RENDER_CB sRender;
void sefInitEffectCf(void)
{
    _sefBattleMode = 0;
    sefDestroyEffectCf();
    sresLoadCfMemory();
    sRender.cb1 = 0;
    sRender.cb2 = sefDrawEffect2D;
}

/* Look an effect name up by ID and kick off its Cf load if found; id is
 * forwarded into srsFileLoadCf's second argument even though that
 * function (see sefLoadEffectCf's single-arg call, already matched)
 * ignores it on this path -- call through a locally cast pointer type
 * rather than re-declaring srsFileLoadCf with a conflicting prototype */
extern int srsEffectNameToID(void *name);
int sefLoadEffectCfName(void *p, void *name)
{
    int id = srsEffectNameToID(name);
    if (id > 0) {
        ((void (*)(void *, int))srsFileLoadCf)(p, id);
    }
    return 0;
}

/* --- line-data slot allocator (128 slots of 0x820 bytes) --- */

/* _lineData is declared above as int[] for sefGetLineAdr's byte
 * arithmetic; the same storage typed as the record array. */
extern SEF_LINE _lineTbl[] __asm__("_lineData");

int sefAllocLineData(void)
{
    int i;

    for (i = 0; i < 128; i++) {
        if (_lineTbl[i].nUsed == 0) {
            _lineTbl[i].nUsed = 1;
            return i;
        }
    }
    return -1;
}

/* Release the line-data handle stored in an effect-data byte slot */
extern void sefFreeLineData(int nLine);
void sefFreeEffectData(void *pBase, int nOfs)
{
    signed char *p;

    if ((unsigned int)nOfs < 32) {
        p = (signed char *)pBase + nOfs;
        if (*p >= 0) {
            sefFreeLineData(*p);
            *p = -1;
        }
    }
}

/* --- inverse-view matrix --- */

extern void *xglStudioGetActiveCamera(void);
extern void MMathRotateMatrixYXZ(void *pDst, void *pSrc, void *pRot);
extern float _invView[16];

/* Build the identity matrix straight out of vf0 by rotating the (0,0,0,1)
 * constant register, then apply the active camera's rotation. No C
 * expression names vf0's constant contents, so the identity fill is real
 * hardware asm. */
void sefCalcInvView(void *pMtx)
{
    void *pCam;

    pCam = xglStudioGetActiveCamera();
    __asm__ __volatile__(".set noreorder\n"
        "vmr32.xyzw $vf1, $vf0\n"
        "vmr32.xyzw $vf2, $vf1\n"
        "vmr32.xyzw $vf3, $vf2\n"
        "sqc2 $vf0, 0x30(%0)\n"
        "sqc2 $vf1, 0x20(%0)\n"
        "sqc2 $vf2, 0x10(%0)\n"
        "sqc2 $vf3, 0x0(%0)\n"
        ".set reorder\n"
        : : "r"(pMtx) : "memory");
    MMathRotateMatrixYXZ(pMtx, pMtx, (char *)pCam + 160);
}

extern void svDrawScheduler(void);
void sefDrawEffect(void)
{
    if (_initialize != 0 && (GameLoopState.nFlags & 0x04000000) == 0) {
        sefCalcInvView(_invView);
        svDrawScheduler();
    }
}

/* --- start/target world positions: an offset-range roll added onto the
 * caller's vector. The 3-component add is VU0 macro mode. --- */

extern void sefGetOfsRange(void *pDst, void *pPrm, int nRange);

void sefGetStartPos(void *pPos, SEF_LINE *p)
{
    void *pOfs;

    pOfs = (char *)p + 0x30;
    sefGetOfsRange(pPos, (char *)p + 0x170, *(short *)p->f1D4);
    __asm__ __volatile__(".set noreorder\n"
        "lqc2 $vf1, 0x0(%0)\n"
        "lqc2 $vf2, 0x0(%1)\n"
        "vadd.xyz $vf1, $vf1, $vf2\n"
        "sqc2 $vf1, 0x0(%1)\n"
        ".set reorder" : : "r"(pOfs), "r"(pPos) : "memory");
}

void sefGetTargetPos(void *pPos, SEF_LINE *p)
{
    void *pOfs;

    pOfs = (char *)p + 0x50;
    sefGetOfsRange(pPos, (char *)p + 0x190, *(short *)p->f1DC);
    __asm__ __volatile__(".set noreorder\n"
        "lqc2 $vf1, 0x0(%0)\n"
        "lqc2 $vf2, 0x0(%1)\n"
        "vadd.xyz $vf1, $vf1, $vf2\n"
        "sqc2 $vf1, 0x0(%1)\n"
        ".set reorder" : : "r"(pOfs), "r"(pPos) : "memory");
}

/* Name-keyed variant of sefLoadMemoryEffectCf; srsMemoryLoadCf really
 * takes three arguments (see sefLoadEffectCfName for the same call
 * through a locally cast pointer). */
int sefLoadMemoryEffectCfName(void *p, void *pName, int nArg)
{
    int nID = srsEffectNameToID(pName);
    if (nID > 0) {
        ((void (*)(void *, int, int))srsMemoryLoadCf)(p, nID, nArg);
    }
    return 0;
}

/* Free every line handle an effect-data block owns and clear its
 * in-use count */
typedef struct { char pad[0x20]; int nUsed; } SEF_EFFDATA;
void sefDestroyEffectData(SEF_EFFDATA *p)
{
    int i;

    if (p->nUsed != 0) {
        for (i = 0; i < 32; i++) {
            sefFreeEffectData(p, i);
        }
    }
    p->nUsed = 0;
}

/* Release every allocated particle handle a line's local-data block
 * owns (256 short slots at +0x600, -1 meaning "free") */
void sefDestroyLocalData(void *pBase)
{
    short *p;
    int i;

    p = (short *)((char *)pBase + 0x600);
    for (i = 0; i < 256; i++) {
        if (*p >= 0) {
            sefFreeLocalData(pBase, i);
        }
        p++;
    }
}

/* Reset an effect-data block: 32 line slots to -1, then the header */
extern void *memset(void *, int, unsigned int);
typedef struct {
    char pad000[0x20];
    int nUsed;        /* 0x20 */
    short f24;        /* 0x24 */
    short f26;        /* 0x26 */
    short f28;        /* 0x28 */
    short pad2A;
    short f2C;        /* 0x2C */
} SEF_EFFDATA2;
void sefInitEffectData(SEF_EFFDATA2 *p, int nUsed, int f26, int f24)
{
    memset(p, -1, 32);
    p->nUsed = nUsed;
    p->f26 = f26;
    p->f24 = f24;
    p->f2C = 0;
    p->f28 = 0;
}

/* --- battle actor table (24 x 144 bytes, live count at +0xD90) --- */

typedef struct {
    char pad000[0x80];
    int *pLight;         /* 0x80 -- also the actor handle sefIs* matches on */
    short nDead;         /* 0x84 -- nonzero: skipped by sefCaclAllTarget */
    short nLightFlag;    /* 0x86 */
    unsigned short nHitSignal;  /* 0x88 */
    short pad08A;
    unsigned short nSeSignal;   /* 0x8C */
    short pad08E;
} SEF_ACTOR;

typedef struct {
    SEF_ACTOR aActor[24];
    char padD80[0xD90 - 0xD80];
    int nActors;         /* 0xD90 */
} SEF_ACTOR_TBL;

extern SEF_ACTOR_TBL _battleActor;


/* --- scheduler slot table (128 x 0xAB0) --- */

/* The walk addresses a header at +0xA90 inside each 0xAB0 slot through
 * its own pointer: the original keeps `addiu +2704` separate from the
 * `lh 2(...)`, which only happens when the sub-object's address is taken
 * rather than the whole offset folded into the load. */
typedef struct {
    short f00;
    short nScript;    /* +0x02 (slot +0xA92) */
} SEF_SCHED_HDR;

extern void sefFreeScheduler(int nIdx);

void sefDestroyScriptScheduler(int nScript)
{
    SEF_SCHED_HDR *p;
    char *pBase;
    int i;

    /* LAUNDER keeps &_scheduler opaque: folded, gcc emits the symbol's
     * %lo and the +0xA90 as one addiu instead of the original's two. */
    pBase = (char *)_scheduler;
    LAUNDER(pBase);
    p = (SEF_SCHED_HDR *)(pBase + 0xA90);
    for (i = 0; i < 128; i++) {
        if (p->nScript == nScript) {
            sefFreeScheduler(i);
        }
        p = (SEF_SCHED_HDR *)((char *)p + 0xAB0);
    }
}

/* Build dst = scale(rotate(translate)) from a rotation vector, a
 * translation vector and a scale vector. The identity fill and the
 * translation row come out of vf0 -- hardware asm. */
extern void MMathRotateMatrixXYZ(void *pDst, void *pSrc, void *pRot);
extern void MMathScaleMatrix(void *pDst, void *pSrc, void *pScale);
void sefCalcRotTransSMatrix(void *pRot, void *pTrans, void *pScale, void *pDst)
{
    __asm__ __volatile__(".set noreorder\n"
        "lqc2 $vf1, 0x0(%1)\n"
        "vmove.w $vf1, $vf0\n"
        "vmr32.xyzw $vf2, $vf0\n"
        "vmr32.xyzw $vf3, $vf2\n"
        "vmr32.xyzw $vf4, $vf3\n"
        "sqc2 $vf1, 0x30(%0)\n"
        "sqc2 $vf2, 0x20(%0)\n"
        "sqc2 $vf3, 0x10(%0)\n"
        "sqc2 $vf4, 0x0(%0)\n"
        ".set reorder" : : "r"(pDst), "r"(pTrans) : "memory");
    MMathRotateMatrixXYZ(pDst, pDst, pRot);
    MMathScaleMatrix(pDst, pDst, pScale);
}

/* Set the "reverse Z" flag when an effect number is one of ten known
 * reversed effects and the type is in [24,32) */
extern short _revDirZ;
extern int revEft_3[] __asm__("revEft.3");
int sefSetReverseDir(int nEftNo, int nType)
{
    unsigned int i;   /* the loop bound test is sltiu, not slti */
    int nKey;

    if ((unsigned int)(nType - 24) < 8) {
        nKey = nEftNo;
        if ((unsigned int)(nEftNo - 2900) < 99) {
            nKey = nEftNo - 100;
        }
        for (i = 0; i < 10; i++) {
            if (nKey == revEft_3[i]) {
                _revDirZ = 1;
                return 1;
            }
        }
    }
    _revDirZ = 0;
    return 0;
}

/* --- sefLerpVectorSC: run sefLerpVector, then scale the float vector it
 * leaves 16 bytes into the state block by 0.1 on the VU0 x-lane. The
 * scale reaches VU0 through the "mfc1 into a GPR, then qmtc2" staging
 * idiom, so the literal has to be a real float operand. Declaring both
 * the constant and the destination pointer INSIDE the if-body is what
 * lets gcc fill the call's and the branch's delay slots the way the
 * original does. --- */
extern int sefLerpVectorRet(void *pTable, void *pState) __asm__("sefLerpVector");
void sefLerpVectorSC(void *pTable, void *pState)
{
    if (sefLerpVectorRet(pTable, pState) != 0) {
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

/* --- sefDestroyEffect / sefDestroyEffectCf: tear the whole effect
 * system down. Identical apart from the reloader-memory argument (1 vs
 * 0). The three globals are wiped with memset, and the last teardown
 * step is a tail call. --- */
extern void sefKillEffect(int nID);
extern void sdvInitAmbient(void);
extern void scDestroyScriptAll(void);
extern void sresFreeReloaderMemory(int nWhich);
extern void sdvDestroyAlters(void);
extern void *memset(void *p, int c, unsigned int n);

void sefDestroyEffect(void)
{
    _sefLoadEftQue = 0;
    sefKillEffect(-1);
    sdvInitAmbient();
    scDestroyScriptAll();
    sresFreeReloaderMemory(1);
    memset(_battlePrm, 0, 52);
    memset(_battleData, 0, 560);
    memset(&_battleActor, 0, 3488);
    sdvDestroyAlters();
}

void sefDestroyEffectCf(void)
{
    _sefLoadEftQue = 0;
    sefKillEffect(-1);
    sdvInitAmbient();
    scDestroyScriptAll();
    sresFreeReloaderMemory(0);
    memset(_battlePrm, 0, 52);
    memset(_battleData, 0, 560);
    memset(&_battleActor, 0, 3488);
    sdvDestroyAlters();
}

/* --- sefKillEffect: free every live scheduler slot whose actor id
 * matches, or all of them when nID is negative. The walker anchors on
 * &e->nActorID (the first address the body forms), which is why the
 * base pointer starts at _scheduler+0xA70 and the "used" flag is read
 * at a negative offset. --- */
typedef struct
{
    char pad000[0xB0];
    char aEffect[32][48];    /* 0x0B0 -- 32 effect-data records */
    int  nUsed;              /* 0x6B0 */
    char pad6B4[0x6E0 - 0x6B4];
    int  nSoundHandle;       /* 0x6E0 */
    char pad6E4[0xA74 - 0x6E4];
    short nEtParam;          /* 0xA74 */
    short padA76;
    short nActorID;          /* 0xA78 */
    char padA7A[0xA8C - 0xA7A];
    int  nFlags;             /* 0xA8C */
    short nState;            /* 0xA90 */
    short nScriptA;          /* 0xA92 */
    short nScriptB;          /* 0xA94 */
    char padA96[0xAB0 - 0xA96];
} SEF_SCHED_SLOT;

extern SEF_SCHED_SLOT _schedSlots[] __asm__("_scheduler");
extern void sefFreeScheduler(int nSlot);

void sefKillEffect(int nID)
{
    int i;

    for (i = 0; i < 128; i++) {
        if (_schedSlots[i].nUsed != 0) {
            if (nID < 0 || _schedSlots[i].nActorID == nID) {
                sefFreeScheduler(i);
            }
        }
    }
}

/* --- sefDestroyScriptScheduler2: free every scheduler slot tagged with
 * this (script, entry) pair. Same walker as sefKillEffect but with no
 * "in use" test -- the tag comparison is the whole filter. --- */
void sefDestroyScriptScheduler2(int nScript, int nEntry)
{
    int i;

    for (i = 0; i < 128; i++) {
        if (_schedSlots[i].nScriptA == nScript && _schedSlots[i].nScriptB == nEntry) {
            sefFreeScheduler(i);
        }
    }
}

/* --- sefGetNullPosition / sefGetWeaponPosition: fetch the world matrix
 * translation of an actor's accessory joint. Same body; the null variant
 * biases the joint index by 8 and allows 16 of them, the weapon variant
 * passes the index through and allows 8. The accessory table pointer is
 * read in the bltz delay slot, i.e. unconditionally, so it has to be
 * loaded before the "no such joint" test in the C too. The quadword copy
 * out of the matrix is an lq/sq pair -- hardware asm. --- */
extern int ACT_jointGetAccessories(void *pAct, int nJoint);

void sefGetNullPosition(SEF_VEC4 *pDst, char *pAct, int nIdx)
{
    if (pAct != 0 && (unsigned int)nIdx < 16) {
        int n = ACT_jointGetAccessories(pAct, nIdx + 8);
        char *pTbl = *(char **)(pAct + 2084);

        if (n < 0 || pTbl == 0) {
            pDst->y = 20.0f;
        } else {
            char *pMtx = (char *)((n << 6) + (int)pTbl) + 48;
            __asm__ __volatile__(".set noreorder\n"
                "lq $8, 0x0(%1)\n"
                "sq $8, 0x0(%0)\n"
                ".set reorder" : : "r"(pDst), "r"(pMtx) : "$8", "memory");
        }
    }
}

void sefGetWeaponPosition(SEF_VEC4 *pDst, char *pAct, int nIdx)
{
    if (pAct != 0 && (unsigned int)nIdx < 8) {
        int n = ACT_jointGetAccessories(pAct, nIdx);
        char *pTbl = *(char **)(pAct + 2084);

        if (n < 0 || pTbl == 0) {
            pDst->y = 20.0f;
        } else {
            char *pMtx = (char *)((n << 6) + (int)pTbl) + 48;
            __asm__ __volatile__(".set noreorder\n"
                "lq $8, 0x0(%1)\n"
                "sq $8, 0x0(%0)\n"
                ".set reorder" : : "r"(pDst), "r"(pMtx) : "$8", "memory");
        }
    }
}

/* --- sefIsFinishEffect2: true when no live scheduler slot is still
 * running this effect number. The hit counter is a movz, and the "any
 * hits at all" answer comes back as sltiu count,1. The loop counter is
 * dead (only the slot pointer is used), so gcc turns the forward loop
 * into the down-counter the original has. --- */
extern int sefCnvEtEffectNo(int nActorID, int nParam);

int sefIsFinishEffect2(int nEftNo)
{
    unsigned int nHits;
    int i;

    nHits = 0;
    if (nEftNo <= 0) {
        return 1;
    }
    for (i = 0; i < 128; i++) {
        if (_schedSlots[i].nUsed != 0) {
            if (sefCnvEtEffectNo(_schedSlots[i].nActorID,
                                 _schedSlots[i].nEtParam) == nEftNo) {
                nHits++;
            }
        }
    }
    return nHits < 1U;
}

/* --- sefFreeSchedulerCf: release a scheduler slot -- stop its sound,
 * destroy all 32 effect-data records, then clear the header. The loop
 * counter is dead so gcc runs it down from 31.
 *
 * The four header stores come out in neither source nor address order;
 * an exhaustive 24-way permutation search picked this one (nState,
 * nUsed, nScriptA, nScriptB). Only two of the 24 orderings reproduce
 * the original's sh/sh/sw/sh sequence. --- */
extern void xglSoundEffectStopID(int nHandle, int a);

void sefFreeSchedulerCf(void *pArg)
{
    SEF_SCHED_SLOT *p;
    char *q;
    int i;

    p = (SEF_SCHED_SLOT *)pArg;
    if (p == 0) {
        return;
    }
    if (p->nUsed == 0) {
        return;
    }
    if (p->nSoundHandle > 0) {
        xglSoundEffectStopID(p->nSoundHandle, 0);
        p->nSoundHandle = 0;
    }
    q = p->aEffect[0];
    for (i = 0; i < 32; i++) {
        sefDestroyEffectData((SEF_EFFDATA *)q);
        q += 48;
    }
    p->nState = 0;
    p->nUsed = 0;
    p->nScriptA = -1;
    p->nScriptB = -1;
}

/* --- line-data slot allocation ------------------------------------- */

/* One line-data record: 65 * 32 bytes. */
typedef struct
{
    char pad0000[0x812];
    short nAlive;            /* 0x812 */
    char pad0814[65 * 32 - 0x814];
} SEF_LINEDATA;

extern SEF_LINEDATA _lineDataTbl[] __asm__("_lineData");
extern void sefDestroyLocalData(void *p);

/* Release a line slot. The `nLine >= 0` arm is dead given the unsigned
 * bound above it, but the original really does test twice -- it is the
 * sefGetLineAdr null convention written out inline. */
void sefFreeLineData(int nLine)
{
    void *p;

    if ((unsigned int)nLine < 128) {
        if (nLine < 0) {
            p = 0;
        } else {
            p = &_lineDataTbl[nLine];
        }
        sefDestroyLocalData(p);
        _lineDataTbl[nLine].nAlive = 0;
    }
}

/* --- sefAllocEffectData: hand out the next free line handle slot in an
 * effect-data block and return the line record it names. --- */
typedef struct
{
    signed char aHandle[32];  /* 0x00 */
    char pad20[0x28 - 0x20];
    short nCount;             /* 0x28 */
    short nDirty;             /* 0x2A */
} SEF_EFFBLOCK;

extern int sefAllocLineData(void);

/* NEAR-MISS (35/35 words, 3 differing). The original fills the bound
 * test's delay slot with the shared `move v0,zero` return value and
 * emits the slot-address addu BEFORE the sltiu; gcc hoists the zero to
 * function entry instead and sinks the addu into the slot. Swept: early
 * returns vs a single accumulator (the accumulator costs an extra
 * callee-saved register and a 36th word), the address computed before
 * and after the bound test, and both declaration orders of the two
 * `p->nCount` reads. Pure delay-slot/scheduling tie-break. */
void *sefAllocEffectData(SEF_EFFBLOCK *p)
{
    int nSlot;
    signed char *pSlot;
    int nLine;

    nSlot = p->nCount;
    pSlot = (signed char *)p + nSlot;
    if ((unsigned int)nSlot >= 32) {
        return 0;
    }
    if (*pSlot >= 0) {
        return 0;
    }
    nLine = sefAllocLineData();
    if (nLine < 0) {
        return 0;
    }
    *pSlot = nLine;
    p->nDirty = 1;
    p->nCount = (unsigned short)p->nCount + 1;
    return &_lineDataTbl[nLine];
}

/* --- sefCnvEtEffectNo: nudge an effect number by a fixed offset when
 * the caller's variant selector disagrees with the band the number
 * falls in. Four bands; the last one is a movz on `nVariant ^ 1`, which
 * is why its adjustment must be the only statement in its arm.
 *
 * NEAR-MISS (35/35 words, 18 differing). The original gives each of the
 * first three bands its OWN `jr ra / move v0,a0` epilogue, reached by
 * fall-through, and branches the adjust path away with a branch-likely
 * carrying the adjustment in the annulled slot. gcc cross-jumps all
 * three epilogues into one shared block and reverses the arm layout.
 * Swept: both arm orders (adjust-first and return-first) in the
 * goto/return mix -- byte-identical output either way; and plain `if`
 * with no goto, which turns all three bands into movz/movn conditional
 * moves and comes out two words short. A jump-optimisation difference,
 * not a source shape. --- */
int sefCnvEtEffectNo(int nEftNo, int nVariant)
{
    if ((unsigned int)(nEftNo - 2027) < 3) {
        if (nVariant != 2) {
            nEftNo -= 3;
            goto out;
        }
        return nEftNo;
    }
    if ((unsigned int)(nEftNo - 2024) < 3) {
        if (nVariant == 2) {
            nEftNo += 3;
            goto out;
        }
        return nEftNo;
    }
    if ((unsigned int)(nEftNo - 2040) < 2) {
        if (nVariant != 1) {
            nEftNo -= 2;
            goto out;
        }
        return nEftNo;
    }
    if ((unsigned int)(nEftNo - 2038) < 2) {
        if (nVariant == 1) {
            nEftNo += 2;
        }
    }
out:
    return nEftNo;
}

/* --- sefCnvWaitEffectNo: resolve a line's "wait" effect number, but
 * only for lines still carrying the 0x4001 placeholder. Small ids index
 * a static table; boss ids go through srsGetBossWaitEftNo; anything else
 * clears the field.
 *
 * NEAR-MISS (32/32 words, correct instruction multiset, ~6 differing).
 * What is left is a pure allocator naming tie-break: the original keeps
 * nID in $a0 / the scaled table offset in $a1 / the boss test in $v1,
 * and we get exactly those four values in the swapped registers, with
 * the sll/sra pair scheduled two slots later. Swept: the 16-bit
 * truncation spelled as `(unsigned short)(nID - 151)` inline (gcc folds
 * the andi away), as an `unsigned short` local (gcc builds the constant
 * with li 0xff69 + addu instead of addiu -151), and as an int local
 * narrowed at the test (this version, which is the one that gives
 * addiu + andi); the table index hoisted into its own local or written
 * as a byte offset (both make gcc issue a SECOND load, an `lh`, instead
 * of deriving the signed value from the single lhu). Closing it would
 * need two --swap-regs pairs plus a reorder, which is more flag debt
 * than 128 bytes is worth. --- */
typedef struct
{
    char pad00[0x10];
    unsigned short nWaitID;  /* 0x10 */
    short pad12;
    short nEffectNo;         /* 0x14 */
    char pad16[0x24 - 0x16]; /* the record really is 0x24 bytes: see the
                              * memset in sefDeleteEffectWait */
} SEF_WAITLINE;

extern unsigned short steftTbl[];
extern int srsGetBossWaitEftNo(int nID);

void sefCnvWaitEffectNo(SEF_WAITLINE *p)
{
    unsigned int nID;
    int nBoss;

    if (p->nEffectNo != 16385) {
        return;
    }
    nID = p->nWaitID;
    nBoss = nID - 151;
#ifdef MATCHING
    /* Zero-cost, zero-register barrier: without it gcc sinks the whole
     * boss-range test past the small-id branch, and the original has it
     * computed before (its sltiu sits in that branch's delay slot). */
    __asm__ __volatile__("");
#endif
    if (nID - 1 < 16) {
        p->nEffectNo = steftTbl[(short)nID];
        return;
    }
    if ((unsigned short)nBoss < 36) {
        p->nEffectNo = srsGetBossWaitEftNo((short)nID);
    } else {
        p->nEffectNo = 0;
    }
}

/* --- sefMemZero: zero a buffer, quadwords first and then a word tail.
 * The 128-bit store is a TImode zero assignment (gcc materialises the
 * zero into a register with `por`, which is what the original does
 * here); both loops are counted down by their own counter, so the byte
 * counts are computed up front. --- */
typedef int SEF_T128 __attribute__((mode(TI)));

void sefMemZero(void *pDst, int nSize)
{
    SEF_T128 *pQ;
    int *pW;
    int nQuads;
    int nWords;
    int i;

    nQuads = nSize >> 4;
    nWords = (nSize & 15) >> 2;
    pQ = (SEF_T128 *)pDst;
    pW = (int *)((char *)pDst + (nQuads << 4));
    for (i = 0; i < nQuads; i++) {
        *pQ = 0;
        pQ++;
    }
    for (i = 0; i < nWords; i++) {
        pW[i] = 0;
    }
}

/* Cold-start the whole effect system: hand the effect heap to the
 * small-block allocator, bring up the sub-systems in order, clear the
 * two battle tables, then mark the driver initialised. */
extern char _eftBuffer[];
extern void smInitilize(void *base, unsigned int size);
extern void sresInitMemoryRes(void);
extern void srsInitCdRead(void);
extern void sevInitPtAllocator(void);
extern void svInitImageMapper(void);
extern void sefInitScheduler(void);
extern void scInitScript(void);
extern void sdvInitSpecialWork(void);
extern void sdvInitAmbient(void);
extern void sresLoadCommonMemory(void);
extern void *memset(void *, int, unsigned int);
extern short _initialize;

void sefInitEffect(void)
{
    smInitilize(_eftBuffer, 0xD4800);
    sresInitMemoryRes();
    srsInitCdRead();
    sevInitPtAllocator();
    svInitImageMapper();
    sefInitScheduler();
    scInitScript();
    memset(_battlePrm, 0, 52);
    memset(_battleData, 0, 560);
    sdvInitSpecialWork();
    sdvInitAmbient();
    sresLoadCommonMemory();
    _sefLoadEftQue = 0;
    _initialize = 1;
}

/* Battle-side counterpart of sefInitEffectCf: swap the 2D-draw callback
 * into the other sRender slot and lazily claim the 256K battle effect
 * buffer. smAlloc's second argument really is the (still null) buffer
 * field -- the original reuses the register it was just loaded into. */
extern void sdvInitAlters(void);
extern void sefDestroyEffect(void);
extern void *smAlloc(unsigned int size, void *p);
typedef struct {
    char pad000[0x38];
    void *pBuf;                       /* 0x38 */
    char pad03C[0x10C - 0x3C];
    short nFileID;                    /* 0x10C */
} SRS_MEMRES;
extern SRS_MEMRES _srsMemRes;

void sefInitEffectBattle(void)
{
    void *pBuf;

    _sefBattleMode = 1;
    sdvInitAlters();
    sefDestroyEffect();
    sRender.cb1 = (int) sefDrawEffect2D;
    sRender.cb2 = 0;
    pBuf = _srsMemRes.pBuf;
    if (pBuf == 0) {
        _srsMemRes.nFileID = -1;
        /* Steering: gcc otherwise folds the just-tested null pointer to
         * $zero and emits a `move a1,zero`; the original passes the
         * register the field was loaded into. */
        LAUNDER(pBuf);
        _srsMemRes.pBuf = smAlloc(0x40000, pBuf);
    }
}

/* --- sefIsFinishEffect: true when nothing is still running (optionally
 * only counting slots belonging to one actor). Same counter/sltiu shape
 * as sefIsFinishEffect2; bit 6 of the slot's 0xA8C flags marks a slot
 * that does not hold the "finished" answer back. --- */
extern int func_A33248(void);

int sefIsFinishEffect(int nID)
{
    unsigned int nHits;
    int i;

    nHits = 0;
    if (func_A33248() != 0) {
        return 0;
    }
    for (i = 0; i < 128; i++) {
        if (_schedSlots[i].nUsed != 0 && !(_schedSlots[i].nFlags & 0x40) &&
            (nID == 0 || _schedSlots[i].nActorID == nID)) {
            nHits++;
        }
    }
    return nHits < 1U;
}

/* --- sefClearEffectCf: walk all 32 effect-data records of a Cf slot
 * and release every line handle each one still holds. Same "line
 * address or null" convention as sefFreeLineData: the handle is tested
 * for negative, and the address formed from it is tested again.
 *
 * NEAR-MISS (43/43 words, 2 differing): the original emits the %hi of
 * the line table (`lui s4,0x6b`) before the outer loop's reversed
 * counter init (`li s3,31`); every spelling here emits them the other
 * way round. Swept: for/do-while/down-counting outer loop, all five
 * declaration orders of the locals, `i = 0` hoisted above the record
 * pointer, and LAUNDER_V on the record pointer (that one also moves the
 * pointer to s0 -- worse). Both instructions are ready in the same
 * block with no dependence between them, so this is purely the EE
 * scheduler's tie-break; permuter territory. --- */
typedef struct
{
    signed char aHandle[32]; /* 0x00 */
    int nUsed;               /* 0x20 */
    char pad24[0x30 - 0x24];
} SEF_EFFREC;

void sefClearEffectCf(void *pArg)
{
    SEF_EFFREC *pRec;
    void *pLine;
    int i;
    int j;
    int h;

    if (pArg == 0) {
        return;
    }
    pRec = (SEF_EFFREC *)((char *)pArg + 176);
    for (i = 0; i < 32; i++) {
        if (pRec->nUsed != 0) {
            for (j = 0; j < 32; j++) {
                h = pRec->aHandle[j];
                if (h >= 0) {
                    pLine = &_lineDataTbl[h];
                    if (pLine != 0) {
                        sefDestroyLocalData(pLine);
                    }
                }
            }
        }
        pRec++;
    }
}

/* --- sefCaclAllTarget: average the world positions of every live
 * battle actor into pDst. The accumulate is VU macro-mode (lqc2 /
 * vadd.xyz / sqc2) and the zeroing store is $vf0, so both are hardware
 * asm. Only the actors that contribute are counted, and the divide is a
 * tail call.
 *
 * NEAR-MISS (38/40 words). Correct logic and register assignment; the
 * two missing words are the actor-count reloads the original puts in
 * the delay slots of the two branch-likelies that skip an actor. Those
 * appear only when the loop latch STARTS with the count reload, so the
 * annulling branches copy it; gcc always schedules the loop-index
 * increment first and copies that instead. Swept: for vs do/while with
 * the guard as `i < t->nActors`, the count read into a named local at
 * the end of the body, LAUNDER and LAUNDER_V on that local (both make
 * gcc keep the count in a register and add `move` copies -- worse),
 * explicit goto/label instead of the `&&`, and both declaration orders
 * of the index and the accumulator (that one does fix which of a3/t0
 * each gets). Latch-schedule tie-break; permuter territory. --- */
extern void MMathDivVectorS(SEF_VEC4 *pDst, SEF_VEC4 *pSrc, float f);

void sefCaclAllTarget(SEF_VEC4 *pDst)
{
    SEF_ACTOR_TBL *t;
    SEF_ACTOR *p;
    char *pPos;
    int i;
    int n;

    PS2_ASM("sqc2 $vf0, 0(%0)" : : "r"(pDst) : "memory");
    t = &_battleActor;
    n = 0;
    i = 0;
    if (i < t->nActors) {
        p = t->aActor;
        do {
            if (p->pLight != 0 && p->nDead == 0) {
                pPos = (char *) p->pLight + 16;
                PS2_ASM(".set noreorder\n"
                    "lqc2 $vf1, 0(%0)\n"
                    "lqc2 $vf2, 0(%1)\n"
                    "vadd.xyz $vf1, $vf1, $vf2\n"
                    "sqc2 $vf1, 0(%0)\n"
                    ".set reorder" : : "r"(pDst), "r"(pPos) : "memory");
                n++;
            }
            i++;
            p++;
        } while (i < t->nActors);
    }
    if (n != 0) {
        MMathDivVectorS(pDst, pDst, (float) n);
    }
}

/* --- sefDeleteEffectWait: build a wait-line record on the stack, let
 * sefCnvWaitEffectNo resolve it, then free every live scheduler slot
 * running the resolved effect. The record is zeroed with memset and is
 * 0x24 bytes even though only the two fields at 0x10/0x14 are used.
 * The resolved number is re-read from the stack on every iteration, so
 * it stays a memory operand in the C too. --- */
void sefDeleteEffectWait(int nWaitID)
{
    SEF_WAITLINE prm;
    int i;

    memset(&prm, 0, sizeof(prm));
    prm.nWaitID = nWaitID;
    prm.nEffectNo = 0x4001;
    sefCnvWaitEffectNo(&prm);
    if (prm.nEffectNo > 0) {
        for (i = 0; i < 128; i++) {
            if (_schedSlots[i].nUsed != 0 &&
                _schedSlots[i].nActorID == prm.nEffectNo) {
                sefDeleteEffect2(&_schedSlots[i]);
            }
        }
    }
}

/* --- VU0 sin/cos: the angle's bit pattern goes to $vf4, a fixed
 * microprogram entry is called, and the result comes back out of $vf1.
 * Genuine hardware, not a compiler idiom -- a port must supply its own
 * sinf/cosf here. --- */
#define VU_SIN(dst, src) \
    PS2_ASM(".set noreorder\n mfc1 $8, %1\n qmtc2 $8, $vf4\n" \
            "vcallms 0xe8\n qmfc2.i $8, $vf1\n mtc1 $8, %0\n" \
            ".set reorder" : "=f"(dst) : "f"(src) : "$8")
#define VU_COS(dst, src) \
    PS2_ASM(".set noreorder\n mfc1 $8, %1\n qmtc2 $8, $vf4\n" \
            "vcallms 0x20\n qmfc2.i $8, $vf1\n mtc1 $8, %0\n" \
            ".set reorder" : "=f"(dst) : "f"(src) : "$8")

extern float sefRandf(void);

/* A random point on a circle of radius fRadius in the XZ plane. */
void sefGetCirclePos(SEF_VEC4 *pDst, float fRadius)
{
    float fAng;
    float fSin;
    float fCos;

    fAng = sefRandf() * 360.0f * 0.017453292f;
    VU_SIN(fSin, fAng);
    pDst->x = fSin * fRadius;
    VU_COS(fCos, fAng);
    pDst->y = 0.0f;
    pDst->w = 1.0f;
    pDst->z = fCos * fRadius;
}

/* --- sefGetSpherePos: a random point on a sphere of radius fRadius.
 * Three LCG draws give a point in a 32768-cube, and VU0 macro mode
 * recentres it, normalises it and scales it out to the radius. The
 * normalise (vsqrt/vdiv through $Q) is genuine hardware. --- */
extern unsigned int sefRandSeed;

void sefGetSpherePos(SEF_VEC4 *pDst, float fRadius)
{
    int aRnd[4];
    unsigned int nA;
    unsigned int nB;
    unsigned int nC;
    float fHalf;

    fHalf = 16384.0f;
    nA = sefRandSeed * 1103515245 + 12345;
    aRnd[0] = (nA >> 16) & 0x7FFF;
    nB = nA * 1103515245 + 12345;
    aRnd[1] = (nB >> 16) & 0x7FFF;
    nC = nB * 1103515245 + 12345;
    aRnd[2] = (nC >> 16) & 0x7FFF;
    sefRandSeed = nC;
    PS2_ASM(".set noreorder\n"
        "lqc2 $vf1, %1\n"
        "mfc1 $2, %2\n"
        "vitof0.xyzw $vf1, $vf1\n"
        "qmtc2 $2, $vf2\n"
        "vsubx.xyz $vf1, $vf1, $vf2x\n"
        "vmul.xyz $vf2, $vf1, $vf1\n"
        "vaddy.x $vf2, $vf2, $vf2y\n"
        "vaddz.x $vf2, $vf2, $vf2z\n"
        "vsqrt $Q, $vf2x\n"
        "vwaitq\n"
        "vaddq.x $vf2, $vf0, $Q\n"
        "vdiv $Q, $vf0w, $vf2x\n"
        "vmove.w $vf2, $vf0\n"
        "vwaitq\n"
        "vmulq.xyz $vf2, $vf1, $Q\n"
        "mfc1 $2, %3\n"
        "qmtc2 $2, $vf1\n"
        "vmulx.xyz $vf2, $vf2, $vf1x\n"
        "sqc2 $vf2, 0(%0)\n"
        ".set reorder"
        : : "r"(pDst), "m"(aRnd[0]), "f"(fHalf), "f"(fRadius) : "$2", "memory");
}

/* --- VU0 macro-mode vector helpers used by the cube/range samplers.
 * All four are genuine hardware: the quadword moves and the .xyz-masked
 * ops have no C equivalent. --- */
#define VU_ITOF(d, s) \
    PS2_ASM(".set noreorder\n lqc2 $vf1, 0(%1)\n" \
            "vitof0.xyzw $vf1, $vf1\n sqc2 $vf1, 0(%0)\n" \
            ".set reorder" : : "r"(d), "r"(s) : "memory")
#define VU_MUL(d, a, b) \
    PS2_ASM(".set noreorder\n lqc2 $vf1, 0(%1)\n lqc2 $vf2, 0(%2)\n" \
            "vmul.xyz $vf1, $vf1, $vf2\n sqc2 $vf1, 0(%0)\n" \
            ".set reorder" : : "r"(d), "r"(a), "r"(b) : "memory")
#define VU_SUB(d, a, b) \
    PS2_ASM(".set noreorder\n lqc2 $vf1, 0(%1)\n lqc2 $vf2, 0(%2)\n" \
            "vsub.xyz $vf1, $vf1, $vf2\n sqc2 $vf1, 0(%0)\n" \
            ".set reorder" : : "r"(d), "r"(a), "r"(b) : "memory")
#define VU_SCALE(d, a, f) \
    PS2_ASM(".set noreorder\n lqc2 $vf1, 0(%1)\n mfc1 $8, %2\n" \
            "qmtc2 $8, $vf2\n vmulx.xyz $vf1, $vf1, $vf2x\n" \
            "sqc2 $vf1, 0(%0)\n .set reorder" \
            : : "r"(d), "r"(a), "f"(f) : "$8", "memory")

/* A random point on the bottom face of a box of integer half-extents
 * pSize, scaled to world units. */
void sefGetCubePosBtm(void *pDst, void *pSize)
{
    SEF_VEC4 vSize;
    SEF_VEC4 vRnd;
    unsigned int nA;
    unsigned int nB;
    unsigned int nC;

    VU_ITOF(&vSize, pSize);
    vRnd.w = 0.0f;
    nA = sefRandSeed * 1103515245 + 12345;
    vRnd.x = (float) (int) ((nA >> 16) & 0x7FFF) * (1.0f / 32767.0f);
    nB = nA * 1103515245 + 12345;
    vRnd.y = (float) (int) ((nB >> 16) & 0x7FFF) * (1.0f / 32767.0f);
    nC = nB * 1103515245 + 12345;
    vRnd.z = (float) (int) ((nC >> 16) & 0x7FFF) * (1.0f / 32767.0f);
    sefRandSeed = nC;
    VU_MUL(&vRnd, &vRnd, &vSize);
    vSize.y = 0.0f;
    VU_SCALE(&vSize, &vSize, 0.5f);
    VU_SUB(&vRnd, &vRnd, &vSize);
    VU_SCALE(pDst, &vRnd, 0.1f);
}

/* Same body as sefGetCubePosBtm without the y-flattening: a random
 * point anywhere inside the box. */
void sefGetCubePos(void *pDst, void *pSize)
{
    SEF_VEC4 vSize;
    SEF_VEC4 vRnd;
    unsigned int nA;
    unsigned int nB;
    unsigned int nC;

    VU_ITOF(&vSize, pSize);
    vRnd.w = 0.0f;
    nA = sefRandSeed * 1103515245 + 12345;
    vRnd.x = (float) (int) ((nA >> 16) & 0x7FFF) * (1.0f / 32767.0f);
    nB = nA * 1103515245 + 12345;
    vRnd.y = (float) (int) ((nB >> 16) & 0x7FFF) * (1.0f / 32767.0f);
    nC = nB * 1103515245 + 12345;
    vRnd.z = (float) (int) ((nC >> 16) & 0x7FFF) * (1.0f / 32767.0f);
    sefRandSeed = nC;
    VU_MUL(&vRnd, &vRnd, &vSize);
    VU_SCALE(&vSize, &vSize, 0.5f);
    VU_SUB(&vRnd, &vRnd, &vSize);
    VU_SCALE(pDst, &vRnd, 0.1f);
}

/* --- sefGetOfsRange: pick a random offset inside the shape named by
 * nType, using the three integer parameters at pPrm. The identity
 * matrix for the cylinder case is built by rotating $vf0 with vmr32,
 * which is why it costs no constant loads. --- */
extern void MMathRotateMatrixX(void *pDst, void *pSrc, float fAng);
extern void MMathRotateMatrixZ(void *pDst, void *pSrc, float fAng);
extern void MMathApplyMatrix(void *pDst, void *pMtx, void *pSrc);
extern void tracePrint(char *pFmt, int nArg);

void sefGetOfsRange(void *pDst, void *pPrmArg, int nType)
{
    int *pPrm;

    float aMtx[16];
    SEF_VEC4 vTmp;
    float fSize;
    float fP0;
    unsigned int nSeed;

    pPrm = (int *) pPrmArg;
    PS2_ASM("sqc2 $vf0, 0(%0)" : : "r"(pDst) : "memory");
    if (nType == 0) {
        if (pPrm[0] != 0) {
            fSize = (float) pPrm[0] * 0.1f;
            sefGetSpherePos(pDst, fSize * sefRandf());
        }
    } else if (nType == 1) {
        if (pPrm[0] != 0) {
            sefGetSpherePos(pDst, (float) pPrm[0] * 0.1f);
        }
    } else if (nType == 2) {
        sefGetCubePos(pDst, pPrm);
    } else if (nType == 3) {
        ((void (*)(void *)) sefGetCubePosTop)(pDst);
    } else if (nType == 4) {
        sefGetCubePosBtm(pDst, pPrm);
    } else if (nType == 5) {
        sefGetCirclePos(&vTmp, (float) pPrm[0] * 0.1f);
        PS2_ASM(".set noreorder\n"
            "vmr32.xyzw $vf1, $vf0\n"
            "vmr32.xyzw $vf2, $vf1\n"
            "vmr32.xyzw $vf3, $vf2\n"
            "sqc2 $vf0, 48(%0)\n"
            "sqc2 $vf1, 32(%0)\n"
            "sqc2 $vf2, 16(%0)\n"
            "sqc2 $vf3, 0(%0)\n"
            ".set reorder" : : "r"(aMtx) : "memory");
        if (pPrm[1] != 0) {
            MMathRotateMatrixX(aMtx, aMtx, (float) pPrm[1] * 0.017453292f);
        }
        if (pPrm[2] != 0) {
            MMathRotateMatrixZ(aMtx, aMtx, (float) pPrm[2] * 0.017453292f);
        }
        MMathApplyMatrix(pDst, aMtx, &vTmp);
    } else if (nType == 6) {
        fP0 = (float) pPrm[0];
        nSeed = sefRandSeed * 1103515245 + 12345;
        sefRandSeed = nSeed;
        sefGetCirclePos(pDst,
            (float) (int) ((nSeed >> 16) & 0x7FFF) * (1.0f / 32767.0f) *
            fP0 * 0.1f);
        ((SEF_VEC4 *) pDst)->y = sefRandf() * (float) pPrm[1] * 0.1f;
    } else {
        tracePrint("bad range prm : %d\n", nType);
    }
}

/* Local-data slot allocator: a 256-entry ring of particle-allocator
 * handles starting at 0x600, with the round-robin cursor at 0x80E.
 * Scan forward from the cursor for a slot holding a negative (free)
 * handle, stopping one short of where we started; the post-loop
 * `i == nEnd` retest is the original's, and it is what makes gcc rotate
 * the loop the way retail did. */
typedef struct {
    char pad0[0x600];
    short tbl[263];
    short nCur;
} SEF_LOCAL_DATA;

extern int sevAllocPtAllocator(void);

int sefAllocLocalData(SEF_LOCAL_DATA *p)
{
    int i;
    int nEnd;
    int h;

    i = p->nCur;
    nEnd = (i - 1) & 0xFF;
    while (i != nEnd && p->tbl[i] >= 0) {
        i = (i + 1) & 0xFF;
    }
    if (i == nEnd) {
        return -1;
    }
    h = sevAllocPtAllocator();
    if (h < 0) {
        return -1;
    }
    p->tbl[i] = h;
    p->nCur = (i + 1) & 0xFF;
    return i;
}

/* Publish one effect record into the shared _battleData slot and hand
 * it to the script engine. The 36-byte header is copied as a struct:
 * it is only 4-aligned, so gcc's block move does the four 8-byte chunks
 * with unaligned ldl/ldr + sdl/sdr pairs and the odd 4-byte tail with a
 * plain lw/sw. Type 55 additionally sets the flag at +22. */
typedef struct {
    int   w0[5];
    short nType;
    short nFlag;
    int   w1[3];
} SEF_EFT_HDR;

extern void sefCnvDeathEffectNo(void *p);
extern void scCreateScript(void *p);

void sefCreateEffect(SEF_EFT_HDR *p)
{
    if (p->nType > 0) {
        if (p->nType == 55) {
            p->nFlag = 1;
        }
        *(SEF_EFT_HDR *)_battleData = *p;
        /* The quadword at +48 is cleared straight out of vf0 -- real
         * hardware asm, no C equivalent. */
        PS2_ASM(".set noreorder\n"
            "sqc2 $vf0, 0x0(%0)\n"
            ".set reorder" : : "r"((char *)_battleData + 48) : "memory");
        sefCnvDeathEffectNo(_battleData);
        scCreateScript(_battleData);
    }
}

/* Wipe the whole scheduler pool and hand every one of its 128 slots a
 * fresh set of 32 effect-data records. Two views of the slot as usual:
 * the three -1 stores walk a pointer based at the slot's 0xA90 tail,
 * while the inner loop starts from the record array at +176 reached
 * through a byte offset off the pool base. */
typedef struct {
    short pad0A90;
    short f0A92;
    short f0A94;
    short f0A96;
    char  pad0A98[0xAB0 - 8];
} SEF_SCHED_TAIL;

extern void sefMemZero(void *p, int size);

void sefInitScheduler(void)
{
    SEF_SCHED_TAIL *q;
    char *pData;
    char *p;
    int i;
    int j;
    /* Steering: gcc gives the hoisted -1 the earlier callee-saved
     * register and the byte offset the later one; retail is the other
     * way round. Emits no code. */
    PIN(int nOfs, "$19");

    sefMemZero(_scheduler, 0x55800);
    pData = (char *)_scheduler + 176;
    q = (SEF_SCHED_TAIL *)((char *)_scheduler + 2704);
    for (i = 0, nOfs = 0; i < 128; i++, nOfs += 0xAB0) {
        q->f0A92 = -1;
        q->f0A94 = -1;
        q->f0A96 = -1;
        p = (char *)(nOfs + (int)pData);
        for (j = 31; j >= 0; j--) {
            sefInitEffectData(p, 0, -1, 0);
            p += 48;
        }
        q++;
    }
}

/* PARKED at 37 diffs -- the instruction sequence is retail's, word for
 * word; every difference is a caller-saved register name (nOfs t0<->a1,
 * base t2<->t1, nEnd a1<->a2) plus one `move v0,a3` / `sh t4,6` swap.
 * What DID have to be got right, and is worth keeping:
 *   - the two address computations are offset-first: `(nOfs + 0x6B0) +
 *     (int)pBase`, through named int temporaries. Written as
 *     `nOfs + 0x6B0 + (int)_scheduler` gcc folds the constant into the
 *     symbol and hoists base+1712 into the preheader (42 diffs); with a
 *     pBase local but no temporary it adds the base first (51).
 *   - _nowScript/_nowEvent must be read HERE, not in the preheader.
 *     gcc lifts both gp loads out as loop invariants; copying them into
 *     `int` locals and passing the pair through LAUNDER2 (the pseudo is
 *     then set twice in the loop, so it is no longer invariant) puts
 *     them back. As `unsigned short` locals the same LAUNDER2 costs two
 *     extra extension moves (56 words).
 * Declaration order does not move the remaining names -- they are all
 * local_alloc temporaries. Permuter territory.
 *
 * Round-robin scheduler-slot allocator over the 128 pool entries: scan
 * forward from the saved cursor, stopping one slot short of it, and
 * claim the first entry whose owner pointer at +0x6B0 is null. The
 * header at +0xA90 is stamped with the current script/event ids. */
typedef struct {
    short f0A90;
    short f0A92;
    short f0A94;
    short f0A96;
} SEF_SCHED_SLOT_HDR;

extern unsigned short _nowScript;
extern unsigned short _nowEvent;

int sefAllocScheduler(void *pOwner)
{
    static int _schridx;
    SEF_SCHED_SLOT_HDR *q;
    void **ppOwner;
    char *pBase;
    int nOfs;
    int nRec;
    int nHdr;
    int i;
    int nEnd;

    nEnd = (_schridx + 127) % 128;
    i = _schridx;
    pBase = (char *)_scheduler;
    while (i != nEnd) {
        i = i % 128;
        nOfs = i * 0xAB0;
        nRec = nOfs + 0x6B0;
        ppOwner = (void **)(nRec + (int)pBase);
        if (*ppOwner == 0) {
            int nScript = _nowScript;
            int nEvent = _nowEvent;

            /* Steering: gcc lifts these two loop-invariant gp loads into
             * the loop preheader; retail reads them here, in the block
             * that claims the slot. Emits no code. */
            LAUNDER2(nScript, nEvent);
            nHdr = nOfs + 0xA90;
            q = (SEF_SCHED_SLOT_HDR *)(nHdr + (int)pBase);
            *ppOwner = pOwner;
            q->f0A96 = -1;
            q->f0A92 = nScript;
            q->f0A94 = nEvent;
            _schridx = (i + 1) % 128;
            q->f0A90 = 0;
            return i;
        }
        i++;
    }
    return -1;
}

/* Release scheduler slot nIdx. Continuous-fire slots (flag 0x202) hand
 * off to sefFreeSchedulerCf; everything else tears down its 32
 * effect-data records, drops a reference on the effect record the slot
 * was playing, and clears the slot header. Retail recomputes
 * nIdx * 0xAB0 after the loop instead of keeping the slot pointer live
 * across the calls -- that is what the repeated subscript gives. */
typedef struct {
    char           pad0000[8];
    unsigned short nRefCount;        /* 0x08 */
    char           pad000A[0x450 - 0xA];
} SEF_EFT_REC;

extern SEF_EFT_REC D_0041E810[];

void sefFreeScheduler(int nIdx)
{
    SEF_SCHED_SLOT *p;
    char *q;
    int i;
    int n;

    if ((unsigned int)nIdx < 128 && _schedSlots[nIdx].nUsed != 0) {
        p = &_schedSlots[nIdx];
        if ((p->nFlags & 0x202) != 0) {
            sefFreeSchedulerCf(p);
            return;
        }
        q = (char *)_schedSlots[nIdx].aEffect;
        for (i = 31; i >= 0; i--) {
            sefDestroyEffectData(q);
            q += 48;
        }
        n = _schedSlots[nIdx].nScriptA;
        if (n >= 0) {
            SEF_EFT_REC *pRec = &D_0041E810[n];
            int nRef = pRec->nRefCount;

            pRec->nRefCount = nRef - 1;
        }
        _schedSlots[nIdx].nUsed = 0;
        _schedSlots[nIdx].nScriptA = -1;
        _schedSlots[nIdx].nScriptB = -1;
        _schedSlots[nIdx].nState = 0;
    }
}

/* --- sev* particle-handle allocator: a 1024-entry free-index ring in
 * front of a 1024 x 0x280 pool.  sevInitPtAllocator seeds the ring with
 * the identity permutation; alloc pops at nRead, free pushes at nWrite. */

typedef struct
{
    char pad0000[0x280];
} SEV_PT;

typedef struct
{
    SEV_PT pt[1024];      /* 0x000000 */
    short  nFree[1024];   /* 0x0A0000 */
    int    nRead;         /* 0x0A0800 */
    int    nWrite;        /* 0x0A0804 */
    int    pad0A0808[2];
} SEV_PTALLOC;

extern SEV_PTALLOC _ptAllocTbl __asm__("_ptAlloc");

void *sevGetPtAllocator(int nIdx) { return &_ptAllocTbl.pt[nIdx]; }
void *sevGetPtAllocator2(int nIdx) { return &_ptAllocTbl.pt[nIdx]; }

/* SWEPT, 12 diffs, whole body matches: retail's prologue keeps ONE
 * register for both roles -- base in $s0, `move $a0,$s0` for the memset,
 * then `addu $s0,$at,$s0` with the +0xA07FE bias materialised in $at,
 * i.e. gas expanding a single large-constant `addu`. Every C spelling
 * tried (pointer local + &p->nFree[1023]; one char* advanced after the
 * calls; explicit (char*)p + 0xA07FE; the bare symbol, which folds the
 * bias into the relocation) makes gcc load the constant into an
 * allocatable register instead of $at, so the add reads s0,s0,v0. No
 * source shape found that reaches the $at form. */
void sevInitPtAllocator(void)
{
    /* Through a pointer local: naming the member on the symbol folds
     * +0xA07FE into the relocation, where retail adds it to a held base. */
    SEV_PTALLOC *p = &_ptAllocTbl;
    short *q;
    int i;

    q = (short *)((char *)p + 0xA07FE);
    memset(p, 0, sizeof(SEV_PTALLOC));
    memset(_battleData, 0, 560);
    for (i = 1023; i >= 0; i--) {
        *q = i;
        q--;
    }
}

int sevAllocPtAllocator(void)
{
    /* A pointer local, not the plain symbol: the member address would
     * otherwise fold into the relocation and lose retail's held base. */
    SEV_PTALLOC *p = &_ptAllocTbl;
    int nRead = p->nRead;
    int nNext = (nRead + 1) & 0x3FF;

    if (nNext == p->nWrite) {
        return -1;
    }
    p->nRead = nNext;
    return p->nFree[nRead];
}

/* SWEPT, 18 diffs, SCHEDULING only: identical multiset. Retail leaves a
 * genuine nop in the `beqz` delay slot and loads p->nWrite in the block
 * ABOVE the range check; gcc fills that slot with the load whichever way
 * the source is written. Flat early-returns (this form), the nested
 * `if (h < 1024) { ... }` form and hoisting/sinking the nRead load were
 * all tried; the nested form is worse (LOGIC 18, handle copied to $a3). */
void sevFreePtAllocator(int nHandle)
{
    SEV_PTALLOC *p = &_ptAllocTbl;
    int nWrite;

    nWrite = p->nWrite;
    if ((unsigned int)nHandle >= 1024) {
        return;
    }
    if (p->nRead == nWrite) {
        return;
    }
    p->nFree[nWrite] = nHandle;
    p->nWrite = (nWrite + 1) & 0x3FF;
}

/* --- SGs*: a GIFtag packet builder.  The packet record holds the write
 * cursor in qwords, the qword index of the tag currently open, that
 * tag's NLOOP, and the packet base.  Every field write reloads nQw and
 * pBase because a store through pBase may alias the record itself. --- */

typedef int SGS_T128 __attribute__((mode(TI)));

/* volatile, and not for matching's sake: the packet these describe is
 * being written by the GIF/DMA side too, so every access re-reads the
 * cursor and the base.  It is also what reproduces retail's reload
 * before every single field store. */
typedef struct
{
    volatile int   nQw;      /* 0x00  write cursor, in qwords */
    volatile int   nTagQw;   /* 0x04  qword index of the open GIFtag */
    volatile int   nNloop;   /* 0x08  that tag's NLOOP */
    char *volatile pBase;    /* 0x0C */
} SGS_PKT;

typedef struct { volatile int x, y, z, w; } SGS_QWI;
typedef struct { volatile unsigned long long lo, hi; } SGS_QWD;
typedef struct { volatile float s, t, q; int pad; } SGS_QWF;
typedef struct { volatile short u, v; } SGS_QWH;

void SGsInitGifPacket(SGS_PKT *p)
{
    p->nNloop = 1;
    p->pBase = (char *)0x70000000;
    p->nQw = 0;
    p->nTagQw = 0;
}

/* SWEPT, 6 diffs, REGISTER only: two independent register swaps
 * ($t0<->$t1 and $v0<->$v1). The volatile reads pin the source order --
 * nQw must be read before pBase, which must be read before the nNloop
 * write -- so there is no statement order left to vary; a `char *pBase`
 * local instead of the folded SGS_QWD address gives a 3-cycle rotation
 * instead, which is worse. Permuter territory. */
void SGsOpenGifPacket(SGS_PKT *p, unsigned long long nPrim,
                      unsigned long long nRegs, unsigned long long nNReg)
{
    int nQw = p->nQw;
    SGS_QWD *pq = (SGS_QWD *)(p->pBase + nQw * 16);

    p->nNloop = 1;
    p->nTagQw = nQw;
    pq->hi = nRegs;
    ((SGS_QWD *)(p->pBase + p->nQw * 16))->lo =
        (nPrim << 47) | (nNReg << 60) | 0x0000400000008000ULL;
    p->nQw = p->nQw + 1;
}

/* SWEPT, 7 diffs, REGISTER only: retail holds the tag address in $v1 and
 * loads through it into $v0, keeping $a0 (the packet) live; gcc reuses
 * $a0 for the loaded value. Both `*q | nNloop` and `nNloop | *q`, the
 * offset-first int temporary and reading nNloop into a local before the
 * address all land on the same assignment. */
void SGsCloseGifPacket(SGS_PKT *p)
{
    int nOfs = p->nTagQw * 16;
    char *pBase = p->pBase;
    int nNloop = p->nNloop;
    volatile unsigned long long *q =
        (volatile unsigned long long *)(nOfs + (int)pBase);

    *q = nNloop | *q;
}

void SGsAddReg(SGS_PKT *p, unsigned long long nReg, unsigned long long nData)
{
    ((SGS_QWD *)(p->pBase + p->nQw * 16))->lo = nData;
    ((SGS_QWD *)(p->pBase + p->nQw * 16))->hi = nReg;
    p->nQw = p->nQw + 1;
}

void SGsAddGifRGBA(SGS_PKT *p, SGS_T128 v)
{
    *(volatile SGS_T128 *)(p->pBase + p->nQw * 16) = v;
    p->nQw = p->nQw + 1;
}

void SGsAddGifXYZ2(SGS_PKT *p, int nX, int nY, int nZ, int nW)
{
    nX = (nX << 4) + 0x7000;
    nY = (nY << 4) + 0x7200;
    *(int *)(p->pBase + p->nQw * 16) = nX;
    *(int *)(p->pBase + p->nQw * 16 + 4) = nY;
    *(int *)(p->pBase + p->nQw * 16 + 8) = nZ;
    *(int *)(p->pBase + p->nQw * 16 + 12) = nW;
    p->nQw = p->nQw + 1;
}

void SGsAddGifData(SGS_PKT *p, SGS_T128 v)
{
    *(volatile SGS_T128 *)(p->pBase + p->nQw * 16) = v;
    p->nQw = p->nQw + 1;
}

/* SWEPT, 19 diffs, REGISTER only: the $v0/$v1 alternation across the
 * four address computations starts on the wrong parity, and the last
 * one takes the dead $a1 instead of continuing the alternation. Retail
 * does the opposite here from what it does in SGsAddGifXYZ2, which has
 * the identical shape but integer arguments -- so the parity is falling
 * out of argument-register pressure, not statement order. Plain stores,
 * volatile stores, a non-volatile pad field and struct-member stores
 * were all tried. */
void SGsAddGifSTQ(SGS_PKT *p, float fS, float fT, float fQ)
{
    ((SGS_QWF *)(p->pBase + p->nQw * 16))->s = fS;
    ((SGS_QWF *)(p->pBase + p->nQw * 16))->t = fT;
    ((SGS_QWF *)(p->pBase + p->nQw * 16))->q = fQ;
    ((SGS_QWF *)(p->pBase + p->nQw * 16))->pad = 0;
    p->nQw = p->nQw + 1;
}

void SGsAddGifUV(SGS_PKT *p, int nU, int nV)
{
    nU = nU << 4;
    nV = nV << 4;
    ((SGS_QWH *)(p->pBase + p->nQw * 16))->u = nU;
    ((SGS_QWH *)(p->pBase + p->nQw * 16))->v = nV;
    p->nQw = p->nQw + 1;
}

/* --- sefLerpVector / sefLerpIVector: advance a keyframe cursor one
 * frame and interpolate between the two straddling records.  The table
 * is packed short[5] records -- a 16-bit time followed by a short[3]
 * value and a pad -- and 1024 in a time field marks the end. --- */

typedef struct
{
    short t;                /* 0x00 */
    short x, y, z;          /* 0x02 */
    short w;                /* 0x08 */
} SEF_KEY;                  /* 10 bytes */

typedef struct
{
    int   nIdx;             /* 0x00  cursor into the key table */
    int   nTime;            /* 0x04  current time */
    char  pad0008[8];
    float aVec[4];          /* 0x10  interpolation result */
} SEF_LERP;

extern SEF_KEY *sefProgressKey5(SEF_KEY *pTbl, SEF_LERP *pKey);
extern void sefLerpIVectorA(void *pSrc, void *pDst);

int sefLerpVector(SEF_KEY *pTbl, SEF_LERP *pKey)
{
    SEF_KEY *p;
    float *pDst = pKey->aVec;
    int t0;
    int t1;
    float fScale;

    p = &pTbl[pKey->nIdx];
    if (p[1].t == 1024) {
        return 0;
    }
    p = sefProgressKey5(pTbl, pKey);
    t0 = p->t;
    t1 = p[1].t;
    if (t0 != t1) {
        fScale = (float)(pKey->nTime - t0) / (float)(t1 - t0);
    /* Same unaligned ldl/ldr + pcgth/pextlh sign-extend as
     * sefLerpVectorB, but over two pointers the caller already holds;
     * dsrl drops each record's leading time field. VU0 hardware. */
    __asm__ __volatile__(".set noreorder\n"
        "mfc1 $9, %3\n"
        "qmtc2 $9, $vf3\n"
        "ldl $8, 0x7(%0)\n"
        "ldr $8, 0x0(%0)\n"
        "ldl $9, 0x7(%1)\n"
        "ldr $9, 0x0(%1)\n"
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
        "sqc2 $vf2, 0x0(%2)\n"
        ".set reorder" : : "r"(p), "r"(p + 1), "r"(pDst), "f"(fScale)
            : "$8", "$9", "$10", "$11", "memory");
        pDst[3] = 1.0f;
    } else {
        sefLerpVectorA(p, pDst);
    }
    return 1;
}

void sefLerpIVector(SEF_KEY *pTbl, SEF_LERP *pKey)
{
    SEF_KEY *p;
    int *pDst = (int *)pKey->aVec;
    int t0;
    int t1;
    float fScale;

    p = &pTbl[pKey->nIdx];
    if (p[1].t == 1024) {
        return;
    }
    p = sefProgressKey5(pTbl, pKey);
    t0 = p->t;
    t1 = p[1].t;
    if (t0 != t1) {
        fScale = (float)(pKey->nTime - t0) / (float)(t1 - t0);
    /* Integer-result twin: the records here are addressed past their
     * time field, so no dsrl, and a vftoi0 on the way out. */
    __asm__ __volatile__(".set noreorder\n"
        "ldl $8, 0x7(%0)\n"
        "ldr $8, 0x0(%0)\n"
        "ldl $9, 0x7(%1)\n"
        "ldr $9, 0x0(%1)\n"
        "mfc1 $10, %3\n"
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
        "sqc2 $vf2, 0x0(%2)\n"
        ".set reorder" : : "r"(&p->x), "r"(&p[1].x), "r"(pDst), "f"(fScale)
            : "$8", "$9", "$10", "$11", "memory");
        return;
    }
    /* Outside the else, as the function's last statement: gcc only turns
     * this into retail's `j sefLerpIVectorA` sibling jump there. */
    sefLerpIVectorA(&p->x, pDst);
}

/* SWEPT, 72 words vs retail's 70 in this steering-free form. Retail
 * computes and stores the four components strictly one at a time; gcc
 * software-pipelines them, hoisting each component's `lh` above the
 * previous `sw`, because type-based aliasing says a short load cannot
 * conflict with an int store. Six `asm("":::"memory")` barriers -- one
 * after each of the first three stores in BOTH arms -- do reproduce
 * retail's order and take it to 33 diffs, 4 of them opcodes, the rest a
 * $a0/$a1 and $s0/$s1 swap; that is far too much steering to carry for
 * 280 bytes in a tree headed for a portable build, so it is recorded
 * rather than committed. Also swept: volatile destination (74 words),
 * one shared local pair for the component values (helped, kept), the
 * goto to a shared negate-and-store tail (helped, kept, and it is what
 * retail's branch into the middle of the degenerate block is), and both
 * operand orders on the final add.
 *
 * Fixed-point twin of sefLerpIVector: no VU0 at all, a 16.16 fraction
 * from an integer divide, and the w component comes out negated. Both
 * degenerate exits (end-of-table, and a zero-length span) fall into the
 * same straight copy; gcc cross-jumps the shared negate-and-store tail,
 * which is what retail's branch into the middle of that block is. */
void sefLerpIVector2(SEF_KEY *pTbl, SEF_LERP *pKey)
{
    SEF_KEY *p;
    int *pDst = (int *)pKey->aVec;
    int t0;
    int t1;
    int f;
    int a;
    int b;
    int n;

    p = &pTbl[pKey->nIdx];
    if (p[1].t != 1024) {
        p = sefProgressKey5(pTbl, pKey);
        t0 = p->t;
        t1 = p[1].t;
        if (t0 != t1) {
            f = ((pKey->nTime - t0) << 16) / (t1 - t0);
            a = p->x;
            b = p[1].x;
            pDst[0] = (((b - a) * f) >> 16) + a;
            a = p->y;
            b = p[1].y;
            pDst[1] = (((b - a) * f) >> 16) + a;
            a = p->z;
            b = p[1].z;
            pDst[2] = (((b - a) * f) >> 16) + a;
            a = p->w;
            b = p[1].w;
            n = (((b - a) * f) >> 16) + a;
            goto store;
        }
    }
    pDst[0] = p->x;
    pDst[1] = p->y;
    pDst[2] = p->z;
    n = p->w;
store:
    pDst[3] = -n;
}
