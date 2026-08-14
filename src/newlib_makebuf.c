/*
 * newlib 1.8.2 libc/stdio/makebuf.c, transcribed verbatim (non-
 * HAVE_BLKSIZE configuration: size is always BUFSIZ and _blksize is
 * pinned to 1024, matching the original's constant loads). struct stat
 * is stock newlib 1.8.2 sys/stat.h with 64-bit longs: st_mode@4 (the
 * original's `lw v0,4(sp)`), 104 bytes -> a 144-byte frame. The
 * _cleanup_r address is stored through _data->__cleanup at offset 60.
 */

#include "newlib_stdio.h"

#define NULL 0
#define BUFSIZ 1024

#define __SLBF 0x0001 /* line buffered */
#define __SNBF 0x0002 /* unbuffered */
#define __SOPT 0x0400 /* do fseek() optimisation */
#define __SNPT 0x0800 /* do not do fseek() optimisation */
#define __SMBF 0x0080 /* _buf is from malloc */

#define S_IFMT  0170000 /* type of file */
#define S_IFCHR 0020000 /* character special */
#define S_IFREG 0100000 /* regular */

typedef short dev_t;
typedef unsigned short ino_t;
typedef unsigned int mode_t;
typedef unsigned short nlink_t;
typedef unsigned short uid_t;
typedef unsigned short gid_t;
typedef long time_t;

struct stat
{
  dev_t st_dev;
  ino_t st_ino;
  mode_t st_mode;
  nlink_t st_nlink;
  uid_t st_uid;
  gid_t st_gid;
  dev_t st_rdev;
  off_t st_size;
  time_t st_atime;
  long st_spare1;
  time_t st_mtime;
  long st_spare2;
  time_t st_ctime;
  long st_spare3;
  long st_blksize;
  long st_blocks;
  long st_spare4[2];
};

/* Only the one reent field this TU stores through is modelled. */
struct _reent_mb
{
  char _pad0[60];
  void (*__cleanup) (); /* 60: cleanup routine address */
};

extern int _fstat_r (struct _reent *, int, struct stat *);
extern _PTR _malloc_r (struct _reent *, size_t);
extern void _cleanup_r ();
extern int isatty (int);

extern fpos_t __sseek ();

/*
 * Allocate a file buffer, or switch to unbuffered I/O.
 * Per the ANSI C standard, ALL tty devices default to line buffered.
 *
 * As a side effect, we set __SOPT or __SNPT (en/dis-able fseek
 * optimization) right after the _fstat() that finds the buffer size.
 */

void
__smakebuf (fp)
     register FILE *fp;
{
  register size_t size, couldbetty;
  register _PTR p;
  struct stat st;

  if (fp->_flags & __SNBF)
    {
      fp->_bf._base = fp->_p = fp->_nbuf;
      fp->_bf._size = 1;
      return;
    }
  if (fp->_file < 0 || _fstat_r (fp->_data, fp->_file, &st) < 0)
    {
      couldbetty = 0;
      size = BUFSIZ;
      /* do not try to optimise fseek() */
      fp->_flags |= __SNPT;
    }
  else
    {
      couldbetty = (st.st_mode & S_IFMT) == S_IFCHR;
      size = BUFSIZ;
      /*
       * Optimize fseek() only if it is a regular file.
       * (The test for __sseek is mainly paranoia.)
       */
      if ((st.st_mode & S_IFMT) == S_IFREG && fp->_seek == __sseek)
	{
	  fp->_flags |= __SOPT;
	  fp->_blksize = 1024;
	}
      else
	fp->_flags |= __SNPT;
    }
  if ((p = _malloc_r (fp->_data, size)) == NULL)
    {
      fp->_flags |= __SNBF;
      fp->_bf._base = fp->_p = fp->_nbuf;
      fp->_bf._size = 1;
    }
  else
    {
      ((struct _reent_mb *) fp->_data)->__cleanup = _cleanup_r;
      fp->_flags |= __SMBF;
      fp->_bf._base = fp->_p = (unsigned char *) p;
      fp->_bf._size = size;
      if (couldbetty && isatty (fp->_file))
	fp->_flags |= __SLBF;
    }
}
