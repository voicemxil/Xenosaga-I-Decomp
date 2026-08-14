/*
 * newlib 1.8.2 libc/stdio/wsetup.c, transcribed verbatim, with the
 * CHECK_INIT / HASUB / FREEUB macros from stdio's local.h expanded-in
 * as newlib wrote them. _impure_ptr sits at 0x4AD9E8, loaded exactly
 * as in libc.c: an array whose [0] is the live reent pointer.
 */

#include "newlib_stdio.h"

#define NULL 0
#define EOF (-1)

#define __SLBF 0x0001 /* line buffered */
#define __SNBF 0x0002 /* unbuffered */
#define __SRD 0x0004  /* OK to read */
#define __SWR 0x0008  /* OK to write */
#define __SRW 0x0010  /* open for reading & writing */
#define __SEOF 0x0020 /* found EOF */

/* Only __sdidinit is needed from the reent structure here. */
struct _reent_ws
{
  char _pad0[56];
  int __sdidinit; /* 56: 1 means stdio has been init'd */
};

extern struct _reent *_impure_ptr[];

#define _REENT (_impure_ptr[0])

extern void _free_r (struct _reent *, char *);
extern void __smakebuf ();
extern void __sinit ();

#define CHECK_INIT(fp) \
  do					\
    {					\
      if ((fp)->_data == 0)		\
	(fp)->_data = _REENT;		\
      if (!((struct _reent_ws *) (fp)->_data)->__sdidinit)	\
	__sinit ((fp)->_data);		\
    }					\
  while (0)

#define	HASUB(fp) ((fp)->_ub._base != NULL)
#define	FREEUB(fp) { \
	if ((fp)->_ub._base != (fp)->_ubuf) \
		_free_r(fp->_data, (char *)(fp)->_ub._base); \
	(fp)->_ub._base = NULL; \
}

/*
 * Various output routines call wsetup to be sure it is safe to write,
 * because either _flags does not include __SWR, or _buf is NULL.
 * _wsetup returns 0 if OK to write, nonzero otherwise.
 */

int
__swsetup (fp)
     register FILE *fp;
{
  /* Make sure stdio is set up.  */

  CHECK_INIT (fp);

  /*
   * If we are not writing, we had better be reading and writing.
   */

  if ((fp->_flags & __SWR) == 0)
    {
      if ((fp->_flags & __SRW) == 0)
	return EOF;
      if (fp->_flags & __SRD)
	{
	  /* clobber any ungetc data */
	  if (HASUB (fp))
	    FREEUB (fp);
	  fp->_flags &= ~(__SRD | __SEOF);
	  fp->_r = 0;
	  fp->_p = fp->_bf._base;
	}
      fp->_flags |= __SWR;
    }

  /*
   * Make a buffer if necessary, then set _w.
   */
  /* NOT NEEDED FOR CYGNUS SPRINTF ONLY jpg */
  if (fp->_bf._base == NULL)
    __smakebuf (fp);

  if (fp->_flags & __SLBF)
    {
      /*
       * It is line buffered, so make _lbfsize be -_bufsize
       * for the putc() macro.  We will change _lbfsize back
       * to 0 whenever we turn off __SWR.
       */
      fp->_w = 0;
      fp->_lbfsize = -fp->_bf._size;
    }
  else
    fp->_w = fp->_flags & __SNBF ? 0 : fp->_bf._size;

  return 0;
}
