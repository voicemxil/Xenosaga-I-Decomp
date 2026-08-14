/*
 * newlib 1.8.2 libc/stdio/fwalk.c, transcribed verbatim. Iterates the
 * _glue chain rooted in the reent structure; __sglue sits at offset
 * 472, per the original's `addiu s2,a0,472`.
 */

#include "newlib_stdio.h"

#define NULL 0

struct _glue
{
  struct _glue *_next;
  int _niobs;
  FILE *_iobs;
};

struct _reent_fwalk
{
  char _pad0[472];
  struct _glue __sglue; /* 472: list of handlers for the FILEs */
};

#define _reent _reent_fwalk

int
_fwalk (ptr, function)
     struct _reent *ptr;
     register int (*function) ();
{
  register FILE *fp;
  register int n, ret = 0;
  register struct _glue *g;

  for (g = &ptr->__sglue; g != NULL; g = g->_next)
    for (fp = g->_iobs, n = g->_niobs; --n >= 0; fp++)
      if (fp->_flags != 0)
	ret |= (*function) (fp);
  return ret;
}
