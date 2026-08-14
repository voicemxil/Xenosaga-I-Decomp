/*
 * newlib 1.8.2 libc/stdio/fread.c, transcribed verbatim.
 */

#include "newlib_stdio.h"

extern _PTR memcpy (_PTR, const _PTR, size_t);
extern int __srefill (FILE *);

size_t
fread (buf, size, count, fp)
     _PTR buf;
     size_t size;
     size_t count;
     FILE *fp;
{
  register size_t resid;
  register char *p;
  register int r;
  size_t total;

  if ((resid = count * size) == 0)
    return 0;
  if (fp->_r < 0)
    fp->_r = 0;
  total = resid;
  p = buf;
  while (resid > (r = fp->_r))
    {
      (void) memcpy ((void *) p, (void *) fp->_p, (size_t) r);
      fp->_p += r;
      /* fp->_r = 0 ... done in __srefill */
      p += r;
      resid -= r;
      if (__srefill (fp))
	{
	  /* no more input: return partial result */
	  return (total - resid) / size;
	}
    }
  (void) memcpy ((void *) p, (void *) fp->_p, resid);
  fp->_r -= resid;
  fp->_p += resid;
  return count;
}
