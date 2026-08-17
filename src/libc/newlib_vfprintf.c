/*
 * newlib vfprintf.c at CVS rev 5bacbf109 (2000-08-16), compiled against
 * its matching era headers. All six functions in this translation unit are
 * byte-exact. Later newlib revisions were measured and are decisively wrong.
 *
 * The vendored source retains the original floating-fragment initialization
 * order: iov_len before iov_base. That source shape keeps the conversion
 * values live across the %f/%g/%e branches under EE GCC 2.96. One targeted
 * delay-slot pin preserves the original assembler's nop before the isnan call.
 */
#include "vfprintf.c"
