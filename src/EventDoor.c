/* EventDoor - map-object door state machine (standby/open/close) */

typedef struct {
    float x;
    float y;
    float z;
    float w;
} __attribute__((aligned(8))) VECTOR8;

/* Target actor's tracked position; only the x/y/z fields at 0x30 are
   touched by this file */
typedef struct {
    unsigned char pad0[0x30];
    float x;                    /* 0x30 */
    float y;                    /* 0x34 */
    float z;                    /* 0x38 */
} DOORTARGETPOS;

typedef struct {
    unsigned char pad0[4];
    signed char nSubState;      /* 0x04 */
    unsigned char pad5[3];
    short nSubCounter;          /* 0x08 */
    unsigned char pad0A[0x2A];
    int nCode1;                 /* 0x34 */
    int nCode2;                 /* 0x38 */
} DOORSUB;

typedef struct {
    unsigned char pad0[0x80];
    DOORTARGETPOS *pTarget;     /* 0x80 */
    unsigned char pad84[0x1C];
    unsigned char nUnkA0;        /* 0xA0 */
    unsigned char padA1;
    unsigned char nState;         /* 0xA2 */
    unsigned char padA3;
    short nId;                     /* 0xA4 */
    short padA6;
    short nCounter;                 /* 0xA8 */
    unsigned char padAA[0x16];
    VECTOR8 pos;                     /* 0xC0 */
    unsigned char padD0[0xD0];
    DOORSUB sub;                       /* 0x1A0 */
} DOOR;

extern char printflg;
extern char D_004CA3B8[];
extern char D_004CA3C8[];

extern int printf(const char *, ...);
extern void CheckDoorPos(DOOR *a, VECTOR8 *pOut);
extern void DoorCommonFunc(DOOR *a);
extern void DoorOpenStanbyFunc(DOOR *a);
extern void xglSoundEffectPosID(int, VECTOR8 *, int, int);
extern void xglSoundEffectStopID(int, int);
extern int xglSoundEffectCheckID(int);

void EventDoorStanbyFunc(DOOR *a);
void EventDoorOpenOpeFunc(DOOR *a);
void EventDoorOpenNowFunc(DOOR *a);
void EventDoorCloseOpeFunc(DOOR *a);

/* Dispatch a door's per-state update function */
/* TODO: near-miss (6/29 words) - jump table, bounds check and cases 0-2
   all match exactly (unsigned char local + switch on it reproduces the
   table). The last case (3, EventDoorCloseOpeFunc) sibling-calls (j) here
   where the original keeps jal + a shared epilogue with case 4/default -
   this is the documented sibling-call blocker (see resume prompt), already
   swept ~50 variants elsewhere with no reproduction. */
void EventDoorFunc(DOOR *a)
{
    unsigned char nState = a->nState;

    switch (nState) {
    case 0:
        EventDoorStanbyFunc(a);
        break;
    case 1:
        EventDoorOpenOpeFunc(a);
        break;
    case 2:
        EventDoorOpenNowFunc(a);
        break;
    case 3:
        EventDoorCloseOpeFunc(a);
        break;
    case 4:
        break;
    }
}

/* Waiting state: track the target position and, once triggered, print a
   debug message, kick the open sound and transition state */
/* TODO: near-miss (33/50 words) - the bnel-likely skip path restores $s0
   from the stack in its delay slot and the nState store lives only once at
   the join; every C shape tried (single if-block, early-return duplicate
   store) makes gcc instead hoist the nState store itself into the delay
   slot and keep a second copy at the join - looks like the same class of
   scheduler-only near-miss as the sibling-call blocker. */
void EventDoorStanbyFunc(DOOR *a)
{
    DOORSUB *sub = &a->sub;
    DOORTARGETPOS *t = a->pTarget;
    VECTOR8 buf;
    signed char nSubState;

    t->x = a->pos.x;
    t->y = a->pos.y;
    t->z = a->pos.z;
    nSubState = sub->nSubState;
    if (nSubState == 1) {
        if (printflg) {
            printf(D_004CA3B8, a->nId);
        }
        if (xglSoundEffectCheckID(sub->nCode1)) {
            xglSoundEffectStopID(sub->nCode1, a->nUnkA0 + 1);
        }
        CheckDoorPos(a, &buf);
        xglSoundEffectPosID(sub->nCode1, &buf, 1, a->nUnkA0 + 1);
    }
    a->nState = nSubState;
}

/* Opening: advance the counter, move the door and check whether it has
   reached the fully-open threshold */
void EventDoorOpenOpeFunc(DOOR *a)
{
    DOORSUB *sub = &a->sub;
    VECTOR8 buf;
    int n = (unsigned short)a->nCounter;

    n++;
    a->nCounter = n;
    CheckDoorPos(a, &buf);
    xglSoundEffectPosID(sub->nCode1, &buf, 1, a->nUnkA0 + 1);
    DoorCommonFunc(a);
    if (a->nCounter >= sub->nSubCounter) {
        a->nState = 2;
    }
}

/* Fully open: hold position while polling for the close trigger */
/* TODO: near-miss (28/46 words) - after the printf, the original computes
   a->nUnkA0+1 into $a1 once (dead - immediately clobbered by the
   xglSoundEffectCheckID call), sets nState=3, then recomputes the same
   a->nUnkA0+1 a second time right before the actual xglSoundEffectStopID
   call. Every natural ordering of the nState/nCounter/CheckID statements
   produces only the single (real) computation; the dead early one looks
   like a 2.96 scheduling artifact, not reachable from source. */
void EventDoorOpenNowFunc(DOOR *a)
{
    DOORSUB *sub = &a->sub;
    VECTOR8 buf;

    a->nCounter = sub->nSubCounter;
    DoorOpenStanbyFunc(a);
    if (sub->nSubState == 0) {
        if (printflg) {
            printf(D_004CA3C8, a->nId);
        }
        a->nState = 3;
        a->nCounter = sub->nSubCounter;
        if (xglSoundEffectCheckID(sub->nCode1)) {
            xglSoundEffectStopID(sub->nCode1, a->nUnkA0 + 1);
        }
        CheckDoorPos(a, &buf);
        xglSoundEffectPosID(sub->nCode2, &buf, 1, a->nUnkA0 + 1);
    }
}

/* Closing: retreat the counter, move the door and check whether it has
   reached the fully-closed threshold */
void EventDoorCloseOpeFunc(DOOR *a)
{
    DOORSUB *sub = &a->sub;
    VECTOR8 buf;
    int n = (unsigned short)a->nCounter;

    n--;
    a->nCounter = n;
    CheckDoorPos(a, &buf);
    xglSoundEffectPosID(sub->nCode2, &buf, 1, a->nUnkA0 + 1);
    DoorCommonFunc(a);
    if (a->nCounter <= 0) {
        a->nState = 0;
    }
}
