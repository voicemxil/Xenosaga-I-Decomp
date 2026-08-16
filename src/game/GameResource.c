/* GameResource table: a simple linear allocator over a fixed resource pool.
 * See src/Game.c (GameResourceWorkAlloc / GameResourceInit / GameResourceReset)
 * for the sibling API this complements -- kept in a separate TU per repo
 * convention (do not touch Game.c). */

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
/* TODO: near-miss, 2 attempts tried. Struct-index form (31/45 words differ,
 * same length -- LOGIC class, register roles for the four field addresses
 * don't line up) is closer than a flat int* form (wrong register count
 * entirely, 34 vs 45 words). Original computes a shared base pointer to
 * .nUnk04 once (&GameResource[0].nUnk04) and indexes off it for both the
 * current and index+1 entries -- needs that exact pointer-sharing idiom. */
unsigned int GameResourceAlloc(unsigned int nSize)
{
    int index;
    unsigned int nAligned;

    index = resource_get_free();
    nAligned = (nSize + 63) & ~63;

    if (index == -1) {
        return 0;
    }

    GameResource[index + 1].nUnk04 = GameResource[index].nUnk04 - nAligned;
    GameResource[index].nUnk04 = nAligned;
    GameResource[index + 1].nId = GameResource[index].nId + nAligned;
    GameResource[index].nUnk0C = 9;
    GameResource[index].nUnk08 = xglSRand();

    return GameResource[index].nId;
}
