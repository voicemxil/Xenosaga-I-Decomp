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

/* double to Bigint. TODO: not registered -- 5 diffs -> 2 diffs this
   session. The "address taken, can't pin" reasoning above turned out to
   be wrong: staging `d1` through a register-pinned ($5/a1) fresh local
   `y2`, forced live with an empty "+r" asm barrier, THEN assigning
   `y = y2` gets the compiler to allocate y itself in $5 to match the
   original (closed 3 of 5 diffs -- the dsll32/dsra32 sign-extend chain
   and the beqz). The pin alone (no barrier) was satisfied by gcc without
   honouring it -- needed the barrier to carry the dependency.
   Remaining 2 diffs are a pure SCHEDULING tie-break: original computes
   `move a0,sp` (address of the &y stack slot) BEFORE `sw a1,0(sp)`
   (spilling y's value there for the address-taken use); we get the store
   first, then the address move. Tried: an extra memory barrier right
   before the `_lo0bits(&y)` call (no effect), hoisting `&y` into an
   explicit `__ULong *py = &y;` local before the call (no effect,
   reverted). permute.py doesn't apply -- this is sub-expression argument
   evaluation order within one call, not a multi-statement store group.
   Otherwise an exact transcription of newlib's d2b with Pack_32
   defined. */
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
  {
    register __ULong y2 __asm__ ("$5") = d1;
    __asm__ volatile ("" : "+r" (y2));
    y = y2;
  }
  if (y)
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

/* dtoa's internal Bigint remainder/quotient step */
static int
quorem (_Bigint * b, _Bigint * S)
{
  int n;
  __Long borrow, y;
  __ULong carry, q, ys;
  __ULong *bx, *bxe, *sx, *sxe;
  __Long z;
  __ULong si, zs;

  n = S->_wds;
  if (b->_wds < n)
    return 0;
  sx = S->_x;
  sxe = sx + --n;
  bx = b->_x;
  bxe = bx + n;
  q = *bxe / (*sxe + 1);	/* ensure q <= true quotient */
  if (q)
    {
      borrow = 0;
      carry = 0;
      do
	{
	  si = *sx++;
	  ys = (si & 0xffff) * q + carry;
	  zs = (si >> 16) * q + (ys >> 16);
	  carry = zs >> 16;
	  y = (*bx & 0xffff) - (ys & 0xffff) + borrow;
	  borrow = y >> 16;
	  Sign_Extend (borrow, y);
	  z = (*bx >> 16) - (zs & 0xffff) + borrow;
	  borrow = z >> 16;
	  Sign_Extend (borrow, z);
	  Storeinc (bx, z, y);
	}
      while (sx <= sxe);
      if (!*bxe)
	{
	  bx = b->_x;
	  while (--bxe > bx && !*bxe)
	    --n;
	  b->_wds = n;
	}
    }
  if (__mcmp (b, S) >= 0)
    {
      q++;
      borrow = 0;
      carry = 0;
      bx = b->_x;
      sx = S->_x;
      do
	{
	  si = *sx++;
	  ys = (si & 0xffff) + carry;
	  zs = (si >> 16) + (ys >> 16);
	  carry = zs >> 16;
	  y = (*bx & 0xffff) - (ys & 0xffff) + borrow;
	  borrow = y >> 16;
	  Sign_Extend (borrow, y);
	  z = (*bx >> 16) - (zs & 0xffff) + borrow;
	  borrow = z >> 16;
	  Sign_Extend (borrow, z);
	  Storeinc (bx, z, y);
	}
      while (sx <= sxe);
      bx = b->_x;
      bxe = bx + n;
      if (!*bxe)
	{
	  while (--bxe > bx && !*bxe)
	    --n;
	  b->_wds = n;
	}
    }
  return q;
}

/* dtoa for IEEE arithmetic (dmg): convert double to ASCII string.
   Transcription of newlib's dtoa.c, IEEE_8087 (little-endian),
   non-_DOUBLE_IS_32BITS, Pack_32 branch -- same branch already selected
   for _d2b/_b2d/_lshift/etc above. ptr->_result/_result_k live in
   struct _reent_full at their real reent.h offsets (64/68), just before
   _p5s (72).

   The game's newlib is NOT 1.8.2 here, it is a slightly later revision.
   Two places prove it, and both are load-bearing for the match:
     - `ilim = ilim1 = -1;` sits BEFORE `switch (mode)`, not inside
       `case 0: case 1:` (the "silence erroneous gcc -Wall warning"
       hoist).  The original's jump-table block pre-stores the default
       into two stack slots before the range check; 1.8.2's shape does
       not, and that region only matches with the hoist.
     - `spec_case = 0;` is hoisted ABOVE `if (mode < 2)` instead of
       living in an `else` arm inside it.  This one is worth a paragraph;
       see below.

   HISTORY OF THE DIFF: 1004 -> 867 (li.d fixes, commit 5013e1a)
   -> 509 (ld-macro fix) -> 501 (spec_case hoist) -> 5 (li.d-before-call
   delay-slot barrier).  Three of those four were toolchain bugs, one was
   source shape.  In order:

   1. tools/fix_cc_asm.py's li.d synthesis (already fixed, 5013e1a):
      irrational constants are pooled in .rdata and loaded with
      lui %hi / ld %lo, not bit-synthesized; "round" constants use a
      16-bit window anchored at the value's highest set bit.

   2. RE_MEM_SYM_REG in fix_cc_asm.py did not accept a symbol with a
      constant displacement, so `ld $5,__mprec_tens-8($2)` (from
      `tens[k-1]`-shaped indexing) fell through to gas's own macro
      expansion and got `daddu` where the original has `addu`.  Two
      words.  Fixed by allowing `sym[+-]N` in the regex.

   3. `spec_case` was the whole stack-layout gap.  With 1.8.2's
      `if (mode < 2) { ... } else spec_case = 0; }` shape, ee-gcc 2.96
      fails to give spec_case a hard register and spills it to a NEW
      4-byte frame slot at sp+64 -- which shifts EVERY later local
      (denorm, b, S, mhi, mlo, ...) up by 4 and repainted ~80 words as
      differing, plus the extra branch/store needed to set a memory
      spec_case on both paths.  The original keeps spec_case in $s0.
      Hoisting the `spec_case = 0;` initialisation above the `if` makes
      the pseudo's definition unconditional; the allocator then keeps it
      in $s0 exactly like the original, the frame layout collapses onto
      the original's, and 501 -> 56 real-differing words in one step.
      (The frame size was 176 bytes in BOTH builds all along -- the
      extra slot came out of otherwise-unused padding, which is why the
      symptom looked like a pure "allocator tie-break" and was
      previously written off as one.  It was not.)

   4. gcc leaves a call in `.set reorder` whenever it did not fill the
      delay slot itself.  gas then steals the immediately preceding
      instruction into that slot.  When the preceding instruction is the
      tail of a li.d expansion (`d.d *= 10.` at $L416: li/dsll32 then
      `jal dpmul`), the original ee-as did NOT steal it -- li.d was a
      real assembler macro there and gas never reorders across a macro
      expansion, so the call kept its nop.  fix_cc_asm.py now barriers a
      li.d synthesis that is immediately followed by a bare jal/j, the
      same rule already applied to cvt.* before a call.  This closed the
      last two-instruction length gap AND, with the lengths finally
      equal, every remaining "differing" branch displacement.
      Scope-checked: across all of src/*.c only libc.c and libm.c emit
      this pattern, and libm.c stays at 19/19 matched.

   REMAINING: 5 words of 1177 (99.6%), all the same thing -- gcc's
   delayed-branch pass (reorg.c) picking a plain branch where the
   original picked the branch-LIKELY form, or vice versa:
     [ 778] orig beqzl  s0  / built beqz  s0
     [ 859] orig blezl  v1  / built blez  v1
     [ 879] orig bgezl  v0  / built bgez  v0
     [ 970] orig bne  a0,v0 / built bnel  a0,v0
     [1006] orig bnez   v0  / built bnezl v0
   Every one has the SAME target and the SAME delay-slot instruction on
   both sides, and in every case that instruction is dead on the
   fall-through path (it is redefined a few insns later), so annulling it
   is optional and both encodings are correct code.  The choice is
   reorg.c's `must_annul` test -- whether `insn_sets_resource_p (trial,
   &opposite_needs)` says the delay insn clobbers something live on the
   opposite thread -- and it disagrees in BOTH directions (3 one way, 2
   the other).

   WHY THE USUAL SOURCE LEVERS CANNOT REACH THIS.  This project has two
   documented idioms that DO steer the annulled-vs-plain choice per site:
   putting a loop-header load at the top of the `do { }` body (rotates it
   into an annulled slot, JTHREAD_getSymbol/JTHREAD_get) and the
   fresh-local walking-pointer copy (`q = p + 1; *ppText = q; p = q;`
   gives plain `beq`, `*ppText = ++p;` gives `beql`,
   UmnEventTextGyouJump).  Both work by changing WHICH instruction lands
   in the delay slot.  Here the right instruction is ALREADY in the slot
   at all five sites -- the whole 1177-word stream is the original's, bit
   for bit, except these five annul flags.  So any source change that
   perturbs reorg's input necessarily perturbs the stream too, and that
   is exactly what is observed.  Tried and measured, 16 variants:
     [ 778] `j = b5 - m5;` then `if (j)`; `if ((j = b5 - m5) != 0)`;
            fresh `int j5` copy               -> all still 5, byte-identical
            `j = b5 - m5; if (b5 != m5)`      -> 28 words (worse)
     [ 859] fresh `int nb2` copy; `if (0 < b2)`; `if (b2 >= 1)`;
            hoisting `s2` into a local ahead of both `if`s
                                              -> all still 5, byte-identical
     [ 879] flattened `if (k_check && __mcmp (b, S) < 0)`;
            `if ((j = __mcmp (b, S)) < 0)`    -> all still 5, byte-identical
     [ 970] fresh `_Bigint *nmhi` copy; `_Bigint *t` for the shared
            multadd result                    -> all still 5, byte-identical
            inverted `if (mlo != mhi)` arms   -> 209 words (worse)
     [1006] fresh `int dsign = delta->_sign;` -> still 5, byte-identical
            `if (delta->_sign) j1 = 1; else ...` and
            `j1 = 1; if (!delta->_sign) ...`  -> 533/540 words (much worse:
            both destroy the ternary's branchless shape and cascade)
   Flag sweep (all neutral or worse): -fno-caller-saves, -fno-peephole,
   -fno-cse-follow-jumps, -fno-expensive-optimizations, -fno-thread-jumps,
   -fno-gcse, -fno-strength-reduce, -fno-defer-pop,
   -fno-optimize-register-move.

   CORROBORATION: this is not a _dtoa_r quirk.  Two other mprec-family
   functions in this file, _pow5mult and quorem, are each stuck at
   exactly ONE differing word, and both are this same class --
   _pow5mult[22] orig `bnez s0` / built `bnezl s0`, quorem[73] orig
   `beqzl v0` / built `beqz v0`.  Verified those two sit at 1 word with
   the PRE-session source and pre-session fix_cc_asm.py as well, so they
   are long-standing near-misses of the same kind, not fallout from the
   fixes above.

   Filed as a reorg annul tie-break -- reachable only from reorg.c's own
   liveness precision (`mark_target_live_regs`), i.e. the compiler build,
   not the C.  Same class as the project's other "not reachable from C"
   walls.  Note triage.py calls these five LOGIC because 4 of the 5 have a
   different primary opcode; that classification is wrong here. */
char *
_dtoa_r (struct _reent * ptr, double _d, int mode, int ndigits, int *decpt,
	 int *sign, char **rve)
{
  int bbits, b2, b5, be, dig, i, ieps, ilim, ilim0, ilim1, j, j1, k, k0,
    k_check, leftright, m2, m5, s2, s5, spec_case, try_quick;
  union double_union d, d2, eps;
  __Long L;
  int denorm;
  __ULong x;
  _Bigint *b, *b1, *delta, *mlo, *mhi, *S;
  double ds;
  char *s, *s0;

  d.d = _d;

  if (((struct _reent_full *) ptr)->_result)
    {
      ((struct _reent_full *) ptr)->_result->_k =
	((struct _reent_full *) ptr)->_result_k;
      ((struct _reent_full *) ptr)->_result->_maxwds =
	1 << ((struct _reent_full *) ptr)->_result_k;
      _Bfree (ptr, ((struct _reent_full *) ptr)->_result);
      ((struct _reent_full *) ptr)->_result = 0;
    }

  if (word0 (d) & Sign_bit)
    {
      /* set sign for everything, including 0's and NaNs */
      *sign = 1;
      word0 (d) &= ~Sign_bit;	/* clear sign bit */
    }
  else
    *sign = 0;

  if ((word0 (d) & Exp_mask) == Exp_mask)
    {
      /* Infinity or NaN */
      *decpt = 9999;
      s = !word1 (d) && !(word0 (d) & 0xfffff) ? "Infinity" : "NaN";
      if (rve)
	*rve = s[3] ? s + 8 : s + 3;
      return s;
    }
  if (!d.d)
    {
      *decpt = 1;
      s = "0";
      if (rve)
	*rve = s + 1;
      return s;
    }

  b = _d2b (ptr, d.d, &be, &bbits);
  if (i = (int) (word0 (d) >> Exp_shift1 & (Exp_mask >> Exp_shift1)))
    {
      d2.d = d.d;
      word0 (d2) &= Frac_mask1;
      word0 (d2) |= Exp_11;

      /* log(x)	~=~ log(1.5) + (x-1.5)/1.5
		 * log10(x)	 =  log(x) / log(10)
		 *		~=~ log(1.5)/log(10) + (x-1.5)/(1.5*log(10))
		 * log10(d) = (i-Bias)*log(2)/log(10) + log10(d2)
		 *
		 * This suggests computing an approximation k to log10(d) by
		 *
		 * k = (i - Bias)*0.301029995663981
		 *	+ ( (d2-1.5)*0.289529654602168 + 0.176091259055681 );
		 *
		 * We want k to be too large rather than too small.
		 * The error in the first-order Taylor series approximation
		 * is in our favor, so we just round up the constant enough
		 * to compensate for any error in the multiplication of
		 * (i - Bias) by 0.301029995663981; since |i - Bias| <= 1077,
		 * and 1077 * 0.30103 * 2^-52 ~=~ 7.2e-14,
		 * adding 1e-13 to the constant term more than suffices.
		 * Hence we adjust the constant term to 0.1760912590558.
		 * (We could get a more accurate k by invoking log10,
		 *  but this is probably not worthwhile.)
		 */

      i -= Bias;
      denorm = 0;
    }
  else
    {
      /* d is denormalized */

      i = bbits + be + (Bias + (P - 1) - 1);
      x = i > 32 ? word0 (d) << 64 - i | word1 (d) >> i - 32
	: word1 (d) << 32 - i;
      d2.d = x;
      word0 (d2) -= 31 * Exp_msk1;	/* adjust exponent */
      i -= (Bias + (P - 1) - 1) + 1;
      denorm = 1;
    }
  ds = (d2.d - 1.5) * 0.289529654602168 + 0.1760912590558
    + i * 0.301029995663981;
  k = (int) ds;
  if (ds < 0. && ds != k)
    k--;			/* want k = floor(ds) */
  k_check = 1;
  if (k >= 0 && k <= Ten_pmax)
    {
      if (d.d < tens[k])
	k--;
      k_check = 0;
    }
  j = bbits - i - 1;
  if (j >= 0)
    {
      b2 = 0;
      s2 = j;
    }
  else
    {
      b2 = -j;
      s2 = 0;
    }
  if (k >= 0)
    {
      b5 = 0;
      s5 = k;
      s2 += k;
    }
  else
    {
      b2 -= k;
      b5 = -k;
      s5 = 0;
    }
  if (mode < 0 || mode > 9)
    mode = 0;
  try_quick = 1;
  if (mode > 5)
    {
      mode -= 4;
      try_quick = 0;
    }
  leftright = 1;
  ilim = ilim1 = -1;		/* Values for cases 0 and 1; done here to */
				/* silence erroneous "gcc -Wall" warning. */
  switch (mode)
    {
    case 0:
    case 1:
      i = 18;
      ndigits = 0;
      break;
    case 2:
      leftright = 0;
      /* no break */
    case 4:
      if (ndigits <= 0)
	ndigits = 1;
      ilim = ilim1 = i = ndigits;
      break;
    case 3:
      leftright = 0;
      /* no break */
    case 5:
      i = ndigits + k + 1;
      ilim = i;
      ilim1 = i - 1;
      if (i <= 0)
	i = 1;
    }
  j = sizeof (__ULong);
  for (((struct _reent_full *) ptr)->_result_k = 0;
       sizeof (_Bigint) - sizeof (__ULong) + j <= i; j <<= 1)
    ((struct _reent_full *) ptr)->_result_k++;
  ((struct _reent_full *) ptr)->_result =
    _Balloc (ptr, ((struct _reent_full *) ptr)->_result_k);
  s = s0 = (char *) ((struct _reent_full *) ptr)->_result;

  if (ilim >= 0 && ilim <= Quick_max && try_quick)
    {
      /* Try to get by with floating-point arithmetic. */

      i = 0;
      d2.d = d.d;
      k0 = k;
      ilim0 = ilim;
      ieps = 2;			/* conservative */
      if (k > 0)
	{
	  ds = tens[k & 0xf];
	  j = k >> 4;
	  if (j & Bletch)
	    {
	      /* prevent overflows */
	      j &= Bletch - 1;
	      d.d /= bigtens[n_bigtens - 1];
	      ieps++;
	    }
	  for (; j; j >>= 1, i++)
	    if (j & 1)
	      {
		ieps++;
		ds *= bigtens[i];
	      }
	  d.d /= ds;
	}
      else if (j1 = -k)
	{
	  d.d *= tens[j1 & 0xf];
	  for (j = j1 >> 4; j; j >>= 1, i++)
	    if (j & 1)
	      {
		ieps++;
		d.d *= bigtens[i];
	      }
	}
      if (k_check && d.d < 1. && ilim > 0)
	{
	  if (ilim1 <= 0)
	    goto fast_failed;
	  ilim = ilim1;
	  k--;
	  d.d *= 10.;
	  ieps++;
	}
      eps.d = ieps * d.d + 7.;
      word0 (eps) -= (P - 1) * Exp_msk1;
      if (ilim == 0)
	{
	  S = mhi = 0;
	  d.d -= 5.;
	  if (d.d > eps.d)
	    goto one_digit;
	  if (d.d < -eps.d)
	    goto no_digits;
	  goto fast_failed;
	}
      if (leftright)
	{
	  /* Use Steele & White method of only
	   * generating digits needed.
	   */
	  eps.d = 0.5 / tens[ilim - 1] - eps.d;
	  for (i = 0;;)
	    {
	      L = d.d;
	      d.d -= L;
	      *s++ = '0' + (int) L;
	      if (d.d < eps.d)
		goto ret1;
	      if (1. - d.d < eps.d)
		goto bump_up;
	      if (++i >= ilim)
		break;
	      eps.d *= 10.;
	      d.d *= 10.;
	    }
	}
      else
	{
	  /* Generate ilim digits, then fix them up. */
	  eps.d *= tens[ilim - 1];
	  for (i = 1;; i++, d.d *= 10.)
	    {
	      L = d.d;
	      d.d -= L;
	      *s++ = '0' + (int) L;
	      if (i == ilim)
		{
		  if (d.d > 0.5 + eps.d)
		    goto bump_up;
		  else if (d.d < 0.5 - eps.d)
		    {
		      while (*--s == '0');
		      s++;
		      goto ret1;
		    }
		  break;
		}
	    }
	}
    fast_failed:
      s = s0;
      d.d = d2.d;
      k = k0;
      ilim = ilim0;
    }

  /* Do we have a "small" integer? */

  if (be >= 0 && k <= Int_max)
    {
      /* Yes. */
      ds = tens[k];
      if (ndigits < 0 && ilim <= 0)
	{
	  S = mhi = 0;
	  if (ilim < 0 || d.d <= 5 * ds)
	    goto no_digits;
	  goto one_digit;
	}
      for (i = 1;; i++)
	{
	  L = d.d / ds;
	  d.d -= L * ds;
	  *s++ = '0' + (int) L;
	  if (i == ilim)
	    {
	      d.d += d.d;
	      if (d.d > ds || d.d == ds && L & 1)
		{
		bump_up:
		  while (*--s == '9')
		    if (s == s0)
		      {
			k++;
			*s = '0';
			break;
		      }
		  ++*s++;
		}
	      break;
	    }
	  if (!(d.d *= 10.))
	    break;
	}
      goto ret1;
    }

  m2 = b2;
  m5 = b5;
  mhi = mlo = 0;
  if (leftright)
    {
      if (mode < 2)
	{
	  i = denorm ? be + (Bias + (P - 1) - 1 + 1) : 1 + P - bbits;
	}
      else
	{
	  j = ilim - 1;
	  if (m5 >= j)
	    m5 -= j;
	  else
	    {
	      s5 += j -= m5;
	      b5 += j;
	      m5 = 0;
	    }
	  if ((i = ilim) < 0)
	    {
	      m2 -= i;
	      i = 0;
	    }
	}
      b2 += i;
      s2 += i;
      mhi = _i2b (ptr, 1);
    }
  if (m2 > 0 && s2 > 0)
    {
      i = m2 < s2 ? m2 : s2;
      b2 -= i;
      m2 -= i;
      s2 -= i;
    }
  if (b5 > 0)
    {
      if (leftright)
	{
	  if (m5 > 0)
	    {
	      mhi = _pow5mult (ptr, mhi, m5);
	      b1 = _multiply (ptr, mhi, b);
	      _Bfree (ptr, b);
	      b = b1;
	    }
	  if (j = b5 - m5)
	    b = _pow5mult (ptr, b, j);
	}
      else
	b = _pow5mult (ptr, b, b5);
    }
  S = _i2b (ptr, 1);
  if (s5 > 0)
    S = _pow5mult (ptr, S, s5);

  /* Check for special case that d is a normalized power of 2. */

  /* The `spec_case = 0` initialisation is hoisted out of the `if` (rather
     than living in an `else` arm inside it, as in newlib 1.8.2) because
     that is what the original does -- and it is what keeps spec_case in
     $s0 instead of a new stack slot.  See the header comment. */
  spec_case = 0;
  if (mode < 2)
    {
      if (!word1 (d) && !(word0 (d) & Bndry_mask) && word0 (d) & Exp_mask)
	{
	  /* The special case */
	  b2 += Log2P;
	  s2 += Log2P;
	  spec_case = 1;
	}
    }

  /* Arrange for convenient computation of quotients:
   * shift left if necessary so divisor has 4 leading 0 bits.
   *
   * Perhaps we should just compute leading 28 bits of S once
   * and for all and pass them and a shift to quorem, so it
   * can do shifts and ors to compute the numerator for q.
   */

  if (i = ((s5 ? 32 - _hi0bits (S->_x[S->_wds - 1]) : 1) + s2) & 0x1f)
    i = 32 - i;
  if (i > 4)
    {
      i -= 4;
      b2 += i;
      m2 += i;
      s2 += i;
    }
  else if (i < 4)
    {
      i += 28;
      b2 += i;
      m2 += i;
      s2 += i;
    }
  if (b2 > 0)
    b = _lshift (ptr, b, b2);
  if (s2 > 0)
    S = _lshift (ptr, S, s2);
  if (k_check)
    {
      if (__mcmp (b, S) < 0)
	{
	  k--;
	  b = _multadd (ptr, b, 10, 0);	/* we botched the k estimate */
	  if (leftright)
	    mhi = _multadd (ptr, mhi, 10, 0);
	  ilim = ilim1;
	}
    }
  if (ilim <= 0 && mode > 2)
    {
      if (ilim < 0 || __mcmp (b, S = _multadd (ptr, S, 5, 0)) <= 0)
	{
	  /* no digits, fcvt style */
	no_digits:
	  k = -1 - ndigits;
	  goto ret;
	}
    one_digit:
      *s++ = '1';
      k++;
      goto ret;
    }
  if (leftright)
    {
      if (m2 > 0)
	mhi = _lshift (ptr, mhi, m2);

      /* Compute mlo -- check for special case
       * that d is a normalized power of 2.
       */

      mlo = mhi;
      if (spec_case)
	{
	  mhi = _Balloc (ptr, mhi->_k);
	  Bcopy (mhi, mlo);
	  mhi = _lshift (ptr, mhi, Log2P);
	}

      for (i = 1;; i++)
	{
	  dig = quorem (b, S) + '0';
	  /* Do we yet have the shortest decimal string
	   * that will round to d?
	   */
	  j = __mcmp (b, mlo);
	  delta = __mdiff (ptr, S, mhi);
	  j1 = delta->_sign ? 1 : __mcmp (b, delta);
	  _Bfree (ptr, delta);
	  if (j1 == 0 && !mode && !(word1 (d) & 1))
	    {
	      if (dig == '9')
		goto round_9_up;
	      if (j > 0)
		dig++;
	      *s++ = dig;
	      goto ret;
	    }
	  if (j < 0 || j == 0 && !mode && !(word1 (d) & 1))
	    {
	      if (j1 > 0)
		{
		  b = _lshift (ptr, b, 1);
		  j1 = __mcmp (b, S);
		  if ((j1 > 0 || j1 == 0 && dig & 1) && dig++ == '9')
		    goto round_9_up;
		}
	      *s++ = dig;
	      goto ret;
	    }
	  if (j1 > 0)
	    {
	      if (dig == '9')
		{		/* possible if i == 1 */
		round_9_up:
		  *s++ = '9';
		  goto roundoff;
		}
	      *s++ = dig + 1;
	      goto ret;
	    }
	  *s++ = dig;
	  if (i == ilim)
	    break;
	  b = _multadd (ptr, b, 10, 0);
	  if (mlo == mhi)
	    mlo = mhi = _multadd (ptr, mhi, 10, 0);
	  else
	    {
	      mlo = _multadd (ptr, mlo, 10, 0);
	      mhi = _multadd (ptr, mhi, 10, 0);
	    }
	}
    }
  else
    for (i = 1;; i++)
      {
	*s++ = dig = quorem (b, S) + '0';
	if (i >= ilim)
	  break;
	b = _multadd (ptr, b, 10, 0);
      }

  /* Round off last digit */

  b = _lshift (ptr, b, 1);
  j = __mcmp (b, S);
  if (j > 0 || j == 0 && dig & 1)
    {
    roundoff:
      while (*--s == '9')
	if (s == s0)
	  {
	    k++;
	    *s++ = '1';
	    goto ret;
	  }
      ++*s++;
    }
  else
    {
      while (*--s == '0');
      s++;
    }
ret:
  _Bfree (ptr, S);
  if (mhi)
    {
      if (mlo && mlo != mhi)
	_Bfree (ptr, mlo);
      _Bfree (ptr, mhi);
    }
ret1:
  _Bfree (ptr, b);
  *s = 0;
  *decpt = k + 1;
  if (rve)
    *rve = s;
  return s0;
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
