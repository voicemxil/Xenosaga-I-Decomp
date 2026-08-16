/* Zero-init helpers for various subsystem state blocks (model system, back buffer, fade control, etc) */

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u_int;

int s_nAlphaGroup;
int s_nNonAlphaGroup;

typedef struct {
    int a00;
    int a04;
    int a08;
    int a0C;
    int a10;
    int a14;
    int a18;
    int a1C;
    int a20;
    int a24;
} BACKBUF;

BACKBUF s_inBackBuffer;

int s_nParentBuf;

typedef struct {
    int nUnk00;
    int nUnk04;
} MAP_HANDLE;

typedef struct {
    u8 pad00[0x10];
    int a10;
    int a14;
    int a18;
    u8 pad1C[0x8];
    int a24;
    int a28;
    int a2C;
    int a30;
} FADECTRL;

/* Clears both alpha-group draw-block counters */
void CONSTRUCT_ALPHA_GROUP(void)
{
    s_nAlphaGroup = 0;
    s_nNonAlphaGroup = 0;
}

/* Resets the sub-window back-buffer descriptor to its default state */
void CONSTRUCT_BACK_BUFFER(void)
{
    s_inBackBuffer.a10 = -1;
    s_inBackBuffer.a14 = 0;
    s_inBackBuffer.a00 = 0;
    s_inBackBuffer.a04 = 0;
    s_inBackBuffer.a08 = 0;
    s_inBackBuffer.a0C = 0;
    s_inBackBuffer.a18 = 0;
    s_inBackBuffer.a1C = 0;
    s_inBackBuffer.a20 = 0;
    s_inBackBuffer.a24 = 0;
}

/* Clears the parent-buffer handle */
void CONSTRUCT_PARENT_BUF(void)
{
    s_nParentBuf = 0;
}

/* Clears a map handle pair */
void CONSTRUCT_MAP_HANDLE(MAP_HANDLE *p)
{
    p->nUnk04 = 0;
    p->nUnk00 = 0;
}

/* Resets a fade-control block to its default (inactive) state */
void CONSTRUCT_FADE_CONTROL(FADECTRL *p)
{
    p->a10 = -1;
    p->a14 = 1;
    p->a18 = 0;
    p->a24 = 0;
    p->a28 = 0;
    p->a2C = 0;
    p->a30 = 0;
}

/* Quadword zero-fill of the global fog/point-light blocks: the original
   clears them with `sq zero`, which is what a mode(TI) store compiles to. */
typedef int MODELSYS_QUAD __attribute__((mode(TI)));

typedef struct {
    int a00;
    int a04;
    int a08;
    int a0C;
} SUBWINDOW;

SUBWINDOW g_aSubWindow;

/* Small-data model-system flags, in memory order (two slots in the run
   belong to other translation units). */
int s_nModel;
int s_nBlocks;
int s_nDirect;
int s_nBlocksAlpha;
int s_nBlocksAlphaLast;
int s_nAnotherStudio;
int s_nUseStealth;
int s_nUseGnosys;
int s_nUseZwrite;
int s_nMapAlphaEntry;
int s_nEffectWrite;
int s_nMapLast;
int s_nPause;
int s_nMenu;
int s_nFrameLockOff;
int s_nMainCameraWarp;
int s_nRenderCancel;
int s_nRenderCancelOld;

MODELSYS_QUAD s_inGblPointC[4];
MODELSYS_QUAD s_inGblFogCol;
MODELSYS_QUAD s_inGblFogPara;

/* Resets every model-system global to its power-on state.
   PARKED (34 diffs, right length).  Blocked on the SAME TI-mode
   zero-store wall documented at Java_xeno_Chr_setPointLightReset__ in
   Java_Chr.c: the original stores $0 directly with `sq zero,0(a0)`, and
   gcc 2.96 always materialises the zero into a register first
   (`por a1,zero,zero`).  That extra def also lets the scheduler
   interleave the six quadword stores into the run of small-data `sw`s,
   which is where the rest of the diff comes from.  This function is a
   second consumer for the requested fix_cc_asm.py peephole (rewrite a
   `por $X,$0,$0` feeding only `sq` into a nop and retarget the stores to
   $0) -- worth 160 bytes here on top of the 132 in Java_Chr.c. */
void CONSTRUCT_MODELSYSTEM(void)
{
    g_aSubWindow.a00 = 2;
    g_aSubWindow.a0C = 0;
    s_nEffectWrite = 1;
    s_nModel = 0;
    s_nBlocks = 1;
    s_nDirect = 0;
    s_nBlocksAlpha = 0;
    s_nBlocksAlphaLast = 0;
    s_nAnotherStudio = 0;
    s_nUseStealth = 0;
    s_nUseGnosys = 0;
    s_nUseZwrite = 0;
    s_nMapAlphaEntry = 0;
    s_nMapLast = 0;
    s_nMainCameraWarp = 0;
    s_nRenderCancel = 0;
    s_nRenderCancelOld = 0;
    s_nFrameLockOff = 0;
    s_nPause = 0;
    s_nMenu = 0;
    g_aSubWindow.a04 = 0;
    g_aSubWindow.a08 = 0;
    s_inGblPointC[0] = 0;
    s_inGblPointC[1] = 0;
    s_inGblPointC[2] = 0;
    s_inGblPointC[3] = 0;
    s_inGblFogCol = 0;
    s_inGblFogPara = 0;
}
