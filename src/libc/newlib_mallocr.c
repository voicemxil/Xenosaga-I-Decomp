/*
 * newlib era mallocr.c, malloc member: malloc_extend_top is byte-exact;
 * _malloc_r itself is WIP (350d, built 200B short of the original at
 * every mallocr CVS rev -- likely a Sony-side config delta).
 */
#define INTERNAL_NEWLIB
#define HAVE_MMAP 0
#define MALLOC_ALIGNMENT 16
#define DEFINE_MALLOC
#include "mallocr.c"
