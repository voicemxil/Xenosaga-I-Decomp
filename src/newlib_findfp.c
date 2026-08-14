/*
 * newlib 1.8.2 libc/stdio/findfp.c, transcribed verbatim: std,
 * __sfmoreglue, __sfp, _cleanup_r, _cleanup, __sinit. The reent layout
 * this TU depends on is confirmed by __sinit's disassembly:
 * __sdidinit@56, __cleanup@60, __sglue@472, __sf[3]@484 (FILE is 88
 * bytes). ENOMEM=12, NDYNAMIC=4, both per newlib's own headers.
 */

#include "newlib_stdio.h"

#define NULL 0
#define ENOMEM 12
#define NDYNAMIC 4 /* add four more whenever necessary */

#define __SRD 0x0004  /* OK to read */
#define __SWR 0x0008  /* OK to write */
#define __SLBF 0x0001 /* line buffered */
#define __SNBF 0x0002 /* unbuffered */

struct _glue
{
  struct _glue *_next;
  int _niobs;
  FILE *_iobs;
};

/* Only the fields this TU touches are modelled; the padding keeps
   every touched field at its original offset. */
struct _reent_ffp
{
  int _errno;             /*   0 */
  char _pad0[52];
  int __sdidinit;         /*  56: 1 means stdio has been init'd */
  void (*__cleanup) ();   /*  60 */
  char _pad1[408];
  struct _glue __sglue;   /* 472: list of handlers for the FILEs */
  FILE __sf[3];           /* 484: first three files */
};

#define _reent _reent_ffp

extern struct _reent *_impure_ptr[];

#define _REENT (_impure_ptr[0])

extern _PTR _malloc_r ();
extern _PTR memset (_PTR, int, size_t);
extern int fflush ();
extern int __sread ();
extern int __swrite ();
extern fpos_t __sseek ();
extern int __sclose ();

void __sinit ();

static void
std (ptr, flags, file, data)
     FILE *ptr;
     struct _reent *data;
{
  ptr->_p = 0;
  ptr->_r = 0;
  ptr->_w = 0;
  ptr->_flags = flags;
  ptr->_file = file;
  ptr->_bf._base = 0;
  ptr->_bf._size = 0;
  ptr->_lbfsize = 0;
  ptr->_cookie = ptr;
  ptr->_read = __sread;
  ptr->_write = __swrite;
  ptr->_seek = __sseek;
  ptr->_close = __sclose;
  ptr->_data = data;
}

struct _glue *
__sfmoreglue (d, n)
     struct _reent *d;
     register int n;
{
  struct _glue *g;
  FILE *p;

  g = (struct _glue *) _malloc_r (d, sizeof (*g) + n * sizeof (FILE));
  if (g == NULL)
    return NULL;
  p = (FILE *) (g + 1);
  g->_next = NULL;
  g->_niobs = n;
  g->_iobs = p;
  memset (p, 0, n * sizeof (FILE));
  return g;
}

/*
 * Find a free FILE for fopen et al.
 */

FILE *
__sfp (d)
     struct _reent *d;
{
  FILE *fp;
  int n;
  struct _glue *g;

  if (!d->__sdidinit)
    __sinit (d);
  for (g = &d->__sglue;; g = g->_next)
    {
      for (fp = g->_iobs, n = g->_niobs; --n >= 0; fp++)
	if (fp->_flags == 0)
	  goto found;
      if (g->_next == NULL &&
	  (g->_next = __sfmoreglue (d, NDYNAMIC)) == NULL)
	break;
    }
  d->_errno = ENOMEM;
  return NULL;

found:
  fp->_flags = 1;		/* reserve this slot; caller sets real flags */
  fp->_p = NULL;		/* no current pointer */
  fp->_w = 0;			/* nothing to read or write */
  fp->_r = 0;
  fp->_bf._base = NULL;		/* no buffer */
  fp->_bf._size = 0;
  fp->_lbfsize = 0;		/* not line buffered */
  fp->_file = -1;		/* no file */
  /* fp->_cookie = <any>; */	/* caller sets cookie, _read/_write etc */
  fp->_ub._base = NULL;		/* no ungetc buffer */
  fp->_ub._size = 0;
  fp->_lb._base = NULL;		/* no line buffer */
  fp->_lb._size = 0;
  fp->_data = d;
  return fp;
}

/*
 * exit() calls _cleanup() through *__cleanup, set whenever we
 * open or buffer a file.  This chicanery is done so that programs
 * that do not use stdio need not link it all in.
 *
 * The name `_cleanup' is, alas, fairly well known outside stdio.
 */

void
_cleanup_r (ptr)
     struct _reent *ptr;
{
  /* (void) _fwalk(fclose); */
  (void) _fwalk (ptr, fflush);	/* `cheating' */
}

void
_cleanup ()
{
  _cleanup_r (_REENT);
}

/*
 * __sinit() is called whenever stdio's internal variables must be set up.
 */

void
__sinit (s)
     struct _reent *s;
{
  /* make sure we clean up on exit */
  s->__cleanup = _cleanup_r;	/* conservative */
  s->__sdidinit = 1;

  std (s->__sf + 0, __SRD, 0, s);
  std (s->__sf + 1, __SWR | __SLBF, 1, s);
  std (s->__sf + 2, __SWR | __SNBF, 2, s);

  s->__sglue._next = NULL;
  s->__sglue._niobs = 3;
  s->__sglue._iobs = &s->__sf[0];
}
