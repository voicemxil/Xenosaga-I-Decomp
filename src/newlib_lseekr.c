/*
 * newlib 1.8.2 libc/reent/lseekr.c, transcribed verbatim. errno is the
 * global at 0x4DC810, addressed absolutely -- same array-decl trick as
 * libc.c to keep it out of small data. The syscall is the flat PS2
 * name (read/write/lseek/fstat), as the original jal targets show.
 */

typedef unsigned int size_t;
typedef void *_PTR;
typedef long off_t;

struct _reent_sc
{
  int _errno; /* 0 */
};

#define _reent _reent_sc

extern int errno[];
#define errno (errno[0])

extern off_t lseek (int, off_t, int);

off_t
_lseek_r (ptr, fd, pos, whence)
     struct _reent *ptr;
     int fd;
     off_t pos;
     int whence;
{
  off_t ret;

  errno = 0;
  if ((ret = lseek (fd, pos, whence)) == (off_t) -1 && errno != 0)
    ptr->_errno = errno;
  return ret;
}
