/*
 * newlib 1.8.2 libc/stdio/fflush.c, transcribed verbatim, with
 * CHECK_INIT from stdio's local.h.
 */

#include "newlib_stdio.h"

#define NULL 0
#define EOF (-1)

#define __SLBF 0x0001 /* line buffered */
#define __SNBF 0x0002 /* unbuffered */
#define __SWR 0x0008  /* OK to write */
#define __SERR 0x0040 /* found error */

/* Only __sdidinit is needed from the reent structure here. */
struct _reent_ff
{
  char _pad0[56];
  int __sdidinit; /* 56: 1 means stdio has been init'd */
};

extern struct _reent *_impure_ptr[];

#define _REENT (_impure_ptr[0])

extern int _fwalk ();
extern void __sinit ();

#define CHECK_INIT(fp) \
  do					\
    {					\
      if ((fp)->_data == 0)		\
	(fp)->_data = _REENT;		\
      if (!((struct _reent_ff *) (fp)->_data)->__sdidinit)	\
	__sinit ((fp)->_data);		\
    }					\
  while (0)

/* Flush a single file, or (if fp is NULL) all files.  */

int
fflush (fp)
     register FILE *fp;
{
  register unsigned char *p;
  register int n, t;

  if (fp == NULL)
    return _fwalk (_REENT, fflush);

  CHECK_INIT (fp);

  t = fp->_flags;
  if ((t & __SWR) == 0 || (p = fp->_bf._base) == NULL)
    return 0;
  n = fp->_p - p;		/* write this much */

  /*
   * Set these immediately to avoid problems with longjmp
   * and to allow exchange buffering (via setvbuf) in user
   * write function.
   */
  fp->_p = p;
  fp->_w = t & (__SLBF | __SNBF) ? 0 : fp->_bf._size;

  while (n > 0)
    {
      t = (*fp->_write) (fp->_cookie, (char *) p, n);
      if (t <= 0)
	{
	  fp->_flags |= __SERR;
	  return EOF;
	}
      p += t;
      n -= t;
    }
  return 0;
}
