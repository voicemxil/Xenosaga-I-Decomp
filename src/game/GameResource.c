/* GameResource table: a simple linear allocator over a fixed resource pool.
 * See src/Game.c (GameResourceWorkAlloc / GameResourceInit / GameResourceReset)
 * for the sibling API this complements -- kept in a separate TU per repo
 * convention (do not touch Game.c). */

#include "matching.h"

typedef struct {
    int nId;                /* 0x00 */
    int nUnk04;              /* 0x04 */
    int nUnk08;               /* 0x08 */
    int nUnk0C;                /* 0x0C */
} GAME_RESOURCE;

extern GAME_RESOURCE GameResource[];

extern int resource_get_free(void);
extern int xglSRand(void);

extern void resource_typeid_translate(int nIndex, int *pType, int *pId);

/* Find the resource slot matching a (type, id) pair; either half may be
   -1 to mean "don't care". Type 5 is never searchable.

   The two field reads are written as GameResource[i].FIELD, NOT through a
   walked `int *`: loop strength reduction then builds the giv itself, and
   its initial value comes out as %hi/%lo(GameResource) plus a separate
   `addiu +8`. A hand-walked pointer folds the +8 into the %lo, which is
   one word shorter -- and that word is what makes the loop-top
   `.p2align 3,,7` pad materialise. */
int GameResourceSearch(int nType, int nId)
{
    int i;
    int t;
    int id;

    if (nType == 5) {
        return -1;
    }
    i = 0;
    do {
        t = nType;
        id = nId;
        resource_typeid_translate(i, &t, &id);
        if (nType != -1 && GameResource[i].nUnk0C != t) {
            goto next;
        }
        if (nId != -1 && GameResource[i].nUnk08 != id) {
            goto next;
        }
        return i;
next:
        i++;
    } while (i < 128);
    return -1;
}

/* Carve a nSize-byte (64-byte aligned) block off the free resource entry
 * returned by resource_get_free(), splitting it into the allocated entry
 * and a shrunk successor entry. Returns the allocated entry's nId, or 0 if
 * no free entry was available. */
unsigned int GameResourceAlloc(unsigned int nSize)
{
    int index = resource_get_free();
    unsigned int nFree;
    unsigned int nAddr;

    if (index == -1) {
        return 0;
    }
    nSize = (nSize + 63) & ~63;
    nFree = GameResource[index].nUnk04;
    nAddr = GameResource[index].nId;
    GameResource[index + 1].nUnk04 = nFree - nSize;
    GameResource[index].nUnk04 = nSize;
    GameResource[index + 1].nId = nAddr + nSize;
    GameResource[index].nUnk0C = 9;
    GameResource[index].nUnk08 = xglSRand();

    return GameResource[index].nId;
}

extern int GameResourceGetIndex(int nId);

/* Resize an already-allocated resource entry in place, absorbing the free
 * successor entry that follows it. Returns the id on success, 0 when the
 * id is unknown or the next entry is not free. */
int GameResourceRealloc(int nId, unsigned int nSize)
{
    int index = GameResourceGetIndex(nId);
    PIN(int nNextType, "$2");

    if (index < 0) {
        return 0;
    }
    /* Retail keeps the tested type in the return-value register, preserving
       a separate zero-return delay slot for this failure path. */
    nNextType = GameResource[index + 1].nUnk0C;
    if (nNextType != -1) {
        return 0;
    }
    {
        int nRest = GameResource[index + 1].nUnk04 + GameResource[index].nUnk04;
        int nAddr = GameResource[index].nId;

        nSize = (nSize + 63) & ~63;
        GameResource[index].nUnk04 = nSize;
        GameResource[index + 1].nId = nAddr + nSize;
        GameResource[index + 1].nUnk04 = nRest - nSize;
    }
    return nId;
}

extern void GameResourceDump(int nNo);
extern void xglSoundSendEffect(int a, int b, int c);
extern int arcfilepreload;

typedef struct {
    short nUnk00;   /* 0x00 */
    char  nUnk02;   /* 0x02 */
    char  nPad03;
    char  nUnk04;   /* 0x04 */
    char  nPad05[15];
} ENEMY_SE_BANK;    /* 0x14 */

extern ENEMY_SE_BANK EnemySeBank[];

/* Release every resource entry from nNo upwards: fold their sizes back into
 * entry nNo, mark it free, blank the rest, and silence the eight enemy SE
 * banks that referenced them. */
void GameResourceReset(int nNo)
{
    int i;
    int nTotal;
    ENEMY_SE_BANK *pBank;

    nTotal = 0;
    for (i = nNo; i < 128; i++) {
        nTotal += GameResource[i].nUnk04;
    }
    GameResource[nNo].nUnk04 = nTotal;
    GameResource[nNo].nUnk0C = -1;
    GameResource[nNo].nUnk08 = 0;
    for (i = nNo + 1; i < 128; i++) {
        GameResource[i].nId = 0;
        GameResource[i].nUnk04 = 0;
        GameResource[i].nUnk08 = 0;
        GameResource[i].nUnk0C = -1;
    }
    pBank = EnemySeBank;
    for (i = 0; i < 8; i++) {
        pBank->nUnk00 = 0;
        pBank->nUnk02 = 0;
        pBank->nUnk04 = 0;
        xglSoundSendEffect(0, 0, i + 4);
        pBank++;
    }
    GameResourceDump(0);
    arcfilepreload = 0;
}
