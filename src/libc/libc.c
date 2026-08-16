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
 *
 * 2026-08-15: this file used to carry hand transcriptions of _dtoa_r
 * and its helpers (quorem, _pow5mult, _ulp, _d2b, _ratio,
 * _mprec_log10, strtodf) as well.  Every one of them has since been
 * superseded by a vendored newlib TU -- newlib_dtoa2.c, newlib_mprec.c,
 * newlib_strtod.c -- which matches byte for byte and owns the
 * decompiled.txt entry, so the transcriptions here were dead: eight
 * near-misses that could never be registered.  Worse than dead, in
 * fact.  libc.o sorts before newlib_*.o, and the link runs with
 * --allow-multiple-definition, so the FINAL IMAGE was taking the
 * mismatching copies out of this file while verify.py happily checked
 * the good ones next door.  Deleted; the analysis that was written up
 * in their comments has moved to the vendored files.
 *
 * Nothing hand-written should be added back here that a vendored newlib
 * TU can provide.  Check for an existing definition before writing one.
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

/* newlib's ctype.h bit-table macros (long here is the target's native
   64-bit register width, not 32 -- see the mprec block below for why
   that matters elsewhere too). */
extern const char _ctype_[];
#define _U 01
#define _L 02
#define _N 04
#define _S 010
#define isspace(c) ((_ctype_+1)[(unsigned)(c)]&_S)
#define isdigit(c) ((_ctype_+1)[(unsigned)(c)]&_N)
#define isalpha(c) ((_ctype_+1)[(unsigned)(c)]&(_U|_L))
#define isupper(c) ((_ctype_+1)[(unsigned)(c)]&_U)

#define ERANGE 34
#define LONG_MAX 9223372036854775807L
#define LONG_MIN (-LONG_MAX-1)

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

/* string to long, from newlib's strtol.c. TODO: not registered --
   otherwise a PERFECT instruction-for-instruction match (confirmed by
   isolating this exact function body in a standalone compile unit,
   where it matches byte-for-byte including every movz/movn/dsrl/
   dsll32 and every __umoddi3/__udivdi3/__muldi3 libcall), but here in
   the full libc.c TU gcc's `.p2align 3,,7` loop-head alignment for
   the leading `do { c = *s++; } while (isspace(c));` pads in an extra
   nop because this function's local offset within OUR object isn't a
   multiple of 8 -- purely a function-position artifact (same class as
   the resume notes' "R5900 short-loop erratum padding" wall), not a
   logic difference. Tried relocating within the file (two positions,
   same result); a true fix would need to control byte-exact offset
   parity, not source shape. */
long
_strtol_r (struct _reent *rptr, const char *nptr, char **endptr, int base)
{
  register const char *s = nptr;
  register unsigned long acc;
  register int c;
  register unsigned long cutoff;
  register int neg = 0, any, cutlim;

  do
    {
      c = *s++;
    }
  while (isspace (c));
  if (c == '-')
    {
      neg = 1;
      c = *s++;
    }
  else if (c == '+')
    c = *s++;
  if ((base == 0 || base == 16) &&
      c == '0' && (*s == 'x' || *s == 'X'))
    {
      c = s[1];
      s += 2;
      base = 16;
    }
  if (base == 0)
    base = c == '0' ? 8 : 10;

  cutoff = neg ? -(unsigned long) LONG_MIN : LONG_MAX;
  cutlim = cutoff % (unsigned long) base;
  cutoff /= (unsigned long) base;
  for (acc = 0, any = 0;; c = *s++)
    {
      if (isdigit (c))
	c -= '0';
      else if (isalpha (c))
	c -= isupper (c) ? 'A' - 10 : 'a' - 10;
      else
	break;
      if (c >= base)
	break;
      if (any < 0 || acc > cutoff || acc == cutoff && c > cutlim)
	any = -1;
      else
	{
	  any = 1;
	  acc *= base;
	  acc += c;
	}
    }
  if (any < 0)
    {
      acc = neg ? LONG_MIN : LONG_MAX;
      rptr->_errno = ERANGE;
    }
  else if (neg)
    acc = -acc;
  if (endptr != 0)
    *endptr = (char *) (any ? s - 1 : nptr);
  return (acc);
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

/* TODO: near-miss (LOGIC, 6/10 words) -- sibling-call blocker RESISTS the
   trailing-memory-barrier fix that closed EnemySound/MAP.c/MMathCalcDir.
   Tried: barrier after the local assignment (still `j dptofp`, gcc hoists
   `ld ra` before the second call regardless); carrying the dependency via
   asm("":"+f"(fRet)) (no change); splitting strtod's result into its own
   double local with a barrier before the dptofp call (ICE -- "f" is not a
   valid asm constraint for a DF-mode value here). Unlike the single-call
   near-misses this closed, strtodf chains two calls where only the SECOND
   is the tail call; the barrier idiom does not reach across the first
   call's `jal` to stop the second from sibcalling. Needs a different
   idiom (e.g. register-pinning ra, or forcing dptofp's argument through a
   real stack slot) -- not attempted here, budget spent. Not registered. */

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
  char _pad0[60];
  _Bigint *_result;
  int _result_k;
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

/* additional mprec.h constants dtoa.c reaches for, IEEE_8087 double
   (non-_DOUBLE_IS_32BITS) branch -- same branch as the word0/word1 and
   Exp_* definitions already declared above for _d2b/_b2d. */
#define Exp_shift1  20
#define Exp_11      ((__ULong) 0x3ff00000L)
#define Frac_mask1  ((__ULong) 0xfffffL)
#define Ten_pmax    22
#define Bletch      0x10
#define Bndry_mask  ((__ULong) 0xfffffL)
#define Log2P       1
#define Quick_max   14
#define Int_max     14
#define n_bigtens   5
#define Sign_bit    ((__ULong) 0x80000000L)

#define bigtens __mprec_bigtens

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
