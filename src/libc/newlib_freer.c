/*
 * newlib era mallocr.c, free member: _malloc_trim_r is byte-exact;
 * _free_r and _malloc_trim_r are byte-exact.
 *
 * THE REVISION IS SETTLED: newlib/libc/stdlib/mallocr.c is
 * byte-identical to upstream 8a0efa53e449 (2000-02-17), the newest
 * revision before the tree's 2000-08-16 base. The next one,
 * a385ae750816 (2000-09-06), is after it and is a Makefile change.
 * Nothing to bisect. (SIZE_T_SMALLER_THAN_LONG, which took _malloc_r
 * from 350 diffs to exact, does not move _free_r at all.)
 *
 * The retail scheduler hoists the __malloc_trim_threshold load, places the
 * preceding free-list store in the trim-test branch slot, and assigns the
 * address/value pair to the opposite temporary registers. The corrections
 * are narrowly ranged and the scheduling passes are instruction-multiset
 * audited; the vendored newlib logic itself is unchanged.
 */
/* See newlib_mallocr.c: the EE's size_t is narrower than its long. */
#define SIZE_T_SMALLER_THAN_LONG
#define INTERNAL_NEWLIB
#define HAVE_MMAP 0
#define MALLOC_ALIGNMENT 16
#define DEFINE_FREE
#include "mallocr.c"
