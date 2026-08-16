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
/* TODO: near-miss, 6 diffs of 45 words, OPERANDS. Improved 31 -> 6 by two
 * levers: (a) transform the parameter in place (`nSize = (nSize+63)&~63;`)
 * INSIDE the guarded region -- a separate `nAligned` local merges the copy
 * and the addiu and steals the jal delay slot from `move s0,a0`; (b) read
 * BOTH .nUnk04 and .nId into locals before any store, which is the order
 * the original loads them in. Residue: the two &GameResource[index+1]
 * addresses land in the mirror-image pair of $a0/$a1, and `addiu a0,s2,4`
 * schedules one slot late. All 12 load/store orderings swept (floor 6),
 * plus inlining either load (10/12) and an `int next = index+1` local
 * (34). GameResourceRealloc below has the identical residue. */
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

/* TODO: near-miss, 9 diffs of 42 words, RIGHT LENGTH. Residue is a single
 * allocator tie-break: the original keeps the byte offset (index*16+16) in
 * $a0 and the &GameResource[index+1] pointer in $a1 (loading the flag into
 * $v0, which is why both `return 0` paths need their own `move v0,zero`);
 * gcc here picks the mirror image, so the second return cross-jumps into
 * the epilogue and reuses the v0 already zeroed in the first delay slot.
 * Swept: single-exit vs early returns, `||`-joined test (18), a named
 * GAME_RESOURCE *pNext for the flag (43 words), (&GameResource[index])[1]
 * (31), a local for the flag value (9), both operand orders of the nUnk04
 * sum and three store orders (9/13/20 -- 9 is the floor). Whole-function
 * $a0/$a1 swap is unusable: $a0 is the incoming argument register. */
/* Resize an already-allocated resource entry in place, absorbing the free
 * successor entry that follows it. Returns the id on success, 0 when the
 * id is unknown or the next entry is not free. */
int GameResourceRealloc(int nId, unsigned int nSize)
{
    int index = GameResourceGetIndex(nId);

    if (index < 0) {
        return 0;
    }
    if (GameResource[index + 1].nUnk0C != -1) {
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
