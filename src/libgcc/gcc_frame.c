/*
 * gcc frame.c at CVS rev ec6bfc9b7ca (1999-12-29, bisected via the
 * gcc git mirror -- consistent with the 2.96-ee-001003 compiler
 * carrying a mainline snapshot), the DWARF2 frame-unwind registry.
 * Sony's build guards the registry with WaitSema/SignalSema on
 * __sce_eh_sema_id via custom gthr hooks (see gccsrc/gthr-ps2.h).
 * Twelve functions are byte-exact; still WIP: __deregister_frame_info
 * (2d, an epilogue restore-order swap the unlock barrier introduces),
 * find_fde (20d), end_fde_sort (131d), execute_cfa_insn (214d),
 * __frame_state_for (51d), decode_stack_op (inlined here,
 * standalone in the original).
 */
#define DWARF2_UNWIND_INFO
#include "frame.c"
