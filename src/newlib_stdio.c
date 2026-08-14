/*
 * newlib 1.8.2 libc/stdio/stdio.c -- the small standard read/write/seek/
 * close cookie functions, transcribed verbatim (K&R definitions and all)
 * from the newlib-1.8.2 tree; shared FILE/_reent modelling lives in
 * newlib_stdio.h.
 */

#include "newlib_stdio.h"

/*
 * Small standard I/O/seek/close functions.
 * These maintain the `known seek offset' for seek optimisation.
 */

int
__sread (cookie, buf, n)
     _PTR cookie;
     char *buf;
     int n;
{
  register FILE *fp = (FILE *) cookie;
  register int ret;

  ret = _read_r (fp->_data, fp->_file, buf, n);

  /* If the read succeeded, update the current offset.  */

  if (ret >= 0)
    fp->_offset += ret;
  else
    fp->_flags &= ~__SOFF;	/* paranoia */
  return ret;
}

int
__swrite (cookie, buf, n)
     _PTR cookie;
     char const *buf;
     int n;
{
  register FILE *fp = (FILE *) cookie;

  if (fp->_flags & __SAPP)
    (void) _lseek_r (fp->_data, fp->_file, (off_t) 0, SEEK_END);
  fp->_flags &= ~__SOFF;	/* in case O_APPEND mode is set */
  return _write_r (fp->_data, fp->_file, buf, n);
}

fpos_t
__sseek (cookie, offset, whence)
     _PTR cookie;
     fpos_t offset;
     int whence;
{
  register FILE *fp = (FILE *) cookie;
  register off_t ret;

  ret = _lseek_r (fp->_data, fp->_file, (off_t) offset, whence);
  if (ret == -1L)
    fp->_flags &= ~__SOFF;
  else
    {
      fp->_flags |= __SOFF;
      fp->_offset = ret;
    }
  return ret;
}

int
__sclose (cookie)
     _PTR cookie;
{
  FILE *fp = (FILE *) cookie;

  return _close_r (fp->_data, fp->_file);
}
