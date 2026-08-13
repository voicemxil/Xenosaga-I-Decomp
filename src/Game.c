/* Game loop helpers - state save/restore, resources, movies, pause and radar */

typedef unsigned char u8;
typedef unsigned short u16;

typedef struct {
    u8 pad0[0xC];
    u16 nSaveA;             /* 0x0C */
    u16 nSaveB;             /* 0x0E */
    int nFlags;             /* 0x10 */
    u8 pad14[0x18];
    void (*pDrawFunc)(void);        /* 0x2C */
    void (*pDrawFunc2)(void);       /* 0x30 */
    u8 pad34[0xAC];
    u16 nStateA;            /* 0xE0 */
    u16 nStateB;            /* 0xE2 */
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
