/* Sequence layer - per-actor/per-unit animation channels (translate, rotate,
   scale, motion) driven either by splines or by linear interpolation */

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u_int;

typedef struct {
    int nFlags;                     /* 0x00 */
    float fFrame;                   /* 0x04 */
    float fSpeed;                   /* 0x08 */
    float fBlendIn;                 /* 0x0C */
    float fBlendOut;                /* 0x10 */
    u8 pad14[0x10];                 /* 0x14 */
    float fRate;                    /* 0x24 */
} ANM;

typedef struct ACTOR {
    int nFlags;                     /* 0x000 */
    u8 pad004[0xC];                 /* 0x004 */
    float fPos[4];                  /* 0x010 */
    u8 pad020[0x30];                /* 0x020 */
    float fRot[4];                  /* 0x050 */
    float fScale[4];                /* 0x060 */
    u8 pad070[0x10];                /* 0x070 */
    u8 nSerial;                     /* 0x080 */
    u8 pad081[0x66F];               /* 0x081 */
    ANM anim;                       /* 0x6F0 */
    u8 pad714[0x35C];               /* 0x714 */
} ACTOR;

typedef struct {
    int nFlags;                     /* 0x00 */
    float fBlendIn;                 /* 0x04 */
    float fSpeed;                   /* 0x08 */
    float fBlend;                   /* 0x0C */
    float fBlendOut;                /* 0x10 */
} UNITMTN;

typedef struct UNIT {
    int nFlags;                     /* 0x000 */
    u8 pad004[0xC];                 /* 0x004 */
    float fPos[4];                  /* 0x010 */
    float fRot[4];                  /* 0x020 */
    float fScale[4];                /* 0x030 */
    u8 pad040[0x60];                /* 0x040 */
    u8 nSerial;                     /* 0x0A0 */
    u8 pad0A1[0x5F];                /* 0x0A1 */
    UNITMTN mtn;                    /* 0x100 */
} UNIT;

/* Spline-driven view of one sequence channel */
typedef struct {
    void *pSpl;                     /* 0x00 */
    short nTime;                    /* 0x04 */
    short nEnd;                     /* 0x06 */
    u16 nMode;                      /* 0x08 */
    u8 pad0A[0x76];                 /* 0x0A */
} SEQ_SPL;

/* Interpolated view of one sequence channel */
typedef struct {
    int nMode;                      /* 0x00 */
    int nCount;                     /* 0x04 */
    u8 pad08[0x18];                 /* 0x08 */
    float fFrom[4];                 /* 0x20 */
    float fTo[4];                   /* 0x30 */
    float fDelta[4];                /* 0x40 */
    u8 pad50[0x30];                 /* 0x50 */
} SEQ_LIN;

typedef union {
    SEQ_SPL spl;                    /* 0x00 */
    SEQ_LIN lin;                    /* 0x00 */
} SEQ_WORK;

/* Motion channel */
typedef struct {
    float fTime;                    /* 0x00 */
    int f04;                        /* 0x04 */
    int nMotion;                    /* 0x08 */
    int f0C;                        /* 0x0C */
    short nBlendIn;                 /* 0x10 */
    short nBlendOut;                /* 0x12 */
    u8 nRate;                       /* 0x14 */
    u8 pad15[0x3];                  /* 0x15 */
    int nFlags;                     /* 0x18 */
    float fSpeed;                   /* 0x1C */
    u8 pad20[0x60];                 /* 0x20 */
} SEQ_MTN;

typedef struct {
    int nState;                     /* 0x000 */
    int nFlags;                     /* 0x004 */
    u8 pad008[0x20];                /* 0x008 */
    void (*pFunc)(ACTOR *);         /* 0x028 */
    u8 pad02C[0x8];                 /* 0x02C */
    int nCount;                     /* 0x034 */
    SEQ_WORK mov;                   /* 0x038 */
    SEQ_WORK rot;                   /* 0x0B8 */
    SEQ_MTN  mtn;                   /* 0x138 */
    SEQ_WORK scl;                   /* 0x1B8 */
    u8 pad238[0x8];                 /* 0x238 */
    float fRand[4];                 /* 0x240 */
    u8 pad250[0x10];                /* 0x250 */
} SEQUENCE;

extern SEQUENCE actSequence[];
extern SEQUENCE unitSequence[];

void SPL_getValueXYZ(float *, void *, float);
float xglAtan2(float, float);
int xglSRand(void);
void ACT_setMotion2(ACTOR *, int, int);
void MAP_setUnitMotion(UNIT *, int);

static int mtnIDLE[4] = { 1, 6, 0xB, 9 };

/* Advance the actor's spline-driven scale channel */
void SEQ_scaleSPL(ACTOR *a)
{
    SEQUENCE *p = &actSequence[a->nSerial];
    SEQ_SPL *s = &p->scl.spl;
    void *pSpl = s->pSpl;
    float v[3];

    if ((p->nState & 0xE0) == 0) {
        p->nState |= 0xE0;
        s->nTime = 0;
    }
    SPL_getValueXYZ(v, pSpl, (float)s->nTime);
    a->fScale[0] = v[0];
    a->fScale[1] = v[1];
    a->fScale[2] = v[2];
    if (s->nEnd < ++s->nTime) {
        p->nFlags &= ~0xE0;
    }
}

/* Advance the actor's spline-driven rotation channel (degrees to radians) */
void SEQ_rotateSPL(ACTOR *a)
{
    SEQUENCE *p = &actSequence[a->nSerial];
    SEQ_SPL *r = &p->rot.spl;
    void *pSpl = r->pSpl;
    float v[3];

    if ((p->nState & 0xE) == 0) {
        p->nState |= 0xE;
        r->nTime = 0;
    }
    SPL_getValueXYZ(v, pSpl, (float)r->nTime);
    a->fRot[0] = v[0] / 180.0f * 3.141592741f;
    a->fRot[1] = v[1] / 180.0f * 3.141592741f;
    a->fRot[2] = v[2] / 180.0f * 3.141592741f;
    if (r->nEnd < ++r->nTime) {
        p->nFlags &= ~0xE;
    }
}

/* Queue one of the four idle motions at random */
void SEQ_setMotion(ACTOR *a)
{
    SEQUENCE *p = &actSequence[a->nSerial];
    SEQ_MTN *m = &p->mtn;
    int r;

    p->nFlags |= 0x10;
    r = xglSRand();
    m->nMotion = mtnIDLE[r & 3];
    m->nFlags = 1;
    m->fSpeed = 1.0f;
    m->nBlendIn = -1;
    m->nBlendOut = -1;
    m->nRate = 8;
}

/* Pick a random destination offset on whichever axis moves further */
void SEQ_setPositionRAND(ACTOR *a)
{
    SEQUENCE *p = &actSequence[a->nSerial];
    SEQ_LIN *m = &p->mov.lin;
    float *t = m->fFrom;

    m->fFrom[3] = 1.0f / 24.0f;
    p->nFlags |= 1;
    m->nCount = -1;
    t[0] = (float)(xglSRand() & 7) - 4.0f;
    t[2] = (float)(xglSRand() & 7) - 4.0f;
    if (__builtin_fabsf(t[0]) < __builtin_fabsf(t[2])) {
        t[0] = t[0] + p->fRand[0];
        t[2] = a->fPos[2];
    } else {
        t[0] = a->fPos[0];
        t[2] = t[2] + p->fRand[2];
    }
}

/* Hand the actor's queued motion to the animation block and step its time */
void SEQ_motion(ACTOR *a)
{
    SEQUENCE *p = &actSequence[a->nSerial];
    SEQ_MTN *m = &p->mtn;
    ANM *w = &a->anim;
    float k;
    float fIn, fOut;

    if ((p->nState & 0x10) == 0) {
        p->nState |= 0x10;
        k = 1.0f / 30.0f;
        w->nFlags |= 1;
        w->fRate = m->nRate * k;
        w->nFlags &= ~1;
        m->nFlags |= 0x20000000;
        w->fSpeed = m->fSpeed * k;
        ACT_setMotion2(a, m->nMotion, m->nFlags);
        if (m->nBlendIn >= 0) {
            fIn = m->nBlendIn * k;
            fOut = m->nBlendOut * k;
            w->fBlendIn = fIn;
            w->fBlendOut = fOut;
            if (w->fSpeed < 0.0f) {
                w->fFrame = fOut;
            } else {
                w->fFrame = fIn;
            }
        }
    }
    if ((p->nFlags & ~0x10) == 0) {
        if ((w->nFlags & 0x1000) != 0) {
            p->nFlags = 0;
        }
    }
    m->fTime += w->fSpeed;
}

/* Advance a unit's spline-driven rotation channel (degrees to radians) */
void SEQ_rotateUnitSPL(UNIT *u)
{
    SEQUENCE *p = &unitSequence[u->nSerial];
    SEQ_SPL *r = &p->rot.spl;
    void *pSpl = r->pSpl;
    float v[3];

    if ((p->nState & 0xE) == 0) {
        p->nState |= 0xE;
        r->nTime = 0;
    }
    SPL_getValueXYZ(v, pSpl, (float)r->nTime);
    u->fRot[0] = v[0] / 180.0f * 3.141592741f;
    u->fRot[1] = v[1] / 180.0f * 3.141592741f;
    u->fRot[2] = v[2] / 180.0f * 3.141592741f;
    if (r->nEnd < ++r->nTime) {
        p->nFlags &= ~0xE;
    }
}

/* Advance a unit's spline-driven scale channel */
void SEQ_scaleUnitSPL(UNIT *u)
{
    SEQUENCE *p = &unitSequence[u->nSerial];
    SEQ_SPL *s = &p->scl.spl;
    void *pSpl = s->pSpl;
    float v[3];

    if ((p->nState & 0xE0) == 0) {
        p->nState |= 0xE0;
        s->nTime = 0;
    }
    SPL_getValueXYZ(v, pSpl, (float)s->nTime);
    u->fScale[0] = v[0];
    u->fScale[1] = v[1];
    u->fScale[2] = v[2];
    if (s->nEnd < ++s->nTime) {
        p->nFlags &= ~0xE0;
    }
}

/* Advance a unit's spline-driven translation channel, facing along the path */
void SEQ_moveUnitSPL(UNIT *u)
{
    SEQUENCE *p = &unitSequence[u->nSerial];
    SEQ_SPL *m = &p->mov.spl;
    void *pSpl = m->pSpl;
    float v[3];

    if ((p->nState & 0x1) == 0) {
        p->nState |= 0x1;
        m->nTime = 0;
    }
    SPL_getValueXYZ(v, pSpl, (float)m->nTime);
    if ((p->nFlags & 0x4) == 0) {
        if ((m->nMode & 0x10) != 0) {
            u->fRot[1] = xglAtan2(v[0] - u->fPos[0], v[2] - u->fPos[2]);
        }
    }
    u->fPos[0] = v[0];
    u->fPos[1] = v[1];
    u->fPos[2] = v[2];
    if (m->nEnd < ++m->nTime) {
        p->nFlags &= ~0x1;
    }
}

/* Hand a unit's queued motion to the map layer, in seconds */
void SEQ_motionUnit(UNIT *u)
{
    SEQUENCE *p = &unitSequence[u->nSerial];
    SEQ_MTN *m = &p->mtn;
    UNITMTN *w = &u->mtn;
    float f;

    if ((p->nState & 0x10) == 0) {
        p->nState |= 0x10;
        MAP_setUnitMotion(u, m->nMotion);
        w->nFlags = m->nFlags;
        if (m->nBlendIn >= 0) {
            f = m->nBlendIn * (1.0f / 30.0f);
            w->fBlend = f;
            w->fBlendIn = f;
            w->fBlendOut = m->nBlendOut * (1.0f / 30.0f);
        }
        w->fSpeed = m->fSpeed * (1.0f / 30.0f);
    }
    if ((p->nFlags & ~0x10) == 0) {
        if ((w->nFlags & 0x1000) != 0) {
            p->nFlags = 0;
        }
    }
}

/* Rotation command state used when an actor turns toward another actor. */
typedef struct {
    void *pTarget;                  /* 0x00 */
    u8 pad04[0x10];                 /* 0x04 */
    int nFrames;                    /* 0x14 */
    u8 pad18[0x4];                  /* 0x18 */
    int nFlags;                     /* 0x1C */
} SEQ_ROT_TARGET;

extern void *D_00348684[3];
void SEQ_rotate(ACTOR *);
void ACT_setMotion(ACTOR *, int);

/* Begin the fixed turn-to-player sequence. */
void SEQ_setRotY2Player(ACTOR *a)
{
    SEQUENCE *p = &actSequence[a->nSerial];
    SEQ_ROT_TARGET *r = (SEQ_ROT_TARGET *)&p->rot;

    p->pFunc = SEQ_rotate;
    r->pTarget = D_00348684[0];
    p->nFlags |= 4;
    r->nFrames = 10;
    r->nFlags |= 2;
    ACT_setMotion(a, 7);
}
