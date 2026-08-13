/* Controller vibration (DUALSHOCK2 pad motor) control. The motor state
 * lives inside GameLoopState (offset 0x2A000, see src/Game.c's partial
 * GAME_LOOP_STATE typedef -- this file only needs raw byte offsets into
 * it, declared as an opaque array so we don't duplicate/collide with
 * that struct) plus two active-flag bytes in PadData. */

extern unsigned char GameLoopState[];
extern unsigned char PadData[];

/* Set the weak-motor vibration value */
void Vibration_Set_Weak(unsigned int nVal)
{
    *(unsigned int *)(GameLoopState + 0x2A004) = nVal;
}

/* TODO: near-miss, 2 attempts tried (raw literal addresses, then via the
 * GameLoopState/PadData symbols). Both fold to a single lui+mem-op per
 * access; the original instead loads a base once (lui v0,0x34; addiu
 * v0,v0,-31104 == &GameLoopState) and then, for EACH field access,
 * separately rematerializes a 0x30000 addend into $at (lui at,3; addu
 * at,at,v0) before applying the small residual store offset -- no CSE of
 * that $at value across the 2-3 accesses that share it. Looks like the
 * field offset is reached through a second pointer/struct layer that
 * itself needs a non-foldable runtime add, not plain byte-offset pointer
 * arithmetic on a single flat symbol. Needs the real GAME_LOOP_STATE
 * layout this deep (0x2A000+) to find that shape. */
void Vibration_Stop(void)
{
    *(unsigned int *)(GameLoopState + 0x2A004) = 0;
    PadData[81] = 0;
    *(unsigned int *)(GameLoopState + 0x2A008) = 0;
    PadData[80] = 0;
}

/* TODO: near-miss, see Vibration_Stop's note -- same base+offset shape. */
void Vibration_Set_Strong(unsigned char nA, unsigned char nB, unsigned int nVal)
{
    *(unsigned int *)(GameLoopState + 0x2A00C) = nVal;
    GameLoopState[0x2A000] = nA;
    GameLoopState[0x2A001] = nB;
    *(unsigned int *)(GameLoopState + 0x2A008) = nVal;
}
