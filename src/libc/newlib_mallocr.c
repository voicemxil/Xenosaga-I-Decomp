/*
 * newlib era mallocr.c, malloc member. malloc_extend_top and _malloc_r
 * are both byte-exact. _malloc_r was the tree's second-longest-standing
 * WIP; the Sony-side config delta earlier sweeps suspected was real and
 * is SIZE_T_SMALLER_THAN_LONG, see below.
 */
/* The EE has 32-bit size_t and 64-bit long, which is exactly the case
   mallocr.c's SIZE_T_SMALLER_THAN_LONG exists for: without it,
   long_sub_size_t(x, y) is a plain (long)(x - y), and a negative
   difference comes out as a positive 0x00000000FFFFFFFF instead of
   sign-extending.  Sony's build defines it -- the original _malloc_r
   computes BOTH y - x and x - y around every size comparison, which is
   the macro's conditional form and nothing else.  Defining it took
   _malloc_r from 350 diff words (and 200 bytes short at every mallocr
   CVS rev, which is what sent earlier sweeps looking for the wrong
   source revision) to byte-exact.  It is a correctness fix as well as a
   matching one. */
#define SIZE_T_SMALLER_THAN_LONG
#define INTERNAL_NEWLIB
#define HAVE_MMAP 0
#define MALLOC_ALIGNMENT 16
#define DEFINE_MALLOC
#include "mallocr.c"
