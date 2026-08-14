/*
 * gcc config/fp-bit.c at CVS rev ec6bfc9b7ca, DOUBLE/GOFAST config
 * (same configuration the hand-transcribed fpbit.c models). From this
 * vendored original, __make_dp and dptoul are byte-exact; the dp
 * arithmetic entry points stay owned by fpbit.c.
 */
#define US_SOFTWARE_GOFAST
#include "fp-bit.c"
