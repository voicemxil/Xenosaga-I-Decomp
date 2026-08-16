/* Game loop helpers - state save/restore, resources, movies, pause and radar */

#include "matching.h"

typedef unsigned char u8;
typedef unsigned short u16;

typedef int TIWORD __attribute__((mode(TI)));

/* A 16-byte quadword the EE moves with a single lq/sq. */
typedef TIWORD GAME_QUAD;

/* Copied through `volatile GAME_QUAD *` locals: that is what keeps each
 * quadword address in its own register (`addiu tN,base,off` + `lq/sq
 * 0(tN)`). A plain member assignment folds the field offset into the
 * lq/sq instead, which is two instructions shorter than the original. */

typedef struct {
    int nFlags;              /* 0x000 */
    int nUnk004;             /* 0x004 */
    u8 pad008[0x10 - 0x8];   /* 0x008 */
    GAME_QUAD quad010;       /* 0x010 */
    u8 pad020[0x50 - 0x20];  /* 0x020 */
    GAME_QUAD quad050;       /* 0x050 */
    u8 pad060[0x84 - 0x60];  /* 0x060 */
    u16 nUnk084;             /* 0x084 */
    short nAlive;            /* 0x086 */
    u8 pad088[0xA70 - 0x88]; /* 0x088 */
} GAME_ACTOR;

typedef struct {
    u8 pad00[0x4];          /* 0x00 */
    GAME_ACTOR *pActor;     /* 0x04 */
    u8 pad08[0x4];          /* 0x08 */
    u16 nSaveA;             /* 0x0C */
    u16 nSaveB;             /* 0x0E */
    int nFlags;             /* 0x10 */
    u8 pad14[0x28 - 0x14];
    int nUnk28;              /* 0x28 */
    void (*pDrawFunc)(void);        /* 0x2C */
    void (*pDrawFunc2)(void);       /* 0x30 */
    u8 pad34[0x50 - 0x34];
    short nUnk50;             /* 0x50 */
    u8 pad52[0xC1 - 0x52];
    u8 nUnkC1;               /* 0xC1 */
    u8 padC2[0xC4 - 0xC2];
    unsigned short nUnkC4;    /* 0xC4 */
    u8 padC6[0xE0 - 0xC6];
    u16 nStateA;            /* 0xE0 */
    u16 nStateB;            /* 0xE2 */
    u8 pad1E4[0x1F0 - 0xE4];
    int nUnk1F0;              /* 0x1F0 */
    short nUnk1F4;             /* 0x1F4 */
    short nUnk1F6;             /* 0x1F6 */
    int nUnk1F8;               /* 0x1F8 */
    u8 pad1FC[0x200 - 0x1FC];
    GAME_QUAD quad200;          /* 0x200 */
    GAME_QUAD quad210;          /* 0x210 */
    short nUnk220;              /* 0x220 */
    u8 pad222[0x230 - 0x222];
    int nUnk230;               /* 0x230 */
    u8 pad234[0x240 - 0x234];
    TIWORD quad240;             /* 0x240 */
    u8 pad250[0x29F40 - 0x250];
    u8 nCfEventLock;            /* 0x29F40 */
    u8 pad29F41[0x29F50 - 0x29F41];
    GAME_QUAD quad29F50;        /* 0x29F50 */
    u8 pad29F60_[0x29F60 - 0x29F60];
    unsigned short nUnk29F60;   /* 0x29F60 */
} GAME_LOOP_STATE;

typedef struct {
    u8 nType;               /* 0x00 */
    u8 nUnk01;              /* 0x01 */
    u8 nId;                 /* 0x02 */
    u8 pad03;               /* 0x03 */
    int aParam[16];         /* 0x04 */
} GAME_DEFOCUS;

typedef struct {
    int nId;                /* 0x00 */
    int nUnk04;
    int nUnk08;
    int nUnk0C;
} GAME_RESOURCE;

typedef struct {
    u8 nUnk00;              /* 0x00 */
    u8 pad01[2];
    signed char nUnk03;     /* 0x03 */
    short nUnk04;           /* 0x04 */
    short nUnk06;           /* 0x06 */
    short nUnk08;           /* 0x08 */
    short nUnk0A;           /* 0x0A */
} BG_DRAW_PARAM;

typedef struct { long long pad[0x1E]; } LIGHT_ENV;

typedef struct {
    u8 pad0[0x40];
    short nActive;          /* 0x40 */
    u8 pad42[0x90 - 0x42];  /* 0x42 */
    int nBuf0;              /* 0x90 */
    int nNo;                /* 0x94 */
    u8 pad98[0x9A - 0x98];  /* 0x98 */
    u8 nUnk9A;              /* 0x9A */
    u8 pad9B[0xB0 - 0x9B];  /* 0x9B */
    int nBuf1;              /* 0xB0 */
    int nBuf2;              /* 0xB4 */
    u8 padB8[0xC9 - 0xB8];  /* 0xB8 */
    u8 nUnkC9;              /* 0xC9 */
    u8 nUnkCA;              /* 0xCA */
    u8 nUnkCB;              /* 0xCB */
} MOVIE_INFO;

extern GAME_LOOP_STATE GameLoopState;
extern GAME_DEFOCUS GameDefocusParam[];
extern GAME_RESOURCE GameResource[];
extern BG_DRAW_PARAM GameBgDrawType1Param;
extern LIGHT_ENV D_003624D0;
extern MOVIE_INFO mi;
extern unsigned char GameResourceWorkReloadTable[];
extern int D_00338698[];
extern char D_004C0618[];
extern unsigned int ShadowEnv[];

char *snapnameno;
int WorkEnd;
int image_004DC554;
unsigned char rate;
int WALK_THRESHOLD_I;
float WALK_THRESHOLD_F;
int RUN_THRESHOLD_I;
float RUN_THRESHOLD_F;
float VECTOR_RATE;
float D_004D7C0C;

int getScriptFlag(void *pScript)
{
    return *(int *)((char *)pScript + 0x124);
}

extern void xglSoundSendEffect(int a, int b, int c);
extern void GameCameraStateSave(void);
extern void GameCameraStateRestore(void);
extern LIGHT_ENV *xglStudioGetLight2(void);
extern void GameStateRestoreKoware(void);
extern void *xglPacketGetCurrent(void);
extern void sceVif1PkRef(void *pkt, unsigned int a1, unsigned int a2, unsigned int a3, unsigned int t0, unsigned int t1);
extern int xglFontGetStringWidth(const char *s);
extern void xglFontPrint(int x, int y, int z, const char *s);
extern int resource_get_free(void);
extern void ACT_init(void);
extern void GameResourceLoad(int a);
extern void MapChangeFadeSet(void);
extern void DefocusMainType09Final(GAME_DEFOCUS *p);
extern void xglMovieClose(MOVIE_INFO *p);
extern void xglMovieOpen(MOVIE_INFO *p, int a);
extern void GameBgDrawType1(void);
extern void nullfunc(void);
extern int xglCdReadFile(char *name, int addr, int a2, int a3);

/* The saved copy of studio camera 0, restored when a cinematic ends */
typedef struct {                 /* 0x5F0 */
    int nUnk00;                    /* 0x000 */
    int nUnk04;                     /* 0x004 */
    long long pad08[20];             /* 0x008 */
    int nUnkA8;                       /* 0x0A8 */
    int nUnkAC;                        /* 0x0AC */
    long long padB0[168];               /* 0x0B0 */
} SAVE_CAM;

extern SAVE_CAM save_cam;
extern SAVE_CAM *xglStudioGetCamera2(int nCamera);
extern TIWORD D_00362EB0;

/* Save studio camera 0 and the current cinematic camera definition */
void GameCameraStateSave(void)
{
    save_cam = *xglStudioGetCamera2(0);
    GameLoopState.quad240 = D_00362EB0;
    GameLoopState.nUnk230 = GameLoopState.nUnk28;
}

extern SAVE_CAM *xglStudioGetActiveCamera(void);

/* Camera interpolation state, cleared when the saved camera is restored */
typedef struct {
    int nUnk00;
    int nUnk04;
    int nUnk08;
    int nUnk0C;
} HOKAN;

extern HOKAN HokanNow;

/* TODO: near-miss, 41 diffs of 57 words, RIGHT LENGTH -- pure register
 * allocation. The shape is settled: the goto-form loop is what produces
 * the original's `beqz` + unconditional `b` (a for(;;)/break gave a
 * `bnezl` and cost two words). Residue: the original spends TEN registers
 * on the 0x5F0 block move (bound v0, loop temps v1/a1/t0/t1, tail temps
 * t2/t3, src walker a2, dst walker a0, camera pointer a3); gcc here
 * coalesces the tail temps with the loop temps and spends only eight, so
 * every role shifts. Not a permutation of the original's registers, so
 * --swap-regs cannot reach it. Swept: all six orderings of the quad
 * copy / GameLoopState scalar / HokanNow.nUnk0C block (41-42), declaring
 * pActive first, splitting the declaration from the call, and typing
 * pActive as int * -- all 42. */
/* Restore studio camera 0 from the snapshot GameCameraStateSave() took and
 * drop every active camera back to camera 0 */
void GameCameraStateRestore(void)
{
    SAVE_CAM *p = xglStudioGetCamera2(0);
    SAVE_CAM *pActive;

    *p = save_cam;
    p->nUnk04 = 4;
    p->nUnkA8 = 0;
    D_00362EB0 = GameLoopState.quad240;
    HokanNow.nUnk0C = 0;
    GameLoopState.nUnk28 = GameLoopState.nUnk230;
    HokanNow.nUnk00 = 0;
    HokanNow.nUnk04 = 0;
    HokanNow.nUnk08 = 0;
loop:
    /* goto form: the original branches back with an unconditional `b` */
    pActive = xglStudioGetActiveCamera();
    if (pActive != 0) {
        pActive->nUnk00 = 0;
        goto loop;
    }
    xglStudioGetCamera2(0)->nUnk00 = 1;
}

extern void xglSoundLoadEffect(char *pName, int nAddr, int nNo);
extern char D_004BE2B0[];

/* Drop the cinematic banks, hand the segment allocator the cinematic
 * window back and reload the shared effect bank at the top of the work
 * area */
void GameCFSoundReload(void)
{
    int nAddr = (WorkEnd + 63) & -64;
    int i;
    /* the loop-bound compare lands in $v0 without this */
    PIN(int more, "$3");

    i = 0;
    do {
        xglSoundSendSwd(0, -1 - i);
        xglSoundSendSmd2(0, i);
        i++;
        more = (i < 8);
    } while (more);
    SsdResetSegmentAllocMode(0xA8000);
    SsdSetSegmentAllocMode(0x70000, 0x10000);
    xglSoundLoadEffect(D_004BE2B0, nAddr, 1);
    GameCFSoundPurgeSub();
}

extern unsigned int GameResourceAlloc(unsigned int nSize);
extern void xglMovieInfoInit(MOVIE_INFO *p);
char GameMovieAlpha;
char GameMovieTransparent;

/* Reserve the movie work area and point the movie info block at its three
 * buffers */
void GameMovieInit(int nNo)
{
    int nSize = nNo * 2;
    int nAddr;

    nAddr = GameResourceAlloc(0x10000 + nSize);
    xglMovieInfoInit(&mi);
    mi.nBuf0 = nAddr;
    nAddr += nSize;
    mi.nBuf1 = nAddr;
    nAddr = 0x8000 + nAddr;
    /* PERM_BEGIN */
    mi.nUnk9A = 1;
    mi.nUnkCB = 0;
    mi.nNo = nNo;
    mi.nBuf2 = nAddr;
    mi.nUnkC9 = 0;
    mi.nUnkCA = 0;
    /* PERM_END */
    GameMovieAlpha = -128;
    GameMovieTransparent = 0;
}

/* Purge every cinematic sound-effect slot */
void GameCFSoundPurgeSub(void)
{
    int i;

    xglSoundSendEffect(0, 0, 2);
    xglSoundSendEffect(0, 0, 3);
    for (i = 0; i < 8; i++) {
        xglSoundSendEffect(0, 0, i + 4);
    }
}

extern void xglSoundSendSwd(void *pData, int nNo);
extern void xglSoundSendSmd2(void *pData, int nNo);
extern void GameCFSoundPurgeSub(void);
extern int SsdResetSegmentAllocMode(int nMode);
extern int SsdSetSegmentAllocMode(int nMode, int nSize);

/* Drop every cinematic sound bank and hand the segment allocator back its
 * default window */
void GameCFSoundPurge(void)
{
    int i;
    /* the loop-bound compare lands in $v0 without this */
    PIN(int more, "$3");

    i = 0;
    do {
        xglSoundSendSwd(0, -1 - i);
        xglSoundSendSmd2(0, i);
        i++;
        more = (i < 8);
    } while (more);
    xglSoundSendEffect(0, 0, 1);
    GameCFSoundPurgeSub();
    SsdResetSegmentAllocMode(0x70000);
    SsdSetSegmentAllocMode(0xA8000, 0x10000);
}

/* Snapshot the camera and lighting environment */
void GameStateSaveCameraLight(void)
{
    GameCameraStateSave();
    D_003624D0 = *xglStudioGetLight2();
}

/* Restore the camera and lighting environment */
void GameStateRestoreCameraLight(void)
{
    GameCameraStateRestore();
    *xglStudioGetLight2() = D_003624D0;
}

extern void GameStateSaveCameraLight(void);

/* TODO: near-miss (26/30 words, SAME LENGTH -- scheduling + a register
 * cascade, no missing/extra instruction). Swept: plain member assignment
 * of a mode(TI) field (folds the offset into the lq/sq: 2 words short),
 * a 16-byte union/struct field (emit_block_move picks DImode ld/sd pairs
 * -- MOVE_MAX is 8 here so move_by_pieces never reaches TImode), a
 * volatile-cast copy macro (unfolds only the STORE address), one shared
 * vs two distinct pointer-pair locals, and an explicit GAME_LOOP_STATE*
 * base local -- all land on the same 26. What is left: the original
 * schedules all four scalar actor loads/stores BEFORE the first lq, and
 * holds the loop-state base in $a0 rather than $v0. */
/* Snapshot the game loop state: window flags and the player actor's
 * transform, then the camera and lighting */
void GameStateSave(void)
{
    GAME_ACTOR *p = GameLoopState.pActor;

    GameLoopState.nStateA = GameLoopState.nSaveA;
    GameLoopState.nStateB = GameLoopState.nSaveB;
    GameLoopState.nUnk1F0 = p->nFlags;
    GameLoopState.nUnk1F8 = p->nUnk004;
    GameLoopState.nUnk1F4 = p->nUnk084;
    GameLoopState.nUnk1F6 = p->nAlive;
    {
        volatile GAME_QUAD *pSrc = &p->quad010;
        volatile GAME_QUAD *pDst = &GameLoopState.quad200;

        *pDst = *pSrc;
    }
    {
        volatile GAME_QUAD *pSrc = &p->quad050;
        volatile GAME_QUAD *pDst = &GameLoopState.quad210;

        *pDst = *pSrc;
    }
    GameLoopState.nUnk220 = GameLoopState.nUnk50;
    GameStateSaveCameraLight();
}

/* Restore the saved game loop state */
void GameStateRestore(void)
{
    GameLoopState.nSaveA = GameLoopState.nStateA;
    GameLoopState.nSaveB = GameLoopState.nStateB;
    GameStateRestoreCameraLight();
    GameStateRestoreKoware();
}

/* Draw the pause screen backdrop */
void GamePauseDispBG(void)
{
    sceVif1PkRef(xglPacketGetCurrent(), (unsigned int)ShadowEnv, 7, 0, 0, 0);
}

/* Draw the pause screen caption */
void GamePauseDispCf(void)
{
    static char pause[] = "\x0b\x0d\x03\x19\x03PAUSE\x19\x02";

    xglFontPrint(0x100 - xglFontGetStringWidth(pause) / 2, 200, 0xFFFFFF, pause);
    GamePauseDispBG();
}

extern int F2I(float f);

/* Set the player movement speed thresholds */
void GameCfPlayerMoveParamSet(float walk, float run, float vectorRate)
{
    WALK_THRESHOLD_I = F2I(walk);
    WALK_THRESHOLD_F = walk;
    RUN_THRESHOLD_I = F2I(run);
    RUN_THRESHOLD_F = run;
    VECTOR_RATE = vectorRate;
}

/* Initialise the player movement speed thresholds */
void GameCfPlayerMoveInit(void)
{
    WALK_THRESHOLD_I = 32;
    WALK_THRESHOLD_F = 32.0f;
    RUN_THRESHOLD_I = 96;
    RUN_THRESHOLD_F = 96.0f;
    VECTOR_RATE = D_004D7C0C;
}

extern unsigned char GameResourceWorkReloadLeader;

/* Find the first free (nUnk0C == -1) resource slot; if its capacity fits
 * nSize, hand back its id, otherwise fall through to a full realloc */
unsigned int GameResourceWorkAlloc(int nSize)
{
    GAME_RESOURCE *p = GameResource;
    int i;

    GameResourceWorkReloadLeader = 0;
    GameResourceWorkReloadTable[0] = 0;
    for (i = 0; i < 0x80; i++) {
        if (p->nUnk0C == -1) {
            if (nSize < p->nUnk04) {
                return p->nId;
            }
            goto fail;
        }
        p++;
    }
fail:
    GameResourceWorkReloadTable[0] = 1;
    return 0x112A000;
}

/* Address of the first free resource slot, clamped to the work area base */
unsigned int GameResourceGetFreeAddr(void)
{
    unsigned int addr = GameResource[resource_get_free()].nId;

    if (addr > 0x1129FFF) {
        return addr;
    }
    return 0x112A000;
}

/* Reload the actor work area, or just kick the map fade */
void GameResourceWorkReload(void)
{
    if (GameResourceWorkReloadTable[0] != 0) {
        ACT_init();
        GameResourceLoad(D_00338698[0]);
    } else {
        MapChangeFadeSet();
    }
}

/* Find the resource slot holding the given id */
int GameResourceGetIndex(int id)
{
    GAME_RESOURCE *p = GameResource;
    int i = 0;
    int cur;

loop:
    if (i < 0x80) {
        cur = p->nId;
        p++;
        if (cur == id) {
            return i;
        }
        i++;
        if (cur != 0) {
            goto loop;
        }
    }
    return -1;
}

/* Snapshot number accessor - stores into the file name template */
int GameSnapShotNumber(int no)
{
    if (no >= 0) {
        *snapnameno = no + '0';
    }
    return *snapnameno - '0';
}

/* Snapshot request poll (stubbed out in the retail build) */
void GameSnapShotCheck(void)
{
}

/* Tear down one defocus effect slot */
void GameDefocusFinalize(int id)
{
    GAME_DEFOCUS *p = &GameDefocusParam[id];

    if (p->nType == 9) {
        DefocusMainType09Final(p);
    }
    p->nId = id;
    p->nUnk01 = 0;
    p->nType = 0;
}

extern void DefocusMain(GAME_DEFOCUS *p);

/* Game.c's view of xgl's shared render state - only the post-effect hook */
typedef struct {
    u8 pad00[0x14];                     /* 0x00 */
    unsigned short nDrawFbp;            /* 0x14 */
    u8 pad16[0x34 - 0x16];              /* 0x16 */
    void (*pDefocusFunc)(GAME_DEFOCUS *); /* 0x34 */
    u8 pad38[0x5C - 0x38];              /* 0x38 */
} GAME_RENDER;

extern GAME_RENDER sRender;

/* Install (or clear) the per-frame defocus driver depending on whether any
 * of the sixteen defocus slots is in use */
void GameDefocusCheck(void)
{
    int i;
    int flag;

    flag = 0;
    i = 0;
    if (GameDefocusParam[0].nType == 0) {
        /* goto-loop: the original leaves this loop unoptimized (the base
         * address is recomputed every iteration and never strength-reduced),
         * which only happens without a NOTE_INSN_LOOP_BEG */
    loop:
        i++;
        if (i >= 16) {
            goto done;
        }
        if (GameDefocusParam[i].nType == 0) {
            goto loop;
        }
    }
    flag = 1;
done:
    if (flag == 0) {
        sRender.pDefocusFunc = 0;
        return;
    }
    sRender.pDefocusFunc = DefocusMain;
}

/* Stop the currently playing movie */
void GameMovieStop(void)
{
    xglMovieClose(&mi);
    mi.nActive = 0;
}

/* Start playing a movie, closing any previous one */
void GameMoviePlay(int id)
{
    if (mi.nActive != 0) {
        xglMovieClose(&mi);
    }
    xglMovieOpen(&mi, id);
}

/* Background draw handler for type 0 (nothing to draw) */
void GameBgDrawType0(void)
{
}

/* Shared do-nothing callback */
void nullfunc(void)
{
}

/* Install the type 1 background draw handler */
void GameBgDrawType1Entry(void)
{
    GameBgDrawType1Param.nUnk00 = 0;
    GameBgDrawType1Param.nUnk03 = -0x10;
    GameBgDrawType1Param.nUnk04 = -0x8000;
    GameBgDrawType1Param.nUnk06 = -0x8000;
    GameBgDrawType1Param.nUnk08 = 0;
    GameBgDrawType1Param.nUnk0A = 0;
    GameLoopState.pDrawFunc = GameBgDrawType1;
    GameLoopState.pDrawFunc2 = nullfunc;
}

/* Load the radar overlay image into the work area */
void GameRadarInit(void)
{
    int addr = (WorkEnd + 15) & -16;

    image_004DC554 = addr;
    WorkEnd = image_004DC554 + xglCdReadFile(D_004C0618, addr, 0, 0);
    rate = 0;
}

extern GAME_ACTOR actor[];
extern void ACT_DrawShadowBegin(void);
extern void ACT_DrawShadowEnd(void);
extern void ACT_DrawShadow(GAME_ACTOR *p);

/* Draw shadows for every live, non-hidden, shadow-enabled actor */
void GameDrawShadow(void)
{
    GAME_ACTOR *p = actor;
    int i;

    ACT_DrawShadowBegin();
    for (i = 0x3F; i >= 0; i--, p++) {
        int flags;

        if (p->nAlive == 0) continue;
        flags = p->nFlags;
        if ((flags & 8) != 0) continue;
        if ((flags & 0x20) == 0) continue;
        ACT_DrawShadow(p);
    }
    ACT_DrawShadowEnd();
}

/* Draw the two pause-screen button hint captions, each centred on screen */
void GamePauseDispEvent(void)
{
    static char msg1[] = "\x0b\x0d\x03\x0c\x20\x80\x20\xa2\xa4\x0c\x80\x80\x80\x19\x03\x20""Button : Skip and proceed\x19\x02";
    static char msg2[] = "\x0b\x0d\x03\x19\x03""START BUTTON : Cancel PAUSE\x19\x02";
    int w;

    GamePauseDispCf();
    w = xglFontGetStringWidth(msg1);
    xglFontPrint(0x100 - w / 2, 0x100, 0xFFFFFF, msg1);
    w = xglFontGetStringWidth(msg2);
    xglFontPrint(0x100 - w / 2, 0x120, 0xFFFFFF, msg2);
}

extern int UnduDataGetHeader(int a, int b);

typedef struct { int f0; int f4; u8 pad8[0x18 - 0x8]; int f18; } CAM_VEC;
extern CAM_VEC CamLookAt;
extern CAM_VEC CamPos;

/* Re-point the camera lookat/pos at a new undu data header, falling back to
 * a default id if the requested one has no header */
void GameCameraChangeID(int id)
{
    int h;

    if (GameLoopState.nUnkC4 != id) {
        h = UnduDataGetHeader(0x100, id);
        if (h == 0) {
            h = UnduDataGetHeader(0x100, 0x8000);
            id = 0x8000;
        }
        GameLoopState.nUnkC1 |= 0x40;
        GameLoopState.nUnkC4 = id;
        CamPos.f18 = h;
        CamPos.f4 = 0;
        CamPos.f0 = 0;
        CamLookAt.f18 = h;
        CamLookAt.f4 = 0;
        CamLookAt.f0 = 0;
    }
}

extern void GameResourceReset(int nNo);
extern void __gnu_compiled_c_0024A378(void);
extern void *GameResourceReadFileCallback;

/* Clear every resource slot, seed slot 0 with the given id/data, and wipe
 * the IOP-side resource module's cached state pointers */
void GameResourceInit(int id, int data)
{
    int clr;
    GAME_RESOURCE *p;
    int i;

    clr = -1;
    p = GameResource;
    for (i = 0x7F; i >= 0; i--, p++) {
        p->nId = 0;
        p->nUnk04 = 0;
        p->nUnk08 = 0;
        p->nUnk0C = clr;
    }
    GameResource[0].nId = id;
    GameResource[0].nUnk04 = data;
    GameResourceReset(0);
    GameResourceReadFileCallback = (void *)__gnu_compiled_c_0024A378;
    *(int *)0x1000000 = 0;
    *(int *)0x1070800 = 0;
    *(int *)0x10B1000 = 0;
    *(int *)0x10B4000 = 0;
    *(int *)0x10E6000 = 0;
    *(int *)0x10E7000 = 0;
}

typedef struct {
    short nUnk00;   /* 0x00 */
    char  nUnk02;   /* 0x02 */
    char  pad03;    /* 0x03 */
    char  nUnk04;   /* 0x04 */
    char  pad05[0x14 - 0x05];
} GAME_SNDCH;

extern GAME_SNDCH D_004DCB40[8];
extern int arcfilepreload;
extern void GameResourceDump(int nFlag);

/* TODO: near-miss (LOGIC per triage.py, 28/71 words differ) -- summed-block
 * pointer arithmetic and the nNo+1 bound recompute are structured
 * differently from source here (register alloc + block-duplication shape),
 * not just a scheduling tie-break. Parked per budget rule after 1 triage
 * pass; logic/results are believed correct. */
/* Reset resource slot nNo: sum the still-referenced data sizes of every
 * slot from nNo on into it, clear the rest of the table past it, and
 * reset the cinematic sound-effect channel array */
void GameResourceReset(int nNo)
{
    GAME_RESOURCE *p;
    int i;
    int sum;

    sum = 0;
    if (nNo < 0x80) {
        for (i = nNo; i < 0x80; i++) {
            sum += GameResource[i].nUnk04;
        }
    }
    p = &GameResource[nNo];
    p->nUnk04 = sum;
    p->nUnk0C = -1;
    p->nUnk08 = 0;
    if (nNo + 1 < 0x80) {
        for (i = nNo + 1; i < 0x80; i++) {
            GameResource[i].nId = 0;
            GameResource[i].nUnk04 = 0;
            GameResource[i].nUnk08 = 0;
            GameResource[i].nUnk0C = -1;
        }
    }
    for (i = 0; i < 8; i++) {
        D_004DCB40[i].nUnk00 = 0;
        D_004DCB40[i].nUnk02 = 0;
        xglSoundSendEffect(0, 0, i + 4);
        D_004DCB40[i].nUnk04 = 0;
    }
    GameResourceDump(0);
    arcfilepreload = 0;
}

extern int arcfilepreload;
extern char scene_txt_buffer[];
extern char *RES_getScenePath(char *pPath, int nScene);
extern int xglCdGetFileSize(char *pPath);

/* Stage the scene script and its archive at the top of memory before the
   scene itself is built. */
void GameResourcePreLoad(int nScene)
{
    char szPath[256];
    char *pExt;
    int nSize;
    int nAddr;

    pExt = RES_getScenePath(szPath, nScene);
    arcfilepreload = 0;
    if (xglCdGetFileSize(szPath) > 0) {
        xglCdReadFile(szPath, (int)scene_txt_buffer, 1, 1);
        pExt[0] = 'a';
        pExt[1] = 0;
        nSize = xglCdGetFileSize(szPath);
        if (nSize > 0) {
            /* Written inside the guard: gcc hoists the address arithmetic
               above the branch on its own, and writing it above the guard
               instead gives the mask and the 0x2000000 base the wrong
               registers. */
            nAddr = 0x2000000 - ((nSize + 2047) & -2048);
            arcfilepreload = nAddr;
            xglCdReadFile(szPath, nAddr, 1, 1);
        }
    }
}

typedef unsigned long GAME_U64;

extern void FlushCache(int nMode);

/* The DIRECT packet that paints the saved back buffer over the frame:
   word 6 is its BITBLTBUF source pointer, word 8 its TEX0 alpha. */
static GAME_U64 BackEnv[20] = {
    0x0UL, 0x5000000900000000UL,
    0x80AB400000008001UL, 0x53531E6EUL,
    0x30000UL, 0x47UL,
    0x0UL, 0x6UL,
    0x0UL, 0x42UL,
    0x8000000080UL, 0x8000000080UL,
    0x0UL, 0x0UL,
    0x71F800006FF8UL, 0x0UL,
    0x1C0000002000UL, 0x0UL,
    0x8DF800008FF8UL, 0x0UL,
};

void DrawBack(int nAlpha)
{
    if (nAlpha < 0) {
        nAlpha = 0;
    }
    /* The framebuffer pointer is read inline: a named local for it costs
       the register tie-break here (the opposite of DrawBackSet, where the
       load has to be staged in one). */
    BackEnv[6] = (0x24120000 | (sRender.nDrawFbp << 5)) | 0x640000000UL;
    BackEnv[8] = ((GAME_U64)nAlpha << 32) | 100;
    FlushCache(0);
    sceVif1PkRef(xglPacketGetCurrent(), BackEnv, 10, 0, 0, 0);
}

typedef struct {
    u8 pad00[0x28];             /* 0x00 */
    unsigned short nButton;     /* 0x28 */
    unsigned short nPress;      /* 0x2A */
    u8 pad2C[0x2E - 0x2C];      /* 0x2C */
    unsigned short nDebugPress; /* 0x2E */
    u8 pad30[0x68 - 0x30];      /* 0x30 */
} GAME_PADDATA;

extern GAME_PADDATA PadData[2];
extern void TWSYS_update(void);
extern void JTHREAD_cntl(void);
extern void PLAY_ctrl(void);
extern void TCAMERA_update(void);
extern void ACT_update(void);
extern void MAP_updateUnit(void);

/* Event-scene game mode: L1+L2+SELECT aborts the scene. */
int GameModeCfEvent(void)
{
    int nFlags = GameLoopState.nFlags & ~1;

    GameLoopState.nFlags = nFlags;
    if (GameLoopState.nCfEventLock == 0 && (PadData[0].nPress & 0x800) &&
        (PadData[0].nButton & 0x10C) == 0x10C) {
        GameLoopState.nFlags = nFlags | 0x80000000;
        return 1;
    }
    TWSYS_update();
    JTHREAD_cntl();
    PLAY_ctrl();
    TCAMERA_update();
    ACT_update();
    MAP_updateUnit();
    return 0;
}

extern void ACT_pauseUpdate(void);
extern void GameDebugMenu(void);

/* NEAR MISS -- 8 diffs, SCHEDULING (identical multiset). gas's reorder
   pass hoists the `lw` of nFlags above the quadword-zero store's address
   macro, and sinks the `and` below the nSaveA load; the original keeps
   both where gcc emitted them. Swept: all six orderings of the three
   guarded statements, `&=` vs explicit read-modify-write, a staged int
   local for the flags, and plain/volatile GAME_QUAD* block-local
   pointers for the quad store (the pointer forms cost 38 diffs, LENGTH).
   The gcc -S emission order already matches the original -- the residue
   is entirely gas, so a source lever cannot reach it; it wants
   --rotate/--rotate-seq if it is worth a flag later.

   Debug-menu game mode: SELECT re-arms the scene skip, then the debug
   menu runs on top of a paused actor update. */
int GameModeDebugMenu(void)
{
    if (PadData[0].nDebugPress & 0x100) {
        GameLoopState.quad29F50 = 0;
        GameLoopState.nFlags &= ~1;
        GameLoopState.nSaveA = GameLoopState.nUnk29F60;
    }
    TWSYS_update();
    ACT_pauseUpdate();
    GameDebugMenu();
    if (((int *)xglStudioGetCamera2(0))[1] == 4) {
        void (*pFunc)(void) = (void (*)(void))GameLoopState.nUnk28;

        if (pFunc != 0) {
            pFunc();
        }
    }
    return 0;
}

extern unsigned char GameCFSoundMenuPurgeFlag;
extern int UmnSimulationNo;
extern int MenuDrillCall;
extern void RES_GetMapEnvSeName(char *pName);
extern char *RES_GetEnemySeName(int nIndex);
extern void xglSoundSequenceNormal2(int nSeq, int nVolume);

/* Reload the menu-side sound set: the map ambience bank, any sequences the
   purge flag says were playing, and every enemy effect bank */
void GameCFSoundMenuReload(void)
{
    char szName[64];
    int nAddr;
    int i;
    char *pName;
    int nSlot;

    nAddr = GameResourceGetFreeAddr();
    xglSoundSendSwd(0, -5);
    xglSoundSendSmd2(0, 4);
    RES_GetMapEnvSeName(szName);
    xglSoundLoadEffect(szName, nAddr, 3);
    for (i = 0; i < 8; i++) {
        if (((GameCFSoundMenuPurgeFlag >> i) & 1) && UmnSimulationNo == 0 &&
            MenuDrillCall == 0) {
            xglSoundSequenceNormal2(i, 127);
        }
    }
    i = 4;
    while (1) {
        pName = RES_GetEnemySeName(i);
        nSlot = i + 4;
        i++;
        if (pName == 0 || *pName == 0) {
            break;
        }
        xglSoundLoadEffect(pName, nAddr, nSlot);
    }
}

typedef struct {
    unsigned short nUnk00;      /* 0x00 */
    unsigned short nId;         /* 0x02 */
} GAME_SOUNDENTRY;

extern GAME_SOUNDENTRY SoundWork[];
extern int SsdGetSeqPlayStatus(int nId);
extern int SsdGetResultValue(int *pValue);
extern void xglSoundSequenceStop2(int nSeq);
extern void EnemySound_StopAll(int nType);

/* Stop the menu-side sound set, remembering in the purge flag which
   sequences were still playing so the reload can restart them */
void GameCFSoundMenuPurge(int nMode)
{
    unsigned short nStatus[8];
    int i;
    int nStat;

    if (nMode == 1) {
        GameCFSoundMenuPurgeFlag = 0;
        xglSoundSendEffect(0, 0, 3);
        for (i = 0; i < 8; i++) {
            /* Both the sequence id and the polled status go through their
               own named locals: letting CSE invent the temporaries costs
               an extra `move` at each site (and the block scope on nId is
               what keeps it in $a0 rather than a callee-saved). */
            {
                int nId = SoundWork[i].nId;

                if (nId != 0xFFFF) {
                    SsdGetSeqPlayStatus(nId);
                    do {
                    } while (SsdGetResultValue((int *)nStatus) < 0);
                    nStat = nStatus[0];
                    if (nStat == 1) {
                        GameCFSoundMenuPurgeFlag |= nStat << i;
                        xglSoundSequenceStop2(i);
                    }
                }
            }
        }
    }
    EnemySound_StopAll(1);
    for (i = 4; i < 8; i++) {
        xglSoundSendEffect(0, 0, i + 4);
    }
}

extern void xglSoundEffectNormalDirect(int nCode);
extern void EvtTools(void);

/* Event-tools game mode: SELECT arms the skip, L1+L2+SELECT aborts. */
int GameModeEvtTools(void)
{
    if (GameLoopState.nCfEventLock == 0 && (PadData[0].nPress & 0x800)) {
        GameLoopState.nCfEventLock = 16;
        if ((PadData[0].nButton & 0x10C) == 0x10C) {
            GameLoopState.nFlags |= 0x80000000;
            return 1;
        }
        xglSoundEffectNormalDirect(4);
        GameLoopState.nFlags &= ~1;
        GameLoopState.quad29F50 = 0;
        GameLoopState.nSaveA = GameLoopState.nUnk29F60;
    }
    TWSYS_update();
    EvtTools();
    if (((int *)xglStudioGetCamera2(0))[1] == 4) {
        void (*pFunc)(void) = (void (*)(void))GameLoopState.nUnk28;

        if (pFunc != 0) {
            pFunc();
        }
    }
    return 0;
}

/* Event-debug game mode: same skip handling as the tools mode, without the
   tools update */
int GameModeEvtDebug(void)
{
    if (GameLoopState.nCfEventLock == 0 && (PadData[0].nPress & 0x800)) {
        GameLoopState.nCfEventLock = 16;
        if ((PadData[0].nButton & 0x10C) == 0x10C) {
            GameLoopState.nFlags |= 0x80000000;
            return 1;
        }
        xglSoundEffectNormalDirect(4);
        GameLoopState.nFlags &= ~1;
        GameLoopState.quad29F50 = 0;
        GameLoopState.nSaveA = GameLoopState.nUnk29F60;
    }
    TWSYS_update();
    if (((int *)xglStudioGetCamera2(0))[1] == 4) {
        void (*pFunc)(void) = (void (*)(void))GameLoopState.nUnk28;

        if (pFunc != 0) {
            pFunc();
        }
    }
    return 0;
}

extern void PartyTimePauseEnd(void);
extern void PauseMenu(void);

/* Paused game mode */
int GameModePause(void)
{
    if (GameLoopState.nCfEventLock == 0 && (PadData[0].nPress & 0x800)) {
        GameLoopState.nCfEventLock = 16;
        if ((PadData[0].nButton & 0x10C) == 0x10C) {
            GameLoopState.nFlags |= 0x80000000;
            return 1;
        }
        xglSoundEffectNormalDirect(4);
        GameLoopState.nFlags &= ~1;
        GameLoopState.quad29F50 = 0;
        GameLoopState.nSaveA = GameLoopState.nUnk29F60;
        PartyTimePauseEnd();
    }
    TWSYS_update();
    PauseMenu();
    if ((unsigned short)(GameLoopState.nUnk29F60 - 2) < 2) {
        ACT_pauseUpdate();
    }
    if (((int *)xglStudioGetCamera2(0))[1] == 4) {
        void (*pFunc)(void) = (void (*)(void))GameLoopState.nUnk28;

        if (pFunc != 0) {
            pFunc();
        }
    }
    return 0;
}

extern void xglFontDebugPrintf(int x, int y, const char *pFmt, ...);
extern char D_004C01B0[];

/* TODO: near-miss, 14 diffs of the right 58 words. The pointer form of
 * the backward scan (a `q = &GameResource[127]` local, walked with `p--`)
 * is what gets the length right -- the array-index forms are 34-40. What
 * is left: the original materialises the scan base as %hi/%lo(GameResource)
 * plus a SEPARATE `addiu +2032` (an LSR giv initial value) where gcc folds
 * 2032 into the addiu, and its bottom test is `bnezl` where gcc emits a
 * plain `bnez` with the same (unannulled) delay slot. Swept: GameResource
 * + 127, a (char *) byte offset, a variable index, and re-using p before
 * the loop (49). LAUNDER(q) gets to 13 and is not worth the steering. */
/* Debug overlay: list sixteen resource-table entries ending at the last
 * used one. */
void GameResourceDump(int nEnable)
{
    GAME_RESOURCE *p;
    int i;
    int j;
    int nY;

    if (nEnable != 0) {
        GAME_RESOURCE *q = &GameResource[127];

        i = 127;
        if (q->nId == 0) {
            p = q;
            do {
                p--;
                i--;
                if (p->nId != 0) {
                    break;
                }
            } while (i != 0);
        }
        i -= 14;
        if (i < 0) {
            i = 0;
        }
        nY = 64;
        j = 15;
        p = &GameResource[i];
        do {
            xglFontDebugPrintf(64, nY, D_004C01B0, i, p->nId, p->nUnk04,
                               p->nUnk0C, p->nUnk08);
            p++;
            j--;
            i++;
            nY += 8;
        } while (j >= 0);
    }
}

extern int printf(const char *pFmt, ...);
extern char D_004C0200[];

/* Arm defocus slot nId with a type and sixteen parameter words; a negative
 * slot tears every slot down instead. */
void GameDefocusSet(int nId, int nType, int *pParam)
{
    int i;

    if (nId >= 16) {
        printf(D_004C0200, nId);
        return;
    }
    if (nId < 0) {
        for (i = 0; i < 16; i++) {
            GameDefocusFinalize(i);
        }
    } else {
        GAME_DEFOCUS *p = &GameDefocusParam[nId];

        GameDefocusFinalize(nId);
        p->nType = nType;
        if (nType > 0) {
            int *pDst = p->aParam;
            int nOfs = 0;
            int j = 15;

            do {
                if (pParam == 0) {
                    *pDst = 0;
                } else {
                    /* offset first, and through an integer cast: the
                       commutative addu's operand order follows the
                       source, and `(char *)pParam + nOfs` gives the
                       mirror image */
                    *pDst = *(int *)(nOfs + (int)pParam);
                }
                j--;
                pDst++;
                nOfs += 4;
            } while (j >= 0);
        }
    }
    GameDefocusCheck();
}
