/*
 * gcc config/fp-bit.c at CVS rev ec6bfc9b7ca, FLOAT/GOFAST config.
 * __make_fp is byte-exact; __unpack_f (17d) and __pack_f (52d) are
 * still WIP.
 */
#define FLOAT
#define US_SOFTWARE_GOFAST
#include "fp-bit.c"
