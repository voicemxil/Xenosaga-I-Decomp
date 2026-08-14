/* tskMenuPauseSkip - event-skip pause: freeze the frame, wait for
   skip/cancel input and restart the clocks */

typedef struct SKIPWORK {
    char pad00[0x10];
    int nFlags10;                  /* 0x10 */
    char pad14[0x29F40 - 0x14];
    unsigned char nSkip;           /* 0x29F40: frames left / result code */
} SKIPWORK;

typedef struct SNDWORK {
    char pad00[0x58];
    unsigned char bPause;          /* 0x58 */
} SNDWORK;

typedef struct SKIPPAD {
    char pad00[0x2A];
    unsigned short hHeld;          /* 0x2A */
} SKIPPAD;

extern SKIPWORK GameLoopState;
extern SNDWORK D_004A90E0;
extern SKIPPAD PadData;
extern int StreamPlayFlag;
extern int StreamStopFlag;

extern void xglSoundEffectNormalDirect(int n);
extern void xglSleep(void);
extern void xglRenderCopyDisp2Draw(void);
extern void GamePauseDispEvent(void);
extern void PartyTimePauseStart(void);
extern void PartyTimePauseEnd(void);
extern void xglSoundStreamStop(int n);
extern void xglSoundStreamMute(void);

/* WIP - PARKED at ~97 diffs: behaviorally complete but the retail object
 * keeps three %hi() bases (0x34/0x49/0x4B) in s2..s4 and recomputes the
 * struct addresses at the tail, while this build folds them; needs the
 * same laundering treatment as tskMenuBg.c once time allows. */
/* Returns 1 when the event was skipped (or pausing is disabled),
 * 0 when the pause was simply cancelled */
int PauseSkipCheck(void)
{
    SKIPWORK *g = &GameLoopState;

    if (g->nFlags10 & 0x90000000) {
        return 1;
    }
    if (g->nSkip != 0) {
        return 0;
    }
    if (!(PadData.hHeld & 0x800)) {
        return 0;
    }
    g->nSkip = 32;
    xglSoundEffectNormalDirect(4);
    D_004A90E0.bPause = 1;
    xglSleep();
    xglRenderCopyDisp2Draw();
    GamePauseDispEvent();
    xglSleep();
    GamePauseDispEvent();
    xglSleep();
    PartyTimePauseStart();
    while (!(PadData.hHeld & 0x10)) {
        if (g->nSkip != 0) {
            g->nSkip--;
        } else if (PadData.hHeld & 0x800) {
            break;
        }
        xglSleep();
    }
    PartyTimePauseEnd();
    xglSoundEffectNormalDirect(4);
    D_004A90E0.bPause = 0;
    g->nSkip = 32;
    if (PadData.hHeld & 0x10) {
        g->nSkip = 64;
        if (StreamPlayFlag == 0) {
            StreamStopFlag = 1;
            xglSoundStreamStop(0);
        }
        return 1;
    }
    xglSoundStreamMute();
    return 0;
}
