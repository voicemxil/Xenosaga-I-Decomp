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
    char pad0000[0x1278];
    unsigned short nUsed;    /* 0x1278 */
    short pad127A;
    short nSerial;           /* 0x127C */
    short pad127E;
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

/* Tear down the eight "special" slots, telling the game loop to drop the
 * defocus each one requested */
extern void GameDefocusSet(int nIdx, int a, int b);
extern short _sdvSpecialWork[] __asm__("_sdvSpecialBuf");

void sdvClearSpecialWork(void)
{
    short *p;
    int i;

    p = _sdvSpecialWork;
    for (i = 0; i < 8; i++) {
        if (*p != 0) {
            GameDefocusSet(i, 0, 0);
            *p = 0;
        }
        p++;
    }
}

/* Kill the alter slot named by a packed (serial << 16 | index) handle,
 * but only if the slot is still live and still carries that serial.
 * The live/serial pair sits in a header at +0x1270 that the original
 * addresses through its own pointer -- hence the separate `addiu +4720`
 * rather than 0x1278/0x127C folded into the loads -- and the slot's byte
 * offset is kept in its own unsigned variable so gcc cannot reassociate
 * base+offset+constant into one address. */
typedef struct
{
    char pad00[8];
    unsigned short nUsed;    /* +0x08 (slot +0x1278) */
    short pad0A;
    short nSerial;           /* +0x0C (slot +0x127C) */
    short pad0E;
} ALTER_HDR;

void sdvKillAlter(int nHandle)
{
    char *pBase;
    unsigned int nOfs;
    unsigned int nHdrOfs;
    int nSerial;
    ALTER_HDR *pHdr;

    if (nHandle < 0) {
        return;
    }
    nSerial = nHandle >> 16;
    pBase = (char *)_sdvAlter;
    nOfs = (nHandle & 0xFFFF) * 0x1280;
    nHdrOfs = nOfs + 0x1270;
    pHdr = (ALTER_HDR *)(pBase + nHdrOfs);
    if (pHdr->nUsed == 0) {
        return;
    }
    if (pHdr->nSerial != nSerial) {
        return;
    }
    sdvDestroyAlter(pBase + nOfs);
}

extern void sdvExecAlter(void *p);

void sdvExecAlters(void)
{
    int i;

    for (i = 0; i < 16; i++) {
        if (_sdvAlter[i].nUsed != 0) {
            sdvExecAlter(&_sdvAlter[i]);
        }
    }
}
