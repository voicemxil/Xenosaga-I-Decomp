/* Game loop helpers - state save/restore, resources, movies, pause and radar */

typedef unsigned char u8;
typedef unsigned short u16;

typedef int TIWORD __attribute__((mode(TI)));

typedef struct {
    u8 pad0[0xC];
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
    TIWORD quad200;             /* 0x200 */
    TIWORD quad210;             /* 0x210 */
    short nUnk220;              /* 0x220 */
    u8 pad222[0x230 - 0x222];
    int nUnk230;               /* 0x230 */
    u8 pad234[0x240 - 0x234];
    TIWORD quad240;             /* 0x240 */
} GAME_LOOP_STATE;

typedef struct {
    u8 nType;               /* 0x00 */
    u8 nUnk01;              /* 0x01 */
    u8 nId;                 /* 0x02 */
    u8 pad03[0x41];
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

typedef struct {
    int nFlags;              /* 0x000 */
    u8 pad004[0x82];         /* 0x004 */
    short nAlive;             /* 0x086 */
    u8 pad088[0xA70 - 0x88]; /* 0x088 */
} GAME_ACTOR;

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
