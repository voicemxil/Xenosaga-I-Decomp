/* Post-effect filter packet builders for the normal-map model renderer */

typedef unsigned char u_char;
typedef unsigned short u_short;
typedef unsigned int u_int;
typedef unsigned long u_long;

typedef struct {
    char pad[0x234];
    int nFilterFlag;         /* 0x234 */
} LAYOUT_FILTER;

void nmlModelSetFilterGunosys(LAYOUT_FILTER *pLayout, int nFlag, void *pTag);
void nmlModelSetFilterStealth(LAYOUT_FILTER *pLayout, int nFlag);

/* TODO: near-miss (LENGTH, 54 orig vs 27 built words) - control flow/logic
 * verified correct against the original disasm (loop tests bit i against the
 * literal constants 1 and 2, so it only ever resolves at i=0 or i=1, but that
 * is not something gcc's register allocator can see). The original spills
 * BOTH parameters (pLayout, pTag) plus 5 loop constants into callee-saved
 * s0-s7/ra, conservatively treating them as live across all 31 possible loop
 * iterations before a call. Neither the 2.96 game compiler nor the 2.9-ee SDK
 * compiler reproduces this: both keep everything in caller-saved temps and
 * emit a much shorter function (tested both -- see mpeg.c's _sequenceHeader
 * for where the 2.9-ee guess DID pay off elsewhere in this session). Also
 * missing: the original's `bnezl` annulled-loop-header shape (mine emits
 * plain `bnez`/`beqz`). Left unregistered; a register-pinning or asm-barrier
 * approach may be needed, not attempted within budget. */
/* Dispatch the active filter flag bit to its Gunosys/Stealth handler */
void nmlFilterSetPacket(LAYOUT_FILTER *pLayout, void *pTag)
{
    int i;
    int nFlags;
    int v;

    nFlags = pLayout->nFilterFlag;
    for (i = 0; i < 31; i++) {
        v = nFlags & (1 << i);
        if (v == 1) {
            nFlags &= ~1;
            nmlModelSetFilterGunosys(pLayout, nFlags, pTag);
            break;
        }
        if (v == 2) {
            nFlags &= ~2;
            nmlModelSetFilterStealth(pLayout, nFlags);
            break;
        }
    }
}
