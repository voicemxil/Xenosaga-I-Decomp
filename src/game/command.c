/* Script command handlers - the "@command" verbs the event script interpreter
 * dispatches to.  Each handler takes the resource type of the command and the
 * script cursor that follows the verb, and returns the new cursor. */

typedef struct {
    int nId;                        /* 0x00 */
    int nUnk04;                     /* 0x04 */
    int nUnk08;                     /* 0x08 */
    int nUnk0C;                     /* 0x0C */
} GAME_RESOURCE;

typedef struct {
    unsigned char pad000[0x10];     /* 0x00 */
    int nFlags;                     /* 0x10 */
} GAME_LOOP_STATE;

extern GAME_RESOURCE GameResource[];
extern GAME_LOOP_STATE GameLoopState;
extern void *model[];
extern int D_00338690[];
extern int arcfileaddr;
extern void *AdrsEnemyPreset;

char *RES_getPath(void);
int RES_loadFile(int nType, int nKind, void *pArg, void *pFunc);
unsigned char *search_key(unsigned char *pScript, void **pKey, int *pValue);
void Enemy_LoadPreset(void *pAdrs);
void GameCfPlayerLoadResource(int nNo);
void GameDiskChange(int nDisk);
void GameMpeg2Play(char *pName, int nMode);
void GameResourceAlloc(int nSize);
void GameResourceRealloc(int nId, int nUnk, int nSize, GAME_RESOURCE *pRes);
void GameResourceReset(int nNo);
void Intermission(int nNo);
int next_arc_size(char *pName);
void SCRIPT_exec2(void);
void SCRIPT_fade(int nMode);
void SCRIPT_frameLock2Battle(void);
void SCRIPT_load2(char *pName);
int sefLoadEffectCfName(char *pName, char *pWork);
int sefLoadMemoryEffectCfName(char *pName, char *pWork, int nSize);

unsigned char *command_enenpcse(int nType, unsigned char *pScript, int nKind);
unsigned char *command_mpeg2_core(int nType, unsigned char *pScript, int nMode);
int command_loadeffect_sub(int nUnused, char *pWork, char *pName);
int command_loadene_sub(int nUnused, char *pWork, char *pName);

/* "@reset <n>" - free everything above resource slot n */
unsigned char *command_reset(int nType, unsigned char *pScript)
{
    int nNo;
    int c;

    nNo = 0;
    while ((unsigned char)(*pScript - '0') < 10) {
        c = *pScript++;
        nNo *= 10;
        nNo += c - '0';
    }
    GameResourceReset(nNo);
    return pScript;
}

/* "@loadact <key>" - queue the actor model named by the script keyword */
unsigned char *command_loadact(int nType, unsigned char *pScript)
{
    unsigned char *p;
    int nKey;

    nKey = 0;
    p = search_key(pScript, model, &nKey);
    RES_loadFile(nType, 0, (void *)nKey, 0);
    return p;
}

/* "@loadface <key>" - same, but the face bank of the keyword */
unsigned char *command_loadface(int nType, unsigned char *pScript)
{
    unsigned char *p;
    int nKey;

    nKey = 0;
    p = search_key(pScript, model, &nKey);
    RES_loadFile(nType, 0, (void *)(nKey + 0x10000), 0);
    return p;
}

/* "@loadtex <n>" - queue texture archive n */
unsigned char *command_loadtex(int nType, unsigned char *pScript)
{
    int nNo;
    int c;

    nNo = 0;
    while ((unsigned char)(*pScript - '0') < 10) {
        c = *pScript++;
        nNo *= 10;
        nNo += c - '0';
    }
    RES_loadFile(nType, 11, (void *)nNo, 0);
    return pScript;
}

/* "@loadmovie <n>" - queue movie n */
unsigned char *command_loadmovie(int nType, unsigned char *pScript)
{
    int nNo;
    int c;

    nNo = 0;
    while ((unsigned char)(*pScript - '0') < 10) {
        c = *pScript++;
        nNo *= 10;
        nNo += c - '0';
    }
    RES_loadFile(nType, 10, (void *)nNo, 0);
    return pScript;
}

/* Loader callback of "@loadeffect": from the archive when one is open */
int command_loadeffect_sub(int nUnused, char *pWork, char *pName)
{
    if (arcfileaddr != 0) {
        return sefLoadMemoryEffectCfName(pName, pWork, next_arc_size(pName));
    }
    return sefLoadEffectCfName(pName, pWork);
}

/* "@loadeffect <name>" - queue the effect file named by the token */
unsigned char *command_loadeffect(int nType, unsigned char *pScript)
{
    char szName[0x100];
    char *p;

    p = szName;
    while (*pScript != ' ' && *pScript != '\t' && *pScript != '\n' && *pScript != '\r') {
        *p++ = *pScript++;
    }
    *p = '\0';
    RES_loadFile(nType, 6, szName, command_loadeffect_sub);
    return pScript;
}

/* Loader callback of "@loadene": enemy preset, from the archive when open */
int command_loadene_sub(int nUnused, char *pWork, char *pName)
{
    if (arcfileaddr != 0) {
        next_arc_size(AdrsEnemyPreset);
    } else {
        Enemy_LoadPreset(AdrsEnemyPreset);
    }
    return 0;
}

/* "@loadene <name>" - queue the enemy set named by the token */
unsigned char *command_loadene(int nType, unsigned char *pScript)
{
    char szName[0x100];
    char *p;

    p = szName;
    while (*pScript != ' ' && *pScript != '\t' && *pScript != '\n' && *pScript != '\r') {
        *p++ = *pScript++;
    }
    *p = '\0';
    RES_loadFile(nType, 8, szName, command_loadene_sub);
    return pScript;
}

/* "@script <name>" - load and run <path><name>.evt unless scripts are muted */
unsigned char *command_script(int nType, unsigned char *pScript)
{
    char szName[0x100];
    char *d;
    char *s;

    d = szName;
    s = RES_getPath();
    while ((*d = *s) != 0) {
        s++;
        d++;
    }
    while (*pScript != ' ' && *pScript != '\t' && *pScript != '\n' && *pScript != '\r') {
        *d++ = *pScript++;
    }
    d[0] = '.';
    d[1] = 'e';
    d[2] = 'v';
    d[3] = 't';
    d[4] = '\0';
    if ((D_00338690[0] & 0x40) == 0) {
        SCRIPT_load2(szName);
        SCRIPT_exec2();
    }
    return pScript;
}

/* "@player" - load the resources of the current field player */
unsigned char *command_player(int nType, unsigned char *pScript)
{
    GameCfPlayerLoadResource(0);
    return pScript;
}

/* Play "data\movie\mpeg2\<token>.pss" with the given fade mode */
unsigned char *command_mpeg2_core(int nType, unsigned char *pScript, int nMode)
{
    static char base[] = "data\\movie\\mpeg2\\";
    char szName[0x100];
    char *d;
    char *s;

    d = szName;
    s = base;
    while ((*d = *s) != 0) {
        s++;
        d++;
    }
    while (*pScript != ' ' && *pScript != '\t' && *pScript != '\n' && *pScript != '\r') {
        *d++ = *pScript++;
    }
    d[0] = '.';
    d[1] = 'p';
    d[2] = 's';
    d[3] = 's';
    d[4] = '\0';
    GameMpeg2Play(szName, nMode);
    return pScript;
}

/* "@mpeg2 <name>" */
unsigned char *command_mpeg2(int nType, unsigned char *pScript)
{
    return command_mpeg2_core(nType, pScript, 2);
}

/* "@mpeg2battle <name>" - fade out first, the movie leads into a battle */
unsigned char *command_mpeg2battle(int nType, unsigned char *pScript)
{
    SCRIPT_fade(2);
    return command_mpeg2_core(nType, pScript, 1);
}

/* "@mpeg2nofade <name>" */
unsigned char *command_mpeg2nofade(int nType, unsigned char *pScript)
{
    return command_mpeg2_core(nType, pScript, 5);
}

/* "@event2battle" - hand the frame lock over to the battle scene */
unsigned char *command_event2battle(int nType, unsigned char *pScript)
{
    SCRIPT_frameLock2Battle();
    return pScript;
}

/* "@player_lock" - stop the player from being controlled */
unsigned char *command_player_lock(int nType, unsigned char *pScript)
{
    GameLoopState.nFlags |= 0x8000;
    return pScript;
}

/* "@enese <name>" */
unsigned char *command_enese(int nType, unsigned char *pScript)
{
    return command_enenpcse(nType, pScript, 0);
}

/* "@npcse <name>" */
unsigned char *command_npcse(int nType, unsigned char *pScript)
{
    return command_enenpcse(nType, pScript, 1);
}

/* "@disk <n>" - ask for the disk the following data lives on */
unsigned char *command_disk(int nType, unsigned char *pScript)
{
    GameDiskChange(pScript[0] - '0');
    return pScript;
}

/* "@save <n>" - run the intermission (save point) screen */
unsigned char *command_save(int nType, unsigned char *pScript)
{
    int nNo;
    int c;

    nNo = 0;
    while ((unsigned char)(*pScript - '0') < 10) {
        c = *pScript++;
        nNo *= 10;
        nNo += c - '0';
    }
    Intermission(nNo);
    return pScript;
}
