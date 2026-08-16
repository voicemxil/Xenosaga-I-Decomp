/*
 * newlib vfprintf.c at CVS rev 5bacbf109 (2000-08-16), vendored verbatim
 * with its own era's headers. vfprintf, cvt, exponent, __sprint and
 * __sbprintf are byte-exact. _vfprintf_r is WIP at 369 diff words.
 *
 * THE REVISION IS SETTLED -- do not re-bisect it. 2026-08-15: every
 * later revision of vfprintf.c up to 2001-05-28 was built and measured
 * against the original (fetch them with `gh api
 * repos/mirror/newlib-cygwin/contents/newlib/libc/stdio/vfprintf.c?ref=SHA`):
 *
 *   5bacbf109f58  2000-08-16  _vfprintf_r 369   <-- this file
 *   6bdac416e9e1  2000-12-06  _vfprintf_r 1407, exponent 3
 *   b294082c46c7  2000-12-08  _vfprintf_r 1394, exponent 3
 *   b6182a09dd28  2000-12-14  _vfprintf_r 1394, exponent 3
 *   e9cd87b82a1f  2001-01-23  _vfprintf_r 1383, exponent 3
 *   6f637037e59f  2001-03-13  _vfprintf_r 1383, exponent 3
 *   188bc140c02e  2001-05-28  _vfprintf_r 1395, exponent 3
 *
 * This was worth checking because the game shipped in 2002 and two of
 * those commits touch _vfprintf_r's exact remaining diff region -- the
 * 2000-12-08 one turns _fpvalue into a union, and the 2001-01-23 one
 * rewrites the `if (ndig)` in the %g block that the last diff cluster
 * sits on. Both are decisively wrong: they also break `exponent`, which
 * is byte-exact here. The game's newlib predates 2000-12-06.
 *
 * WHAT IS ACTUALLY LEFT (369 words, one cause). The build is TWO WORDS
 * LONGER than the original, 1434 vs 1432, and the divergence starts at
 * word 1114. There the original has `sw s5,0(s3)` -- it is holding a
 * value in a callee-saved register across the %g block -- and our build
 * has `lw v0,476(sp)` + `sw v0,4(s3)`, reloading it from the stack. One
 * extra word per occurrence. Everything from word 1111 to the end (318
 * of the 369) is that shift, and almost every isolated diff below 1111
 * is a branch displacement that will vanish once the lengths agree. It
 * is a single register-allocation decision: permuter work, or a PIN,
 * and a PIN in vendored upstream source is not worth having.
 *
 * RULED OUT, do not repeat:
 *  - per-site --unfill-gcc-slots. The standing theory was that this was
 *    the blocker and only needed site selection. Site selection now
 *    exists (see tools/fix_cc_asm.py and tools/slot_sites.py) and I
 *    swept all 281 of this function's gcc-filled slots one at a time:
 *    not one improves, and most are catastrophic. The direction is
 *    simply wrong -- unfilling INSERTS a nop, and this build is already
 *    too long.
 *  - "--pin-slot-nop _vfprintf_r:4" keeps `ld $4,504($sp)` out of the
 *    `jal isnan` delay slot: 369 -> 361. Real but partial, and it does
 *    not touch the cause.
 *  - cflags += -fno-regmove on top of that: 361 -> 312 with the length
 *    finally equal, at the cost of a new a0/v1 swap cluster around the
 *    movz at word ~1390. Not wired for that reason.
 *  - every other -fno-* (cse-follow-jumps, cse-skip-blocks, gcse,
 *    thread-jumps, expensive-optimizations, rerun-cse-after-loop,
 *    rerun-loop-opt, strength-reduce, caller-saves, function-cse,
 *    peephole) is neutral or worse; several break the byte-exact
 *    siblings.
 */
#include "vfprintf.c"
