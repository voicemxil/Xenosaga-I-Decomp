/*
 * Soft-float double-precision arithmetic library, adapted from GCC's
 * classic gcc/config/fp-bit.c (gcc-3.4.0 tag; the file was removed from
 * later gcc releases). The R5900 has no double-precision FPU, so every
 * `double` operation in the original game goes through a renamed copy of
 * this file: DOUBLE config (FLOAT undefined) + US_SOFTWARE_GOFAST, which
 * is exactly the macro configuration whose entry-point names match the
 * disassembly: dpadd, dpsub, dpmul, dpdiv, dpcmp, litodp, dptoli, dptofp,
 * fptodp, __unpack_d, __pack_d, _fpadd_parts, __fpcmp_parts_d.
 *
 * Kept as close to the real source's shape as possible -- this is not a
 * fresh reimplementation, it's the original file with its macros resolved
 * for this one configuration.
 */

typedef int SItype;
typedef unsigned int USItype;
typedef short HItype;
typedef unsigned short UHItype;
typedef long long DItype;
typedef unsigned long long UDItype;
typedef float SFtype;
typedef double DFtype;
typedef int CMPtype;

#define MAX_USI_INT  (~(USItype)0)
#define MAX_SI_INT   ((SItype) (MAX_USI_INT >> 1))
#define BITS_PER_SI  32

/* DOUBLE configuration (FLOAT undefined). */
#define NGARDS    8L
#define GARDROUND 0x7f
#define GARDMASK  0xff
#define GARDMSB   0x80
#define EXPBITS 11
#define EXPBIAS 1023
#define FRACBITS 52
#define EXPMAX (0x7ff)
#define QUIET_NAN 0x8000000000000LL
#define FRAC_NBITS 64
#define FRACHIGH  0x8000000000000000LL
#define FRACHIGH2 0xc000000000000000LL

typedef UDItype fractype;
typedef USItype halffractype;
typedef DFtype FLO_type;
typedef DItype intfrac;

#define pack_d __pack_d
#define unpack_d __unpack_d
#define __fpcmp_parts __fpcmp_parts_d

/* US_SOFTWARE_GOFAST, DOUBLE branch renames. */
#define add 		dpadd
#define sub 		dpsub
#define multiply 	dpmul
#define divide 		dpdiv
#define compare 		dpcmp
#define si_to_float 	litodp
#define float_to_si 	dptoli
#define df_to_sf 	dptofp

/* Not target'd by any of the 13 functions here, but the real macro table
   has them and other TUs may need them later. */
#define usi_to_float 	__floatunsidf
#define float_to_usi 	dptoul
#define negate 		__negdf2

/* These come from real.h/tm.h in a real GCC build; MIPS defines neither
   specially, so both take the standard IEEE-754 defaults. */
#define LARGEST_EXPONENT_IS_NORMAL(x) 0
#define ROUND_TOWARDS_ZERO 0

/* Confirmed by disassembly: __unpack_d treats any exp==0 encoding as a
   flat zero (no shift-to-normalize loop), and __pack_d saturates a too-
   small exponent straight to a zero encoding (no shift-into-denormal
   loop) -- both are exactly what NO_DENORMALS does to fp-bit.c's
   #ifdef/#else blocks. Sony built this file without denormal support. */
#define NO_DENORMALS 1

/* __pack_d's repeated load-mask-or-store triples (a2 &= ~fieldmask;
   a2 |= fieldbits; ... x3, once per field) are the bitfield-store
   codegen shape, not the plain-shift dst.value_raw |= ... form -- so
   Sony's target tm.h defines this. */
#define FLOAT_BIT_ORDER_MISMATCH 1

#define INLINE __inline__

/* Preserve the sticky-bit when shifting fractions to the right.  */
#define LSHIFT(a) { a = (a & 1) | (a >> 1); }

#define NORMAL_EXPMIN (-(EXPBIAS)+1)
#define IMPLICIT_1 ((fractype)1<<(FRACBITS+NGARDS))
#define IMPLICIT_2 ((fractype)1<<(FRACBITS+1+NGARDS))

/* F_D_BITOFF is the number of bits offset between the MSB of the mantissa
   of a float and of a double (assumes only two float types). */
#define F_D_BITOFF (52+8-(23+7))

typedef enum
{
  CLASS_SNAN,
  CLASS_QNAN,
  CLASS_ZERO,
  CLASS_NUMBER,
  CLASS_INFINITY
} fp_class_type;

typedef struct
{
  fp_class_type class;
  unsigned int sign;
  int normal_exp;

  union
    {
      fractype ll;
      halffractype l[2];
    } fraction;
} fp_number_type;

typedef union
{
  FLO_type value;
  fractype value_raw;
  halffractype words[2];

#ifdef FLOAT_BIT_ORDER_MISMATCH
  struct
    {
      fractype fraction:FRACBITS __attribute__ ((packed));
      unsigned int exp:EXPBITS __attribute__ ((packed));
      unsigned int sign:1 __attribute__ ((packed));
    }
  bits;
#endif
}
FLO_union_type;

const fp_number_type __thenan_df = { CLASS_SNAN, 0, 0, {(fractype) 0} };

INLINE
static fp_number_type *
nan (void)
{
  return (fp_number_type *) (& __thenan_df);
}

INLINE
static int
isnan ( fp_number_type *  x)
{
  return x->class == CLASS_SNAN || x->class == CLASS_QNAN;
}

INLINE
static int
isinf ( fp_number_type *  x)
{
  return x->class == CLASS_INFINITY;
}

INLINE
static int
iszero ( fp_number_type *  x)
{
  return x->class == CLASS_ZERO;
}

extern FLO_type pack_d ( fp_number_type * );

/* __pack_d, 280 bytes. */
FLO_type
pack_d ( fp_number_type *  src)
{
  FLO_union_type dst;
  fractype fraction = src->fraction.ll;
  int sign = src->sign;
  int exp = 0;

  if (LARGEST_EXPONENT_IS_NORMAL (FRAC_NBITS) && (isnan (src) || isinf (src)))
    {
      exp = EXPMAX;
      fraction = ((fractype) 1 << FRACBITS) - 1;
    }
  else if (isnan (src))
    {
      exp = EXPMAX;
      if (src->class == CLASS_QNAN || 1)
	{
	  fraction |= QUIET_NAN;
	}
    }
  else if (isinf (src))
    {
      exp = EXPMAX;
      fraction = 0;
    }
  else if (iszero (src))
    {
      exp = 0;
      fraction = 0;
    }
  else if (fraction == 0)
    {
      exp = 0;
    }
  else
    {
      if (src->normal_exp < NORMAL_EXPMIN)
	{
#ifdef NO_DENORMALS
	  /* Go straight to a zero representation if denormals are not
	     supported.  The denormal handling would be harmless but
	     isn't unnecessary.  */
	  exp = 0;
	  fraction = 0;
#else /* NO_DENORMALS */
	  /* This number's exponent is too low to fit into the bits
	     available in the number, so we'll store 0 in the exponent and
	     shift the fraction to the right to make up for it.  */

	  int shift = NORMAL_EXPMIN - src->normal_exp;

	  exp = 0;

	  if (shift > FRAC_NBITS - NGARDS)
	    {
	      fraction = 0;
	    }
	  else
	    {
	      int lowbit = (fraction & (((fractype)1 << shift) - 1)) ? 1 : 0;
	      fraction = (fraction >> shift) | lowbit;
	    }
	  if ((fraction & GARDMASK) == GARDMSB)
	    {
	      if ((fraction & (1 << NGARDS)))
		fraction += GARDROUND + 1;
	    }
	  else
	    {
	      fraction += GARDROUND;
	    }
	  if (fraction >= IMPLICIT_1)
	    {
	      exp += 1;
	    }
	  fraction >>= NGARDS;
#endif /* NO_DENORMALS */
	}
      else if (!LARGEST_EXPONENT_IS_NORMAL (FRAC_NBITS)
	       && src->normal_exp > EXPBIAS)
	{
	  exp = EXPMAX;
	  fraction = 0;
	}
      else
	{
	  exp = src->normal_exp + EXPBIAS;
	  if (!ROUND_TOWARDS_ZERO)
	    {
	      /* IF the gard bits are the all zero, but the first, then we're
		 half way between two numbers, choose the one which makes the
		 lsb of the answer 0.  */
	      if ((fraction & GARDMASK) == GARDMSB)
		{
		  if (fraction & (1 << NGARDS))
		    fraction += GARDROUND + 1;
		}
	      else
		{
		  fraction += GARDROUND;
		}
	      if (fraction >= IMPLICIT_2)
		{
		  fraction >>= 1;
		  exp += 1;
		}
	    }
	  fraction >>= NGARDS;

	  if (LARGEST_EXPONENT_IS_NORMAL (FRAC_NBITS) && exp > EXPMAX)
	    {
	      exp = EXPMAX;
	      fraction = ((fractype) 1 << FRACBITS) - 1;
	    }
	}
    }

#ifdef FLOAT_BIT_ORDER_MISMATCH
  dst.bits.fraction = fraction;
  dst.bits.exp = exp;
  dst.bits.sign = sign;
#else
  dst.value_raw = fraction & ((((fractype)1) << FRACBITS) - (fractype)1);
  dst.value_raw |= ((fractype) (exp & ((1 << EXPBITS) - 1))) << FRACBITS;
  dst.value_raw |= ((fractype) (sign & 1)) << (FRACBITS | EXPBITS);
#endif

  return dst.value;
}

/* __unpack_d, 172 bytes. */
void
unpack_d (FLO_union_type * src, fp_number_type * dst)
{
  fractype fraction;
  int exp;
  int sign;

  fraction = src->value_raw & ((((fractype)1) << FRACBITS) - 1);
  exp = ((int)(src->value_raw >> FRACBITS)) & ((1 << EXPBITS) - 1);
  sign = ((int)(src->value_raw >> (FRACBITS + EXPBITS))) & 1;

  dst->sign = sign;
  if (exp == 0)
    {
      if (fraction == 0
#ifdef NO_DENORMALS
	  || 1
#endif
	  )
	{
	  dst->class = CLASS_ZERO;
	}
      else
	{
	  /* Zero exponent with nonzero fraction - it's denormalized,
	     so there isn't a leading implicit one - we'll shift it so
	     it gets one.  */
	  dst->normal_exp = exp - EXPBIAS + 1;
	  fraction <<= NGARDS;

	  dst->class = CLASS_NUMBER;
	  while (fraction < IMPLICIT_1)
	    {
	      fraction <<= 1;
	      dst->normal_exp--;
	    }
	  dst->fraction.ll = fraction;
	}
    }
  else if (!LARGEST_EXPONENT_IS_NORMAL (FRAC_NBITS) && exp == EXPMAX)
    {
      /* Huge exponent*/
      if (fraction == 0)
	{
	  dst->class = CLASS_INFINITY;
	}
      else
	{
	  if (fraction & QUIET_NAN)
	    {
	      dst->class = CLASS_QNAN;
	    }
	  else
	    {
	      dst->class = CLASS_SNAN;
	    }
	  dst->fraction.ll = fraction;
	}
    }
  else
    {
      /* Nothing strange about this number */
      dst->normal_exp = exp - EXPBIAS;
      dst->class = CLASS_NUMBER;
      dst->fraction.ll = (fraction << NGARDS) | IMPLICIT_1;
    }
}

/* _fpadd_parts, 604 bytes. */
static fp_number_type *
_fpadd_parts (fp_number_type * a,
	      fp_number_type * b,
	      fp_number_type * tmp)
{
  intfrac tfraction;

  int a_normal_exp;
  int b_normal_exp;
  fractype a_fraction;
  fractype b_fraction;

  if (isnan (a))
    {
      return a;
    }
  if (isnan (b))
    {
      return b;
    }
  if (isinf (a))
    {
      /* Adding infinities with opposite signs yields a NaN.  */
      if (isinf (b) && a->sign != b->sign)
	return nan ();
      return a;
    }
  if (isinf (b))
    {
      return b;
    }
  if (iszero (b))
    {
      if (iszero (a))
	{
	  *tmp = *a;
	  tmp->sign = a->sign & b->sign;
	  return tmp;
	}
      return a;
    }
  if (iszero (a))
    {
      return b;
    }

  /* Got two numbers. shift the smaller and increment the exponent till
     they're the same */
  {
    int diff;

    a_normal_exp = a->normal_exp;
    b_normal_exp = b->normal_exp;
    a_fraction = a->fraction.ll;
    b_fraction = b->fraction.ll;

    diff = a_normal_exp - b_normal_exp;

    if (diff < 0)
      diff = -diff;
    if (diff < FRAC_NBITS)
      {
	while (a_normal_exp > b_normal_exp)
	  {
	    b_normal_exp++;
	    LSHIFT (b_fraction);
	  }
	while (b_normal_exp > a_normal_exp)
	  {
	    a_normal_exp++;
	    LSHIFT (a_fraction);
	  }
      }
    else
      {
	/* Somethings's up.. choose the biggest */
	if (a_normal_exp > b_normal_exp)
	  {
	    b_normal_exp = a_normal_exp;
	    b_fraction = 0;
	  }
	else
	  {
	    a_normal_exp = b_normal_exp;
	    a_fraction = 0;
	  }
      }
  }

  if (a->sign != b->sign)
    {
      if (a->sign)
	{
	  tfraction = -a_fraction + b_fraction;
	}
      else
	{
	  tfraction = a_fraction - b_fraction;
	}
      if (tfraction >= 0)
	{
	  tmp->sign = 0;
	  tmp->normal_exp = a_normal_exp;
	  tmp->fraction.ll = tfraction;
	}
      else
	{
	  tmp->sign = 1;
	  tmp->normal_exp = a_normal_exp;
	  tmp->fraction.ll = -tfraction;
	}
      /* and renormalize it */

      while (tmp->fraction.ll < IMPLICIT_1 && tmp->fraction.ll)
	{
	  tmp->fraction.ll <<= 1;
	  tmp->normal_exp--;
	}
    }
  else
    {
      tmp->sign = a->sign;
      tmp->normal_exp = a_normal_exp;
      tmp->fraction.ll = a_fraction + b_fraction;
    }
  tmp->class = CLASS_NUMBER;
  /* Now the fraction is added, we have to shift down to renormalize the
     number */

  if (tmp->fraction.ll >= IMPLICIT_2)
    {
      LSHIFT (tmp->fraction.ll);
      tmp->normal_exp++;
    }
  return tmp;
}

/* dpadd, 88 bytes. */
FLO_type
add (FLO_type arg_a, FLO_type arg_b)
{
  fp_number_type a;
  fp_number_type b;
  fp_number_type tmp;
  fp_number_type *res;
  FLO_union_type au, bu;

  au.value = arg_a;
  bu.value = arg_b;

  unpack_d (&au, &a);
  unpack_d (&bu, &b);

  res = _fpadd_parts (&a, &b, &tmp);

  return pack_d (res);
}

/* dpsub, 100 bytes. */
FLO_type
sub (FLO_type arg_a, FLO_type arg_b)
{
  fp_number_type a;
  fp_number_type b;
  fp_number_type tmp;
  fp_number_type *res;
  FLO_union_type au, bu;

  au.value = arg_a;
  bu.value = arg_b;

  unpack_d (&au, &a);
  unpack_d (&bu, &b);

  b.sign ^= 1;

  res = _fpadd_parts (&a, &b, &tmp);

  return pack_d (res);
}

static inline __attribute__ ((__always_inline__)) fp_number_type *
_fpmul_parts ( fp_number_type *  a,
	       fp_number_type *  b,
	       fp_number_type * tmp)
{
  fractype low = 0;
  fractype high = 0;

  if (isnan (a))
    {
      a->sign = a->sign != b->sign;
      return a;
    }
  if (isnan (b))
    {
      b->sign = a->sign != b->sign;
      return b;
    }
  if (isinf (a))
    {
      if (iszero (b))
	return nan ();
      a->sign = a->sign != b->sign;
      return a;
    }
  if (isinf (b))
    {
      if (iszero (a))
	{
	  return nan ();
	}
      b->sign = a->sign != b->sign;
      return b;
    }
  if (iszero (a))
    {
      a->sign = a->sign != b->sign;
      return a;
    }
  if (iszero (b))
    {
      b->sign = a->sign != b->sign;
      return b;
    }

  /* Calculate the mantissa by multiplying both numbers to get a
     twice-as-wide number.  */
  {
    /* fractype is DImode, but we need the result to be twice as wide.
       Assuming a widening multiply from DImode to TImode is not
       available, build one by hand.  */
    /* Written as UDItype locals (rather than USItype, per the letter of
       the real source) because this ee-gcc build recognizes the narrower
       (UDItype)(USItype)x * y shape as a WIDEN_MULT_EXPR and lowers it to
       a native 32x32->64 multu -- but the original object calls __muldi3
       for each partial product (confirmed by disassembly: 4 jal __muldi3
       for pp_ll/pp_hl/pp_lh/pp_hh), meaning Sony's actual build did not
       fold this. The R5900 has no native 64x64 multiply hardware either
       way; this shape just avoids the narrowing optimization so both
       multiplicands reach the multiply as genuine UDItype operands.  */
    UDItype nl = a->fraction.ll & 0xffffffffULL;
    UDItype nh = a->fraction.ll >> BITS_PER_SI;
    UDItype ml = b->fraction.ll & 0xffffffffULL;
    UDItype mh = b->fraction.ll >> BITS_PER_SI;
    UDItype pp_ll = ml * nl;
    UDItype pp_hl = mh * nl;
    UDItype pp_lh = ml * nh;
    UDItype pp_hh = mh * nh;
    UDItype res2 = 0;
    UDItype res0 = 0;
    UDItype ps_hh__ = pp_hl + pp_lh;
    if (ps_hh__ < pp_hl)
      res2 += (UDItype)1 << BITS_PER_SI;
    pp_hl = (UDItype)(USItype)ps_hh__ << BITS_PER_SI;
    res0 = pp_ll + pp_hl;
    if (res0 < pp_ll)
      res2++;
    res2 += (ps_hh__ >> BITS_PER_SI) + pp_hh;
    high = res2;
    low = res0;
  }

  tmp->normal_exp = a->normal_exp + b->normal_exp
    + FRAC_NBITS - (FRACBITS + NGARDS);
  tmp->sign = a->sign != b->sign;
  while (high >= IMPLICIT_2)
    {
      tmp->normal_exp++;
      if (high & 1)
	{
	  low >>= 1;
	  low |= FRACHIGH;
	}
      high >>= 1;
    }
  while (high < IMPLICIT_1)
    {
      tmp->normal_exp--;

      high <<= 1;
      if (low & FRACHIGH)
	high |= 1;
      low <<= 1;
    }
  /* rounding is tricky. if we only round if it won't make us round later.  */
  if (!ROUND_TOWARDS_ZERO && (high & GARDMASK) == GARDMSB)
    {
      if (high & (1 << NGARDS))
	{
	  /* half way, so round to even */
	  high += GARDROUND + 1;
	}
      else if (low)
	{
	  /* but we really weren't half way */
	  high += GARDROUND + 1;
	}
    }
  tmp->fraction.ll = high;
  tmp->class = CLASS_NUMBER;
  return tmp;
}

/* dpmul, 684 bytes. */
FLO_type
multiply (FLO_type arg_a, FLO_type arg_b)
{
  fp_number_type a;
  fp_number_type b;
  fp_number_type tmp;
  fp_number_type *res;
  FLO_union_type au, bu;

  au.value = arg_a;
  bu.value = arg_b;

  unpack_d (&au, &a);
  unpack_d (&bu, &b);

  res = _fpmul_parts (&a, &b, &tmp);

  return pack_d (res);
}

static inline __attribute__ ((__always_inline__)) fp_number_type *
_fpdiv_parts (fp_number_type * a,
	      fp_number_type * b)
{
  fractype bit;
  fractype numerator;
  fractype denominator;
  fractype quotient;

  if (isnan (a))
    {
      return a;
    }
  if (isnan (b))
    {
      return b;
    }

  a->sign = a->sign ^ b->sign;

  if (isinf (a) || iszero (a))
    {
      if (a->class == b->class)
	return nan ();
      return a;
    }

  if (isinf (b))
    {
      a->fraction.ll = 0;
      a->normal_exp = 0;
      return a;
    }
  if (iszero (b))
    {
      a->class = CLASS_INFINITY;
      return a;
    }

  /* Calculate the mantissa by multiplying both 64bit numbers to get a
     128 bit number */
  {
    /* quotient =
       ( numerator / denominator) * 2^(numerator exponent -  denominator exponent)
     */

    a->normal_exp = a->normal_exp - b->normal_exp;
    numerator = a->fraction.ll;
    denominator = b->fraction.ll;

    if (numerator < denominator)
      {
	/* Fraction will be less than 1.0 */
	numerator *= 2;
	a->normal_exp--;
      }
    bit = IMPLICIT_1;
    quotient = 0;
    /* ??? Does divide one bit at a time.  Optimize.  */
    while (bit)
      {
	if (numerator >= denominator)
	  {
	    quotient |= bit;
	    numerator -= denominator;
	  }
	bit >>= 1;
	numerator *= 2;
      }

    if (!ROUND_TOWARDS_ZERO && (quotient & GARDMASK) == GARDMSB)
      {
	if (quotient & (1 << NGARDS))
	  {
	    /* half way, so round to even */
	    quotient += GARDROUND + 1;
	  }
	else if (numerator)
	  {
	    /* but we really weren't half way, more bits exist */
	    quotient += GARDROUND + 1;
	  }
      }

    a->fraction.ll = quotient;
    return (a);
  }
}

/* dpdiv, 372 bytes. */
FLO_type
divide (FLO_type arg_a, FLO_type arg_b)
{
  fp_number_type a;
  fp_number_type b;
  fp_number_type *res;
  FLO_union_type au, bu;

  au.value = arg_a;
  bu.value = arg_b;

  unpack_d (&au, &a);
  unpack_d (&bu, &b);

  res = _fpdiv_parts (&a, &b);

  return pack_d (res);
}

/* __fpcmp_parts_d, 288 bytes.
   according to the demo, fpcmp returns a comparison with 0... thus
   a<b -> -1
   a==b -> 0
   a>b -> +1
 */
int
__fpcmp_parts (fp_number_type * a, fp_number_type * b)
{
  if (isnan (a) || isnan (b))
    {
      return 1;			/* how to indicate unordered compare? */
    }
  if (isinf (a) && isinf (b))
    {
      /* +inf > -inf, but +inf != +inf */
      return b->sign - a->sign;
    }
  /* but not both...  */
  if (isinf (a))
    {
      return a->sign ? -1 : 1;
    }
  if (isinf (b))
    {
      return b->sign ? 1 : -1;
    }
  if (iszero (a) && iszero (b))
    {
      return 0;
    }
  if (iszero (a))
    {
      return b->sign ? 1 : -1;
    }
  if (iszero (b))
    {
      return a->sign ? -1 : 1;
    }
  /* now both are "normal".  */
  if (a->sign != b->sign)
    {
      /* opposite signs */
      return a->sign ? -1 : 1;
    }
  /* same sign; exponents? */
  if (a->normal_exp > b->normal_exp)
    {
      return a->sign ? -1 : 1;
    }
  if (a->normal_exp < b->normal_exp)
    {
      return a->sign ? 1 : -1;
    }
  /* same exponents; check size.  */
  if (a->fraction.ll > b->fraction.ll)
    {
      return a->sign ? -1 : 1;
    }
  if (a->fraction.ll < b->fraction.ll)
    {
      return a->sign ? 1 : -1;
    }
  /* after all that, they're equal.  */
  return 0;
}

/* dpcmp, 76 bytes. */
CMPtype
compare (FLO_type arg_a, FLO_type arg_b)
{
  fp_number_type a;
  fp_number_type b;
  FLO_union_type au, bu;

  au.value = arg_a;
  bu.value = arg_b;

  unpack_d (&au, &a);
  unpack_d (&bu, &b);

  return __fpcmp_parts (&a, &b);
}

/* litodp, 184 bytes. */
FLO_type
si_to_float (SItype arg_a)
{
  fp_number_type in;

  in.class = CLASS_NUMBER;
  in.sign = arg_a < 0;
  if (!arg_a)
    {
      in.class = CLASS_ZERO;
    }
  else
    {
      in.normal_exp = FRACBITS + NGARDS;
      if (in.sign)
	{
	  /* Special case for minint, since there is no +ve integer
	     representation for it */
	  if (arg_a == (- MAX_SI_INT - 1))
	    {
	      return (FLO_type)(- MAX_SI_INT - 1);
	    }
	  in.fraction.ll = (-arg_a);
	}
      else
	in.fraction.ll = arg_a;

      while (in.fraction.ll < ((fractype)1 << (FRACBITS + NGARDS)))
	{
	  in.fraction.ll <<= 1;
	  in.normal_exp -= 1;
	}
    }
  return pack_d (&in);
}

/* dptoli, 156 bytes. */
SItype
float_to_si (FLO_type arg_a)
{
  fp_number_type a;
  SItype tmp;
  FLO_union_type au;

  au.value = arg_a;
  unpack_d (&au, &a);

  if (iszero (&a))
    return 0;
  if (isnan (&a))
    return 0;
  /* get reasonable MAX_SI_INT...  */
  if (isinf (&a))
    return a.sign ? (-MAX_SI_INT)-1 : MAX_SI_INT;
  /* it is a number, but a small one */
  if (a.normal_exp < 0)
    return 0;
  if (a.normal_exp > BITS_PER_SI - 2)
    return a.sign ? (-MAX_SI_INT)-1 : MAX_SI_INT;
  tmp = a.fraction.ll >> ((FRACBITS + NGARDS) - a.normal_exp);
  return a.sign ? (-tmp) : (tmp);
}

/* dptofp, 84 bytes -- df_to_sf. __make_fp lives in the FLOAT-config copy
   of this file, not implemented here; declare it extern like the real
   fp-bit.c does across its two TUs. */
extern SFtype __make_fp (fp_class_type, unsigned int, int, USItype);

SFtype
df_to_sf (DFtype arg_a)
{
  fp_number_type in;
  USItype sffrac;
  FLO_union_type au;

  au.value = arg_a;
  unpack_d (&au, &in);

  sffrac = in.fraction.ll >> F_D_BITOFF;

  /* We set the lowest guard bit in SFFRAC if we discarded any non
     zero bits.  */
  if ((in.fraction.ll & (((USItype) 1 << F_D_BITOFF) - 1)) != 0)
    sffrac |= 1;

  return __make_fp (in.class, in.sign, in.normal_exp, sffrac);
}

/* fptodp, 60 bytes -- sf_to_df from the FLOAT-config copy of this file.
   __unpack_f is that copy's unpack_d (not implemented here); it fills a
   fp_number_type shaped for the FLOAT config, where fractype is USItype
   (32 bits), not this file's UDItype -- so its fraction union needs only
   4-byte alignment and sits at offset 12, not 16. Using this file's own
   fp_number_type here (64-bit fraction, padded to offset 16) gave a
   16-byte-oversized frame and a wrong offset in the disassembly; this
   private, FLOAT-shaped struct matches the real callee's layout.
   __make_dp is this copy's constructor and takes the widened UDItype
   fraction, so it's still declared against the normal fp_class_type. */
typedef struct
{
  fp_class_type class;
  unsigned int sign;
  int normal_exp;

  union
    {
      USItype ll;
      UHItype l[2];
    } fraction;
} fp_number_type_sf;

typedef union { SFtype value; USItype value_raw; } SFLO_union_type;
extern void __unpack_f (SFLO_union_type *, fp_number_type_sf *);
extern DFtype __make_dp (fp_class_type, unsigned int, int, UDItype);

DFtype
fptodp (SFtype arg_a)
{
  fp_number_type_sf in;
  SFLO_union_type au;

  au.value = arg_a;
  __unpack_f (&au, &in);

  return __make_dp (in.class, in.sign, in.normal_exp,
		    ((UDItype) in.fraction.ll) << F_D_BITOFF);
}
