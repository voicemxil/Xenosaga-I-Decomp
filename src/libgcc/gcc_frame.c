/*
 * gcc frame.c (EE-GCC 2.9-ee-991111 source, newlib/gccsrc/), the
 * DWARF2 frame-unwind registry. Eight functions are byte-exact with
 * the libgcc recipe (-mlong32, barriers, expand-sym-loads); the
 * fde-sorting and CFA-execution half still diffs -- likely the 2.96
 * compiler's own frame.c revision -- and stays unregistered.
 */
#define DWARF2_UNWIND_INFO
#include "frame.c"
