#include "matching.h"

/* Steering: an empty volatile asm emits nothing, but it stops gcc 2.96's
 * sibling-call pass from rewriting the trailing `jal eMessageMain` +
 * return into `j eMessageMain`. The original build did not sibcall-convert
 * the switch arm that falls into the epilogue. Vanishes in a portable
 * build. */
#ifdef MATCHING
#define NO_SIBCALL() __asm__ __volatile__("")
#else
#define NO_SIBCALL() ((void) 0)
#endif

/* tskFile* - the memory-card / save-file screen tasks.
 *
 * Each entry point is a task handler called with the task node (whose
 * nState at +0x10 splits set-up from the per-frame step) and the
 * screen's own work block. */

/* A 0x44-byte eMessage object. */
typedef struct {
    char pad00;
    unsigned char nFont;       /* 0x01 */
    char pad02[2];
    short nX;                  /* 0x04 */
    short nY;                  /* 0x06 */
    int nColor;                /* 0x08 */
    char pad0C[0x18 - 0x0C];
    char *pText;               /* 0x18 */
    char pad1C[0x44 - 0x1C];
} EMESSAGE;

typedef struct TSK_NODE {
    char pad000[0x10];
    unsigned char nState;      /* 0x10 */
} TSK_NODE;

typedef struct {
    unsigned char bBusy : 1;   /* 0x00 bit 0: a card operation is running */
    char pad001[1];
    unsigned char nMsg;        /* 0x02: which "...ing" caption to show */
    unsigned char nState;      /* 0x03 */
} FILE_WORK;

extern FILE_WORK *FileWork;

extern void eMessageSet(EMESSAGE *pMsg, char *pText);
extern void eMessageMain(EMESSAGE *pMsg);
extern void MoveSlide(short *pPos, short *pTarget, float fSpeed);

/* --- the "Saving... / Loading... / Formatting..." progress caption --- */

typedef struct {
    char pad000[4];
    int nColor;                /* 0x04 */
    unsigned char bShown;      /* 0x08 */
    char pad009[1];
    short nTarget;             /* 0x0A */
    EMESSAGE msg;              /* 0x0C */
} FILE_EX_WORK;

/* Slides the busy caption in from the right while the card is working
 * (states 44/45/80/132) and back off-screen once it finishes; the
 * caption itself is picked by FileWork->nMsg, except for the format
 * state which has its own text and a higher resting line. */
void tskFileEx(TSK_NODE *node, FILE_EX_WORK *w)
{
    static char *msg00[] = { "Saving...", "Loading...", "Formatting..." };

    switch (node->nState) {
    case 0:
        w->nColor = 0x00FFFFFF;
        eMessageSet(&w->msg, 0);
        w->msg.nColor = w->nColor;
        w->msg.nFont = 32;
        w->bShown = 0;
        break;
    case 2:
        switch (FileWork->nState) {
        case 44:
        case 45:
        case 80:
        case 132:
            if (FileWork->bBusy) {
                if (w->bShown == 0) {
                    w->bShown = 1;
                    w->msg.nX = 544;
                    if (FileWork->nState == 132) {
                        w->nTarget = 192;
                        w->msg.pText = msg00[2];
                        w->msg.nY = 192;
                    } else {
                        w->nTarget = 224;
                        w->msg.nY = 256;
                        w->msg.pText = msg00[FileWork->nMsg];
                    }
                }
            }
            break;
        default:
            w->nTarget = -140;
            if (w->msg.nX == -140) {
                w->bShown = 0;
            }
            break;
        }
        if (w->bShown != 0) {
            MoveSlide(&w->msg.nX, &w->nTarget, 3.0f);
            eMessageMain(&w->msg);
            NO_SIBCALL();
        }
        break;
    }
}
