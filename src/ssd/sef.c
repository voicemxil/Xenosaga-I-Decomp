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

extern void sefLerpVectorSC(void *a, void *b);
extern void sefLerpIVector(void *a, void *b);
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
    char pad084[0x86 - 0x84];
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

/* Raise or clear bit 15 of every live actor's light flags. Note the
 * original dereferences pLight on the null branch too -- the null test
 * only selects which mask is applied, it does not skip the store, which
 * is why the two arms have to share one exit through a goto. */
void sefSetLightFlag(void)
{
    SEF_ACTOR *p;
    int i;
    int *pFlags;
    int nFlags;

    i = 0;
    if (_battleActor.nActors > 0) {
        p = _battleActor.aActor;
        do {
            pFlags = p->pLight;
            if (pFlags == 0) {
                goto clear;
            }
            if (p->nLightFlag > 0) {
                nFlags = *pFlags | 0x8000;
            } else {
clear:
                nFlags = *pFlags & ~0x8000;
            }
            *pFlags = nFlags;
            i++;
            p++;
        } while (i < _battleActor.nActors);
    }
}

/* Bump the hit / sound-effect signal counter of the actor holding this
 * handle. The search is a goto loop entered at the comparison, so gcc's
 * loop optimiser never sees a natural loop: the slot address is rebuilt
 * from the table base every iteration and the live count re-read. Any
 * for/while/do-while spelling gets strength-reduced to a pointer walk
 * with the count hoisted, which is 11 instructions off. */
void sefSetHitSignal(int *pHandle)
{
    SEF_ACTOR *p;
    int i;

    if (pHandle != 0) {
        p = _battleActor.aActor;
        i = 0;
        if (_battleActor.nActors > 0) {
loop:
            if (p->pLight != pHandle) {
                i++;
                p = &_battleActor.aActor[i];
                if (i < _battleActor.nActors) {
                    goto loop;
                }
            } else {
                p->nHitSignal++;
            }
        }
    }
}

void sefSetSeSignal(int *pHandle)
{
    SEF_ACTOR *p;
    int i;

    if (pHandle != 0) {
        p = _battleActor.aActor;
        i = 0;
        if (_battleActor.nActors > 0) {
loop:
            if (p->pLight != pHandle) {
                i++;
                p = &_battleActor.aActor[i];
                if (i < _battleActor.nActors) {
                    goto loop;
                }
            } else {
                p->nSeSignal++;
            }
        }
    }
}

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
    char pad000[0x6B0];
    int  nUsed;              /* 0x6B0 */
    char pad6B4[0xA78 - 0x6B4];
    short nActorID;          /* 0xA78 */
    char padA7A[0xA92 - 0xA7A];
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
