/*
 * Actor-layer types, lifted verbatim from src/game/ACT.c (the offsets
 * were derived there function by function; the comments are original).
 * Other TUs that model ACTOR partially should migrate here as they are
 * touched.
 */

#ifndef GAME_ACTOR_H
#define GAME_ACTOR_H

#include "common.h"

typedef struct {
    u_int nSize;                    /* 0x00 */
    u8 nType;                       /* 0x04 */
    u8 nUnknown;                    /* 0x05 */
    u16 nState;                     /* 0x06 */
    u_int pData;                    /* 0x08 */
    u_int pName;                    /* 0x0C */
} RSRCITEM;

typedef struct {
    u_int pBase;                    /* 0x00 */
    u_int pCurrent;                 /* 0x04 */
    u_int nSize;                    /* 0x08 */
    RSRCITEM *pItems;               /* 0x0C */
    u16 nItemCount;                 /* 0x10 */
    u16 nItemCapacity;              /* 0x12 */
} RSRC;

typedef struct {
    u16 nUnk0;                      /* 0x00 */
    u16 nUnk2;                      /* 0x02 */
    u16 nUnk4;                      /* 0x04 */
    u16 nMatrixNum;                 /* 0x06 */
    u16 nElementNum;                /* 0x08 */
    u16 nExElementNum;              /* 0x0A */
} JOINT;

typedef struct {
    u8 pad00[0x14];                 /* 0x00 */
    u16 nFlags;                     /* 0x14 */
} ANM;

typedef struct ACTOR {
    int nFlags;                     /* 0x000 */
    void (*pUpdate)(struct ACTOR *);/* 0x004 */
    int nUnk008;                    /* 0x008 */
    u8 pad00C[0x10];                /* 0x00C */
    float fUnk01C;                  /* 0x01C */
    u8 pad020[0x1C];                /* 0x020 */
    int nUnk03C;                    /* 0x03C */
    u8 pad040[0xC];                 /* 0x040 */
    int nUnk04C;                    /* 0x04C */
    u8 pad050[0x10];                /* 0x050 */
    float fScale[4];                /* 0x060 */
    u8 pad070[0x10];                /* 0x070 */
    u8 nSerial;                     /* 0x080 */
    u8 nSignal;                     /* 0x081 */
    u8 nMatrixType;                 /* 0x082 */
    u8 pad083[0x1];                 /* 0x083 */
    short nUnk084;                  /* 0x084 */
    short nAlive;                   /* 0x086 */
    short nUnk088;                  /* 0x088 */
    u8 pad08A[0x6];                 /* 0x08A */
    u8 nShadowType;                 /* 0x090 */
    u8 nShadowSize;                 /* 0x091 */
    u8 pad092[0x42E];               /* 0x092 */
    int nVMObject;                  /* 0x4C0 */
    u8 pad4C4[0x4];                 /* 0x4C4 */
    int nUndu;                      /* 0x4C8 */
    u8 pad4CC[0x1CC];               /* 0x4CC */
    int nJointUsed;                 /* 0x698 */
    int nJointId;                   /* 0x69C */
    u8 pad6A0[0x40];                /* 0x6A0 */
    u8 aJointBusy[0x10];            /* 0x6E0 */
    u8 anim[0x14];                  /* 0x6F0 */
    u16 nAnimFlags;                 /* 0x704 */
    u8 pad706[0x16];                /* 0x706 */
    int pAnimData;                  /* 0x71C */
    int pUserData;                  /* 0x720 */
    u8 pad724[0x6C];                /* 0x724 */
    u8 producer[0x68];              /* 0x790 */
    int *pAccessories;              /* 0x7F8 */
    int nMoveElementID;             /* 0x7FC */
    u8 pad800[0x24];                /* 0x800 */
    void *pMatrix;                  /* 0x824 */
    void *pMatrixWork;              /* 0x828 */
    void *pMatrixEx;                /* 0x82C */
    void *pMatrixEx2;               /* 0x830 */
    u8 pad834[0xC];                 /* 0x834 */
    u8 model[0x90];                 /* 0x840 */
    void *pModelRes;                /* 0x8D0 */
    void *pTexture;                 /* 0x8D4 */
    void *pJoint;                   /* 0x8D8 */
    int pPack[8];                   /* 0x8DC */
    struct ACTOR *pParent;          /* 0x8FC */
    int nChildNum;                  /* 0x900 */
    struct ACTOR *pChild[7];        /* 0x904 */
    u8 pad920[0x150];               /* 0x920 */
} ACTOR;

#endif /* GAME_ACTOR_H */
