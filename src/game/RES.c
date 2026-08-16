/* RES - resource table lookups: enemy SE banks, motion base IDs, foot step data */

#include "matching.h"

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

/* Pins carry the original's register roles: the destination walks in $a3
   and BOTH the se-table entry and the foot_name pointer live in $a2 (the
   original reuses the one register for the two successive pointers).
   Pinning only the destination, or the two source pointers to different
   registers, both leave 12-13 diffs. */
/* Build "XX_" + the current foot step name for the party leader */
void RES_GetLeaderSeName(char *pName)
{
    PIN(char *pDst, "$7");
    PIN(char *p, "$6");
    PIN(char *q, "$6");

    pDst = pName;
    p = RES_leader_se_table[D_004A1854[0]];
    pDst[0] = p[0];
    pDst[1] = p[1];
    pDst[2] = '_';
    pDst += 3;
    q = foot_name;
    while ((*pDst = *q) != 0) {
        q++;
        pDst++;
    }
}

/* Build "<path>cfNNNN.txt" (or "evNNNN.txt" for the event range) and return
   a pointer at the extension so the caller can swap it.

   The leading digit goes through the local `c`: computed with the other
   three, stored last. Written as `pDst[2] = n % 10 + '0'` after the
   extension stores, its divisor's `li 10` is materialised AFTER the four
   character constants and every one of $t4..$t7 ends up holding the wrong
   value. Store order of the four extension characters is from a 24-way
   sweep. */
char *RES_getScenePath(char *pDst, int nScene)
{
    char *p;
    int n;
    int c;

    p = RES_getPath();
    while ((*pDst = *p) != 0) {
        p++;
        pDst++;
    }
    if (nScene <= 0x0FFFFFFF) {
        pDst[0] = 'c';
        pDst[1] = 'f';
    } else {
        pDst[0] = 'e';
        pDst[1] = 'v';
    }
    n = nScene & 0x0FFFFFFF;
    pDst[5] = n % 10 + '0';
    n = n / 10;
    pDst[4] = n % 10 + '0';
    n = n / 10;
    pDst[3] = n % 10 + '0';
    n = n / 10;
    c = n % 10 + '0';
    pDst[6] = '.';
    pDst[8] = 'x';
    pDst[7] = 't';
    pDst[9] = 't';
    pDst[10] = 0;
    pDst[2] = c;
    return pDst + 7;
}

extern unsigned char ModelPath;
extern void RES_GetMdlFileNameSub(char *pDst);

/* TODO: near-miss, 26 diffs at the right 46 words. `int c` read through an
   `unsigned char *`, with every use spelled `(char)c`, is what produces
   the original's lbu + sll 24 (+ sra 24) shapes and the `move v0,v1` that
   copies the entry byte into the loop variable -- `char c` gives lb and 41
   words, `unsigned char c` with a cast only on the comparison gives 43.
   Residue: gcc CSEs the two `(char)c` sign-extensions across the loop back
   edge and carries the shifted value in a second register, where the
   original recomputes the shift at the loop top and uses a throwaway temp
   for the exit test. Swept: LAUNDER / LAUNDER_V on c in three positions,
   and goto (35), for(;;)+break (26) and while (32) loop forms. */
/* Build the model file name. With ModelPath set, the name is built into a
   scratch buffer and a "test\" directory is spliced in after the first
   backslash on the way out. */
void RES_GetMdlFileName(char *pDst)
{
    char szName[256];
    unsigned char *p;
    char *q;
    int c;
    int nDone;

    if (ModelPath == 0) {
        RES_GetMdlFileNameSub(pDst);
        return;
    }
    RES_GetMdlFileNameSub(szName);
    q = pDst;
    p = (unsigned char *)szName;
    nDone = 0;
    c = *p;
    if ((char)c == 0) {
        return;
    }
    *q = c;
    do {
        /* the source pointer is unsigned char * and c is char: the lbu
           plus the sll/sra sign-extension pair, and the `move` that
           copies the loaded byte into the loop variable, are both that
           type mismatch */
        if ((char)c == '\\' && nDone == 0) {
            q[5] = c;
            nDone = 1;
            q[1] = 't';
            q[2] = 'e';
            q[3] = 's';
            q[4] = 't';
            q += 5;
        }
        p++;
        q++;
        c = *p;
        *q = c;
    } while ((char)c != 0);
}

extern char *MAP_getPath(void);
extern int xglCdGetFileSize(char *pName);
extern void readreq(char *pName, int nArg);
extern char RES_dummyPath[];

/* TODO: near-miss, 16 diffs at the right 88 words. Three shapes are
   settled and load-bearing: a plain `while (*p) { *d++ = *p++; }` gives
   the b-to-test form with an lb test and a separate lbu for the move
   (loops 1, 2 and 5); a guard whose pointer expression DIFFERS from the
   walker's (`if (pFile[0])` with a separate walker) keeps the guard,
   where `p = x; if (*p)` around a do-while gets folded back into the
   b-to-test form; and the fallback path's dst/src walkers must be
   block-local to that arm (worth 8 diffs and the bgtzl). Residue: the
   original's third loop CSEs its bottom load into the next iteration's
   value (one lbu + a move) while the fourth reloads (lb + lbu), and gcc
   has them the other way round; and gcc keeps a copy of the dummy-path
   base where the original walks the materialised address itself. Swept:
   goto forms (84 words), unsigned-char walkers and an explicit
   loop-carried `unsigned char c` (21). */
/* Build "<map path><dir><file>" in a scratch buffer and queue the read.
   When that path does not exist, fall back to "<dummy path><file>". */
int RES_loadFileSubMapSub(char *pDir, char *pFile, int nArg)
{
    char szPath[256];
    char *d;
    char *p;
    int nSize;

    d = szPath;
    p = MAP_getPath();
    while (*p != 0) {
        *d = *p;
        d++;
        p++;
    }
    p = pDir;
    while (*p != 0) {
        *d = *p;
        d++;
        p++;
    }
    if (pFile[0] != 0) {
        unsigned char *u = (unsigned char *)pFile;

        do {
            *d = *u;
            d++;
            u++;
        } while (*u != 0);
    }
    *d = 0;
    nSize = xglCdGetFileSize(szPath);
    if (nSize <= 0) {
        char *d2 = szPath;
        char *p2 = RES_dummyPath;

        if (*p2 != 0) {
            do {
                *d2 = *p2;
                d2++;
                p2++;
            } while (*p2 != 0);
        }
        p2 = pFile;
        while (*p2 != 0) {
            *d2 = *p2;
            d2++;
            p2++;
        }
        *d2 = 0;
        nSize = xglCdGetFileSize(szPath);
    }
    readreq(szPath, nArg);
    return nSize;
}
