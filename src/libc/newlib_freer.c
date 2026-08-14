/*
 * newlib era mallocr.c, free member: _malloc_trim_r is byte-exact;
 * _free_r itself is 11 diff words and stays unregistered.
 */
#define INTERNAL_NEWLIB
#define HAVE_MMAP 0
#define MALLOC_ALIGNMENT 16
#define DEFINE_FREE
#include "mallocr.c"
