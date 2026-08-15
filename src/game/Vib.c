/* Controller vibration (DUALSHOCK2 pad motor) control. The motor state
 * lives inside GameLoopState (offset 0x2A000, see src/Game.c's partial
 * GAME_LOOP_STATE typedef -- this file only needs raw byte offsets into
 * it, declared as an opaque array so we don't duplicate/collide with
 * that struct) plus two active-flag bytes in PadData. */

#include "matching.h"

extern unsigned char GameLoopState[];
extern unsigned char PadData[];

/* Set the weak-motor vibration value */
void Vibration_Set_Weak(unsigned int nVal)
{
    *(unsigned int *)(GameLoopState + 0x2A004) = nVal;
}

/* The 0x2A0xx accesses must go through a *register* base. Written as
 * `GameLoopState + 0x2A004` gcc folds the symbol and the offset into one
 * %hi/%lo pair per access; the original instead materialises &GameLoopState
 * once and lets the assembler expand each big-offset store into
 * lui $at,3 / addu $at,$at,base / store. LAUNDER makes the base opaque so
 * the fold cannot happen. LAUNDER2 (not two LAUNDERs) additionally fixes
 * the order in which the two symbol lo-halves are materialised. */
void Vibration_Stop(void)
{
    unsigned char *pState = GameLoopState;
    unsigned char *pPad = PadData;
    LAUNDER2(pState, pPad);
    *(unsigned int *)(pState + 0x2A004) = 0;
    pPad[81] = 0;
    *(unsigned int *)(pState + 0x2A008) = 0;
    pPad[80] = 0;
}


/* Same register-base shape as Vibration_Stop. Two extra subtleties: the
 * 0x2A008 and 0x2A00C stores come out in the opposite order from the
 * source, and the original leaves the jr-ra delay slot empty -- gas would
 * otherwise hoist the last store into it, so SCHED_NOP supplies the pad. */
void Vibration_Set_Strong(unsigned char nA, unsigned char nB, unsigned int nVal)
{
    unsigned char *pState = GameLoopState;

    LAUNDER(pState);
    *(unsigned int *)(pState + 0x2A008) = nVal;
    pState[0x2A000] = nA;
    pState[0x2A001] = nB;
    *(unsigned int *)(pState + 0x2A00C) = nVal;
    SCHED_NOP();
}
