/* Play-control (PLAY) subsystem - global control block accessor, timing hook and observer slots */

typedef struct OBSERVER {
    int nId;                  /* 0x0 */
    int nArg;                  /* 0x4 */
    short nA;                   /* 0x8 */
    short nB;                   /* 0xA */
} OBSERVER;

typedef struct PLAYCTRL {
    char pad000[0x8];
    int nTimeChart;             /* 0x8 */
    char pad00C[0x4C - 0xC];
    OBSERVER observers[32];      /* 0x4C */
} PLAYCTRL;

PLAYCTRL playControl;

/* Return the single global play-control block */
PLAYCTRL *PLAY_getCurrent(void)
{
    return &playControl;
}

/* Install the active time chart pointer */
void PLAY_setTimeChart(PLAYCTRL *p, int val)
{
    p->nTimeChart = val;
}

/* Claim the first free observer slot and fill it in */
void PLAY_setObserver(PLAYCTRL *p, int a1, int a2, int a3, int a4)
{
    OBSERVER *slot;
    int i;

    slot = 0;
    for (i = 0; i < 32; i++) {
        if (p->observers[i].nId == 0) {
            slot = &p->observers[i];
            break;
        }
    }
    if (slot != 0) {
        slot->nId = a3;
        slot->nArg = a4;
        slot->nB = a2;
        slot->nA = a1;
    }
}

/* Stubbed out camera load hook */
void PLAY_loadCamera(void)
{
}
