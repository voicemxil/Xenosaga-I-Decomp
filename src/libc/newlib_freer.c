/*
 * newlib era mallocr.c, free member: _malloc_trim_r is byte-exact;
 * _free_r itself is 11 diff words and stays unregistered.
 *
 * 2026-08-14: the 11d cluster is the __malloc_trim_threshold load --
 * gcc itself splits it (lui %hi / ld %lo), but the original schedules
 * `ori $3,$8,1` and the two sw's INTO the load shadow and gives the
 * loaded value $4 with the address in $3 (ours: value $3, address $4,
 * uses ordered before the lui).  Ruled out: every fixer-flag ablation
 * (identical with or without --barrier-return-store/--barrier-branch-
 * move/--expand-sym-loads), -msplit-addresses (bit-for-bit no-op in
 * this gcc config), -fno-schedule-insns/-fno-schedule-insns2 (much
 * worse, break _malloc_trim_r too).  Pure sched2+regalloc tie-break,
 * same hard-tail category as _vfprintf_r's ndig cluster.
 */
/* See newlib_mallocr.c: the EE's size_t is narrower than its long. */
#define SIZE_T_SMALLER_THAN_LONG
#define INTERNAL_NEWLIB
#define HAVE_MMAP 0
#define MALLOC_ALIGNMENT 16
#define DEFINE_FREE
#include "mallocr.c"
