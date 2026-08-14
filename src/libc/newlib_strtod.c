/*
 * newlib 1.9.0 libc/stdlib/strtod.c: strtodf is byte-exact; _strtod_r
 * itself is still WIP (~674 diff words, register allocation) and
 * stays unregistered.
 *
 * 2026-08-14: the vendored source is line-identical to the era-2000
 * CVS tree (/opt/newlib-2000), so the ~56-word duplicated block at
 * built word ~306 is compiler behavior, not a revision mismatch: the
 * `k = nd < DBL_DIG + 1 ? ...; rv.d = y; if (k > 9) ...` tail exists
 * twice because cross-jumping failed to merge two regalloc-divergent
 * copies that the original's compile merged.  Ruled out (each singly,
 * on top of the TU's normal settings): -fno-thread-jumps (bit-for-bit
 * no-op), -fno-cse-follow-jumps (672d, still wrong length),
 * -fno-cse-skip-blocks/-fno-peephole/-fno-caller-saves/
 * -fno-function-cse (no change), -fno-gcse/-fno-strength-reduce/
 * -fno-rerun-cse-after-loop/-fno-expensive-optimizations/
 * -fno-schedule-insns(2)/-fno-regmove/-fno-delayed-branch (all worse,
 * several also break strtod/strtodf).
 */
#include "strtod.c"
