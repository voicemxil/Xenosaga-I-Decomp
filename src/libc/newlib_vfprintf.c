/*
 * newlib vfprintf.c at CVS rev 5bacbf109 (2000-08-16 -- the exact
 * vintage the binary was built from, bisected via the newlib git
 * history), vendored verbatim with its own era's headers. vfprintf,
 * cvt, exponent, __sprint and __sbprintf are byte-exact.
 * _vfprintf_r itself is still WIP -- see the TODO in decompiled.txt.
 *
 * 2026-08-14 session notes (unwired, reproduce via ccpipe with the
 * TU's configure.py settings plus the extras below):
 *  - fixflags += "--pin-slot-nop _vfprintf_r:4" keeps `ld $4,504($sp)`
 *    out of the `jal isnan` delay slot (the original leaves a literal
 *    nop there): 369 -> 361 diff words.  Site index = 4th reorder-mode
 *    branch/jump of _vfprintf_r, see pin_slot_nops in fix_cc_asm.py.
 *    The `jal isinf` slot (gcc-filled, addu $22) already matches.
 *  - cflags += "-fno-regmove" on top of that: 361 -> 312 and the
 *    LENGTH mismatch disappears (5728/5728).  Cost: a new a0/v1-swap
 *    cluster around the movz at word ~1390; not wired for that reason.
 *  - every other -fno-* tried (cse-follow-jumps, cse-skip-blocks,
 *    gcse, thread-jumps, expensive-optimizations, rerun-cse-after-loop,
 *    rerun-loop-opt, strength-reduce, caller-saves, function-cse,
 *    peephole) is neutral or worse, several break the byte-exact
 *    sibling functions.
 *  - remaining core: ONE regalloc decision around the %g block --
 *    `ndig` (the int at 480($sp), &ndig is cvt's last arg) is loaded
 *    once into a0 in the bgtz slot at word ~901 and stays live through
 *    `slt v0,v1,a0` (word 1020) and `sw a0,4(s3)` (1023); our build
 *    loads it into v0, loses it to the slt result, and reloads from
 *    480($sp) at words ~1023/1218 (and 476($sp) at ~1114).  The
 *    bnezl-vs-bnez + slot-content diff at 1021 and the sw s5,0(s3)
 *    placement drift at ~999/1156 are downstream of the same choice.
 */
#include "vfprintf.c"
