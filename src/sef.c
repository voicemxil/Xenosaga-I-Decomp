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

extern char D_0040D69C[];
void *svGetImageItem(int idx) { return D_0040D69C + idx * 36; }

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

extern int _draw3D;
extern int _nEffect2D;
extern int _nowImage;
extern void svDrawSchedulerParticle(void);
void svDrawScheduler(void) {
    _draw3D = 0;
    _nEffect2D = 0;
    _nowImage = 0;
    svDrawSchedulerParticle();
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

extern int _sdvMapRgb[];
extern int _sdvAmbient[];
extern void sdvSetAmbient(void *a, void *b);

extern int _sefLoadEftQue;
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
    if (a < 0) return 0;
    if (b < 0) return 0;
    return (char *)_ptAlloc + b * 0x280;
}

extern void srsAnalyzeEftNo(void *p, int *a, int *b);
extern void sefSearchMapperIndex2(void *p, int a, int b);
void sefSearchMapperIndex(void *p) {
    int a, b;
    srsAnalyzeEftNo(p, &a, &b);
    sefSearchMapperIndex2(p, a, b);
}
