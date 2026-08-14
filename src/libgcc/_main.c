/*
 * libgcc2.c L__main member (full vendored libgcc2.c in newlib/gccsrc):
 * __main is byte-exact; __do_global_ctors (47d) and __do_global_dtors
 * (24d, lives in L_exit) are still WIP.
 */
#define L__main
#include "libgcc2.c"
