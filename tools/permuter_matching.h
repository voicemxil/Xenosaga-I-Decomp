/* Portable (no-op) definitions of the steering macros, forced into the
 * permuter's copy of a translation unit.
 *
 * decomp-permuter parses C with pycparser, which cannot read GCC inline
 * asm -- and every steering construct in include/matching.h expands to
 * an asm statement under MATCHING. Preprocessing a source with the real
 * matching.h therefore yields something the permuter rejects outright.
 *
 * Forcing the portable definitions means the permuter searches for a
 * source shape that matches WITHOUT steering. That is the right search:
 * a shape that needs no PIN or LAUNDER is strictly better than one that
 * does, because steering is portability debt (see include/matching.h).
 * If the permuter finds a zero-score candidate, you get a match AND you
 * get to delete the steering.
 */
#ifndef MATCHING_H
#define MATCHING_H
#define PIN(decl, reg) decl
#define LAUNDER(x) ((void) 0)
#define LAUNDER2(x, y) ((void) 0)
#define LAUNDER_V(x) ((void) 0)
#define PASSTHRU(dst, src) ((dst) = (src))
#define PASSTHRU_V(dst, src) ((dst) = (src))
#define SCHED_NOP() ((void) 0)
#endif
