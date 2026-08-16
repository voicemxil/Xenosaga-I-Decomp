/*
 * newlib era mallocr.c, free member: _malloc_trim_r is byte-exact;
 * _free_r is WIP at 11 diff words and stays unregistered.
 *
 * THE REVISION IS SETTLED: newlib/libc/stdlib/mallocr.c is
 * byte-identical to upstream 8a0efa53e449 (2000-02-17), the newest
 * revision before the tree's 2000-08-16 base. The next one,
 * a385ae750816 (2000-09-06), is after it and is a Makefile change.
 * Nothing to bisect. (SIZE_T_SMALLER_THAN_LONG, which took _malloc_r
 * from 350 diffs to exact, does not move _free_r at all.)
 *
 * The 11d cluster is the __malloc_trim_threshold load. gcc emits
 *     ori $3 / sw / sw / lui $4,%hi / dsll / ld $3,%lo($4) / dsrl /
 *     sltu $2,$2,$3 / bne / daddu $4,$17,$0
 * and the original emits the same ten instructions with the ADDRESS in
 * $3 and the VALUE in $4 (the opposite assignment), the lui/ld hoisted
 * to the top of the block, and the `sw $3,4($9)` moved down into the
 * bne's delay slot. So it is a reorder PLUS a two-register role swap
 * confined to three instructions -- no available flag expresses that
 * (--swap-regs is whole-function, and $4 is live elsewhere in the
 * block), and a scoped rename would not be auditable the way
 * tools/audit_swaps.py audits a pure reorder.
 *
 * RULED OUT: every fixer-flag ablation (identical with or without
 * --barrier-return-store / --barrier-branch-move / --expand-sym-loads);
 * -msplit-addresses (bit-for-bit no-op in this gcc config);
 * -fno-schedule-insns / -fno-schedule-insns2 (much worse, they break
 * _malloc_trim_r too); and a cflag sweep of -fno-move-all-movables,
 * -fno-reduce-all-givs, -fno-inline, -finline-functions, -fno-defer-pop,
 * -mno-check-zero-division and -mgpopt (all exactly neutral),
 * -fno-optimize-register-move (19d), -fno-force-mem (22d and it breaks
 * _malloc_trim_r), -mlong32 (159d) and -fno-omit-frame-pointer (182d).
 */
/* See newlib_mallocr.c: the EE's size_t is narrower than its long. */
#define SIZE_T_SMALLER_THAN_LONG
#define INTERNAL_NEWLIB
#define HAVE_MMAP 0
#define MALLOC_ALIGNMENT 16
#define DEFINE_FREE
#include "mallocr.c"
