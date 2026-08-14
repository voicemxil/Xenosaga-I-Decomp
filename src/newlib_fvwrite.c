/*
 * newlib 1.8.2 libc/stdio/fvwrite.c, transcribed verbatim, with the
 * __siov/__suio descriptors from fvwrite.h and the cantwrite() macro
 * from stdio's local.h.
 */

#include "newlib_stdio.h"

#define EOF (-1)
#define BUFSIZ 1024

#define __SLBF 0x0001 /* line buffered */
#define __SNBF 0x0002 /* unbuffered */
#define __SWR 0x0008  /* OK to write */
#define __SERR 0x0040 /* found error */
#define __SSTR 0x0200 /* this is an sprintf/snprintf string */

#define NULL 0

/* I/O descriptors for __sfvwrite(), from fvwrite.h. */
struct __siov
{
  const _PTR iov_base;
  size_t iov_len;
};
struct __suio
{
  struct __siov *uio_iov;
  int uio_iovcnt;
  int uio_resid;
};

extern _PTR memmove (_PTR, const _PTR, size_t);
extern _PTR memchr (const _PTR, int, size_t);
extern int fflush (FILE *);
extern int __swsetup (FILE *);

/* Return true iff the given FILE cannot be written now.  */
#define	cantwrite(fp) \
  ((((fp)->_flags & __SWR) == 0 || (fp)->_bf._base == NULL) && \
   __swsetup(fp))

#define	MIN(a, b) ((a) < (b) ? (a) : (b))
#define	COPY(n)	  (void) memmove((void *) fp->_p, (void *) p, (size_t) (n))

#define GETIOV(extra_work) \
  while (len == 0) \
    { \
      extra_work; \
      p = iov->iov_base; \
      len = iov->iov_len; \
      iov++; \
    }

/*
 * Write some memory regions.  Return zero on success, EOF on error.
 *
 * This routine is large and unsightly, but most of the ugliness due
 * to the three different kinds of output buffering is handled here.
 */

int
__sfvwrite (fp, uio)
     register FILE *fp;
     register struct __suio *uio;
{
  register size_t len;
  register const char *p;
  register struct __siov *iov;
  register int w, s;
  char *nl;
  int nlknown, nldist;

  if ((len = uio->uio_resid) == 0)
    return 0;

  /* make sure we can write */
  if (cantwrite (fp))
    return EOF;

  iov = uio->uio_iov;
  len = 0;
  if (fp->_flags & __SNBF)
    {
      /*
       * Unbuffered: write up to BUFSIZ bytes at a time.
       */
      do
	{
	  GETIOV (;);
	  w = (*fp->_write) (fp->_cookie, p, MIN (len, BUFSIZ));
	  if (w <= 0)
	    goto err;
	  p += w;
	  len -= w;
	}
      while ((uio->uio_resid -= w) != 0);
    }
  else if ((fp->_flags & __SLBF) == 0)
    {
      /*
       * Fully buffered: fill partially full buffer, if any,
       * and then flush.  If there is no partial buffer, write
       * one _bf._size byte chunk directly (without copying).
       *
       * String output is a special case: write as many bytes
       * as fit, but pretend we wrote everything.  This makes
       * snprintf() return the number of bytes needed, rather
       * than the number used, and avoids its write function
       * (so that the write function can be invalid).
       */
      do
	{
	  GETIOV (;);
	  w = fp->_w;
	  if (fp->_flags & __SSTR)
	    {
	      if (len < w)
		w = len;
	      COPY (w);		/* copy MIN(fp->_w,len), */
	      fp->_w -= w;
	      fp->_p += w;
	      w = len;		/* but pretend copied all */
	    }
	  else if (fp->_p > fp->_bf._base && len > w)
	    {
	      /* fill and flush */
	      COPY (w);
	      /* fp->_w -= w; *//* unneeded */
	      fp->_p += w;
	      if (fflush (fp))
		goto err;
	    }
	  else if (len >= (w = fp->_bf._size))
	    {
	      /* write directly */
	      w = (*fp->_write) (fp->_cookie, p, w);
	      if (w <= 0)
		goto err;
	    }
	  else
	    {
	      /* fill and done */
	      w = len;
	      COPY (w);
	      fp->_w -= w;
	      fp->_p += w;
	    }
	  p += w;
	  len -= w;
	}
      while ((uio->uio_resid -= w) != 0);
    }
  else
    {
      /*
       * Line buffered: like fully buffered, but we
       * must check for newlines.  Compute the distance
       * to the first newline (including the newline),
       * or `infinity' if there is none, then pretend
       * that the amount to write is MIN(len,nldist).
       */
      nlknown = 0;
      do
	{
	  GETIOV (nlknown = 0);
	  if (!nlknown)
	    {
	      nl = memchr ((void *) p, '\n', len);
	      nldist = nl ? nl + 1 - p : len + 1;
	      nlknown = 1;
	    }
	  s = MIN (len, nldist);
	  w = fp->_w + fp->_bf._size;
	  if (fp->_p > fp->_bf._base && s > w)
	    {
	      COPY (w);
	      /* fp->_w -= w; */
	      fp->_p += w;
	      if (fflush (fp))
		goto err;
	    }
	  else if (s >= (w = fp->_bf._size))
	    {
	      w = (*fp->_write) (fp->_cookie, p, w);
	      if (w <= 0)
		goto err;
	    }
	  else
	    {
	      w = s;
	      COPY (w);
	      fp->_w -= w;
	      fp->_p += w;
	    }
	  if ((nldist -= w) == 0)
	    {
	      /* copied the newline: flush and forget */
	      if (fflush (fp))
		goto err;
	      nlknown = 0;
	    }
	  p += w;
	  len -= w;
	}
      while ((uio->uio_resid -= w) != 0);
    }
  return 0;

err:
  fp->_flags |= __SERR;
  return EOF;
}
