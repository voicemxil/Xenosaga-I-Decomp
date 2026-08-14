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

extern void sefInitClipViewVolume(void *p);
extern void sefClipViewVolumeA(void *p, void *q);
void sefClipViewVolume(void *a, void *b) {
    sefInitClipViewVolume((char *)b + 1264);
    sefClipViewVolumeA(a, b);
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
    char pad80E[0x814 - 0x80E];
    unsigned short f814;
    short f816;
} SEF_LINE;

extern float _gravity;
extern float _height;
extern float _colision;
extern void sefLerpVectorSC(void *a, void *b);
extern void sefLerpIVector(void *a, void *b);
extern void sefProgressInt(void *a, void *b);
extern void sefGetPosition(void *a, void *b, int c);
extern int sefExecLineGlobalData(void *a, int b);
extern int sefExecLineLocalData(void *a, int b);

/* TODO: near-miss, 2/106 words differ (register tie-break, unreachable from
 * C -- matches the "Allocator-order tie-breaks NOT reachable from C" wall).
 * The very first decrement of f804 is tested via the sll<<16/bgez idiom;
 * gcc's store-forwarding then reuses that shifted v1 (sra) for the later
 * reload of f804 inside the f80C/f814-guarded branch, where the original
 * emits a fresh `lh`. Tried: memory barrier (kills the bnezl delay-slot
 * fold instead), volatile pointer cast (destabilizes the whole function's
 * register allocation), flat-pointer-cast store/reload (no effect),
 * register-pinning icnt to $2 (destabilizes allocation). permute.py
 * confirms no statement reordering changes the schedule. Not registered in
 * config/decompiled.txt -- do not add without closing these 2 words. */
int sefExecLineData(SEF_LINE *p, int mode)
{
    unsigned short mode2 = p->f806;
    short v0;
    float scale;
    void *p1D4;
    int local;
    p->f802 = -1;

    {
        int icnt;
        if (p->f80A != 0) {
            int val = (unsigned short)p->f804 - 1;
            p->f804 = val;
            if ((val << 16) < 0) {
                if (p->f80C == 0) {
                    p->f804 = 0;
                    return 0;
                }
                if (p->f814 == 0) {
                    p->f804 = 0;
                    return 0;
                }
                icnt = *(short *)((char *)p + 0x804);
            } else {
                icnt = val;
            }
        } else {
            icnt = p->f804;
        }
        if (icnt < 0) {
            p->f804 = -1;
        }
    }
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
