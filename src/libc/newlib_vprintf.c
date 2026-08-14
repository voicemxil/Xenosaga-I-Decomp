/*
 * newlib 1.8.2 libc/stdio/vprintf.c, transcribed verbatim. stdout is
 * _impure_ptr->_stdout (_stdout sits at offset 8 of struct _reent,
 * after _errno and _stdin), per the original's `lw a0,8(v1)`.
 */

#include "newlib_stdio.h"

typedef char *va_list;

/* Only the leading stdio stream pointers are modelled. */
struct _reent_vp
{
  int _errno;    /* 0 */
  FILE *_stdin;  /* 4 */
  FILE *_stdout; /* 8 */
};

#define _reent _reent_vp

extern struct _reent *_impure_ptr[];

#define stdout (_impure_ptr[0]->_stdout)

extern int vfprintf (FILE *, const char *, va_list);

int
vprintf (fmt, ap)
     char const *fmt;
     va_list ap;
{
  return vfprintf (stdout, fmt, ap);
}
