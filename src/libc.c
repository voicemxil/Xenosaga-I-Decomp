/*
 * newlib libc, statically linked into SLUS_204.69.
 * Same build as src/libm.c: ee-gcc 2.96 at -O2 -G0 (soft-float doubles).
 */

typedef int __int32_t;
typedef unsigned int __uint32_t;
typedef unsigned int size_t;

/* Only the two reent fields this translation unit touches are modelled;
   the padding keeps _rand_next at its original offset. */
struct _reent
{
  int _errno;
  char _pad0[164];
  unsigned long long _rand_next;
};

extern struct _reent *_impure_ptr;

#define _REENT _impure_ptr

typedef union
{
  double value;
  struct
  {
    __uint32_t lsw;
    __uint32_t msw;
  } parts;
} ieee_double_shape_type;

#define EXTRACT_WORDS(ix0,ix1,d)				\
do {								\
  ieee_double_shape_type ew_u;					\
  ew_u.value = (d);						\
  (ix0) = ew_u.parts.msw;					\
  (ix1) = ew_u.parts.lsw;					\
} while (0)

extern void *memmove (void *, const void *, size_t);
extern long _strtol_r (struct _reent *, const char *, char **, int);
extern unsigned long _strtoul_r (struct _reent *, const char *, char **, int);
extern double _strtod_r (struct _reent *, const char *, char **);
extern long strtol (const char *, char **, int);
extern double strtod (const char *, char **);
extern int raise (int);
extern void _exit (int) __attribute__ ((noreturn));
extern int _raise_r (struct _reent *, int);
extern void *_signal_r (struct _reent *, int, void *);
extern int _init_signal_r (struct _reent *);
extern int __sigtramp_r (struct _reent *, int);

/* &errno for the current reentrancy structure */
int *
__errno (void)
{
  return &_REENT->_errno;
}

/* memmove with the arguments the other way round */
void
bcopy (const void *b1, void *b2, size_t length)
{
  memmove (b2, b1, length);
}

/* string to int */
int
atoi (const char *s)
{
  return (int) strtol (s, (char **) 0, 10);
}

/* string to long */
long
atol (const char *s)
{
  return strtol (s, (char **) 0, 10);
}

/* string to double */
double
atof (const char *s)
{
  return strtod (s, (char **) 0);
}

/* raise SIGABRT then leave */
void
abort (void)
{
  raise (6);
  _exit (1);
}

/* non-zero when x is an infinity */
int
isinf (double x)
{
  __int32_t hx, lx;
  EXTRACT_WORDS (hx, lx, x);
  hx &= 0x7fffffff;
  hx |= (__uint32_t) (lx | (-lx)) >> 31;
  hx = 0x7ff00000 - hx;
  return 1 - (int) ((__uint32_t) (hx | (-hx)) >> 31);
}

/* non-zero when x is a NaN */
int
isnan (double x)
{
  __int32_t hx, lx;
  EXTRACT_WORDS (hx, lx, x);
  hx &= 0x7fffffff;
  hx |= (__uint32_t) (lx | (-lx)) >> 31;
  hx = 0x7ff00000 - hx;
  return (int) (((__uint32_t) (hx)) >> 31);
}

/* seed the pseudo-random generator */
void
srand (unsigned int seed)
{
  _REENT->_rand_next = seed;
}

/* raise a signal on the current reentrancy structure */
int
raise (int sig)
{
  return _raise_r (_REENT, sig);
}

/* install a signal handler */
void *
signal (int sig, void *func)
{
  return _signal_r (_REENT, sig, func);
}

/* set up the signal table */
int
_init_signal (void)
{
  return _init_signal_r (_REENT);
}

/* signal trampoline */
int
__sigtramp (int sig)
{
  return __sigtramp_r (_REENT, sig);
}

/* string to long */
long
strtol (const char *s, char **ptr, int base)
{
  return _strtol_r (_REENT, s, ptr, base);
}

/* string to unsigned long */
unsigned long
strtoul (const char *s, char **ptr, int base)
{
  return _strtoul_r (_REENT, s, ptr, base);
}

/* string to double */
double
strtod (const char *s, char **ptr)
{
  return _strtod_r (_REENT, s, ptr);
}

/* string to float */
float
strtodf (const char *s, char **ptr)
{
  return (float) strtod (s, ptr);
}

/* ------------------------------------------------------------------ */
/* rand.c, mallocr.c, mprec.c, reent syscall wrappers                 */
/* ------------------------------------------------------------------ */

typedef struct _Bigint
{
  struct _Bigint *_next;
  int _k, _maxwds, _sign, _wds;
  unsigned long _x[1];
} _Bigint;

extern int errno;
extern void *sbrk (int);
extern int close (int);
extern int getpid (void);
extern void __malloc_lock (struct _reent *);
extern void __malloc_unlock (struct _reent *);
extern void *_malloc_r (struct _reent *, size_t);
extern void _free_r (struct _reent *, void *);
extern _Bigint *_Balloc (struct _reent *, int);
extern void *_setlocale_r (struct _reent *, int, const char *);
extern void *_localeconv_r (struct _reent *);

/* the reent fields the allocator and mprec reach for */
struct _reent_full
{
  int _errno;
  char _pad0[72];
  _Bigint **_freelist;
};

/* linear congruential pseudo-random generator */
long
rand (void)
{
  _REENT->_rand_next = _REENT->_rand_next * 6364136223846793005LL + 1;
  return (long) ((_REENT->_rand_next >> 32) & 0x7fffffff);
}

/* allocate from the reentrant heap under the malloc lock */
void *
malloc (size_t nbytes)
{
  void *result;

  __malloc_lock (_REENT);
  result = _malloc_r (_REENT, nbytes);
  __malloc_unlock (_REENT);
  return result;
}

/* return a block to the reentrant heap under the malloc lock */
void
free (void *aptr)
{
  __malloc_lock (_REENT);
  _free_r (_REENT, aptr);
  __malloc_unlock (_REENT);
}

/* push a Bigint back on its free list */
void
_Bfree (struct _reent *ptr, _Bigint * v)
{
  if (v)
    {
      v->_next = ((struct _reent_full *) ptr)->_freelist[v->_k];
      ((struct _reent_full *) ptr)->_freelist[v->_k] = v;
    }
}

/* one-word Bigint holding i */
_Bigint *
_i2b (struct _reent * ptr, int i)
{
  _Bigint *b;

  b = _Balloc (ptr, 1);
  b->_x[0] = i;
  b->_wds = 1;
  return b;
}

/* reentrant getpid */
int
_getpid_r (struct _reent *ptr)
{
  return getpid ();
}

/* reentrant sbrk */
void *
_sbrk_r (struct _reent *ptr, int incr)
{
  char *ret;

  errno = 0;
  if ((ret = (char *) sbrk (incr)) == (char *) -1 && errno != 0)
    ptr->_errno = errno;
  return ret;
}

/* reentrant close */
int
_close_r (struct _reent *ptr, int fd)
{
  int ret;

  errno = 0;
  if ((ret = close (fd)) == -1 && errno != 0)
    ptr->_errno = errno;
  return ret;
}

/* stdin-on-a-string reader: always at end of file */
int
eofread (void *cookie, char *buf, int len)
{
  return 0;
}

/* the current locale conversion table */
void *
localeconv (void)
{
  return _localeconv_r (_REENT);
}

/* select a locale */
void *
setlocale (int category, const char *locale)
{
  return _setlocale_r (_REENT, category, locale);
}
