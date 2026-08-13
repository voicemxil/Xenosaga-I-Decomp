/* RES - resource table lookups: enemy SE banks, motion base IDs, foot step data */

typedef struct {
    unsigned short nId;             /* 0x00 */
    unsigned char nType;            /* 0x02 */
    unsigned char nFoot;            /* 0x03 */
    char szName[0x10];              /* 0x04 */
} RES_ENEMYSE;

typedef struct {
    char *pName;                    /* 0x00 */
    unsigned short nMotID[14];      /* 0x04 */
} RES_MTNLIST;

typedef struct {
    unsigned char pad000[0x6];      /* 0x00 */
    unsigned char nFootStep;        /* 0x06 */
} RES_SEDATA;

typedef struct {
    char pad000[0x8DC];             /* 0x000 */
    RES_SEDATA *pSeData;            /* 0x8DC */
} RES_CHR;

extern RES_ENEMYSE EnemySeBank[8];
extern RES_MTNLIST cfMtnList[];
extern char env_name[];
extern unsigned char ScenePath;
extern char *RES_path_table[] __asm__("tbl.0_00366420");
extern char *RES_leader_se_table[] __asm__("tbl.1_00366470");
extern char foot_name[];
extern unsigned short D_004A1854[];

char *RES_getPath(void)
{
    if (ScenePath >= 20) {
        ScenePath = 0;
    }
    return RES_path_table[ScenePath];
}

/* Sound bank selector for an enemy SE set */
int RES_GetEnemySeBank(int nId)
{
    int i;
    int nBank;

    for (i = 0, nBank = 0x40000; i < 8; i++, nBank += 0x10000) {
        if (EnemySeBank[i].nId == nId) {
            return nBank;
        }
    }
    return -1;
}

/* SE type of a registered enemy */
int RES_GetEnemySeType(int nId)
{
    int i;

    for (i = 0; i < 8; i++) {
        if (EnemySeBank[i].nId == nId) {
            return EnemySeBank[i].nType;
        }
    }
    return -1;
}

/* Foot step set of a registered enemy */
int RES_GetEnemySeFoot(int nId)
{
    int i;

    for (i = 0; i < 8; i++) {
        if (EnemySeBank[i].nId == nId) {
            return EnemySeBank[i].nFoot;
        }
    }
    return -1;
}

/* SE file name of the n-th enemy slot */
char *RES_GetEnemySeName(int nIndex)
{
    if (nIndex < 8) {
        return EnemySeBank[nIndex].szName;
    }
    return 0;
}

/* Base motion ID of the group a motion ID belongs to */
int RES_GetMotBaseID(unsigned short nId)
{
    RES_MTNLIST *p;
    unsigned short *q;
    int nBase;
    int nCur;

    for (p = cfMtnList; p->pName != 0; p++) {
        q = p->nMotID;
        nBase = *q;
        while (*q != 0) {
            nCur = *q;
            if (nCur == nId) {
                return nBase;
            }
            q++;
        }
    }
    return 1;
}

/* Foot step number of the SE data attached to a character */
int RES_GetFootStepNo(RES_CHR *pChr)
{
    if (pChr->pSeData == 0) {
        return 0;
    }
    return pChr->pSeData->nFootStep;
}

/* Copy the current map environment SE name */
void RES_GetMapEnvSeName(char *pName)
{
    char *p;

    p = env_name;
    while ((*pName = *p) != 0) {
        p++;
        pName++;
    }
}

/* Name of the motion group a motion ID belongs to */
char *RES_GetMotBaseName(unsigned short nId)
{
    RES_MTNLIST *p;
    unsigned short *q;
    int nCur;

    for (p = cfMtnList; p->pName != 0; p++) {
        q = p->nMotID;
        while (*q != 0) {
            nCur = *q;
            if (nCur == nId) {
                return p->pName;
            }
            q++;
        }
    }
    return cfMtnList[0].pName;
}

/* Build "XX_" + the current foot step name for the party leader */
void RES_GetLeaderSeName(char *pName)
{
    char *p;
    char *q;

    p = RES_leader_se_table[D_004A1854[0]];
    pName[0] = p[0];
    pName[1] = p[1];
    pName[2] = '_';
    pName += 3;
    q = foot_name;
    while ((*pName = *q) != 0) {
        q++;
        pName++;
    }
}
