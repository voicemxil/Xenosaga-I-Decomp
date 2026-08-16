/*
 * newlib libc/stdlib/dtoa.c (era rev, vendored verbatim): _dtoa_r --
 * 4,708 bytes, the project's longest-standing WIP -- and quorem, both
 * byte-exact with the full newlib recipe (era headers, -G0, barriers,
 * --expand-sym-loads, gccinc va_arg override). The name is a leftover
 * from when libc.c also carried a hand transcription of _dtoa_r and its
 * helpers; that is gone now and this file is the only definition.
 *
 * Two details of the transcription are worth keeping, because they
 * date the whole of the game's libc and generalise past this file: the
 * game's newlib is NOT 1.8.2 here but a slightly later revision, and
 * two hoists prove it -- `ilim = ilim1 = -1;` sits before `switch
 * (mode)` rather than inside `case 0: case 1:` (the "silence erroneous
 * gcc -Wall warning" hoist), and `spec_case = 0;` is hoisted above
 * `if (mode < 2)` instead of living in an else arm inside it. Both are
 * load-bearing for the match.
 */
#include "dtoa.c"
