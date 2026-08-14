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
