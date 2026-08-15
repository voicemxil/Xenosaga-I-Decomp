#include "matching.h"

/* Enemy layer - field encounter actors: damage reactions, sound, and script commands */

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u_int;

typedef struct {
    u8 pad00[0x1A];                 /* 0x00 */
    short nUnk1A;                   /* 0x1A */
    u8 pad1C[0x2];                  /* 0x1C */
    short nUnk1E;                   /* 0x1E */
} ACTWORK;

typedef struct ACTOR {
    int nFlags;                     /* 0x000 */
    u8 pad004[0xC];                 /* 0x004 */
    float fPos[4];                  /* 0x010 */
    u8 pad020[0x30];                /* 0x020 */
    float fDir[4];                  /* 0x050 */
    u8 pad060[0x20];                /* 0x060 */
    u8 nSerial;                     /* 0x080 */
    u8 pad081[0x5];                 /* 0x081 */
    short nAlive;                   /* 0x086 */
    u8 pad088[0x38];                /* 0x088 */
    ACTWORK work;                   /* 0x0C0 */
    u8 pad0E0[0xC];                 /* 0x0E0 */
    int nCode;                      /* 0x0EC */
    u8 pad0F0[0x600];               /* 0x0F0 */
    int nAnimFlags;                 /* 0x6F0 */
    float fMotionSpeed;             /* 0x6F4 */
    float fStiff;                   /* 0x6F8 */
    float fMotionIn;                /* 0x6FC */
    float fMotionOut;               /* 0x700 */
    u16 nMotion;                    /* 0x704 */
    short nUnk706;                  /* 0x706 */
    u8 pad708[0xA];                 /* 0x708 */
    short nUnk712;                  /* 0x712 */
    u8 pad714[0x2D0];               /* 0x714 */
    float fAngle;                   /* 0x9E4 */
    u8 pad9E8[0x6];                 /* 0x9E8 */
    short nLookAt;                  /* 0x9EE */
    u8 pad9F0[0x80];                /* 0x9F0 */
} ACTOR;

typedef struct {
    u8 pad0000[0x10];               /* 0x0000 */
    float fMove[4];                 /* 0x0010 */
    float fMoveWork[4];             /* 0x0020 */
    u8 pad0030[0x10];               /* 0x0030 */
    u16 nUnk0040;                   /* 0x0040 */
    u8 pad0042[0x2];                /* 0x0042 */
    u16 nWait;                      /* 0x0044 */
    short nWaitLimit;               /* 0x0046 */
    u8 pad0048[0x2];                /* 0x0048 */
    char nType;                     /* 0x004A */
    u8 pad004B[0x3];                /* 0x004B */
    u16 nStiff;                     /* 0x004E */
    u8 pad0050[0x1A];               /* 0x0050 */
    u8 nUnk006A;                    /* 0x006A */
    u8 pad006B[0x2025];             /* 0x006B */
    short nAction[256][4];          /* 0x2090 */
    short nActionArg[256][4];       /* 0x2890 */
    short nActionSet;               /* 0x3090 */
    u16 nActionStep;                /* 0x3092 */
    u8 pad3094[0x70E];              /* 0x3094 */
    char nFreeze;                   /* 0x37A2 */
    char nTurn;                     /* 0x37A3 */
    short nTarget;                  /* 0x37A4 */
    u8 pad37A6[0xA];                /* 0x37A6 */
    float fFreeze;                  /* 0x37B0 */
    u8 pad37B4[0x1C];               /* 0x37B4 */
    float fScale;                   /* 0x37D0 */
    float fScaleFrom;               /* 0x37D4 */
    float fScaleTo;                 /* 0x37D8 */
    short nScaleTime;               /* 0x37DC */
    short nScaleTimeMax;            /* 0x37DE */
    int nStatus;                    /* 0x37E0 */
    u8 pad37E4[0x1C];               /* 0x37E4 */
    float fSacFrom[4];              /* 0x3800 */
    float fSacTo[4];                /* 0x3810 */
    short nSacMove;                 /* 0x3820 */
    short nSacTime;                 /* 0x3822 */
    float fTurnFrom;                /* 0x3824 */
    float fTurnTo;                  /* 0x3828 */
    short nSacTurn;                 /* 0x382C */
    short nSacTurnTime;             /* 0x382E */
    u8 pad3830[0x24];               /* 0x3830 */
    float fStiff;                   /* 0x3854 */
    u8 pad3858[0x48];               /* 0x3858 */
    int nStiffEffect;               /* 0x38A0 */
    u8 pad38A4[0x6];                /* 0x38A4 */
    short nLight;                   /* 0x38AA */
    u8 pad38AC[0x4];                /* 0x38AC */
} ENEPC;

typedef struct {
    u16 nId;                        /* 0x00 */
    u16 nUnk02;                     /* 0x02 */
} SOUNDENTRY;

typedef struct {
    u8 pad00[0x20];                 /* 0x00 */
    SOUNDENTRY aEntry[1];           /* 0x20 */
} SOUNDWORK;

typedef struct {
    u8 pad00[0x4];                  /* 0x00 */
    ACTOR *pPlayer;                 /* 0x04 */
    u8 pad08[0xDC];                 /* 0x08 */
} GAME_LOOP_STATE;

extern ENEPC enepc[];
extern ACTOR actor[];
extern SOUNDWORK SoundWork;
extern GAME_LOOP_STATE GameLoopState;

void Enemy_Init(ACTOR *);
void Enemy_Pause(ACTOR *);
void Enemy_ActionReady(ACTOR *, int);
void EnemySound_Stop(ACTOR *, char);
void Check_Encount(ACTOR *, int, int);
int Get_ActorNumber(int);
void Homing_Search(ACTOR *);
float Get_Angle(float *, float *);
int RES_GetEnemySeBank(int);
int RES_GetEnemySeType(int);
void sefDeleteEffectCf(int);
void ACT_setMotion(ACTOR *, int);
void Actor_LookAt_Set(ACTOR *, int, float *);
void Actor_LookAt_Release(ACTOR *);
void xglSoundEffectStopID(int, int);
void xglSoundEffectStopDirect(int);
void xglSoundEffectPosID(int, float *, int, int);
void SsdFadeoutEffect(int, int, int);
int xglSRand(void);
void TM_Enemy_Move_Step(int, float *, float);
void ACT_setMotion2(ACTOR *, int, int);
int xglCdReadFile(char *, void *, int, int);
char *strcat(char *, char *);

extern int D_0033869C[];
extern char *WorkEnd;
extern void *AdrsEnemyPreset;
extern char *AdrsEnemySpline;
extern char *AdrsEnemyExclamation;
extern char *AdrsEnemyQuestion;
extern char *AdrsEnemySphere;
extern char *AdrsEnemySquare;

/* React to an explosion hit: stop, face the player and pick a recovery action */
void Enemy_Damage_Explosion(ACTOR *a)
{
    ENEPC *p;
    int nFlag;
    PIN(GAME_LOOP_STATE *pgls, "$3");

    p = &enepc[a->nSerial];
    p->fMove[0] = 0.0f;
    p->fMove[2] = 0.0f;
    TM_Enemy_Move_Step(a->nSerial, &p->fMoveWork[0], p->fMove[0]);
    pgls = &GameLoopState;
    a->fAngle = Get_Angle(&a->fPos[0], &pgls->pPlayer->fPos[0]);
    if ((short)p->nWait++ >= p->nWaitLimit) {
        a->nFlags &= ~0x800000;
        nFlag = p->nUnk006A;
        if (nFlag & 1) {
            Enemy_ActionReady(a, 7);
        } else {
            Enemy_ActionReady(a, 6);
        }
    }
}

/* React to an electric-attack hit once the reaction wait elapses */
void Enemy_Damage_Electric(ACTOR *a)
{
    ENEPC *p;

    p = &enepc[a->nSerial];
    p->nWait++;
    if ((short)p->nWait >= p->nWaitLimit) {
        p->nUnk0040 = 0;
        a->nFlags &= ~0x800000;
        Homing_Search(a);
        Enemy_ActionReady(a, 2);
    }
}

/* React to a seal-attack hit once the reaction wait elapses */
void Enemy_Damage_Seal(ACTOR *a)
{
    ENEPC *p;

    p = &enepc[a->nSerial];
    p->nWait++;
    if ((short)p->nWait >= p->nWaitLimit) {
        p->nUnk0040 = 0;
        a->nFlags &= ~0x800000;
        Homing_Search(a);
        Enemy_ActionReady(a, 1);
    }
}

/* Count down the stiffen timer and resume normal behaviour when it expires */
void Enemy_Stiff(ACTOR *a)
{
    ENEPC *p;
    int nLeft;
    int nEffect;

    p = &enepc[a->nSerial];
    nLeft = p->nStiff - 1;
    p->nStiff = nLeft;
    if ((short)nLeft <= 0) {
        nEffect = p->nStiffEffect;
        p->nUnk006A &= 0xFE;
        p->nStiff = 0;
        a->fStiff = p->fStiff;
        a->nFlags &= ~0x1000000;
        sefDeleteEffectCf(nEffect);
        p->nStiffEffect = 0;
        Homing_Search(a);
        Enemy_ActionReady(a, 2);
    } else {
        a->fStiff = 0.0f;
    }
}

/* Turn toward the player while the "found the player" reaction plays */
void Enemy_Found(ACTOR *a)
{
    ENEPC *p;

    p = &enepc[a->nSerial];
    if ((short)p->nWait++ >= p->nWaitLimit) {
        Enemy_ActionReady(a, 6);
    }
    if (a->nAlive != 0x2201) {
        a->fAngle = Get_Angle(&a->fPos[0], &GameLoopState.pPlayer->fPos[0]);
    }
}

/* Hold a paused enemy until its wait expires, then choose its next action */
void Enemy_Pause(ACTOR *a)
{
    ACTOR *pActor;
    ENEPC *p;
    int nRandom;

    p = enepc;
    p += a->nSerial;
    pActor = a;
    if ((u8)((u8)p->nType + 251) < 2) {
        if (pActor->nFlags & 0x400000) {
            pActor->fAngle = Get_Angle(&pActor->fPos[0], &GameLoopState.pPlayer->fPos[0]);
        }
    }
    if (p->nWaitLimit - 1 <= (short)p->nWait++) {
        switch ((u8)p->nType) {
        case 1:
        case 2:
        case 3:
        case 8:
        case 9:
            Enemy_ActionReady(pActor, 1);
            break;
        case 5:
        case 6:
            Enemy_ActionReady(pActor, 4);
            if (p->nStatus & 0x10) {
                p->nUnk0040 += 2;
            }
            break;
        case 12:
            Enemy_ActionReady(pActor, 14);
            break;
        case 13:
            Enemy_ActionReady(pActor, 15);
            break;
        case 4:
        case 7:
        case 10:
        case 11:
        default:
            nRandom = (u16)(xglSRand() & 1);
            switch (nRandom) {
            case 0:
                Enemy_ActionReady(pActor, 4);
                break;
            case 1:
                Enemy_ActionReady(pActor, 2);
                break;
            }
            break;
        }
    }
}

/* Hold the threat pose until one frame before the reaction ends */
void Enemy_Threat(ACTOR *a)
{
    ENEPC *p;

    p = &enepc[a->nSerial];
    if (p->nWaitLimit - 1 <= (short)p->nWait++) {
        Enemy_ActionReady(a, 4);
        if (p->nStatus & 0x10) {
            p->nUnk0040 += 2;
        }
    }
}

/* Step through the unique-action list, ending the sequence at its terminator */
void Enemy_Unique(ACTOR *a)
{
    ENEPC *p;

    p = &enepc[a->nSerial];
    if (p->nWaitLimit - 1 <= (short)p->nWait++) {
        p->nActionStep++;
        if (p->nAction[p->nActionSet][(short)p->nActionStep] != -1) {
            Enemy_ActionReady(a, 9);
        } else {
            Enemy_ActionReady(a, 4);
            if (p->nStatus & 0x10) {
                p->nUnk0040 += 2;
            }
        }
    }
}

/* Play a positional enemy sound effect */
void EnemySound(ACTOR *a, short nId, char nCh, char nForce)
{
    int nBank;

    if (nId == 2 || nId == 4) {
        if (nForce == 0) {
            if (RES_GetEnemySeType(a->nAlive) == 1) {
                return;
            }
        }
    }
    if (a->nFlags & 8) {
        return;
    }
    nBank = RES_GetEnemySeBank(a->nAlive);
    if (nBank == -1) {
        return;
    }
    if (nCh != 1) {
        return;
    }
    xglSoundEffectPosID(nBank + (u16)nId, &a->fPos[0], 1, a->nSerial + 1);
    __asm__ volatile("" ::: "memory");
}

/* Stop one enemy sound effect by id */
void EnemySoundEnd(ACTOR *a, short nId)
{
    xglSoundEffectStopID(RES_GetEnemySeBank(a->nAlive) + (u16)nId, a->nSerial + 1);
    __asm__ volatile("" ::: "memory");
}

/* Stop every enemy sound plus the shared encounter channels */
void EnemySound_StopAll(char nType)
{
    ACTOR *p;
    short i;

    p = actor;
    for (i = 0; i < 16; i++) {
        EnemySound_Stop(p, nType);
        p++;
    }
    xglSoundEffectStopDirect(0x10008);
    xglSoundEffectStopDirect(0x10009);
    xglSoundEffectStopDirect(0x1000B);
}

/* Stop or fade out all of one enemy's sound channels */
void EnemySound_Stop(ACTOR *a, char nType)
{
    SOUNDWORK *sw;
    int nBank;
    short i;

    i = 1;
    nBank = RES_GetEnemySeBank(a->nAlive);
    sw = &SoundWork;
    do {
        if (nType == 1) {
            SsdFadeoutEffect((sw->aEntry[(nBank + (u16)i) >> 16].nId << 16) + (u16)i,
                             0x12C, a->nSerial + 1);
        } else {
            xglSoundEffectStopDirect(nBank + (u16)i);
        }
        i++;
    } while (i < 8);
}

/* Start a motion on an enemy with blend-in/out and playback speed 
   TODO: near-miss - fix_cc_asm.py appends a spurious nop after each cvt.s.w that follows an mtc1; the original has none */
void Enemy_Command_Motion(ACTOR *a, int nMotion, char nLoop, int nIn, int nOut,
                          int nNoBlend, int nSpeed)
{
    int nFlag;
    int nFlag2;

    nFlag = (nLoop == 1) ? 8 : 0;
    nFlag2 = nFlag | 1;
    ACT_setMotion2(a, nMotion | 0x8000, (nNoBlend == 0) ? nFlag2 : nFlag);
    if (nLoop != 1) {
        a->nUnk706 = 0;
        a->nUnk712 = 9;
    }
    a->fStiff = nSpeed / 100.0f * (1.0f / 30.0f);
    if (nIn != -1) {
        a->fMotionIn = nIn * (1.0f / 30.0f);
    }
    if (nOut != -1) {
        a->fMotionOut = nOut * (1.0f / 30.0f);
    }
    if (nIn != -1) {
        a->fMotionSpeed = a->fMotionIn;
    }
}

/* Change an enemy's freeze mode, saving and restoring its motion speed */
void Enemy_Command_Freeze(ACTOR *a, int nMode)
{
    ENEPC *p;
    u8 nFreeze;

    p = &enepc[a->nSerial];
    nFreeze = (u8)p->nFreeze;
    if (p->nFreeze == nMode) {
        return;
    }
    if ((u_int)(nFreeze - 1) < 3 && (u_int)(nMode - 1) < 3) {
        return;
    }
    p->nFreeze = nMode;
    switch (nMode) {
    case 0:
        if (p->fFreeze != -1000.0f) {
            a->fStiff = p->fFreeze;
        }
        break;
    case 1:
        p->fFreeze = -1000.0f;
        break;
    case 2:
        p->fFreeze = a->fStiff;
        a->fStiff = 0.0f;
        break;
    case 3:
        p->fFreeze = a->fStiff;
        a->fStiff = (1.0f / 30.0f);
        break;
    }
}

/* Set the turning mode of an enemy */
void Enemy_Command_Turn(ACTOR *a, char nMode)
{
    ENEPC *p;

    p = &enepc[a->nSerial];
    switch (nMode) {
    case 0:
        p->nTurn = 1;
        break;
    case 1:
        p->nTurn = 0;
        break;
    case 2:
        p->nTurn = (p->nTurn != 1);
        break;
    }
}

/* Set the lighting mode of an enemy */
void Enemy_Command_Light(ACTOR *a, char nMode)
{
    ENEPC *p;

    p = &enepc[a->nSerial];
    switch (nMode) {
    case 0:
        p->nLight = 1;
        break;
    case 1:
        p->nLight = 0;
        break;
    case 2:
        p->nLight = (p->nLight != 1);
        break;
    }
}

/* Enable or disable free-fall for an enemy */
void Enemy_Command_Stop_FreeFall(ACTOR *a, char nMode)
{
    switch (nMode) {
    case 0:
        a->nAnimFlags |= 0xC0000000;
        break;
    case 1:
        a->nAnimFlags &= 0x3FFFFFFF;
        break;
    }
}

/* Set an enemy's behaviour type and pause it */
void Enemy_Command_Type(ACTOR *a, char nType)
{
    enepc[a->nSerial].nType = nType;
    Enemy_Pause(a);
    __asm__ volatile("" ::: "memory");
}

/* Store an enemy script code and restart the enemy */
void Enemy_Command_Code(ACTOR *a, int nCode)
{
    a->nCode = nCode;
    Enemy_Init(a);
    __asm__ volatile("" ::: "memory");
}

/* Point an enemy at an actor, or at the player for id 100 */
void Enemy_Command_Target(ACTOR *a, int nId)
{
    ENEPC *p;

    p = &enepc[a->nSerial];
    if (nId == 100) {
        p->nTarget = GameLoopState.pPlayer->nSerial;
    } else {
        p->nTarget = Get_ActorNumber(nId);
    }
}

/* Make an enemy look at an actor, the player, or nothing
   TODO: near-miss - the Actor_LookAt_Release call sibling-calls (j) here, the
   original keeps jal + shared epilogue. Unlike the single-exit functions
   the trailing-barrier fix closed, this call sits in a switch case whose
   `break` target is a shared epilogue that OTHER cases also branch to; a
   trailing __asm__ volatile("":::"memory") after the call (tried) does not
   stop gcc from duplicating the reg-restore + sibcall into this arm --
   apparently a shared-epilogue-duplication decision made independently of
   the barrier. Same class as the documented-unsolved EventDoorFunc /
   tskUmnObjectTaskMain near-misses (see resume prompt). */
void Enemy_Command_LookAt(ACTOR *a, int nId)
{
    switch (nId) {
    case -1:
        Actor_LookAt_Release(a);
        break;
    case 100:
        Actor_LookAt_Set(a, 2, &GameLoopState.pPlayer->fPos[0]);
        a->nLookAt = GameLoopState.pPlayer->nSerial;
        break;
    default:
        Actor_LookAt_Set(a, 2, &actor[Get_ActorNumber(nId)].fPos[0]);
        a->nLookAt = Get_ActorNumber(nId);
        break;
    }
}

/* Set an enemy's model scale, optionally interpolated over time */
void Enemy_Command_Scale(ACTOR *a, int nScale, int nTime)
{
    ENEPC *p;

    p = &enepc[a->nSerial];
    if (nTime == 0) {
        p->nScaleTime = -1;
        p->fScale = nScale / 100.0f;
        p->fScaleTo = nScale / 100.0f;
        ACT_setMotion(a, a->nMotion);
        __asm__ volatile("" ::: "memory");
    } else {
        p->nScaleTimeMax = nTime;
        p->nScaleTime = 0;
        p->fScaleFrom = p->fScale;
        p->fScaleTo = nScale / 100.0f;
    }
}

/* Queue an action into the first free slot of an enemy's action list */
void Enemy_Command_Action(ACTOR *a, int nSet, int nAct, int nArg)
{
    ACTWORK *w;
    ENEPC *p;
    short *q;
    short i;

    w = &a->work;
    p = &enepc[a->nSerial];
    q = &p->nAction[nSet][0];
    for (i = 0; i < 4; i++) {
        if (*q == -1) {
            q[0] = nAct;
            q[0x400] = nArg;
            w->nUnk1A = 0;
            w->nUnk1E = 0;
            return;
        }
        q++;
    }
}

/* Force an encounter check against an enemy */
void Enemy_Command_Encount(ACTOR *a, char nType)
{
    Check_Encount(a, 1, nType);
    __asm__ volatile("" ::: "memory");
}

/* Move an enemy immediately, or start a timed slide to a new position */
void Enemy_Command_Sac_Move(ACTOR *a, int nTime, float fX, float fZ)
{
    ENEPC *p;

    p = &enepc[a->nSerial];
    if (nTime == 0) {
        a->fPos[0] = fX;
        a->fPos[2] = fZ;
        p->nSacMove = -1;
    } else {
        p->nStatus |= 0x10000;
        p->nSacTime = nTime;
        p->nSacMove = 0;
        p->fSacFrom[0] = a->fPos[0];
        p->fSacFrom[1] = a->fPos[1];
        p->fSacFrom[2] = a->fPos[2];
        p->fSacFrom[3] = a->fPos[3];
        p->fSacTo[0] = fX;
        p->fSacTo[1] = a->fPos[1];
        p->fSacTo[2] = fZ;
        p->fSacTo[3] = a->fPos[3];
    }
}

/* Turn an enemy immediately, or start a timed turn to a new angle */
void Enemy_Command_Sac_Turn(ACTOR *a, int nTime, short nIdx, float fAngle, float fDir)
{
    ENEPC *p;

    p = &enepc[a->nSerial];
    fAngle = fAngle / 180.0f * 3.141592741f;
    if (fDir <= 0.0f) {
        if (a->fDir[nIdx] < fAngle) {
            fAngle -= 6.283185482f;
        }
    } else {
        if (fAngle < a->fDir[nIdx]) {
            fAngle += 6.283185482f;
        }
    }
    if (nTime == 0) {
        a->fDir[nIdx] = fAngle;
        p->nSacTurn = -1;
        a->fAngle = fAngle;
    } else {
        p->nStatus |= 0x10000;
        p->nSacTurnTime = nTime;
        p->fTurnFrom = a->fDir[nIdx];
        p->fTurnTo = fAngle;
        p->nSacTurn = 0;
    }
}

/* Load the enemy tables and marker models into the work heap */
void Enemy_SystemInit(void)
{
    int nSize;

    D_0033869C[0] = 0;
    AdrsEnemyPreset = WorkEnd;
    nSize = xglCdReadFile("data\\matumoto\\enemy.dat", WorkEnd, 0, 0);
    WorkEnd += nSize;
    AdrsEnemySpline = WorkEnd;
    nSize = xglCdReadFile("data\\matumoto\\spline.dat", WorkEnd, 0, 0);
    WorkEnd += nSize;
    AdrsEnemyExclamation = WorkEnd;
    nSize = xglCdReadFile("data\\matumoto\\bikkuri.lex", WorkEnd, 0, 0);
    WorkEnd += nSize;
    AdrsEnemyQuestion = WorkEnd;
    nSize = xglCdReadFile("data\\matumoto\\hatena.lex", WorkEnd, 0, 0);
    WorkEnd += nSize;
    AdrsEnemySphere = WorkEnd;
    nSize = xglCdReadFile("data\\matumoto\\maru.lex", WorkEnd, 0, 0);
    WorkEnd += nSize;
    AdrsEnemySquare = WorkEnd;
    nSize = xglCdReadFile("data\\matumoto\\sikaku.lex", WorkEnd, 0, 0);
    WorkEnd += nSize;
}

/* Read one enemy's preset file by name into the given buffer */
void Enemy_LoadPreset(void *pBuf, char *pName)
{
    char aPath[0x100] = "data\\matumoto\\";

    strcat(aPath, pName);
    strcat(aPath, ".dat");
    AdrsEnemyPreset = pBuf;
    xglCdReadFile(aPath, pBuf, 0, 0);
}

extern u16 D_00490DB8[];
float Get_Distance3D(float *, float *);
void Check_Discovery(ACTOR *);

/* Alert nearby enemies that can hear a sound at the supplied position
   TODO: near-miss - exact instruction count and control flow, but gcc hoists
   actor+0x128 and enepc+0x38AC as separate induction bases.  The original
   keeps actor and nested-work pointers plus byte offsets, then materializes
   the enepc base after Get_Distance3D; this cascades through the saved regs. */
void Enemy_FindByEar(ACTOR *pIgnore, float *pPos)
{
    ACTOR *pActor;
    float fDistance;
    short i;

    if ((D_00490DB8[0] & 0x200) == 0) {
        for (i = 0; i < 64; i++) {
            pActor = &actor[i];
            if (pIgnore != pActor) {
                if (*(float *)((char *)&actor[i] + 0x128) != 0.0f) {
                    fDistance = Get_Distance3D(pPos, &pActor->fPos[0]);
                    if (enepc[i].nFreeze == 0 &&
                        *(short *)((char *)&enepc[i] + 0x38AC) <= 0 &&
                        fDistance < *(float *)((char *)&actor[i] + 0x128)) {
                        Check_Discovery(pActor);
                    }
                }
            }
        }
    }
}
