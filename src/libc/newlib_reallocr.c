/*
 * newlib 1.8.2 _realloc_r: compiles the vendored mallocr.c exactly as
 * newlib's build does for the realloc member. MALLOC_ALIGNMENT=16 --
 * PS2 malloc aligns to the quadword, visible in the original's
 * request2size constants (+19 / &-16 instead of +11 / &-8).
 */
/* See newlib_mallocr.c: the EE's size_t is narrower than its long. */
#define SIZE_T_SMALLER_THAN_LONG
#define INTERNAL_NEWLIB
#define HAVE_MMAP 0
#define MALLOC_ALIGNMENT 16
#define DEFINE_REALLOC
#include "mallocr.c"
