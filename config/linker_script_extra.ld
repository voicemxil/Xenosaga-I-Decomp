/* Hand-maintained linker supplement.
 *
 * INCLUDEd by config/SLUS_204.69.pinned.ld. Unlike undefined_syms_auto.txt /
 * undefined_funcs_auto.txt / symbol_addrs.txt, this file is NOT produced by
 * Splat, so nothing here is lost when `splat split` regenerates those.
 *
 * See docs/LINKING.md for how the final link is wired together.
 */

/* Entry point of the retail executable (readelf -h elf/SLUS_204.69 reports
 * 0x200008; config/symbol_addrs.txt names that address ENTRYPOINT). Without
 * this, ld picks its own entry and the ELF header does not match the
 * original, which matters for running the build under PCSX2. */
ENTRY(ENTRYPOINT);

/* Absolute addresses referenced by the ov02 overlay assembly that Splat did
 * not emit into undefined_syms_auto.txt. Every one of these is a bare
 * %hi/%lo address pair with no label at the target -- either PS2 hardware /
 * scratchpad memory, or an unaligned interior offset of a cod-segment
 * object that has no symbol of its own. Defining them as absolute values
 * reproduces the exact immediates the original build encoded (verified
 * against the lui/addiu bytes in asm/ov02/2DD000.s).
 *
 * Addresses below 0x00200000 and the 0x0124xxxx / 0x02A0xxxx values are EE
 * physical / uncached-accelerated memory addresses, not program symbols. */
D_100020   = 0x00100020;   /* EE scratchpad-relative work address */
D_100060   = 0x00100060;
D_1000010  = 0x01000010;
D_1245C58  = 0x01245C58;
D_2A007F8  = 0x02A007F8;
D_490DC0   = 0x00490DC0;   /* interior of .data (cod 0x338600..0x4ADE54) */
D_4A1A92   = 0x004A1A92;   /* interior of .data, deliberately 2-byte aligned */

/* The 5-byte-stride table of window-select message ids at 0x0036C200.
 * Splat named the interior entries it saw referenced (D_0036C205, ...20A,
 * ...20F, ...219, ...) but not the base or the ...214 entry, which are only
 * reached from CharListMake_Acc / CharListMake_Gun. Acc indexes the table at
 * runtime, so the base has to exist as a symbol of its own. */
D_0036C200 = 0x0036C200;
D_0036C214 = 0x0036C214;
