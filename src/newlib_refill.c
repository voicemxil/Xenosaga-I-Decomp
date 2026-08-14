/*
 * newlib 1.8.2 libc/stdio/refill.c, transcribed verbatim: lflush and
 * __srefill. One deviation from the 1.8.2 tree: stock 1.8.2 lflush has
 * the classic precedence bug `== __SLBF | __SWR`, which folds to an
 * unconditional fflush tail call (20 bytes). The original is 52 bytes
 * of real flag test -- Sony's newlib carried the fixed form
 * `== (__SLBF | __SWR)`, so that is what this file uses.
 * CHECK_INIT/HASUB/FREEUB from stdio's local.h.
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
#define __SERR 0x0040 /* found error */
#define __SMOD 0x2000 /* true => fgetline modified _buf */

/* Only __sdidinit is needed from the reent structure here. */
struct _reent_rf
{
  char _pad0[56];
  int __sdidinit; /* 56: 1 means stdio has been init'd */
};

extern struct _reent *_impure_ptr[];

#define _REENT (_impure_ptr[0])

extern int fflush (FILE *);
extern int _fwalk ();
extern void __sinit ();
extern void __smakebuf ();
extern void _free_r (struct _reent *, char *);

#define CHECK_INIT(fp) \
  do					\
    {					\
      if ((fp)->_data == 0)		\
	(fp)->_data = _REENT;		\
      if (!((struct _reent_rf *) (fp)->_data)->__sdidinit)	\
	__sinit ((fp)->_data);		\
    }					\
  while (0)

#define	HASUB(fp) ((fp)->_ub._base != NULL)
#define	FREEUB(fp) { \
	if ((fp)->_ub._base != (fp)->_ubuf) \
		_free_r(fp->_data, (char *)(fp)->_ub._base); \
	(fp)->_ub._base = NULL; \
}

static int
lflush (fp)
     FILE *fp;
{
  if ((fp->_flags & (__SLBF | __SWR)) == (__SLBF | __SWR))
    return fflush (fp);
  return 0;
}

/*
 * Refill a stdio buffer.
 * Return EOF on eof or error, 0 otherwise.
 */

int
__srefill (fp)
     register FILE *fp;
{
  /* make sure stdio is set up */

  CHECK_INIT (fp);

  fp->_r = 0;			/* largely a convenience for callers */

  /* SysV does not make this test; take it out for compatibility */
  if (fp->_flags & __SEOF)
    return EOF;

  /* if not already reading, have to be reading and writing */
  if ((fp->_flags & __SRD) == 0)
    {
      if ((fp->_flags & __SRW) == 0)
	return EOF;
      /* switch to reading */
      if (fp->_flags & __SWR)
	{
	  if (fflush (fp))
	    return EOF;
	  fp->_flags &= ~__SWR;
	  fp->_w = 0;
	  fp->_lbfsize = 0;
	}
      fp->_flags |= __SRD;
    }
  else
    {
      /*
       * We were reading.  If there is an ungetc buffer,
       * we must have been reading from that.  Drop it,
       * restoring the previous buffer (if any).  If there
       * is anything in that buffer, return.
       */
      if (HASUB (fp))
	{
	  FREEUB (fp);
	  if ((fp->_r = fp->_ur) != 0)
	    {
	      fp->_p = fp->_up;
	      return 0;
	    }
	}
    }

  if (fp->_bf._base == NULL)
    __smakebuf (fp);

  /*
   * Before reading from a line buffered or unbuffered file,
   * flush all line buffered output files, per the ANSI C
   * standard.
   */

  if (fp->_flags & (__SLBF | __SNBF))
    (void) _fwalk (fp->_data, lflush);
  fp->_p = fp->_bf._base;
  fp->_r = (*fp->_read) (fp->_cookie, (char *) fp->_p, fp->_bf._size);
  fp->_flags &= ~__SMOD;	/* buffer contents are again pristine */
  if (fp->_r <= 0)
    {
      if (fp->_r == 0)
	fp->_flags |= __SEOF;
      else
	{
	  fp->_r = 0;
	  fp->_flags |= __SERR;
	}
      return EOF;
    }
  return 0;
}
