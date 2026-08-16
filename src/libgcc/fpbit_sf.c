/*
 * gcc config/fp-bit.c at CVS rev ec6bfc9b7ca, FLOAT/GOFAST config.
 *
 * Sony's build diverges from the vendored file in exactly two functions,
 * so FINE_GRAINED_LIBRARIES excludes L_pack_sf/L_unpack_sf from the
 * include and this wrapper carries the target's modified pack_d/unpack_d
 * (reconstructed from the disassembly, following the fpbit.c precedent of
 * keeping the original file's shape):
 *   - FLOAT_BIT_ORDER_MISMATCH is on (pack_d stores through the bitfield
 *     union member; the insertion masks in the original are the bitfield
 *     read-modify-write sequence).
 *   - no denormal support: unpack_d classes any exp==0 input as ZERO, and
 *     pack_d flushes results below NORMAL_EXPMIN to zero instead of
 *     shifting the fraction (the nan/inf unpack cases are gone too).
 *   - pack_d's overflow test is normal_exp > EXPBIAS + 1, not > EXPBIAS
 *     (the original's slti 129), so exp==255 can be produced by a normal
 *     result as well as by the overflow flush.
 */
#define FLOAT
#define US_SOFTWARE_GOFAST
#define FLOAT_BIT_ORDER_MISMATCH

/* Only these three members exist in the original: the R5900 does the
   float arithmetic and comparisons in the COP1, so fpadd/fpsub/fpmul/
   fpdiv/fpcmp/sitofp/fptosi/__negsf2 and the float _fpadd_parts and
   __fpcmp_parts_f are all ABSENT from the shipped ELF. Selecting them
   anyway emitted ten dead symbols, one of which (_fpadd_parts) collided
   with fpbit_df.c's real one. */
#define FINE_GRAINED_LIBRARIES
#define L_make_sf
#define L_sf_to_df
#define L_thenan_sf
#include "fp-bit.c"

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
  else
    {
      /* Nothing strange about this number */
      dst->normal_exp = exp - EXPBIAS;
      dst->class = CLASS_NUMBER;
      dst->fraction.ll = (fraction << NGARDS) | IMPLICIT_1;
    }
}

FLO_type
pack_d ( fp_number_type *  src)
{
  FLO_union_type dst;
  fractype fraction = src->fraction.ll;
  int sign = src->sign;
  int exp = 0;

  if (iszero (src))
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
	  exp = 0;
	  fraction = 0;
	}
      else if (src->normal_exp > EXPBIAS + 1)
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
