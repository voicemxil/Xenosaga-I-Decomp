/*
 * gcc frame.c, the DWARF2 frame-unwind registry, vendored at CVS rev
 * ec6bfc9b7ca (1999-12-29) plus the two later upstream commits that the
 * EE compiler snapshot carries -- see newlib/gccsrc/frame.c, where both
 * are applied and annotated:
 *
 *   78a0d70cdf55 (2000-02-01) find_fde: the fde_array scan becomes a
 *       do-while so the compiler can see it always runs once.
 *   89d7f003d32b (2000-06-08) start_fde_sort/end_fde_sort: the erratic
 *       array test goes away and the linear malloc is guarded on count.
 *
 * Both were found by asking WHICH REVISION rather than which flag: the
 * ee-gcc 2.96 snapshot is dated 2000-10-03, so any frame.c commit before
 * that date is in scope, and `gh api repos/gcc-mirror/gcc/commits
 * ?path=gcc/frame.c` lists them in a second. Between them they took
 * find_fde from 20 diff words to byte-exact, end_fde_sort from 138, and
 * frame_init -- which is inlined start_fde_sort -- from 90. No fixer
 * flags are involved in any of the three.
 *
 * find_fde is worth a specific warning. Before the do-while patch its
 * diff looked like an s1/s2 register-allocation disagreement, and
 * --swap-regs find_fde:17-18 duly cut it from 20 words to 13. That was
 * a symptom, not the cause: the register assignment fell out of the
 * loop shape, and with the right loop shape the function matches with
 * no flag at all. A near-miss that looks like regalloc can still be a
 * source-revision problem.
 *
 * frame_init has no symbol of its own in the shipped ELF -- it is
 * static, and the name `frame_init` there belongs to an unrelated game
 * function in EW.c. It is registered by address, at 0x0032D028, in the
 * gap between search_fdes and find_fde.
 *
 * Sony's build guards the registry with WaitSema/SignalSema on
 * __sce_eh_sema_id via custom gthr hooks (see gccsrc/gthr-ps2.h).
 *
 * STILL WIP, both the same shape -- our build is LONGER than the
 * original, by 34 words and 32 words respectively:
 *   execute_cfa_insn   248d, 267 orig vs 301 built
 *   __frame_state_for   83d, 110 orig vs 142 built
 * The original also has a standalone decode_stack_op (0x0032D448, 72
 * words) that no revision of frame.c in the gcc history contains. It
 * decodes DWARF expression opcodes -- DW_OP_deref, DW_OP_plus_uconst,
 * DW_OP_regN, DW_OP_bregN, DW_OP_regx, DW_OP_bregx -- and writes
 * through a frame_state that has fields this revision's frame.h does
 * not: an `indirect` flag byte and a second offset. Its stores also
 * pin the target's FIRST_PSEUDO_REGISTER at 123, not the 76 in
 * gccsrc/tconfig.h: cfa_reg lands at offset 1016 = 24 + 8*124, and
 * saved[123] at 1143 = 1020 + 123. Getting those two functions will
 * mean finding the frame.h/frame.c pair that has decode_stack_op and
 * setting FIRST_PSEUDO_REGISTER to the EE's real value; it is not a
 * codegen problem.
 */
#define DWARF2_UNWIND_INFO
#include "frame.c"
