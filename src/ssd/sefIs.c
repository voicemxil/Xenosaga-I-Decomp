/* Battle scene effect/scheduler predicates (sefIs* family) */

/* Whether an actor id falls in the boss-actor id range */
int sefIsBossID(int id)
{
    return (unsigned int)(id - 0x97) < 0x24;
}

typedef struct {
    int field0;
    char pad[0xAB0 - 4];
} SCHED_ENTRY;

extern SCHED_ENTRY D_0065AFB0[];

/* A scheduler slot >= the table size is considered dead; otherwise dead
 * iff its first word is zero */
int sefIsDeadSchduler(int idx)
{
    if ((unsigned int)idx >= 0x80) {
        return 1;
    }
    return D_0065AFB0[idx].field0 == 0;
}

extern int D_00794370[];

/* Whether a boss has been entered this battle */
int sefIsEntryBoss(void)
{
    return D_00794370[0];
}

/* --- battle-actor signal predicates --- */

/* NEAR-MISS (both walkers, 5 diffs each, 21/21 instructions): the
 * original loads the actor count into a scratch register, tests it, then
 * COPIES it into the loop-bound register (`lw v1` / `blez v1` /
 * `move a2,v1`); every shape tried here lets gcc load straight into the
 * loop pseudo and pad with a nop instead. Swept: guard-clause returns vs
 * one shared tail return; `for` vs `do/while` vs `goto` loop; the count
 * read once, read twice (CSE folds it to one), and re-read inside the
 * loop latch (that one stops the hoist entirely and reloads each
 * iteration -- worse). A pure allocator tie-break; permuter territory. */

typedef struct
{
    char pad000[0x80];
    int nID;            /* 0x80 */
    char pad084[0x88 - 0x84];
    short nHit;         /* 0x88 */
    short pad08A;
    short nSeSignal;    /* 0x8C */
    short pad08E;
} BATTLE_ACTOR;

typedef struct
{
    BATTLE_ACTOR aActor[24];   /* 0x000 */
    char padD80[0xD90 - 0xD80];
    int nActors;               /* 0xD90 */
} BATTLE_ACTOR_TBL;

extern BATTLE_ACTOR_TBL _battleActor;

/* Whether the actor with this id raised its hit flag this frame */
int sefIsHitActor(int nID)
{
    BATTLE_ACTOR *p;
    int i, n;

    if (nID != 0) {
        n = _battleActor.nActors;
        if (n > 0) {
            i = 0;
            p = _battleActor.aActor;
            do {
                if (p->nID == nID) {
                    return p->nHit != 0;
                }
                i++;
                p++;
            } while (i < n);
        }
    }
    return 0;
}

/* Same walk, reading the sound-effect signal instead */
int sefIsSeSignal(int nID)
{
    BATTLE_ACTOR *p;
    int i, n;

    if (nID != 0) {
        n = _battleActor.nActors;
        if (n > 0) {
            i = 0;
            p = _battleActor.aActor;
            do {
                if (p->nID == nID) {
                    return p->nSeSignal != 0;
                }
                i++;
                p++;
            } while (i < n);
        }
    }
    return 0;
}

/* --- battle-actor signal setters (share the table above) --- */

/* Raise the hit signal on the actor slot owning this object */
void sefSetHitSignal(int nID)
{
    BATTLE_ACTOR *p;
    int i;

    if (nID == 0) {
        return;
    }
    for (i = 0; i < _battleActor.nActors; i++) {
        p = &_battleActor.aActor[i];
        if (p->nID == nID) {
            p->nHit++;
            break;
        }
    }
}

/* Same walk, raising the sound-effect signal instead */
void sefSetSeSignal(int nID)
{
    BATTLE_ACTOR *p;
    int i;

    if (nID == 0) {
        return;
    }
    for (i = 0; i < _battleActor.nActors; i++) {
        p = &_battleActor.aActor[i];
        if (p->nID == nID) {
            p->nSeSignal++;
            break;
        }
    }
}
