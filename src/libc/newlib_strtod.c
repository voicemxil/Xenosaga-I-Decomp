/*
 * newlib libc/stdlib/strtod.c. strtodf is byte-exact; _strtod_r is WIP
 * at 674 diff words (built 1003 words vs 965 original).
 *
 * THE REVISION IS SETTLED. 2026-08-15: this file is byte-identical to
 * upstream c87be3e4d6ae (2000-04-17), which is the newest revision of
 * strtod.c before the tree's 2000-08-16 newlib base -- the only later
 * one in the whole mirror is 52cb9e6934c1 (2001-04-20), and it changes
 * exactly one line, `return strtod (s00, se);` to `return _strtod_r
 * (_REENT, s00, se);` in strtodf, which is byte-exact as it stands.
 * There is no revision left to try.
 *
 * So the ~56-word duplicated block at built word ~306 is compiler
 * behaviour: the `k = nd < DBL_DIG + 1 ? ...; rv.d = y; if (k > 9) ...`
 * tail exists twice because cross-jumping failed to merge two
 * regalloc-divergent copies that the original's compile merged. Same
 * category as _vfprintf_r and _free_r -- permuter work.
 *
 * RULED OUT (each singly, on top of the TU's normal settings):
 *  - -fno-thread-jumps (bit-for-bit no-op), -fno-cse-follow-jumps
 *    (672d, still the wrong length), -fno-cse-skip-blocks, -fno-peephole,
 *    -fno-caller-saves, -fno-function-cse (no change), -fno-gcse,
 *    -fno-strength-reduce, -fno-rerun-cse-after-loop,
 *    -fno-expensive-optimizations, -fno-schedule-insns(2), -fno-regmove,
 *    -fno-delayed-branch (all worse, several also break strtod/strtodf).
 *  - config macros: ROUND_BIASED 672 (noise), RND_PRODQUOT, NO_REENT,
 *    Honor_FLT_ROUNDS, Check_FLT_ROUNDS, Avoid_Underflow, SET_INEXACT,
 *    NO_STRTOD_BIGCOMP and IEEE_Arith are all exactly neutral;
 *    Sudden_Underflow is worse (780).
 */
#include "strtod.c"
