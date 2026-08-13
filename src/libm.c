/*
 * fdlibm / newlib libm, statically linked into SLUS_204.69.
 * Compiled with ee-gcc 2.96 at -O2 -G8 (doubles are soft-float, so they
 * live in 64-bit GPRs; floats use the R5900 COP1).
 */

typedef int __int32_t;
typedef unsigned int __uint32_t;

/* A double as its two 32-bit halves (little endian). */
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

#define GET_HIGH_WORD(i,d)					\
do {								\
  ieee_double_shape_type gh_u;					\
  gh_u.value = (d);						\
  (i) = gh_u.parts.msw;						\
} while (0)

#define GET_LOW_WORD(i,d)					\
do {								\
  ieee_double_shape_type gl_u;					\
  gl_u.value = (d);						\
  (i) = gl_u.parts.lsw;						\
} while (0)

#define INSERT_WORDS(d,ix0,ix1)					\
do {								\
  ieee_double_shape_type iw_u;					\
  iw_u.parts.msw = (ix0);					\
  iw_u.parts.lsw = (ix1);					\
  (d) = iw_u.value;						\
} while (0)

#define SET_HIGH_WORD(d,v)					\
do {								\
  ieee_double_shape_type sh_u;					\
  sh_u.value = (d);						\
  sh_u.parts.msw = (v);						\
  (d) = sh_u.value;						\
} while (0)

#define SET_LOW_WORD(d,v)					\
do {								\
  ieee_double_shape_type sl_u;					\
  sl_u.value = (d);						\
  sl_u.parts.lsw = (v);						\
  (d) = sl_u.value;						\
} while (0)

/* A float as a 32-bit word. */
typedef union
{
  float value;
  __uint32_t word;
} ieee_float_shape_type;

#define GET_FLOAT_WORD(i,d)					\
do {								\
  ieee_float_shape_type gf_u;					\
  gf_u.value = (d);						\
  (i) = gf_u.word;						\
} while (0)

#define SET_FLOAT_WORD(d,i)					\
do {								\
  ieee_float_shape_type sf_u;					\
  sf_u.word = (i);						\
  (d) = sf_u.value;						\
} while (0)

extern double fabs (double);
extern double copysign (double, double);
extern double scalbn (double, int);
extern double floor (double);
extern float fabsf (float);
extern float copysignf (float, float);
extern float scalbnf (float, int);
extern float floorf (float);

/* |x| */
double
fabs (double x)
{
  __uint32_t high;
  GET_HIGH_WORD (high, x);
  SET_HIGH_WORD (x, high & 0x7fffffff);
  return x;
}

/* |x| */
float
fabsf (float x)
{
  __uint32_t ix;
  GET_FLOAT_WORD (ix, x);
  SET_FLOAT_WORD (x, ix & 0x7fffffff);
  return x;
}

/* |x| with the sign of y */
double
copysign (double x, double y)
{
  __uint32_t hx, hy;
  GET_HIGH_WORD (hx, x);
  GET_HIGH_WORD (hy, y);
  SET_HIGH_WORD (x, (hx & 0x7fffffff) | (hy & 0x80000000));
  return x;
}

/* |x| with the sign of y */
float
copysignf (float x, float y)
{
  __uint32_t ix, iy;
  GET_FLOAT_WORD (ix, x);
  GET_FLOAT_WORD (iy, y);
  SET_FLOAT_WORD (x, (ix & 0x7fffffff) | (iy & 0x80000000));
  return x;
}

/*
 * The R5900 COP1 has no denormals, infinities or NaNs, so newlib's
 * FLT_UWORD_* configuration macros collapse for the float routines.
 */
#define FLT_UWORD_IS_FINITE(x)		1
#define FLT_UWORD_IS_NAN(x)		0
#define FLT_UWORD_IS_INFINITE(x)	0
#define FLT_UWORD_MAX			0x7f7fffffL
#define FLT_UWORD_EXP_MAX		128
#define FLT_UWORD_LOG_MAX		0x42b17217L
#define FLT_UWORD_LOG_2MAX		0x42b2d4fcL
#define HUGE				((float) 3.40282346638528860e+38)
#define FLT_UWORD_IS_ZERO(x)		((x) < 0x00800000L)
#define FLT_UWORD_IS_SUBNORMAL(x)	0
#define FLT_UWORD_MIN			0x00800000L
#define FLT_UWORD_EXP_MIN		-126
#define FLT_UWORD_LOG_MIN		0x42aeac50L
#define FLT_SMALLEST_EXP		1
#define FLT_LARGEST_EXP			(128 + 127)

static const float huge_f = 1.00000002e30;  /* 1.0e30; ee-gcc truncates, so spell the exact float */
static const float tiny_f = 1.00000005e-30; /* 1.0e-30 */

extern float __ieee754_sqrtf (float);
extern double __ieee754_sqrt (double);

/* largest integer <= x */
float
floorf (float x)
{
  __int32_t i0, j0;
  __uint32_t i, ix;
  GET_FLOAT_WORD (i0, x);
  ix = (i0 & 0x7fffffff);
  j0 = (ix >> 23) - 0x7f;
  if (j0 < 23)
    {
      if (j0 < 0)
	{			/* raise inexact if x != 0 */
	  if (huge_f + x > (float) 0.0)
	    {			/* return 0*sign(x) if |x|<1 */
	      if (i0 >= 0)
		{
		  i0 = 0;
		}
	      else if (!FLT_UWORD_IS_ZERO (ix))
		{
		  i0 = 0xbf800000;
		}
	    }
	}
      else
	{
	  i = (0x007fffff) >> j0;
	  if ((i0 & i) == 0)
	    return x;		/* x is integral */
	  if (huge_f + x > (float) 0.0)
	    {			/* raise inexact flag */
	      if (i0 < 0)
		i0 += (0x00800000) >> j0;
	      i0 &= (~i);
	    }
	}
    }
  else
    {
      if (!FLT_UWORD_IS_FINITE (ix))
	return x + x;		/* inf or NaN */
      else
	return x;		/* x is integral */
    }
  SET_FLOAT_WORD (x, i0);
  return x;
}

/* x * 2**n */
float
scalbnf (float x, int n)
{
  __int32_t k, ix;
  __uint32_t hx;

  GET_FLOAT_WORD (ix, x);
  hx = ix & 0x7fffffff;
  k = hx >> 23;			/* extract exponent */
  if (FLT_UWORD_IS_ZERO (hx))
    return x;			/* +-0 */
  if (!FLT_UWORD_IS_FINITE (hx))
    return x + x;		/* NaN or Inf */
  if (FLT_UWORD_IS_SUBNORMAL (hx))
    {
      x *= (float) 3.355443200e+07;
      GET_FLOAT_WORD (ix, x);
      k = ((ix & 0x7f800000) >> 23) - 25;
      if (n < -50000)
	return tiny_f * x;	/*underflow */
    }
  k = k + n;
  if (k > FLT_LARGEST_EXP)
    return huge_f * copysignf (huge_f, x);	/* overflow  */
  if (k > 0)			/* normal result */
    {
      SET_FLOAT_WORD (x, (ix & 0x807fffff) | (k << 23));
      return x;
    }
  if (k < FLT_SMALLEST_EXP)
    {
      if (n > 50000)		/* in case integer overflow in n+k */
	return huge_f * copysignf (huge_f, x);	/*overflow */
      else
	return tiny_f * copysignf (tiny_f, x);	/*underflow */
    }
  k += 25;			/* subnormal result */
  SET_FLOAT_WORD (x, (ix & 0x807fffff) | (k << 23));
  return x * (float) 2.9802322388e-08;
}

/* sqrt(x), bit by bit */
float
__ieee754_sqrtf (float x)
{
  float z;
  __int32_t sign = (__int32_t) 0x80000000;
  __uint32_t r, hx;
  __int32_t ix, s, q, m, t, i;

  GET_FLOAT_WORD (ix, x);
  hx = ix & 0x7fffffff;

  /* take care of Inf and NaN */
  if (!FLT_UWORD_IS_FINITE (hx))
    return x * x + x;		/* sqrt(NaN)=NaN, sqrt(+inf)=+inf
				   sqrt(-inf)=sNaN */
  /* take care of zero and -ves */
  if (FLT_UWORD_IS_ZERO (hx))
    return x;			/* sqrt(+-0) = +-0 */
  if (ix < 0)
    return (x - x) / (x - x);	/* sqrt(-ve) = sNaN */

  /* normalize x */
  m = (ix >> 23);
  if (FLT_UWORD_IS_SUBNORMAL (hx))
    {				/* subnormal x */
      for (i = 0; (ix & 0x00800000) == 0; i++)
	ix <<= 1;
      m -= i - 1;
    }
  m -= 127;			/* unbias exponent */
  ix = (ix & 0x007fffff) | 0x00800000;
  if (m & 1)			/* odd m, double x to make it even */
    ix += ix;
  m >>= 1;			/* m = [m/2] */

  /* generate sqrt(x) bit by bit */
  ix += ix;
  q = s = 0;			/* q = sqrt(x) */
  r = 0x01000000;		/* r = moving bit from right to left */

  while (r != 0)
    {
      t = s + r;
      if (t <= ix)
	{
	  s = t + r;
	  ix -= t;
	  q += r;
	}
      ix += ix;
      r >>= 1;
    }

  /* use floating add to find out rounding direction */
  if (ix != 0)
    {
      z = 1.0f - tiny_f;	/* trigger inexact flag */
      if (z >= 1.0f)
	{
	  z = 1.0f + tiny_f;
	  if (z > 1.0f)
	    q += 2;
	  else
	    q += (q & 1);
	}
    }
  ix = (q >> 1) + 0x3f000000;
  ix += (m << 23);
  SET_FLOAT_WORD (z, ix);
  return z;
}

/* ------------------------------------------------------------------ */
/* e_sqrt.c                                                           */
/* ------------------------------------------------------------------ */

static const double one_sqrt = 1.0, tiny_sqrt = 1.0e-300;

double
__ieee754_sqrt (double x)
{
  double z;
  __int32_t sign = (__int32_t) 0x80000000;
  __int32_t ix0, s0, q, m, t, i;
  __uint32_t r, t1, s1, ix1, q1;

  EXTRACT_WORDS (ix0, ix1, x);

  /* take care of Inf and NaN */
  if ((ix0 & 0x7ff00000) == 0x7ff00000)
    {
      return x * x + x;		/* sqrt(NaN)=NaN, sqrt(+inf)=+inf
				   sqrt(-inf)=sNaN */
    }
  /* take care of zero */
  if (ix0 <= 0)
    {
      if (((ix0 & (~sign)) | ix1) == 0)
	return x;		/* sqrt(+-0) = +-0 */
      else if (ix0 < 0)
	return (x - x) / (x - x);	/* sqrt(-ve) = sNaN */
    }
  /* normalize x */
  m = (ix0 >> 20);
  if (m == 0)
    {				/* subnormal x */
      while (ix0 == 0)
	{
	  m -= 21;
	  ix0 |= (ix1 >> 11);
	  ix1 <<= 21;
	}
      for (i = 0; (ix0 & 0x00100000) == 0; i++)
	ix0 <<= 1;
      m -= i - 1;
      ix0 |= (ix1 >> (32 - i));
      ix1 <<= i;
    }
  m -= 1023;			/* unbias exponent */
  ix0 = (ix0 & 0x000fffff) | 0x00100000;
  if (m & 1)
    {				/* odd m, double x to make it even */
      ix0 += ix0 + ((ix1 & sign) >> 31);
      ix1 += ix1;
    }
  m >>= 1;			/* m = [m/2] */

  /* generate sqrt(x) bit by bit */
  ix0 += ix0 + ((ix1 & sign) >> 31);
  ix1 += ix1;
  q = q1 = s0 = s1 = 0;		/* [q,q1] = sqrt(x) */
  r = 0x00200000;		/* r = moving bit from right to left */

  while (r != 0)
    {
      t = s0 + r;
      if (t <= ix0)
	{
	  s0 = t + r;
	  ix0 -= t;
	  q += r;
	}
      ix0 += ix0 + ((ix1 & sign) >> 31);
      ix1 += ix1;
      r >>= 1;
    }

  r = sign;
  while (r != 0)
    {
      t1 = s1 + r;
      t = s0;
      if ((t < ix0) || ((t == ix0) && (t1 <= ix1)))
	{
	  s1 = t1 + r;
	  if (((t1 & sign) == sign) && (s1 & sign) == 0)
	    s0 += 1;
	  ix0 -= t;
	  if (ix1 < t1)
	    ix0 -= 1;
	  ix1 -= t1;
	  q1 += r;
	}
      ix0 += ix0 + ((ix1 & sign) >> 31);
      ix1 += ix1;
      r >>= 1;
    }

  /* use floating add to find out rounding direction */
  if ((ix0 | ix1) != 0)
    {
      z = one_sqrt - tiny_sqrt;	/* trigger inexact flag */
      if (z >= one_sqrt)
	{
	  z = one_sqrt + tiny_sqrt;
	  if (q1 == (__uint32_t) 0xffffffff)
	    {
	      q1 = 0;
	      q += 1;
	    }
	  else if (z > one_sqrt)
	    {
	      if (q1 == (__uint32_t) 0xfffffffe)
		q += 1;
	      q1 += 2;
	    }
	  else
	    q1 += (q1 & 1);
	}
    }
  ix0 = (q >> 1) + 0x3fe00000;
  ix1 = q1 >> 1;
  if ((q & 1) == 1)
    ix1 |= sign;
  ix0 += (m << 20);
  INSERT_WORDS (z, ix0, ix1);
  return z;
}

/* ------------------------------------------------------------------ */
/* s_floor.c                                                          */
/* ------------------------------------------------------------------ */

static const double huge_d = 1.0e300;

double
floor (double x)
{
  __int32_t i0, i1, j0;
  __uint32_t i, j;
  EXTRACT_WORDS (i0, i1, x);
  j0 = ((i0 >> 20) & 0x7ff) - 0x3ff;
  if (j0 < 20)
    {
      if (j0 < 0)
	{			/* raise inexact if x != 0 */
	  if (huge_d + x > 0.0)
	    {			/* return 0*sign(x) if |x|<1 */
	      if (i0 >= 0)
		{
		  i0 = i1 = 0;
		}
	      else if (((i0 & 0x7fffffff) | i1) != 0)
		{
		  i0 = 0xbff00000;
		  i1 = 0;
		}
	    }
	}
      else
	{
	  i = (0x000fffff) >> j0;
	  if (((i0 & i) | i1) == 0)
	    return x;		/* x is integral */
	  if (huge_d + x > 0.0)
	    {			/* raise inexact flag */
	      if (i0 < 0)
		i0 += (0x00100000) >> j0;
	      i0 &= (~i);
	      i1 = 0;
	    }
	}
    }
  else if (j0 > 51)
    {
      if (j0 == 0x400)
	return x + x;		/* inf or NaN */
      else
	return x;		/* x is integral */
    }
  else
    {
      i = ((__uint32_t) (0xffffffff)) >> (j0 - 20);
      if ((i1 & i) == 0)
	return x;		/* x is integral */
      if (huge_d + x > 0.0)
	{			/* raise inexact flag */
	  if (i0 < 0)
	    {
	      if (j0 == 20)
		i0 += 1;
	      else
		{
		  j = i1 + (1 << (52 - j0));
		  if (j < (__uint32_t) i1)
		    i0 += 1;	/* got a carry */
		  i1 = j;
		}
	    }
	  i1 &= (~i);
	}
    }
  INSERT_WORDS (x, i0, i1);
  return x;
}

/* ------------------------------------------------------------------ */
/* s_scalbn.c                                                         */
/* ------------------------------------------------------------------ */

static const double
  two54 = 1.80143985094819840000e+16,
  twom54 = 5.55111512312578270212e-17,
  huge_s = 1.0e+300, tiny_s = 1.0e-300;

double
scalbn (double x, int n)
{
  __int32_t k, hx, lx;
  EXTRACT_WORDS (hx, lx, x);
  k = (hx & 0x7ff00000) >> 20;	/* extract exponent */
  if (k == 0)
    {				/* 0 or subnormal x */
      if ((lx | (hx & 0x7fffffff)) == 0)
	return x;		/* +-0 */
      x *= two54;
      GET_HIGH_WORD (hx, x);
      k = ((hx & 0x7ff00000) >> 20) - 54;
      if (n < -50000)
	return tiny_s * x;	/*underflow */
    }
  if (k == 0x7ff)
    return x + x;		/* NaN or Inf */
  k = k + n;
  if (k > 0x7fe)
    return huge_s * copysign (huge_s, x);	/* overflow  */
  if (k > 0)			/* normal result */
    {
      SET_HIGH_WORD (x, (hx & 0x800fffff) | (k << 20));
      return x;
    }
  if (k <= -54)
    {
      if (n > 50000)		/* in case integer overflow in n+k */
	return huge_s * copysign (huge_s, x);	/*overflow */
      return tiny_s * copysign (tiny_s, x);	/*underflow */
    }
  k += 54;			/* subnormal result */
  SET_HIGH_WORD (x, (hx & 0x800fffff) | (k << 20));
  return x * twom54;
}

/* ------------------------------------------------------------------ */
/* k_sin.c                                                            */
/* ------------------------------------------------------------------ */

static const double
  half = 5.00000000000000000000e-01,
  S1 = -1.66666666666666324348e-01,
  S2 = 8.33333333332248946124e-03,
  S3 = -1.98412698298579493134e-04,
  S4 = 2.75573137070700676789e-06,
  S5 = -2.50507602534068634195e-08, S6 = 1.58969099521155010221e-10;

double
__kernel_sin (double x, double y, int iy)
{
  double z, r, v;
  __int32_t ix;
  GET_HIGH_WORD (ix, x);
  ix &= 0x7fffffff;		/* high word of x */
  if (ix < 0x3e400000)		/* |x| < 2**-27 */
    {
      if ((int) x == 0)
	return x;
    }				/* generate inexact */
  z = x * x;
  v = z * x;
  r = S2 + z * (S3 + z * (S4 + z * (S5 + z * S6)));
  if (iy == 0)
    return x + v * (S1 + z * r);
  else
    return x - ((z * (half * y - v * r) - y) - v * S1);
}

/* ------------------------------------------------------------------ */
/* k_cos.c                                                            */
/* ------------------------------------------------------------------ */

static const double
  one_c = 1.00000000000000000000e+00,
  C1 = 4.16666666666666019037e-02,
  C2 = -1.38888888888741095749e-03,
  C3 = 2.48015872894767294178e-05,
  C4 = -2.75573143513906633035e-07,
  C5 = 2.08757232129817482790e-09, C6 = -1.13596475577881948265e-11;

double
__kernel_cos (double x, double y)
{
  double a, hz, z, r, qx;
  __int32_t ix;
  GET_HIGH_WORD (ix, x);
  ix &= 0x7fffffff;		/* ix = |x|'s high word */
  if (ix < 0x3e400000)
    {				/* if x < 2**27 */
      if (((int) x) == 0)
	return one_c;		/* generate inexact */
    }
  z = x * x;
  r = z * (C1 + z * (C2 + z * (C3 + z * (C4 + z * (C5 + z * C6)))));
  if (ix < 0x3FD33333)		/* if |x| < 0.3 */
    return one_c - (0.5 * z - (z * r - x * y));
  else
    {
      if (ix > 0x3fe90000)
	{			/* x > 0.78125 */
	  qx = 0.28125;
	}
      else
	{
	  INSERT_WORDS (qx, ix - 0x00200000, 0);	/* x/4 */
	}
      hz = 0.5 * z - qx;
      a = one_c - qx;
      return a - (hz - (z * r - x * y));
    }
}

/* ------------------------------------------------------------------ */
/* k_rem_pio2.c                                                       */
/* ------------------------------------------------------------------ */

static const int init_jk[] = { 2, 3, 4, 6 };	/* initial value for jk */

static const double PIo2[] = {
  1.57079625129699707031e+00,	/* 0x3FF921FB, 0x40000000 */
  7.54978941586159635335e-08,	/* 0x3E74442D, 0x00000000 */
  5.39030252995776476554e-15,	/* 0x3CF84698, 0x80000000 */
  3.28200341580791294123e-22,	/* 0x3B78CC51, 0x60000000 */
  1.27065575308067607349e-29,	/* 0x39F01B83, 0x80000000 */
  1.22933308981111328932e-36,	/* 0x387A2520, 0x40000000 */
  2.73370053816464559624e-44,	/* 0x36E38222, 0x80000000 */
  2.16741683877804819444e-51,	/* 0x3569F31D, 0x00000000 */
};

static const double
  zero_k = 0.0,
  one_k = 1.0,
  two24_k = 1.67772160000000000000e+07, twon24 = 5.96046447753906250000e-08;

int
__kernel_rem_pio2 (double *x, double *y, int e0, int nx, int prec,
		   const __int32_t *ipio2)
{
  __int32_t jz, jx, jv, jp, jk, carry, n, iq[20], i, j, k, m, q0, ih;
  double z, fw, f[20], fq[20], q[20];

  /* initialize jk */
  jk = init_jk[prec];
  jp = jk;

  /* determine jx,jv,q0, note that 3>q0 */
  jx = nx - 1;
  jv = (e0 - 3) / 24;
  if (jv < 0)
    jv = 0;
  q0 = e0 - 24 * (jv + 1);

  /* set up f[0] to f[jx+jk] where f[jx+jk] = ipio2[jv+jk] */
  j = jv - jx;
  m = jx + jk;
  for (i = 0; i <= m; i++, j++)
    f[i] = (j < 0) ? zero_k : (double) ipio2[j];

  /* compute q[0],q[1],...q[jk] */
  for (i = 0; i <= jk; i++)
    {
      for (j = 0, fw = 0.0; j <= jx; j++)
	fw += x[j] * f[jx + i - j];
      q[i] = fw;
    }

  jz = jk;
recompute:
  /* distill q[] into iq[] reversingly */
  for (i = 0, j = jz, z = q[jz]; j > 0; i++, j--)
    {
      fw = (double) ((__int32_t) (twon24 * z));
      iq[i] = (__int32_t) (z - two24_k * fw);
      z = q[j - 1] + fw;
    }

  /* compute n */
  z = scalbn (z, q0);		/* actual value of z */
  z -= 8.0 * floor (z * 0.125);	/* trim off integer >= 8 */
  n = (__int32_t) z;
  z -= (double) n;
  ih = 0;
  if (q0 > 0)
    {				/* need iq[jz-1] to determine n */
      i = (iq[jz - 1] >> (24 - q0));
      n += i;
      iq[jz - 1] -= i << (24 - q0);
      ih = iq[jz - 1] >> (23 - q0);
    }
  else if (q0 == 0)
    ih = iq[jz - 1] >> 23;
  else if (z >= 0.5)
    ih = 2;

  if (ih > 0)
    {				/* q > 0.5 */
      n += 1;
      carry = 0;
      for (i = 0; i < jz; i++)
	{			/* compute 1-q */
	  j = iq[i];
	  if (carry == 0)
	    {
	      if (j != 0)
		{
		  carry = 1;
		  iq[i] = 0x1000000 - j;
		}
	    }
	  else
	    iq[i] = 0xffffff - j;
	}
      if (q0 > 0)
	{			/* rare case: chance is 1 in 12 */
	  switch (q0)
	    {
	    case 1:
	      iq[jz - 1] &= 0x7fffff;
	      break;
	    case 2:
	      iq[jz - 1] &= 0x3fffff;
	      break;
	    }
	}
      if (ih == 2)
	{
	  z = one_k - z;
	  if (carry != 0)
	    z -= scalbn (one_k, q0);
	}
    }

  /* check if recomputation is needed */
  if (z == zero_k)
    {
      j = 0;
      for (i = jz - 1; i >= jk; i--)
	j |= iq[i];
      if (j == 0)
	{			/* need recomputation */
	  for (k = 1; iq[jk - k] == 0; k++);	/* k = no. of terms needed */

	  for (i = jz + 1; i <= jz + k; i++)
	    {			/* add q[jz+1] to q[jz+k] */
	      f[jx + i] = (double) ipio2[jv + i];
	      for (j = 0, fw = 0.0; j <= jx; j++)
		fw += x[j] * f[jx + i - j];
	      q[i] = fw;
	    }
	  jz += k;
	  goto recompute;
	}
    }

  /* chop off zero terms */
  if (z == 0.0)
    {
      jz -= 1;
      q0 -= 24;
      while (iq[jz] == 0)
	{
	  jz--;
	  q0 -= 24;
	}
    }
  else
    {				/* break z into 24-bit if necessary */
      z = scalbn (z, -q0);
      if (z >= two24_k)
	{
	  fw = (double) ((__int32_t) (twon24 * z));
	  iq[jz] = (__int32_t) (z - two24_k * fw);
	  jz += 1;
	  q0 += 24;
	  iq[jz] = (__int32_t) fw;
	}
      else
	iq[jz] = (__int32_t) z;
    }

  /* convert integer "bit" chunk to floating-point value */
  fw = scalbn (one_k, q0);
  for (i = jz; i >= 0; i--)
    {
      q[i] = fw * (double) iq[i];
      fw *= twon24;
    }

  /* compute PIo2[0,...,jp]*q[jz,...,0] */
  for (i = jz; i >= 0; i--)
    {
      for (fw = 0.0, k = 0; k <= jp && k <= jz - i; k++)
	fw += PIo2[k] * q[i + k];
      fq[jz - i] = fw;
    }

  /* compress fq[] into y[] */
  switch (prec)
    {
    case 0:
      fw = 0.0;
      for (i = jz; i >= 0; i--)
	fw += fq[i];
      y[0] = (ih == 0) ? fw : -fw;
      break;
    case 1:
    case 2:
      fw = 0.0;
      for (i = jz; i >= 0; i--)
	fw += fq[i];
      y[0] = (ih == 0) ? fw : -fw;
      fw = fq[0] - fw;
      for (i = 1; i <= jz; i++)
	fw += fq[i];
      y[1] = (ih == 0) ? fw : -fw;
      break;
    case 3:			/* painful */
      for (i = jz; i > 0; i--)
	{
	  fw = fq[i - 1] + fq[i];
	  fq[i] += fq[i - 1] - fw;
	  fq[i - 1] = fw;
	}
      for (i = jz; i > 1; i--)
	{
	  fw = fq[i - 1] + fq[i];
	  fq[i] += fq[i - 1] - fw;
	  fq[i - 1] = fw;
	}
      for (fw = 0.0, i = jz; i >= 2; i--)
	fw += fq[i];
      if (ih == 0)
	{
	  y[0] = fq[0];
	  y[1] = fq[1];
	  y[2] = fw;
	}
      else
	{
	  y[0] = -fq[0];
	  y[1] = -fq[1];
	  y[2] = -fw;
	}
    }
  return n & 7;
}

/* ------------------------------------------------------------------ */
/* e_rem_pio2.c                                                       */
/* ------------------------------------------------------------------ */

/*
 * Table of constants for 2/pi, 396 hex digits (476 decimal) of 2/pi.
 */
static const __int32_t two_over_pi[] = {
  0xA2F983, 0x6E4E44, 0x1529FC, 0x2757D1, 0xF534DD, 0xC0DB62,
  0x95993C, 0x439041, 0xFE5163, 0xABDEBB, 0xC561B7, 0x246E3A,
  0x424DD2, 0xE00649, 0x2EEA09, 0xD1921C, 0xFE1DEB, 0x1CB129,
  0xA73EE8, 0x8235F5, 0x2EBB44, 0x84E99C, 0x7026B4, 0x5F7E41,
  0x3991D6, 0x398353, 0x39F49C, 0x845F8B, 0xBDF928, 0x3B1FF8,
  0x97FFDE, 0x05980F, 0xEF2F11, 0x8B5A0A, 0x6D1F6D, 0x367ECF,
  0x27CB09, 0xB74F46, 0x3F669E, 0x5FEA2D, 0x7527BA, 0xC7EBE5,
  0xF17B3D, 0x0739F7, 0x8A5292, 0xEA6BFB, 0x5FB11F, 0x8D5D08,
  0x560330, 0x46FC7B, 0x6BABF0, 0xCFBC20, 0x9AF436, 0x1DA9E3,
  0x91615E, 0xE61B08, 0x659985, 0x5F14A0, 0x68408D, 0xFFD880,
  0x4D7327, 0x310606, 0x1556CA, 0x73A8C9, 0x60E27B, 0xC08C6B,
};

static const __int32_t npio2_hw[] = {
  0x3FF921FB, 0x400921FB, 0x4012D97C, 0x401921FB, 0x401F6A7A, 0x4022D97C,
  0x4025FDBB, 0x402921FB, 0x402C463A, 0x402F6A7A, 0x4031475C, 0x4032D97C,
  0x40346B9C, 0x4035FDBB, 0x40378FDB, 0x403921FB, 0x403AB41B, 0x403C463A,
  0x403DD85A, 0x403F6A7A, 0x40407E4C, 0x4041475C, 0x4042106C, 0x4042D97C,
  0x4043A28C, 0x40446B9C, 0x404534AC, 0x4045FDBB, 0x4046C6CB, 0x40478FDB,
  0x404858EB, 0x404921FB,
};

static const double
  zero_r = 0.00000000000000000000e+00,
  half_r = 5.00000000000000000000e-01,
  two24_r = 1.67772160000000000000e+07,
  invpio2 = 6.36619772367581382433e-01,
  pio2_1 = 1.57079632673412561417e+00,
  pio2_1t = 6.07710050650619224932e-11,
  pio2_2 = 6.07710050630396597660e-11,
  pio2_2t = 2.02226624879595063154e-21,
  pio2_3 = 2.02226624871116645580e-21, pio2_3t = 8.47842766036889956997e-32;

__int32_t
__ieee754_rem_pio2 (double x, double *y)
{
  double z, w, t, r, fn;
  double tx[3];
  __int32_t e0, i, j, nx, n, ix, hx;
  __uint32_t low;

  GET_HIGH_WORD (hx, x);	/* high word of x */
  ix = hx & 0x7fffffff;
  if (ix <= 0x3fe921fb)		/* |x| ~<= pi/4 , no need for reduction */
    {
      y[0] = x;
      y[1] = 0;
      return 0;
    }
  if (ix < 0x4002d97c)
    {				/* |x| < 3pi/4, special case with n=+-1 */
      if (hx > 0)
	{
	  z = x - pio2_1;
	  if (ix != 0x3ff921fb)
	    {			/* 33+53 bit pi is good enough */
	      y[0] = z - pio2_1t;
	      y[1] = (z - y[0]) - pio2_1t;
	    }
	  else
	    {			/* near pi/2, use 33+33+53 bit pi */
	      z -= pio2_2;
	      y[0] = z - pio2_2t;
	      y[1] = (z - y[0]) - pio2_2t;
	    }
	  return 1;
	}
      else
	{			/* negative x */
	  z = x + pio2_1;
	  if (ix != 0x3ff921fb)
	    {			/* 33+53 bit pi is good enough */
	      y[0] = z + pio2_1t;
	      y[1] = (z - y[0]) + pio2_1t;
	    }
	  else
	    {			/* near pi/2, use 33+33+53 bit pi */
	      z += pio2_2;
	      y[0] = z + pio2_2t;
	      y[1] = (z - y[0]) + pio2_2t;
	    }
	  return -1;
	}
    }
  if (ix <= 0x413921fb)
    {				/* |x| ~<= 2^19*(pi/2), medium size */
      t = fabs (x);
      n = (__int32_t) (t * invpio2 + half_r);
      fn = (double) n;
      r = t - fn * pio2_1;
      w = fn * pio2_1t;		/* 1st round good to 85 bit */
      if (n < 32 && ix != npio2_hw[n - 1])
	{
	  y[0] = r - w;		/* quick check no cancellation */
	}
      else
	{
	  __uint32_t high;
	  j = ix >> 20;
	  y[0] = r - w;
	  GET_HIGH_WORD (high, y[0]);
	  i = j - ((high >> 20) & 0x7ff);
	  if (i > 16)
	    {			/* 2nd iteration needed, good to 118 */
	      t = r;
	      w = fn * pio2_2;
	      r = t - w;
	      w = fn * pio2_2t - ((t - r) - w);
	      y[0] = r - w;
	      GET_HIGH_WORD (high, y[0]);
	      i = j - ((high >> 20) & 0x7ff);
	      if (i > 49)
		{		/* 3rd iteration need, 151 bits acc */
		  t = r;	/* will cover all possible cases */
		  w = fn * pio2_3;
		  r = t - w;
		  w = fn * pio2_3t - ((t - r) - w);
		  y[0] = r - w;
		}
	    }
	}
      y[1] = (r - y[0]) - w;
      if (hx < 0)
	{
	  y[0] = -y[0];
	  y[1] = -y[1];
	  return -n;
	}
      else
	return n;
    }
  /*
   * all other (large) arguments
   */
  if (ix >= 0x7ff00000)
    {				/* x is inf or NaN */
      y[0] = y[1] = x - x;
      return 0;
    }
  /* set z = scalbn(|x|,ilogb(x)-23) */
  GET_LOW_WORD (low, x);
  SET_LOW_WORD (z, low);
  e0 = (ix >> 20) - 1046;	/* e0 = ilogb(z)-23; */
  SET_HIGH_WORD (z, ix - ((__int32_t) (e0 << 20)));
  for (i = 0; i < 2; i++)
    {
      tx[i] = (double) ((__int32_t) (z));
      z = (z - tx[i]) * two24_r;
    }
  tx[2] = z;
  nx = 3;
  while (tx[nx - 1] == zero_r)
    nx--;			/* skip zero term */
  n = __kernel_rem_pio2 (tx, y, e0, nx, 2, two_over_pi);
  if (hx < 0)
    {
      y[0] = -y[0];
      y[1] = -y[1];
      return -n;
    }
  return n;
}

/* ------------------------------------------------------------------ */
/* s_sin.c / s_cos.c                                                  */
/* ------------------------------------------------------------------ */

double
sin (double x)
{
  double y[2], z = 0.0;
  __int32_t n, ix;

  /* High word of x. */
  GET_HIGH_WORD (ix, x);

  /* |x| ~< pi/4 */
  ix &= 0x7fffffff;
  if (ix <= 0x3fe921fb)
    return __kernel_sin (x, z, 0);

  /* sin(Inf or NaN) is NaN */
  else if (ix >= 0x7ff00000)
    return x - x;

  /* argument reduction needed */
  else
    {
      n = __ieee754_rem_pio2 (x, y);
      switch (n & 3)
	{
	case 0:
	  return __kernel_sin (y[0], y[1], 1);
	case 1:
	  return __kernel_cos (y[0], y[1]);
	case 2:
	  return -__kernel_sin (y[0], y[1], 1);
	default:
	  return -__kernel_cos (y[0], y[1]);
	}
    }
}

double
cos (double x)
{
  double y[2], z = 0.0;
  __int32_t n, ix;

  /* High word of x. */
  GET_HIGH_WORD (ix, x);

  /* |x| ~< pi/4 */
  ix &= 0x7fffffff;
  if (ix <= 0x3fe921fb)
    return __kernel_cos (x, z);

  /* cos(Inf or NaN) is NaN */
  else if (ix >= 0x7ff00000)
    return x - x;

  /* argument reduction needed */
  else
    {
      n = __ieee754_rem_pio2 (x, y);
      switch (n & 3)
	{
	case 0:
	  return __kernel_cos (y[0], y[1]);
	case 1:
	  return -__kernel_sin (y[0], y[1], 1);
	case 2:
	  return -__kernel_cos (y[0], y[1]);
	default:
	  return __kernel_sin (y[0], y[1], 1);
	}
    }
}

/* ------------------------------------------------------------------ */
/* e_fmod.c                                                           */
/* ------------------------------------------------------------------ */

static const double one_f = 1.0, Zero[] = { 0.0, -0.0, };

double
__ieee754_fmod (double x, double y)
{
  __int32_t n, hx, hy, hz, ix, iy, sx, i;
  __uint32_t lx, ly, lz;

  EXTRACT_WORDS (hx, lx, x);
  EXTRACT_WORDS (hy, ly, y);
  sx = hx & 0x80000000;		/* sign of x */
  hx ^= sx;			/* |x| */
  hy &= 0x7fffffff;		/* |y| */

  /* purge off exception values */
  if ((hy | ly) == 0 || (hx >= 0x7ff00000) ||	/* y=0,or x not finite */
      ((hy | ((ly | -ly) >> 31)) > 0x7ff00000))	/* or y is NaN */
    return (x * y) / (x * y);
  if (hx <= hy)
    {
      if ((hx < hy) || (lx < ly))
	return x;		/* |x|<|y| return x */
      if (lx == ly)
	return Zero[(__uint32_t) sx >> 31];	/* |x|=|y| return x*0 */
    }

  /* determine ix = ilogb(x) */
  if (hx < 0x00100000)
    {				/* subnormal x */
      if (hx == 0)
	{
	  for (ix = -1043, i = lx; i > 0; i <<= 1)
	    ix -= 1;
	}
      else
	{
	  for (ix = -1022, i = (hx << 11); i > 0; i <<= 1)
	    ix -= 1;
	}
    }
  else
    ix = (hx >> 20) - 1023;

  /* determine iy = ilogb(y) */
  if (hy < 0x00100000)
    {				/* subnormal y */
      if (hy == 0)
	{
	  for (iy = -1043, i = ly; i > 0; i <<= 1)
	    iy -= 1;
	}
      else
	{
	  for (iy = -1022, i = (hy << 11); i > 0; i <<= 1)
	    iy -= 1;
	}
    }
  else
    iy = (hy >> 20) - 1023;

  /* set up {hx,lx}, {hy,ly} and align y to x */
  if (ix >= -1022)
    hx = 0x00100000 | (0x000fffff & hx);
  else
    {				/* subnormal x, shift x to normal */
      n = -1022 - ix;
      if (n <= 31)
	{
	  hx = (hx << n) | (lx >> (32 - n));
	  lx <<= n;
	}
      else
	{
	  hx = lx << (n - 32);
	  lx = 0;
	}
    }
  if (iy >= -1022)
    hy = 0x00100000 | (0x000fffff & hy);
  else
    {				/* subnormal y, shift y to normal */
      n = -1022 - iy;
      if (n <= 31)
	{
	  hy = (hy << n) | (ly >> (32 - n));
	  ly <<= n;
	}
      else
	{
	  hy = ly << (n - 32);
	  ly = 0;
	}
    }

  /* fix point fmod */
  n = ix - iy;
  while (n--)
    {
      hz = hx - hy;
      lz = lx - ly;
      if (lx < ly)
	hz -= 1;
      if (hz < 0)
	{
	  hx = hx + hx + (lx >> 31);
	  lx = lx + lx;
	}
      else
	{
	  if ((hz | lz) == 0)	/* return sign(x)*0 */
	    return Zero[(__uint32_t) sx >> 31];
	  hx = hz + hz + (lz >> 31);
	  lx = lz + lz;
	}
    }
  hz = hx - hy;
  lz = lx - ly;
  if (lx < ly)
    hz -= 1;
  if (hz >= 0)
    {
      hx = hz;
      lx = lz;
    }

  /* convert back to floating value and restore the sign */
  if ((hx | lx) == 0)		/* return sign(x)*0 */
    return Zero[(__uint32_t) sx >> 31];
  while (hx < 0x00100000)
    {				/* normalize x */
      hx = hx + hx + (lx >> 31);
      lx = lx + lx;
      iy -= 1;
    }
  if (iy >= -1022)
    {				/* normalize output */
      hx = ((hx - 0x00100000) | ((iy + 1023) << 20));
      INSERT_WORDS (x, hx | sx, lx);
    }
  else
    {				/* subnormal output */
      n = -1022 - iy;
      if (n <= 20)
	{
	  lx = (lx >> n) | ((__uint32_t) hx << (32 - n));
	  hx >>= n;
	}
      else if (n <= 31)
	{
	  lx = (hx << (32 - n)) | (lx >> n);
	  hx = sx;
	}
      else
	{
	  lx = hx >> (n - 32);
	  hx = sx;
	}
      INSERT_WORDS (x, hx | sx, lx);
      x *= one_f;		/* create necessary signal */
    }
  return x;			/* exact output */
}

/* ------------------------------------------------------------------ */
/* s_atan.c                                                           */
/* ------------------------------------------------------------------ */

static const double atanhi[] = {
  4.63647609000806093515e-01,	/* atan(0.5)hi 0x3FDDAC67, 0x0561BB4F */
  7.85398163397448278999e-01,	/* atan(1.0)hi 0x3FE921FB, 0x54442D18 */
  9.82793723247329054082e-01,	/* atan(1.5)hi 0x3FEF730B, 0xD281F69B */
  1.57079632679489655800e+00,	/* atan(inf)hi 0x3FF921FB, 0x54442D18 */
};

static const double atanlo[] = {
  2.26987774529616870924e-17,	/* atan(0.5)lo 0x3C7A2B7F, 0x222F65E2 */
  3.06161699786838301793e-17,	/* atan(1.0)lo 0x3C81A626, 0x33145C07 */
  1.39033110312309984516e-17,	/* atan(1.5)lo 0x3C700788, 0x7AF0CBBD */
  6.12323399573676603587e-17,	/* atan(inf)lo 0x3C91A626, 0x33145C07 */
};

static const double aT[] = {
  3.33333333333329318027e-01,	/* 0x3FD55555, 0x5555550D */
  -1.99999999998764832476e-01,	/* 0xBFC99999, 0x9998EBC4 */
  1.42857142725034663711e-01,	/* 0x3FC24924, 0x920083FF */
  -1.11111104054623557880e-01,	/* 0xBFBC71C6, 0xFE231671 */
  9.09088713343650656196e-02,	/* 0x3FB745CD, 0xC54C206E */
  -7.69187620504482999495e-02,	/* 0xBFB3B0F2, 0xAF749A6D */
  6.66107313738753120669e-02,	/* 0x3FB10D66, 0xA0D03D51 */
  -5.83357013379057348645e-02,	/* 0xBFADDE2D, 0x52DEFD9A */
  4.97687799461593236017e-02,	/* 0x3FA97B4B, 0x24760DEB */
  -3.65315727442169155270e-02,	/* 0xBFA2B444, 0x2C6A6C2F */
  1.62858201153657823623e-02,	/* 0x3F90AD3A, 0xE322DA11 */
};

static const double one_a = 1.0, huge_a = 1.0e300;

double
atan (double x)
{
  double w, s1, s2, z;
  __int32_t ix, hx, id;

  GET_HIGH_WORD (hx, x);
  ix = hx & 0x7fffffff;
  if (ix >= 0x44100000)
    {				/* if |x| >= 2^66 */
      __uint32_t low;
      GET_LOW_WORD (low, x);
      if (ix > 0x7ff00000 || (ix == 0x7ff00000 && (low != 0)))
	return x + x;		/* NaN */
      if (hx > 0)
	return atanhi[3] + atanlo[3];
      else
	return -atanhi[3] - atanlo[3];
    }
  if (ix < 0x3fdc0000)
    {				/* |x| < 0.4375 */
      if (ix < 0x3e200000)
	{			/* |x| < 2^-29 */
	  if (huge_a + x > one_a)
	    return x;		/* raise inexact */
	}
      id = -1;
    }
  else
    {
      x = fabs (x);
      if (ix < 0x3ff30000)
	{			/* |x| < 1.1875 */
	  if (ix < 0x3fe60000)
	    {			/* 7/16 <=|x|<11/16 */
	      id = 0;
	      x = (2.0 * x - one_a) / (2.0 + x);
	    }
	  else
	    {			/* 11/16<=|x|< 19/16 */
	      id = 1;
	      x = (x - one_a) / (x + one_a);
	    }
	}
      else
	{
	  if (ix < 0x40038000)
	    {			/* |x| < 2.4375 */
	      id = 2;
	      x = (x - 1.5) / (one_a + 1.5 * x);
	    }
	  else
	    {			/* 2.4375 <= |x| < 2^66 */
	      id = 3;
	      x = -1.0 / x;
	    }
	}
    }
  /* end of argument reduction */
  z = x * x;
  w = z * z;
  /* break sum from i=0 to 10 aT[i]z**(i+1) into odd and even poly */
  s1 = z * (aT[0] + w * (aT[2] + w * (aT[4] + w * (aT[6] + w * (aT[8]
								+ w *
								aT[10])))));
  s2 = w * (aT[1] + w * (aT[3] + w * (aT[5] + w * (aT[7] + w * aT[9]))));
  if (id < 0)
    return x - x * (s1 + s2);
  else
    {
      z = atanhi[id] - ((x * (s1 + s2) - atanlo[id]) - x);
      return (hx < 0) ? -z : z;
    }
}

/* ------------------------------------------------------------------ */
/* e_atan2.c                                                          */
/* ------------------------------------------------------------------ */

static const double
  tiny_at = 1.0e-300,
  zero_at = 0.0,
  pi_o_4 = 7.8539816339744827900E-01,
  pi_o_2 = 1.5707963267948965580E+00,
  pi_at = 3.1415926535897931160E+00, pi_lo = 1.2246467991473531772E-16;

double
__ieee754_atan2 (double y, double x)
{
  double z;
  __int32_t k, m, hx, hy, ix, iy;
  __uint32_t lx, ly;

  EXTRACT_WORDS (hx, lx, x);
  ix = hx & 0x7fffffff;
  EXTRACT_WORDS (hy, ly, y);
  iy = hy & 0x7fffffff;
  if (((ix | ((lx | -lx) >> 31)) > 0x7ff00000) ||
      ((iy | ((ly | -ly) >> 31)) > 0x7ff00000))	/* x or y is NaN */
    return x + y;
  if ((hx - 0x3ff00000 | lx) == 0)
    return atan (y);		/* x=1.0 */
  m = ((hy >> 31) & 1) | ((hx >> 30) & 2);	/* 2*sign(x)+sign(y) */

  /* when y = 0 */
  if ((iy | ly) == 0)
    {
      switch (m)
	{
	case 0:
	case 1:
	  return y;		/* atan(+-0,+anything)=+-0 */
	case 2:
	  return pi_at + tiny_at;	/* atan(+0,-anything) = pi */
	case 3:
	  return -pi_at - tiny_at;	/* atan(-0,-anything) =-pi */
	}
    }
  /* when x = 0 */
  if ((ix | lx) == 0)
    return (hy < 0) ? -pi_o_2 - tiny_at : pi_o_2 + tiny_at;

  /* when x is INF */
  if (ix == 0x7ff00000)
    {
      if (iy == 0x7ff00000)
	{
	  switch (m)
	    {
	    case 0:
	      return pi_o_4 + tiny_at;	/* atan(+INF,+INF) */
	    case 1:
	      return -pi_o_4 - tiny_at;	/* atan(-INF,+INF) */
	    case 2:
	      return 3.0 * pi_o_4 + tiny_at;	/*atan(+INF,-INF) */
	    case 3:
	      return -3.0 * pi_o_4 - tiny_at;	/*atan(-INF,-INF) */
	    }
	}
      else
	{
	  switch (m)
	    {
	    case 0:
	      return zero_at;	/* atan(+...,+INF) */
	    case 1:
	      return -zero_at;	/* atan(-...,+INF) */
	    case 2:
	      return pi_at + tiny_at;	/* atan(+...,-INF) */
	    case 3:
	      return -pi_at - tiny_at;	/* atan(-...,-INF) */
	    }
	}
    }
  /* when y is INF */
  if (iy == 0x7ff00000)
    return (hy < 0) ? -pi_o_2 - tiny_at : pi_o_2 + tiny_at;

  /* compute y/x */
  k = (iy - ix) >> 20;
  if (k > 60)
    z = pi_o_2 + 0.5 * pi_lo;	/* |y/x| >  2**60 */
  else if (hx < 0 && k < -60)
    z = 0.0;			/* |y|/x < -2**60 */
  else
    z = atan (fabs (y / x));	/* safe to do y/x */
  switch (m)
    {
    case 0:
      return z;			/* atan(+,+) */
    case 1:
      {
	__uint32_t zh;
	GET_HIGH_WORD (zh, z);
	SET_HIGH_WORD (z, zh ^ 0x80000000);
      }
      return z;			/* atan(-,+) */
    case 2:
      return pi_at - (z - pi_lo);	/* atan(+,-) */
    default:			/* case 3 */
      return (z - pi_lo) - pi_at;	/* atan(-,-) */
    }
}

/* ------------------------------------------------------------------ */
/* e_acos.c                                                           */
/* ------------------------------------------------------------------ */

static const double
  one_ac = 1.00000000000000000000e+00,
  pi_ac = 3.14159265358979311600e+00,
  pio2_hi = 1.57079632679489655800e+00,
  pio2_lo = 6.12323399573676603587e-17,
  pS0 = 1.66666666666666657415e-01,
  pS1 = -3.25565818622400915405e-01,
  pS2 = 2.01212532134862925881e-01,
  pS3 = -4.00555345006794114027e-02,
  pS4 = 7.91534994289814532176e-04,
  pS5 = 3.47933107596021167570e-05,
  qS1 = -2.40339491173441421878e+00,
  qS2 = 2.02094576023350569471e+00,
  qS3 = -6.88283971605453293030e-01, qS4 = 7.70381505559019352791e-02;

double
__ieee754_acos (double x)
{
  double z, p, q, r, w, s, c, df;
  __int32_t hx, ix;
  GET_HIGH_WORD (hx, x);
  ix = hx & 0x7fffffff;
  if (ix >= 0x3ff00000)
    {				/* |x| >= 1 */
      __uint32_t lx;
      GET_LOW_WORD (lx, x);
      if (((ix - 0x3ff00000) | lx) == 0)
	{			/* |x|==1 */
	  if (hx > 0)
	    return 0.0;		/* acos(1) = 0  */
	  else
	    return pi_ac + 2.0 * pio2_lo;	/* acos(-1)= pi */
	}
      return (x - x) / (x - x);	/* acos(|x|>1) is NaN */
    }
  if (ix < 0x3fe00000)
    {				/* |x| < 0.5 */
      if (ix <= 0x3c600000)
	return pio2_hi + pio2_lo;	/*if|x|<2**-57 */
      z = x * x;
      p = z * (pS0 + z * (pS1 + z * (pS2 + z * (pS3 + z * (pS4 + z * pS5)))));
      q = one_ac + z * (qS1 + z * (qS2 + z * (qS3 + z * qS4)));
      r = p / q;
      return pio2_hi - (x - (pio2_lo - x * r));
    }
  else if (hx < 0)
    {				/* x < -0.5 */
      z = (one_ac + x) * 0.5;
      p = z * (pS0 + z * (pS1 + z * (pS2 + z * (pS3 + z * (pS4 + z * pS5)))));
      q = one_ac + z * (qS1 + z * (qS2 + z * (qS3 + z * qS4)));
      s = __ieee754_sqrt (z);
      r = p / q;
      w = r * s - pio2_lo;
      return pi_ac - 2.0 * (s + w);
    }
  else
    {				/* x > 0.5 */
      z = (one_ac - x) * 0.5;
      s = __ieee754_sqrt (z);
      df = s;
      SET_LOW_WORD (df, 0);
      c = (z - df * df) / (s + df);
      p = z * (pS0 + z * (pS1 + z * (pS2 + z * (pS3 + z * (pS4 + z * pS5)))));
      q = one_ac + z * (qS1 + z * (qS2 + z * (qS3 + z * qS4)));
      r = p / q;
      w = r * s + c;
      return 2.0 * (df + w);
    }
}

/* ------------------------------------------------------------------ */
/* e_pow.c                                                            */
/* ------------------------------------------------------------------ */

static const double
  bp[] = { 1.0, 1.5, },
  dp_h[] = { 0.0, 5.84962487220764160156e-01, },
  dp_l[] = { 0.0, 1.35003920212974897128e-08, },
  zero_p = 0.0,
  one_p = 1.0,
  two_p = 2.0,
  two53 = 9007199254740992.0,
  huge_p = 1.0e300,
  tiny_p = 1.0e-300,
  L1 = 5.99999999999994648725e-01,
  L2 = 4.28571428578550184252e-01,
  L3 = 3.33333329818377432918e-01,
  L4 = 2.72728123808534006489e-01,
  L5 = 2.30660745775561754067e-01,
  L6 = 2.06975017800338417784e-01,
  P1 = 1.66666666666666019037e-01,
  P2 = -2.77777777770155933842e-03,
  P3 = 6.61375632143793436117e-05,
  P4 = -1.65339022054652515390e-06,
  P5 = 4.13813679705723846039e-08,
  lg2 = 6.93147180559945286227e-01,
  lg2_h = 6.93147182464599609375e-01,
  lg2_l = -1.90465429995776804525e-09,
  ovt = 8.0085662595372944372e-0017,
  cp = 9.61796693925975554329e-01,
  cp_h = 9.61796700954437255859e-01,
  cp_l = -7.02846165095275826516e-09,
  ivln2 = 1.44269504088896338700e+00,
  ivln2_h = 1.44269502162933349609e+00,
  ivln2_l = 1.92596299112661746887e-08;

double
__ieee754_pow (double x, double y)
{
  double z, ax, z_h, z_l, p_h, p_l;
  double y1, t1, t2, r, s, t, u, v, w;
  __int32_t i, j, k, yisint, n;
  __int32_t hx, hy, ix, iy;
  __uint32_t lx, ly;

  EXTRACT_WORDS (hx, lx, x);
  EXTRACT_WORDS (hy, ly, y);
  ix = hx & 0x7fffffff;
  iy = hy & 0x7fffffff;

  /* y==zero: x**0 = 1 */
  if ((iy | ly) == 0)
    return one_p;

  /* +-NaN return x+y */
  if (ix > 0x7ff00000 || ((ix == 0x7ff00000) && (lx != 0)) ||
      iy > 0x7ff00000 || ((iy == 0x7ff00000) && (ly != 0)))
    return x + y;

  /* determine if y is an odd int when x < 0
   * yisint = 0    ... y is not an integer
   * yisint = 1    ... y is an odd int
   * yisint = 2    ... y is an even int
   */
  yisint = 0;
  if (hx < 0)
    {
      if (iy >= 0x43400000)
	yisint = 2;		/* even integer y */
      else if (iy >= 0x3ff00000)
	{
	  k = (iy >> 20) - 0x3ff;	/* exponent */
	  if (k > 20)
	    {
	      j = ly >> (52 - k);
	      if ((j << (52 - k)) == ly)
		yisint = 2 - (j & 1);
	    }
	  else if (ly == 0)
	    {
	      j = iy >> (20 - k);
	      if ((j << (20 - k)) == iy)
		yisint = 2 - (j & 1);
	    }
	}
    }

  /* special value of y */
  if (ly == 0)
    {
      if (iy == 0x7ff00000)
	{			/* y is +-inf */
	  if (((ix - 0x3ff00000) | lx) == 0)
	    return y - y;	/* inf**+-1 is NaN */
	  else if (ix >= 0x3ff00000)	/* (|x|>1)**+-inf = inf,0 */
	    return (hy >= 0) ? y : zero_p;
	  else			/* (|x|<1)**-,+inf = inf,0 */
	    return (hy < 0) ? -y : zero_p;
	}
      if (iy == 0x3ff00000)
	{			/* y is  +-1 */
	  if (hy < 0)
	    return one_p / x;
	  else
	    return x;
	}
      if (hy == 0x40000000)
	return x * x;		/* y is  2 */
      if (hy == 0x3fe00000)
	{			/* y is  0.5 */
	  if (hx >= 0)		/* x >= +0 */
	    return __ieee754_sqrt (x);
	}
    }

  ax = fabs (x);
  /* special value of x */
  if (lx == 0)
    {
      if (ix == 0x7ff00000 || ix == 0 || ix == 0x3ff00000)
	{
	  z = ax;		/*x is +-0,+-inf,+-1 */
	  if (hy < 0)
	    z = one_p / z;	/* z = (1/|x|) */
	  if (hx < 0)
	    {
	      if (((ix - 0x3ff00000) | yisint) == 0)
		{
		  z = (z - z) / (z - z);	/* (-1)**non-int is NaN */
		}
	      else if (yisint == 1)
		z = -z;		/* (x<0)**odd = -(|x|**odd) */
	    }
	  return z;
	}
    }

  /* (x<0)**(non-int) is NaN */
  if ((((__uint32_t) hx >> 31) - 1 | yisint) == 0)
    return (x - x) / (x - x);

  s = one_p;			/* s (sign of result -ve**odd) = -1 else = 1 */
  if ((((__uint32_t) hx >> 31) - 1 | (yisint - 1)) == 0)
    s = -one_p;			/* (-ve)**(odd int) */

  /* |y| is huge */
  if (iy > 0x41e00000)
    {				/* if |y| > 2**31 */
      if (iy > 0x43f00000)
	{			/* if |y| > 2**64, must o/uflow */
	  if (ix <= 0x3fefffff)
	    return (hy < 0) ? huge_p * huge_p : tiny_p * tiny_p;
	  if (ix >= 0x3ff00000)
	    return (hy > 0) ? huge_p * huge_p : tiny_p * tiny_p;
	}
      /* over/underflow if x is not close to one */
      if (ix < 0x3fefffff)
	return (hy < 0) ? s * huge_p * huge_p : s * tiny_p * tiny_p;
      if (ix > 0x3ff00000)
	return (hy > 0) ? s * huge_p * huge_p : s * tiny_p * tiny_p;
      /* now |1-x| is tiny <= 2**-20, suffice to compute
         log(x) by x-x^2/2+x^3/3-x^4/4 */
      t = ax - one_p;		/* t has 20 trailing zeros */
      w = (t * t) * (0.5 - t * (0.3333333333333333333333 - t * 0.25));
      u = ivln2_h * t;		/* ivln2_h has 21 sig. bits */
      v = t * ivln2_l - w * ivln2;
      t1 = u + v;
      SET_LOW_WORD (t1, 0);
      t2 = v - (t1 - u);
    }
  else
    {
      double ss, s2, s_h, s_l, t_h, t_l;
      n = 0;
      /* take care subnormal number */
      if (ix < 0x00100000)
	{
	  ax *= two53;
	  n -= 53;
	  GET_HIGH_WORD (ix, ax);
	}
      n += ((ix) >> 20) - 0x3ff;
      j = ix & 0x000fffff;
      /* determine interval */
      ix = j | 0x3ff00000;	/* normalize ix */
      if (j <= 0x3988E)
	k = 0;			/* |x|<sqrt(3/2) */
      else if (j < 0xBB67A)
	k = 1;			/* |x|<sqrt(3)   */
      else
	{
	  k = 0;
	  n += 1;
	  ix -= 0x00100000;
	}
      SET_HIGH_WORD (ax, ix);

      /* compute ss = s_h+s_l = (x-1)/(x+1) or (x-1.5)/(x+1.5) */
      u = ax - bp[k];		/* bp[0]=1.0, bp[1]=1.5 */
      v = one_p / (ax + bp[k]);
      ss = u * v;
      s_h = ss;
      SET_LOW_WORD (s_h, 0);
      /* t_h=ax+bp[k] High */
      t_h = zero_p;
      SET_HIGH_WORD (t_h,
		     ((ix >> 1) | 0x20000000) + 0x00080000 + (k << 18));
      t_l = ax - (t_h - bp[k]);
      s_l = v * ((u - s_h * t_h) - s_h * t_l);
      /* compute log(ax) */
      s2 = ss * ss;
      r = s2 * s2 * (L1 + s2 * (L2 + s2 * (L3 + s2 * (L4 + s2 * (L5 +
								 s2 * L6)))));
      r += s_l * (s_h + ss);
      s2 = s_h * s_h;
      t_h = 3.0 + s2 + r;
      SET_LOW_WORD (t_h, 0);
      t_l = r - ((t_h - 3.0) - s2);
      /* u+v = ss*(1+...) */
      u = s_h * t_h;
      v = s_l * t_h + t_l * ss;
      /* 2/(3log2)*(ss+...) */
      p_h = u + v;
      SET_LOW_WORD (p_h, 0);
      p_l = v - (p_h - u);
      z_h = cp_h * p_h;		/* cp_h+cp_l = 2/(3*log2) */
      z_l = cp_l * p_h + p_l * cp + dp_l[k];
      /* log2(ax) = (ss+..)*2/(3*log2) = n + dp_h + z_h + z_l */
      t = (double) n;
      t1 = (((z_h + z_l) + dp_h[k]) + t);
      SET_LOW_WORD (t1, 0);
      t2 = z_l - (((t1 - t) - dp_h[k]) - z_h);
    }

  /* split up y into y1+y2 and compute (y1+y2)*(t1+t2) */
  y1 = y;
  SET_LOW_WORD (y1, 0);
  p_l = (y - y1) * t1 + y * t2;
  p_h = y1 * t1;
  z = p_l + p_h;
  EXTRACT_WORDS (j, i, z);
  if (j >= 0x40900000)
    {				/* z >= 1024 */
      if (((j - 0x40900000) | i) != 0)	/* if z > 1024 */
	return s * huge_p * huge_p;	/* overflow */
      else
	{
	  if (p_l + ovt > z - p_h)
	    return s * huge_p * huge_p;	/* overflow */
	}
    }
  else if ((j & 0x7fffffff) >= 0x4090cc00)
    {				/* z <= -1075 */
      if (((j - 0xc090cc00) | i) != 0)	/* z < -1075 */
	return s * tiny_p * tiny_p;	/* underflow */
      else
	{
	  if (p_l <= z - p_h)
	    return s * tiny_p * tiny_p;	/* underflow */
	}
    }
  /*
   * compute 2**(p_h+p_l)
   */
  i = j & 0x7fffffff;
  k = (i >> 20) - 0x3ff;
  n = 0;
  if (i > 0x3fe00000)
    {				/* if |z| > 0.5, set n = [z+0.5] */
      n = j + (0x00100000 >> (k + 1));
      k = ((n & 0x7fffffff) >> 20) - 0x3ff;	/* new k for n */
      t = zero_p;
      SET_HIGH_WORD (t, n & ~(0x000fffff >> k));
      n = ((n & 0x000fffff) | 0x00100000) >> (20 - k);
      if (j < 0)
	n = -n;
      p_h -= t;
    }
  t = p_l + p_h;
  SET_LOW_WORD (t, 0);
  u = t * lg2_h;
  v = (p_l - (t - p_h)) * lg2 + t * lg2_l;
  z = u + v;
  w = v - (z - u);
  t = z * z;
  t1 = z - t * (P1 + t * (P2 + t * (P3 + t * (P4 + t * P5))));
  r = (z * t1) / (t1 - two_p) - (w + z * w);
  z = one_p - (r - z);
  GET_HIGH_WORD (j, z);
  j += (n << 20);
  if ((j >> 20) <= 0)
    z = scalbn (z, n);		/* subnormal output */
  else
    SET_HIGH_WORD (z, j);
  return s * z;
}

/* ------------------------------------------------------------------ */
/* _IEEE_LIBM wrappers                                                */
/* ------------------------------------------------------------------ */

extern float __ieee754_asinf (float);
extern float __ieee754_atan2f (float, float);
extern float __ieee754_fmodf (float, float);

/* acos(x) */
double
acos (double x)
{
  return __ieee754_acos (x);
}

/* atan2(y,x) */
double
atan2 (double y, double x)
{
  return __ieee754_atan2 (y, x);
}

/* fmod(x,y) */
double
fmod (double x, double y)
{
  return __ieee754_fmod (x, y);
}

/* pow(x,y) */
double
pow (double x, double y)
{
  return __ieee754_pow (x, y);
}

/* sqrt(x) */
double
sqrt (double x)
{
  return __ieee754_sqrt (x);
}

/* asinf(x) */
float
asinf (float x)
{
  return __ieee754_asinf (x);
}

/* atan2f(y,x) */
float
atan2f (float y, float x)
{
  return __ieee754_atan2f (y, x);
}

/* fmodf(x,y) */
float
fmodf (float x, float y)
{
  return __ieee754_fmodf (x, y);
}

/* sqrtf(x) */
float
sqrtf (float x)
{
  return __ieee754_sqrtf (x);
}

/* ------------------------------------------------------------------ */
/* k_sinf.c                                                           */
/* ------------------------------------------------------------------ */

static const float
  half_kf = 5.0000001490e-01,	/* 0x3f000000 */
  S1f = -1.6666667536e-01,	/* 0xbe2aaaab */
  S2f = 8.3333340008e-03,	/* 0x3c088889 */
  S3f = -1.9841270478e-04,	/* 0xb9500d01 */
  S4f = 2.7557314866e-06,	/* 0x3638ef1b */
  S5f = -2.5050760133e-08,	/* 0xb2d72f34 */
  S6f = 1.5896910524e-10;	/* 0x2f2ec9d3 */

float
__kernel_sinf (float x, float y, int iy)
{
  float z, r, v;
  __int32_t ix;
  GET_FLOAT_WORD (ix, x);
  ix &= 0x7fffffff;		/* high word of x */
  if (ix < 0x32000000)		/* |x| < 2**-27 */
    {
      if ((int) x == 0)
	return x;
    }				/* generate inexact */
  z = x * x;
  v = z * x;
  r = S2f + z * (S3f + z * (S4f + z * (S5f + z * S6f)));
  if (iy == 0)
    return x + v * (S1f + z * r);
  else
    return x - ((z * (half_kf * y - v * r) - y) - v * S1f);
}

/* ------------------------------------------------------------------ */
/* k_cosf.c                                                           */
/* ------------------------------------------------------------------ */

static const float
  one_cf = 1.0,
  C1f = 4.1666668840e-02,	/* 0x3d2aaaab */
  C2f = -1.3888889516e-03,	/* 0xbab60b61 */
  C3f = 2.4801588097e-05,	/* 0x37d00d01 */
  C4f = -2.7557315008e-07,	/* 0xb493f27c */
  C5f = 2.0875723927e-09,	/* 0x310f74f6 */
  C6f = -1.1359647814e-11;	/* 0xad47d74e */

float
__kernel_cosf (float x, float y)
{
  float a, hz, z, r, qx;
  __int32_t ix;
  GET_FLOAT_WORD (ix, x);
  ix &= 0x7fffffff;		/* ix = |x|'s high word */
  if (ix < 0x32000000)
    {				/* if x < 2**27 */
      if (((int) x) == 0)
	return one_cf;		/* generate inexact */
    }
  z = x * x;
  r = z * (C1f + z * (C2f + z * (C3f + z * (C4f + z * (C5f + z * C6f)))));
  if (ix < 0x3e99999a)		/* if |x| < 0.3 */
    return one_cf - ((float) 0.5 * z - (z * r - x * y));
  else
    {
      if (ix > 0x3f480000)
	{			/* x > 0.78125 */
	  qx = (float) 0.28125;
	}
      else
	{
	  SET_FLOAT_WORD (qx, ix - 0x01000000);	/* x/4 */
	}
      hz = (float) 0.5 * z - qx;
      a = one_cf - qx;
      return a - (hz - (z * r - x * y));
    }
}

/* ------------------------------------------------------------------ */
/* k_tanf.c                                                           */
/* ------------------------------------------------------------------ */

static const float
  one_t = 1.0,
  pio4_t = 7.8539814055e-01,	/* 0x3f490fda */
  pio4lo_t = 3.7748947967e-08,	/* 0x33222168 */
  Tt[] = {
  3.3333335072e-01,		/* 0x3eaaaaab */
  1.3333334401e-01,		/* 0x3e088889 */
  5.3968255408e-02,		/* 0x3d5d0dd1 */
  2.1869488526e-02,		/* 0x3cb327a4 */
  8.8632397819e-03,		/* 0x3c11371f */
  3.5920790979e-03,		/* 0x3b6b6916 */
  1.4562094875e-03,		/* 0x3abede48 */
  5.8804127912e-04,		/* 0x3a1a26c8 */
  2.4646314705e-04,		/* 0x398137b9 */
  7.8179446064e-05,		/* 0x38a3f445 */
  7.1407253927e-05,		/* 0x3895c07a */
  -1.8558638203e-05,		/* 0xb79bae5f */
  2.5907306281e-05,		/* 0x37d95384 */
};

float
__kernel_tanf (float x, float y, int iy)
{
  float z, r, v, w, s;
  __int32_t ix, hx;
  GET_FLOAT_WORD (hx, x);
  ix = hx & 0x7fffffff;		/* high word of |x| */
  if (ix < 0x31800000)		/* x < 2**-28 */
    {
      if ((int) x == 0)
	{			/* generate inexact */
	  if ((ix | (iy + 1)) == 0)
	    return one_t / fabsf (x);
	  else
	    return (iy == 1) ? x : -one_t / x;
	}
    }
  if (ix >= 0x3f2ca140)
    {				/* |x|>=0.6744 */
      if (hx < 0)
	{
	  x = -x;
	  y = -y;
	}
      z = pio4_t - x;
      w = pio4lo_t - y;
      x = z + w;
      y = 0.0;
    }
  z = x * x;
  w = z * z;
  /* Break x^5*(T[1]+x^2*T[2]+...) into
   *   x^5(T[1]+x^4*T[3]+...+x^20*T[11]) +
   *   x^5(x^2*(T[2]+x^4*T[4]+...+x^22*[T12]))
   */
  r = Tt[1] + w * (Tt[3] + w * (Tt[5] + w * (Tt[7] + w * (Tt[9]
							  + w * Tt[11]))));
  v = z * (Tt[2] + w * (Tt[4] + w * (Tt[6] + w * (Tt[8] + w * (Tt[10]
							       +
							       w *
							       Tt[12])))));
  s = z * x;
  r = y + z * (s * (r + v) + y);
  r += Tt[0] * s;
  w = x + r;
  if (ix >= 0x3f2ca140)
    {
      v = (float) iy;
      return (float) (1 - ((hx >> 30) & 2)) * (v - (float) 2.0 * (x -
								 (w * w /
								  (w + v) -
								  r)));
    }
  if (iy == 1)
    return w;
  else
    {				/* if allow error up to 2 ulp,
				   simply return -1.0/(x+r) here */
      /*  compute -1.0/(x+r) accurately */
      float a, t;
      __int32_t i;
      z = w;
      GET_FLOAT_WORD (i, z);
      SET_FLOAT_WORD (z, i & 0xfffff000);
      v = r - (z - x);		/* z+v = r+x */
      t = a = -(float) 1.0 / w;	/* a = -1.0/w */
      GET_FLOAT_WORD (i, t);
      SET_FLOAT_WORD (t, i & 0xfffff000);
      s = (float) 1.0 + t * z;
      return t + a * (s + t * v);
    }
}
