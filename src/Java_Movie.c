/* Native bindings for the script VM's xeno.Movie and xeno.Scene classes */

typedef union {
    int i;
    unsigned int u;
    float f;
    short h;
    unsigned char c;
    void *p;
} JVAL;

typedef struct {
    char pad00[0x10];
    int nOffset;
} JFIELD;

int classJava_xeno_Movie;
short GameMovieFrame;
unsigned char GameMovieTransparent;
unsigned char GameMovieAlpha;

extern char D_004DC120[];
extern char D_004D16C0[];
extern char D_004DC128[];

int loadConstString(char *, int);
JFIELD *lookupClassField(int, int, int);
void GameMovieInit(int);
void GameMoviePlay(char *);
void GameMovieStop(void);
void SCENE_start(void *, int, int, int);

/* Prepare the movie player for a movie id */
void Java_xeno_Movie_init__I(void *pEnv, JVAL *pArgs, JVAL *pRet)
{
    GameMovieInit(pArgs[1].i << 10);
}

/* Build the .ipu path from the movie number and start playback */
void Java_xeno_Movie_start__I(void *pEnv, JVAL *pArgs, JVAL *pRet)
{
    static char name[] = "data\\movie\\full\\mv0000.ipu";
    int nId;

    nId = pArgs[1].i;
    name[21] = nId % 10 + '0';
    nId /= 10;
    name[20] = nId % 10 + '0';
    nId /= 10;
    name[19] = nId % 10 + '0';
    nId /= 10;
    name[18] = nId % 10 + '0';
    GameMoviePlay(name);
}

/* Stop the running movie */
void Java_xeno_Movie_stop__(void *pEnv, JVAL *pArgs, JVAL *pRet)
{
    GameMovieStop();
}

/* Publish the movie frame counter and pull the blend settings back out */
void Java_xeno_Movie_update__(void *pEnv, JVAL *pArgs, JVAL *pRet)
{
    char *obj = (char *)pArgs[0].p;

    *(int *)(obj + lookupClassField(classJava_xeno_Movie, loadConstString(D_004DC120, -1), 0)->nOffset) = GameMovieFrame;
    GameMovieTransparent = *(unsigned char *)(obj + lookupClassField(classJava_xeno_Movie, loadConstString(D_004D16C0, -1), 0)->nOffset);
    GameMovieAlpha = *(unsigned char *)(obj + lookupClassField(classJava_xeno_Movie, loadConstString(D_004DC128, -1), 0)->nOffset);
}

/* Hand the whole call straight to the scene runner */
void Java_xeno_Scene_start__ILjava_lang_Object_(void *pEnv, JVAL *pArgs, JVAL *pRet)
{
    SCENE_start(pEnv, pArgs[0].i, pArgs[1].i, pArgs[2].i);
}

/* Scene.stop() - not implemented in the retail build */
void Java_xeno_Scene_stop__(void *pEnv, JVAL *pArgs, JVAL *pRet)
{
}
