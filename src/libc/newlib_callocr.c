/*
 * newlib 1.8.2 _calloc_r: the calloc member of vendored mallocr.c.
 * Same configuration as newlib_reallocr.c.
 */
/* See newlib_mallocr.c: the EE's size_t is narrower than its long. */
#define SIZE_T_SMALLER_THAN_LONG
#define INTERNAL_NEWLIB
#define HAVE_MMAP 0
#define MALLOC_ALIGNMENT 16
#define DEFINE_CALLOC
#include "mallocr.c"
