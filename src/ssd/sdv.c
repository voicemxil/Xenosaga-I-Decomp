/* Battle scene ambient/alter-state functions (sdv* family) */

extern int _sdvAmbFrame;
extern int _sdvAmbState;
void sdvInitAmbient(void) {
    _sdvAmbFrame = 0;
    _sdvAmbState = 0;
}

extern void sdvSetAmbStateSub(int a, int b, int c);
void sdvSetAmbState(int a, int b) { sdvSetAmbStateSub(a, b, 0); }
void sdvSetAmbState2(int a, int b) { sdvSetAmbStateSub(a, b, 1); }

extern void xglSoundEffectNormalID(int id, int a);
void sdvPlaySound(int id) { xglSoundEffectNormalID(id, 0); }

extern void sefMemZero(void *p, int size);
void sdvInitAlter(void *p) { sefMemZero(p, 0x1280); }
void sdvDestroyAlter(void *p) { sefMemZero(p, 0x1280); }

extern int _sdvMapRgb[];
extern int _sdvAmbient[];
extern void sdvSetAmbient(void *a, void *b);
void sdvRestoreAmbient(void) { sdvSetAmbient(_sdvMapRgb, _sdvAmbient); }

extern int _sdvSpecialBuf[];
extern void *memset(void *, int, unsigned int);
void sdvInitSpecialWork(void) { memset(_sdvSpecialBuf, 0, 0x10); }

/* --- ambient light plumbing --- */

extern void *xglStudioGetLight2(void);
extern void xglLightIntensityAmbient(void *pLight, void *pIntensity);
extern void func_A2C3D8(void *p);

void sdvSetAmbient(void *pRgb, void *pAmbient)
{
    xglLightIntensityAmbient(xglStudioGetLight2(), pAmbient);
    func_A2C3D8(pRgb);
}

/* --- the 16 alter-state slots (0x1280 bytes apiece) --- */

typedef struct
{
    char pad[0x1280];
} SDV_ALTER;

extern SDV_ALTER _sdvAlter[];

void sdvInitAlters(void)
{
    int i;
    SDV_ALTER *p;

    p = _sdvAlter;
    for (i = 0; i < 16; i++) {
        sdvInitAlter(p);
        p++;
    }
}

void sdvDestroyAlters(void)
{
    int i;
    SDV_ALTER *p;

    p = _sdvAlter;
    for (i = 0; i < 16; i++) {
        sdvDestroyAlter(p);
        p++;
    }
}

/* Snapshot the current ambient/map colours into _sdvAmbient/_sdvMapRgb
 * and reset the ambient fade. The two saves are 128-bit quadword copies
 * (lq/sq) -- MMI, not expressible in C. */
extern void *func_A2C3F8(void);
extern int _sdvAmbient[4];
extern int _sdvMapRgb[4];

void sdvSaveAmbient(void)
{
    void *pLight;
    void *pRgb;

    pLight = xglStudioGetLight2();
    pRgb = func_A2C3F8();
    __asm__ __volatile__("lq $8, 0x0(%1)\n\tsq $8, 0x0(%0)"
                         : : "r"(_sdvAmbient), "r"(pLight) : "$8", "memory");
    __asm__ __volatile__("lq $8, 0x0(%1)\n\tsq $8, 0x0(%0)"
                         : : "r"(_sdvMapRgb), "r"(pRgb) : "$8", "memory");
    _sdvAmbFrame = 0;
    _sdvAmbState = 0;
}
