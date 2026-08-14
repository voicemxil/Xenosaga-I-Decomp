/*
 * newlib 1.8.2 libc/stdio/vsprintf.c, transcribed verbatim: vsprintf
 * and vsprintf_r.
 */

#include "newlib_stdio.h"

typedef char *va_list;

#define INT_MAX 2147483647

#define __SWR 0x0008  /* OK to write */
#define __SSTR 0x0200 /* this is an sprintf/snprintf string */

extern struct _reent *_impure_ptr[];

#define _REENT (_impure_ptr[0])

extern int vfprintf (FILE *, const char *, va_list);

int
vsprintf (str, fmt, ap)
     char *str;
     char const *fmt;
     va_list ap;
{
  int ret;
  FILE f;

  f._flags = __SWR | __SSTR;
  f._bf._base = f._p = (unsigned char *) str;
  f._bf._size = f._w = INT_MAX;
  f._data = _REENT;
  ret = vfprintf (&f, fmt, ap);
  *f._p = 0;
  return ret;
}

int
vsprintf_r (ptr, str, fmt, ap)
     struct _reent *ptr;
     char *str;
     char const *fmt;
     va_list ap;
{
  int ret;
  FILE f;

  f._flags = __SWR | __SSTR;
  f._bf._base = f._p = (unsigned char *) str;
  f._bf._size = f._w = INT_MAX;
  f._data = ptr;
  ret = vfprintf (&f, fmt, ap);
  *f._p = 0;
  return ret;
}
