/*
 * gcc config/fp-bit.c at CVS rev ec6bfc9b7ca (1999-12-29), DOUBLE/GOFAST
 * config -- the R5900 has no double-precision FPU, so every `double`
 * operation in the game goes through this file.
 *
 * This TU replaces the old hand-transcribed src/libgcc/fpbit.c: every
 * one of its twelve byte-exact functions is reproduced from the vendored
 * original, and dpmul -- which was a 117-diff wall in the transcription
 * -- is byte-exact here.  Hand-transcription was the whole problem: the
 * copy was made from the gcc-3.4.0 fp-bit.c, whose multiply had been
 * rewritten since 1999.
 *
 * Sony's build diverges from the vendored file in exactly two functions,
 * so FINE_GRAINED_LIBRARIES excludes L_pack_df/L_unpack_df from the
 * include and this wrapper carries the target's pack_d/unpack_d -- the
 * same arrangement fpbit_sf.c uses for the float pair, and for the same
 * reason:
 *   - FLOAT_BIT_ORDER_MISMATCH is on (pack_d stores through the bitfield
 *     union member; __pack_d's three load-mask-or-store triples in the
 *     disassembly are the bitfield read-modify-write sequence, not the
 *     plain-shift dst.value_raw |= ... form).
 *   - no denormal support: unpack_d classes any exp==0 input as ZERO and
 *     pack_d flushes a below-NORMAL_EXPMIN result straight to a zero
 *     encoding instead of shifting the fraction into a denormal.  That is
 *     exactly what later gcc's NO_DENORMALS does to these two #ifdef
 *     blocks; the 1999 fp-bit.c predates the macro, so the two functions
 *     are carried here with the denormal arms removed.  Unlike the float
 *     pair, the double pair keeps its nan/inf handling.
 */
#define US_SOFTWARE_GOFAST
#define FLOAT_BIT_ORDER_MISMATCH

#define FINE_GRAINED_LIBRARIES
#define L_addsub_df
#define L_mul_df
#define L_div_df
#define L_fpcmp_parts_df
#define L_compare_df
#define L_si_to_df
#define L_df_to_si
#define L_df_to_usi
#define L_make_df
#define L_df_to_sf
#define L_thenan_df
#include "fp-bit.c"

/* L_unpack_df, with the NO_DENORMALS arm of the exp==0 case taken. */
void
unpack_d (FLO_union_type * src, fp_number_type * dst)
{
  fractype fraction;
  int exp;
  int sign;

  fraction = src->bits.fraction;
  exp = src->bits.exp;
  sign = src->bits.sign;

  dst->sign = sign;
  if (exp == 0)
    {
      /* Hmm.  Looks like 0 */
      dst->class = CLASS_ZERO;
    }
  else if (exp == EXPMAX)
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

/* L_pack_df, with the NO_DENORMALS arm of the too-small-exponent case. */
FLO_type
pack_d ( fp_number_type *  src)
{
  FLO_union_type dst;
  fractype fraction = src->fraction.ll;	/* wasn't unsigned before? */
  int sign = src->sign;
  int exp = 0;

  if (isnan (src))
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
	  /* Go straight to a zero representation: this build has no
	     denormals.  */
	  exp = 0;
	  fraction = 0;
	}
      else if (src->normal_exp > EXPBIAS)
	{
	  exp = EXPMAX;
	  fraction = 0;
	}
      else
	{
	  exp = src->normal_exp + EXPBIAS;
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
	      /* Add a one to the guards to round up */
	      fraction += GARDROUND;
	    }
	  if (fraction >= IMPLICIT_2)
	    {
	      fraction >>= 1;
	      exp += 1;
	    }
	  fraction >>= NGARDS;
	}
    }

  dst.bits.fraction = fraction;
  dst.bits.exp = exp;
  dst.bits.sign = sign;

  return dst.value;
}
