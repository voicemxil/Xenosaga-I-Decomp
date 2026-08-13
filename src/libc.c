/*
 * newlib (~1.8.x vintage) libc, statically linked into SLUS_204.69.
 * Built with the 2.96 game compiler at -O2 -G8, same as the rest of the
 * game code -- confirmed via the _i2b prologue fingerprint (8-byte
 * per-saved-register stride) and by every function below matching once
 * the small-data heuristic is defeated (see _impure_ptr below). Do NOT
 * move this file into SDK_FILES / the 2.9-ee compiler.
 *
 * Reentrancy convention: _impure_ptr must be declared as an incomplete
 * array (`struct _reent *_impure_ptr[]`) and dereferenced as
 * `_impure_ptr[0]`, NOT as a plain `extern struct _reent *_impure_ptr`.
 * At -G8 a plain pointer-sized extern is small-data eligible and gcc
 * emits a 1-instruction %gp_rel load; the original always uses the
 * 2-instruction %hi/%lo absolute load, because the real _impure_ptr
 * symbol lives far from gp. Declaring it as an array of unknown size
 * makes the total size unknowable to gcc, which forces %hi/%lo. This
 * one change took 13 functions in this file from near-miss to exact
 * match (__errno, srand, raise, signal, _init_signal, __sigtramp,
 * strtol, strtoul, strtod, malloc, free, localeconv, setlocale).
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

extern struct _reent *_impure_ptr[];

#define _REENT (_impure_ptr[0])

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
  while (1)
    {
      raise (6);
      _exit (1);
    }
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

extern float dptofp (double);

/* TODO: near-miss (LOGIC, 6/10 words) -- sibling-call blocker. The
   original keeps `jal dptofp` + a real epilogue; 2.96 always sibcalls
   the trailing non-void call as `j dptofp` with the epilogue hoisted
   before it. Same unsolved class as the ~9 MAP/Enemy near-misses in
   XENOSAGA_RESUME_PROMPT.md -- neither an intermediate local nor
   inlining the call into the return expression changes it. Not
   registered. */
/* string to float */
float
strtodf (const char *s, char **ptr)
{
  return dptofp (strtod (s, ptr));
}

/* ------------------------------------------------------------------ */
/* rand.c, mallocr.c, mprec.c, reent syscall wrappers                 */
/* ------------------------------------------------------------------ */

typedef struct _Bigint
{
  struct _Bigint *_next;
  int _k, _maxwds, _sign, _wds;
  __uint32_t _x[1];
} _Bigint;

extern int errno[];
#define errno (errno[0])
extern void *sbrk (int);
extern int close (int);
extern int getpid (void);
extern void __malloc_lock (struct _reent *);
extern void __malloc_unlock (struct _reent *);
extern void *_malloc_r (struct _reent *, size_t);
extern void _free_r (struct _reent *, void *);
extern void *_calloc_r (struct _reent *, size_t, size_t);
extern void *memcpy (void *, const void *, size_t);
extern int strcmp (const char *, const char *);

/* only the two locale fields this translation unit touches are
   modelled; offsets from newlib's sys/reent.h field order (errno, 3
   FILE* + _inc + a 25-byte emergency buffer, then the category/locale
   pair, padded to a 4-byte boundary same as the mprec fields above). */
struct _reent_locale
{
  int _errno;
  char _pad0[44];
  int _current_category;
  const char *_current_locale;
};

char *
_setlocale_r (struct _reent *p, int category, const char *locale)
{
  if (locale)
    {
      if (strcmp (locale, "C") && strcmp (locale, ""))
	return 0;
      ((struct _reent_locale *) p)->_current_category = category;
      ((struct _reent_locale *) p)->_current_locale = locale;
    }
  return "C";
}

struct lconv
{
  char *decimal_point;
  char *thousands_sep;
  char *grouping;
  char *int_curr_symbol;
  char *currency_symbol;
  char *mon_decimal_point;
  char *mon_thousands_sep;
  char *mon_grouping;
  char *positive_sign;
  char *negative_sign;
  char int_frac_digits;
  char frac_digits;
  char p_cs_precedes;
  char p_sep_by_space;
  char n_cs_precedes;
  char n_sep_by_space;
  char p_sign_posn;
  char n_sign_posn;
};

static const struct lconv lconv =
{
  ".", "", "", "", "", "", "", "", "", "",
  127, 127, 127, 127,
  127, 127, 127, 127,
};

struct lconv *
_localeconv_r (struct _reent *data)
{
  return (struct lconv *) &lconv;
}

/* the reent fields the allocator and mprec reach for -- offsets taken
   from newlib's sys/reent.h field order (errno, 3 FILE* + _inc + a
   25-byte emergency buffer + locale fields + __sdidinit + __cleanup,
   then the mprec _result/_result_k/_p5s/_freelist quartet). */
struct _reent_full
{
  int _errno;
  char _pad0[68];
  _Bigint *_p5s;
  _Bigint **_freelist;
};

/* reent.c knows this value */
#define _Kmax 15
#define _CONST const
#define NULL ((void *) 0)

extern void _Bfree (struct _reent *, _Bigint *);
extern _Bigint *_i2b (struct _reent *, int);

typedef __uint32_t __ULong;
typedef __int32_t __Long;

/* IEEE_8087 (little-endian) word access, from newlib's mprec.h */
union double_union
{
  double d;
  __ULong i[2];
};
#define word0(x) (x.i[1])
#define word1(x) (x.i[0])

#define Exp_shift   20
#define Exp_msk1    ((__ULong) 0x100000L)
#define Exp_mask    ((__ULong) 0x7ff00000L)
#define P           53
#define Bias        1023
#define Exp_1       ((__ULong) 0x3ff00000L)
#define Ebits       11
#define Frac_mask   ((__ULong) 0xfffffL)

/* arithmetic right shift on this target sign-extends already */
#define Sign_Extend(a,b) /* no-op */

#define Storeinc(a,b,c) (((unsigned short *)a)[1] = (unsigned short)b, \
((unsigned short *)a)[0] = (unsigned short)c, a++)

#define Bcopy(x,y) memcpy ((char *) &x->_sign, (char *) &y->_sign, \
y->_wds * sizeof (__Long) + 2 * sizeof (int))

_Bigint *
_Balloc (struct _reent *ptr, int k)
{
  int x;
  _Bigint *rv;

  if (((struct _reent_full *) ptr)->_freelist == NULL)
    {
      ((struct _reent_full *) ptr)->_freelist =
	(_Bigint **) _calloc_r (ptr, sizeof (_Bigint *), _Kmax + 1);
      if (((struct _reent_full *) ptr)->_freelist == NULL)
	{
	  return NULL;
	}
    }

  if (rv = ((struct _reent_full *) ptr)->_freelist[k])
    {
      ((struct _reent_full *) ptr)->_freelist[k] = rv->_next;
    }
  else
    {
      x = 1 << k;
      rv = (_Bigint *) _calloc_r (ptr, 1,
				   sizeof (_Bigint) + (x - 1) * sizeof (rv->_x));
      if (rv == NULL)
	return NULL;
      rv->_k = k;
      rv->_maxwds = x;
    }
  rv->_sign = rv->_wds = 0;
  return rv;
}

/* multiply Bigint b by m and add a */
_Bigint *
_multadd (struct _reent *ptr, _Bigint * b, int m, int a)
{
  int i, wds;
  __ULong *x, y;
  __ULong xi, z;
  _Bigint *b1;

  wds = b->_wds;
  x = b->_x;
  i = 0;
  do
    {
      xi = *x;
      y = (xi & 0xffff) * m + a;
      z = (xi >> 16) * m + (y >> 16);
      a = (int) (z >> 16);
      *x++ = (z << 16) + (y & 0xffff);
    }
  while (++i < wds);
  if (a)
    {
      if (wds >= b->_maxwds)
	{
	  b1 = _Balloc (ptr, b->_k + 1);
	  Bcopy (b1, b);
	  _Bfree (ptr, b);
	  b = b1;
	}
      b->_x[wds++] = a;
      b->_wds = wds;
    }
  return b;
}

/* convert a decimal digit string to a Bigint */
_Bigint *
_s2b (struct _reent * ptr, const char *s, int nd0, int nd, __ULong y9)
{
  _Bigint *b;
  int i, k;
  __Long x, y;

  x = (nd + 8) / 9;
  for (k = 0, y = 1; x > y; y <<= 1, k++)
    ;
  b = _Balloc (ptr, k);
  b->_x[0] = y9;
  b->_wds = 1;

  i = 9;
  if (9 < nd0)
    {
      s += 9;
      do
	b = _multadd (ptr, b, 10, *s++ - '0');
      while (++i < nd0);
      s++;
    }
  else
    s += 10;
  for (; i < nd; i++)
    b = _multadd (ptr, b, 10, *s++ - '0');
  return b;
}

/* number of leading zero bits */
int
_hi0bits (register __ULong x)
{
  register int k = 0;

  if (!(x & 0xffff0000))
    {
      k = 16;
      x <<= 16;
    }
  if (!(x & 0xff000000))
    {
      k += 8;
      x <<= 8;
    }
  if (!(x & 0xf0000000))
    {
      k += 4;
      x <<= 4;
    }
  if (!(x & 0xc0000000))
    {
      k += 2;
      x <<= 2;
    }
  if (!(x & 0x80000000))
    {
      k++;
      if (!(x & 0x40000000))
	return 32;
    }
  return k;
}

/* number of trailing zero bits */
int
_lo0bits (__ULong * y)
{
  register int k;
  register __ULong x = *y;

  if (x & 7)
    {
      if (x & 1)
	return 0;
      if (x & 2)
	{
	  *y = x >> 1;
	  return 1;
	}
      *y = x >> 2;
      return 2;
    }
  k = 0;
  if (!(x & 0xffff))
    {
      k = 16;
      x >>= 16;
    }
  if (!(x & 0xff))
    {
      k += 8;
      x >>= 8;
    }
  if (!(x & 0xf))
    {
      k += 4;
      x >>= 4;
    }
  if (!(x & 0x3))
    {
      k += 2;
      x >>= 2;
    }
  if (!(x & 1))
    {
      k++;
      x >>= 1;
      if (!x & 1)
	return 32;
    }
  *y = x;
  return k;
}

/* Bigint multiply */
_Bigint *
_multiply (struct _reent * ptr, _Bigint * a, _Bigint * b)
{
  _Bigint *c;
  int k, wa, wb, wc;
  __ULong carry, y, z;
  __ULong *x, *xa, *xae, *xb, *xbe, *xc, *xc0;
  __ULong z2;

  if (a->_wds < b->_wds)
    {
      c = a;
      a = b;
      b = c;
    }
  k = a->_k;
  wa = a->_wds;
  wb = b->_wds;
  wc = wa + wb;
  if (wc > a->_maxwds)
    k++;
  c = _Balloc (ptr, k);
  for (x = c->_x, xa = x + wc; x < xa; x++)
    *x = 0;
  xa = a->_x;
  xae = xa + wa;
  xb = b->_x;
  xbe = xb + wb;
  xc0 = c->_x;
  for (; xb < xbe; xb++, xc0++)
    {
      if (y = *xb & 0xffff)
	{
	  x = xa;
	  xc = xc0;
	  carry = 0;
	  do
	    {
	      z = (*x & 0xffff) * y + (*xc & 0xffff) + carry;
	      carry = z >> 16;
	      z2 = (*x++ >> 16) * y + (*xc >> 16) + carry;
	      carry = z2 >> 16;
	      Storeinc (xc, z2, z);
	    }
	  while (x < xae);
	  *xc = carry;
	}
      if (y = *xb >> 16)
	{
	  x = xa;
	  xc = xc0;
	  carry = 0;
	  z2 = *xc;
	  do
	    {
	      z = (*x & 0xffff) * y + (*xc >> 16) + carry;
	      carry = z >> 16;
	      Storeinc (xc, z, z2);
	      z2 = (*x++ >> 16) * y + (*xc & 0xffff) + carry;
	      carry = z2 >> 16;
	    }
	  while (x < xae);
	  *xc = z2;
	}
    }
  for (xc0 = c->_x, xc = xc0 + wc; wc > 0 && !*--xc; --wc)
    ;
  c->_wds = wc;
  return c;
}

/* multiply a Bigint by a power of 5. TODO: not registered -- 1-word
   LOGIC diff. The `if (!(p5 = ...->_p5s)) { init }` guard (literal
   transcription of the original) compiles to `bnezl` here but the
   original uses plain `bnez`, with the shared post-if instruction
   (`andi v0,s1,1`) duplicated after the init block either way (so it's
   functionally a scheduler choice, not a logic difference); tried
   splitting the assignment out of the condition, no change. 3 attempts
   spent, leaving as TODO per the budget rule. */
_Bigint *
_pow5mult (struct _reent * ptr, _Bigint * b, int k)
{
  _Bigint *b1, *p5, *p51;
  int i;
  static _CONST int p05[3] = { 5, 25, 125 };

  if (i = k & 3)
    b = _multadd (ptr, b, p05[i - 1], 0);

  if (!(k >>= 2))
    return b;
  if (!(p5 = ((struct _reent_full *) ptr)->_p5s))
    {
      p5 = ((struct _reent_full *) ptr)->_p5s = _i2b (ptr, 625);
      p5->_next = 0;
    }
  for (;;)
    {
      if (k & 1)
	{
	  b1 = _multiply (ptr, b, p5);
	  _Bfree (ptr, b);
	  b = b1;
	}
      if (!(k >>= 1))
	break;
      if (!(p51 = p5->_next))
	{
	  p51 = p5->_next = _multiply (ptr, p5, p5);
	  p51->_next = 0;
	}
      p5 = p51;
    }
  return b;
}

/* shift a Bigint left k bits */
_Bigint *
_lshift (struct _reent * ptr, _Bigint * b, int k)
{
  int i, k1, n, n1;
  _Bigint *b1;
  __ULong *x, *x1, *xe, z;

  n = k >> 5;
  k1 = b->_k;
  n1 = n + b->_wds + 1;
  for (i = b->_maxwds; n1 > i; i <<= 1)
    k1++;
  b1 = _Balloc (ptr, k1);
  x1 = b1->_x;
  for (i = 0; i < n; i++)
    *x1++ = 0;
  x = b->_x;
  xe = x + b->_wds;
  if (k &= 0x1f)
    {
      k1 = 32 - k;
      z = 0;
      do
	{
	  *x1++ = *x << k | z;
	  z = *x++ >> k1;
	}
      while (x < xe);
      if (*x1 = z)
	++n1;
    }
  else
    do
      *x1++ = *x++;
    while (x < xe);
  b1->_wds = n1 - 1;
  _Bfree (ptr, b);
  return b1;
}

/* compare two Bigints */
int
__mcmp (_Bigint * a, _Bigint * b)
{
  __ULong *xa, *xa0, *xb, *xb0;
  int i, j;

  i = a->_wds;
  j = b->_wds;
  if (i -= j)
    return i;
  xa0 = a->_x;
  xa = xa0 + j;
  xb0 = b->_x;
  xb = xb0 + j;
  for (;;)
    {
      if (*--xa != *--xb)
	return *xa < *xb ? -1 : 1;
      if (xa <= xa0)
	break;
    }
  return 0;
}

/* Bigint subtract, a - b */
_Bigint *
__mdiff (struct _reent * ptr, _Bigint * a, _Bigint * b)
{
  _Bigint *c;
  int i, wa, wb;
  __Long borrow, y;
  __ULong *xa, *xae, *xb, *xbe, *xc;
  __Long z;

  i = __mcmp (a, b);
  if (!i)
    {
      c = _Balloc (ptr, 0);
      c->_wds = 1;
      c->_x[0] = 0;
      return c;
    }
  if (i < 0)
    {
      c = a;
      a = b;
      b = c;
      i = 1;
    }
  else
    i = 0;
  c = _Balloc (ptr, a->_k);
  c->_sign = i;
  wa = a->_wds;
  xa = a->_x;
  xae = xa + wa;
  wb = b->_wds;
  xb = b->_x;
  xbe = xb + wb;
  xc = c->_x;
  borrow = 0;
  do
    {
      y = (*xa & 0xffff) - (*xb & 0xffff) + borrow;
      borrow = y >> 16;
      Sign_Extend (borrow, y);
      z = (*xa++ >> 16) - (*xb++ >> 16) + borrow;
      borrow = z >> 16;
      Sign_Extend (borrow, z);
      Storeinc (xc, z, y);
    }
  while (xb < xbe);
  while (xa < xae)
    {
      y = (*xa & 0xffff) + borrow;
      borrow = y >> 16;
      Sign_Extend (borrow, y);
      z = (*xa++ >> 16) + borrow;
      borrow = z >> 16;
      Sign_Extend (borrow, z);
      Storeinc (xc, z, y);
    }
  while (!*--xc)
    wa--;
  c->_wds = wa;
  return c;
}

/* the smallest number that can be added to a double to produce a
   value distinguishable from it (one unit in the last place). TODO:
   not registered -- otherwise byte-identical except the final `move
   v0,a1` return copy: the original leaves it BEFORE `jr ra` with an
   unfilled delay-slot nop, we get it filled INTO the delay slot (a
   strictly more efficient schedule). Tried staging through a fresh
   local and an empty asm memory barrier before return, neither
   changes it -- looks like the same class of delay-slot-fill
   scheduler quirk as _ratio's `dpdiv` call below. */
double
_ulp (double _x)
{
  union double_union x, a;
  register __Long L;

  x.d = _x;

  L = (word0 (x) & Exp_mask) - (P - 1) * Exp_msk1;
  if (L > 0)
    {
      word0 (a) = L;
      word1 (a) = 0;
    }
  else
    {
      L = -L >> Exp_shift;
      if (L < Exp_shift)
	{
	  word0 (a) = 0x80000 >> L;
	  word1 (a) = 0;
	}
      else
	{
	  word0 (a) = 0;
	  L -= Exp_shift;
	  word1 (a) = L >= 31 ? 1 : 1 << 31 - L;
	}
    }
  return a.d;
}

/* Bigint to double */
double
_b2d (_Bigint * a, int *e)
{
  __ULong *xa, *xa0, w, y, z;
  int k;
  union double_union d;
#define d0 word0(d)
#define d1 word1(d)

  xa0 = a->_x;
  xa = xa0 + a->_wds;
  y = *--xa;
  k = _hi0bits (y);
  *e = 32 - k;
  if (k < Ebits)
    {
      d0 = Exp_1 | y >> Ebits - k;
      w = xa > xa0 ? *--xa : 0;
      d1 = y << (32 - Ebits) + k | w >> Ebits - k;
      goto ret_d;
    }
  z = xa > xa0 ? *--xa : 0;
  if (k -= Ebits)
    {
      d0 = Exp_1 | y << k | z >> 32 - k;
      y = xa > xa0 ? *--xa : 0;
      d1 = z << k | y >> 32 - k;
    }
  else
    {
      d0 = Exp_1 | y;
      d1 = z;
    }
ret_d:
#undef d0
#undef d1
  return d.d;
}

/* double to Bigint. TODO: not registered -- the `y = d1` temp lands in
   $2 (v0) here but $5 (a1, the now-dead high half of the packed
   double argument) in the original; a register pin doesn't apply
   because `&y` is taken (see the "address taken" pinning caveat in
   the resume notes) and a fresh-local-for-the-final-use trick doesn't
   apply either since y's FIRST use is the address-taken one. Otherwise
   an exact transcription of newlib's d2b with Pack_32 defined. */
_Bigint *
_d2b (struct _reent * ptr, double _d, int *e, int *bits)
{
  union double_union d;
  _Bigint *b;
  int de, i, k;
  __ULong *x, y, z;
#define d0 word0(d)
#define d1 word1(d)
  d.d = _d;

  b = _Balloc (ptr, 1);
  x = b->_x;

  z = d0 & Frac_mask;
  d0 &= 0x7fffffff;
  if (de = (int) (d0 >> Exp_shift))
    z |= Exp_msk1;
  if (y = d1)
    {
      if (k = _lo0bits (&y))
	{
	  x[0] = y | z << 32 - k;
	  z >>= k;
	}
      else
	x[0] = y;
      i = b->_wds = (x[1] = z) ? 2 : 1;
    }
  else
    {
      k = _lo0bits (&z);
      x[0] = z;
      i = b->_wds = 1;
      k += 32;
    }
  if (de)
    {
      *e = de - Bias - (P - 1) + k;
      *bits = P - k;
    }
  else
    {
      *e = de - Bias - (P - 1) + 1 + k;
      *bits = 32 * i - _hi0bits (x[i - 1]);
    }
#undef d0
#undef d1
  return b;
}

/* ratio of two Bigints as a double. TODO: not registered -- compiles
   to the expected `dpdiv` libcall (confirming _ratio is genuinely
   PS2-routed through the same double-division helper as everywhere
   else, not FPU div.d), but the original leaves the `jal dpdiv` delay
   slot as an explicit `nop` with `move a1,t0` scheduled BEFORE the
   call, while we get `move a1,t0` filled INTO the delay slot -- one
   fewer instruction overall (50 vs 51). Same delay-slot-fill
   scheduler class as _ulp above; not source-reachable so far. */
double
_ratio (_Bigint * a, _Bigint * b)
{
  union double_union da, db;
  int k, ka, kb;

  da.d = _b2d (a, &ka);
  db.d = _b2d (b, &kb);
  k = ka - kb + 32 * (a->_wds - b->_wds);
  if (k > 0)
    word0 (da) += k * Exp_msk1;
  else
    {
      k = -k;
      word0 (db) += k * Exp_msk1;
    }
  return da.d / db.d;
}

_CONST double
  __mprec_tens[] =
{
  1e0, 1e1, 1e2, 1e3, 1e4, 1e5, 1e6, 1e7, 1e8, 1e9,
  1e10, 1e11, 1e12, 1e13, 1e14, 1e15, 1e16, 1e17, 1e18, 1e19,
  1e20, 1e21, 1e22, 1e23, 1e24
};

_CONST double __mprec_bigtens[] = { 1e16, 1e32, 1e64, 1e128, 1e256 };
_CONST double __mprec_tinytens[] = { 1e-16, 1e-32, 1e-64, 1e-128, 1e-256 };

#define tens __mprec_tens

/* log10 of a small power of ten, used by dtoa. TODO: not registered --
   the dig>=24 branch needs raw double immediates (1.0, then *=10) which
   the modern assembler can't emit as li.d (see wip/libm.c note); the
   original almost certainly never hits this path in-game (dig>=24 is
   outside the digit range dtoa ever asks for) so it is not worth the
   union-encoded-constant rewrite that would desync the common path. */
double
_mprec_log10 (int dig)
{
  double v;

  if (dig < 24)
    return tens[dig];
  v = tens[24];
  while (dig > 24)
    {
      v *= tens[1];
      dig--;
    }
  return v;
}

/* linear congruential pseudo-random generator */
int
rand (void)
{
  unsigned long long next;

  next = _REENT->_rand_next * 6364136223846793005LL + 1;
  _REENT->_rand_next = next;
  return (int) ((next >> 32) & 0x7fffffff);
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
