#!/usr/bin/env python3
"""Post-process ee-gcc -S output so the modern assembler reproduces the
original ee-as encodings. Applied by the ninja cc rule between compile
and assemble. Transformations:

- move -> daddu (the original assembler's encoding for register moves)
- wrap `la`, symbol-operand load/store macros, and FP conversions in
  .set mips1 so they expand with 32-bit addiu/addu (not daddiu/daddu)
  and are accepted under -march=r5900
- li.s: wrap in .set mips1, and add the mtc1 hazard nop the original
  ee-as inserted -- but only when the next instruction is an FP compare
  (c.*.s), matching the original's hazard rule
- do not add a fallthrough hazard nop after an FP load in a COP1 likely-
  branch delay slot; that delay slot is annulled on the fallthrough path
"""
import argparse
import re
import struct
import sys

# Loads among MEM_OPS: wrapping these in .set mips1 (to control address
# expansion) also makes gas honour mips1 LOAD DELAY slots and insert a nop
# the R5900 never needed. Add .set noreorder for loads so only the address
# expansion is affected, not the delay behaviour.
LOAD_OPS = ("lw", "lh", "lb", "lbu", "lhu", "lwc1", "l.s")
MEM_OPS = ("sw", "sh", "sb", "swc1", "lw", "lh", "lb", "lbu", "lhu",
           "lwc1", "s.s", "l.s")
# sd/ld deliberately excluded from MEM_OPS (and so from RE_MEM/RE_MEM_ABS):
# those regexes' fix is `.set mips1`, but sd/ld are true 64-bit ops that
# don't exist under mips1 -- gas silently splits them into a pair of 32-bit
# sw/lw halves instead of just fixing the address macro. That's fine (and
# needed) for gp-relative symbol operands, which assemble correctly with no
# wrapping at all. Only the register-base-plus-large-numeric-offset case
# (RE_MEM_BIGOFF) needs sd/ld handling, and it gets a hand-expanded macro
# instead of the mips1 wrap -- see the RE_MEM_BIGOFF handling below.
MEM_OPS_BIGOFF = MEM_OPS + ("sd", "ld")

RE_MOVE = re.compile(r'\tmove\t(\$[0-9a-z]+),(\$[0-9a-z]+)')
# gcc's integer divide-by-zero trap is emitted as the one-operand
# `break 7`.  The original ee-as put that 7 in the LOW code field
# (bits 6-15), giving 0x000001cd; modern gas puts a one-operand code in
# the HIGH field (bits 16-25), giving 0x0007000d.  Spelling it as the
# two-operand `break 0,7` gets the original encoding out of both.  The
# original image contains 223 copies of 0x000001cd and not one
# 0x0007000d, so this is unconditional -- every integer / or % in the
# tree depends on it.
RE_BREAK = re.compile(r'^\tbreak\t([0-9]+)\s*$')
RE_LA = re.compile(r'^\tla[ \t](.*)$')
# Symbol operands can be named globals (`foo($2)`) or compiler-generated
# local labels (`$L41($2)`, most commonly jump tables).  Both are assembler
# macros whose indexed address expansion must use the original ee-as 32-bit
# `addu`, not modern gas's `daddu`.
RE_MEM = re.compile(r'^\t(' + '|'.join(re.escape(op) for op in MEM_OPS) +
                    r')[ \t]([^,]*),((?:[A-Za-z_.]|\$L).*)$')
# Same load/store macro, but against a numeric absolute address (e.g.
# `sw $4,1879048248` from `((S *)0x70000038)->f = x`). No parentheses, so
# it is a macro to expand rather than a plain register-offset access.
RE_MEM_ABS = re.compile(r'^\t(' + '|'.join(re.escape(op) for op in MEM_OPS) +
                        r')[ \t]([^,]*),(-?[0-9]+)[ \t]*$')
# A load/store with a register base and an offset too large for the 16-bit
# immediate field is also a macro: gas expands it as lui + daddu + op, but
# the original ee-as used addu. Wrap in .set mips1 to get the 32-bit form.
RE_MEM_BIGOFF = re.compile(r'^\t(' + '|'.join(re.escape(op) for op in MEM_OPS_BIGOFF) +
                           r')[ \t]([^,]*),(-?[0-9]+)(\(\$[a-z0-9]+\))[ \t]*$')
# ld/sd indexed by a NAMED symbol plus a register (not a numeric offset),
# e.g. `ld $2,Zero($3)` from `return Zero[idx];` on a >8-byte const array
# (not gp-relative at -G8). Same daddu-vs-addu problem as RE_MEM_BIGOFF,
# but the base is a symbol, not a numeric literal, so the hi/lo split
# can't be computed here -- gas's own %hi()/%lo() operators do it instead.
# The original reuses the destination register as the address-computation
# scratch (lui v0,%hi; addu v0,v0,v1; ld v0,%lo(v0)) rather than $1/$at,
# so mirror that exactly. Found via __ieee754_fmod's `Zero[sx>>31]`.
# The symbol may carry a constant displacement (`__mprec_tens-8($2)`, from
# _dtoa_r's `tens[k-1]`-style indexing); gas folds it into %lo() exactly
# the same way, so accept it here too rather than letting that form fall
# through to gas's own daddu-based expansion.
# Loads may use their destination as the address scratch, so any
# integer load with a symbol(+offset)(reg) address gets the same manual
# lui/addu/%lo expansion (confirmed on _strtoul_r's `_ctype_+1($17)`
# lbu, where the mips1-wrap fallback materializes a load-delay nop the
# original does not have). Stores must not clobber their source, so
# they keep the old sd/ld-only handling.
RE_MEM_SYM_REG = re.compile(
    r'^\t(sd|ld)[ \t]([^,]*),([A-Za-z_][A-Za-z0-9_.]*(?:[+-][0-9]+)?)'
    r'\((\$[a-z0-9]+)\)[ \t]*$')
# Opt-in (--expand-sym-loads) variant covering the narrower integer
# loads: dest doubles as the address scratch, exactly the original's
# lui/addu/%lo shape for e.g. _strtoul_r's `lbu $2,_ctype_+1($17)`.
# The default mips1-wrap fallback materializes a load-delay nop there
# that the original does not have -- but several game TUs rely on that
# wrap shape, so this must not be blanket behavior.
RE_MEM_SYM_REG_LOAD = re.compile(
    r'^\t(lb|lbu|lh|lhu|lw|lwu)[ \t]([^,]*),'
    r'([A-Za-z_][A-Za-z0-9_.]*(?:[+-][0-9]+)?)'
    r'\((\$[a-z0-9]+)\)[ \t]*$')
RE_LIS = re.compile(r'^\tli\.s[ \t](.*)$')
# li.d loads a 64-bit double-precision bit pattern into a GPR (this target's
# soft-float double ABI packs a double into one 64-bit register/register
# pair, not a COP1 register -- confirmed by real li.d destinations being
# plain integer regs like $5/$16/$17 feeding libcalls such as dpadd/dpmul).
# The r5900 assembler rejects the li.d mnemonic outright ("opcode not
# supported on this processor"), so every use is a hard build blocker --
# this rewrite is unconditionally safe (nothing that used li.d could ever
# have assembled before). Reverse-engineered from the ORIGINAL binary's own
# codegen for 64-bit immediates (e.g. _strtod_r's `li $17,0xbff0` +
# `dsll32 $17,$17,0x10` for -1.0): strip trailing zero bits from the 64-bit
# pattern, load the remaining significant bits with the shortest li/lui+ori
# sequence, then shift back into place with dsll/dsll32. That path is only
# actually cheap (2 real instructions: lui/li + dsll32) when the low 32
# bits are entirely zero, which is exactly when the original uses it.
#
# When the low 32 bits are NOT all zero (irrational-looking constants, e.g.
# dtoa.c's 0.289529654602168), the original does NOT synthesize bit-by-bit
# at all -- confirmed on _dtoa_r, whose quick-path constants are emitted as
# `lui $at,%hi(LCx)` / `ld $a1,%lo(LCx)($at)`, loading from a table of
# 8-byte-strided rodata slots (three consecutive constants sit at
# 0x4d79a0/0x4d79a8/0x4d79b0). This is the standard MIPS "big constant ->
# literal pool" behavior: a table load is only 2 instructions, cheaper than
# the 5-6 instruction hi32/lo32-halves-OR'd synthesis that used to be here
# (and was itself unverified against real bytes). Route this case through a
# synthesized `.rdata` literal pool instead.
RE_LID = re.compile(r'^\tli\.d\t(\$[0-9a-zA-Z]+),\s*(\S+)$')
RE_CVT = re.compile(r'^\t(cvt\.[a-z.]+|trunc\.w\.s)[ \t](.*)$')
# FP loads whose result feeds a COP1 compute need the hazard nop the
# original ee-as inserted (modern gas does not).
# Only mtc1 and the li.s macro (which expands to lui+mtc1) carry the
# COP1 move hazard the original ee-as padded. A plain lwc1/l.s load
# does not -- including it inserted nops the original never had.
# Only the li.s macro (lui $at + mtc1 $at) carries the COP1 move hazard
# the original ee-as padded. A bare mtc1 does not: `mtc1 $zero,$fN`
# (the 0.0f constant) has no nop after it in the original, and an
# mtc1 feeding a conversion is already handled by the RE_CVT rule below.
RE_FPLOAD = re.compile(r'^\t(li\.s)[ \t]')


# Set when the file is assembled with `as -G0` (see configure.py's
# FILE_ASFLAGS_OVERRIDE / asflags_for). gas's own small-data threshold
# defaults to -G8 regardless of what -G was passed to gcc, so a li.s
# whose constant has non-zero low 16 bits normally expands as a .lit4
# %gp_rel LOAD (no hazard) -- but at `as -G0` gas can never place it in
# .lit4, so it ALWAYS expands to lui+ori+mtc1 and DOES carry the hazard.
# Found on libm.c's floorf: with -G0 passed through to the assembler,
# the huge_f li.s (low16 = 0xf2ca, nonzero) still needs the nop the
# unconditional low16-zero heuristic below was built to skip.
ASSUME_NO_LIT4 = False


def fp_hazard_dest(line):
    """FP register written by an mtc1-class instruction, or None.

    The COP1 move hazard is REGISTER-dependent, not opcode-dependent: the
    original pads only when the next FP instruction READS the register the
    mtc1 just wrote. (CheckSlope has `mtc1 $zero,$f5` followed directly by
    `mul.s $f1,$f1,$f0` with no nop; SEQ_motion has `mtc1 $v1,$f1` + nop +
    `mul.s $f1,$f1,$f20`.) A li.s whose constant has non-zero low 16 bits
    is normally expanded by gas as a .lit4 LOAD, not lui+mtc1, so it
    carries no hazard at all -- UNLESS the file is assembled at `as -G0`
    (ASSUME_NO_LIT4), in which case it always expands to lui+ori+mtc1 and
    always carries the hazard, low bits or not.
    """
    m = re.match(r'^\tmtc1\t\$\w+,(\$f\d+)', line)
    if m:
        return m.group(1)
    m = re.match(r'^\tli\.s\t(\$f\d+),\s*(\S+)', line)
    if m:
        if ASSUME_NO_LIT4:
            return m.group(1)
        try:
            bits = struct.unpack('<I', struct.pack('<f', float(m.group(2))))[0]
        except ValueError:
            return m.group(1)
        return m.group(1) if (bits & 0xFFFF) == 0 else None
    return None


def fp_reg_num(tok):
    """$fN -> N with the low bit masked off (its even-aligned pair), else None."""
    m = re.match(r'^\$f(\d+)$', tok)
    return (int(m.group(1)) & ~1) if m else None


def reads_fp_reg(insn, reg, pair=False):
    """True if `insn` reads `reg` as a source operand.

    With `pair`, an operand also matches when it merely shares an
    even-aligned register PAIR with `reg` -- (operand & ~1) == (reg & ~1).
    That is what the original ee-as did: classic gas's insn_uses_reg()
    masks the low bit off both sides for the FP register class, because
    with 32-bit FPRs $fN and $f(N^1) are the two halves of one double and
    gas never distinguished which half a delay applied to (its own source
    comment: "this is not optimal, because it will introduce an
    unnecessary NOP between lwc1 $f0 and swc1 $f1").  Modern gas does not
    pad the R5900 this way at all, so the pad has to be replayed here.

    Opt-in per file/function via --fp-pair-hazard: it is the ORIGINAL
    assembler's rule, but files whose near-misses were already papered
    over with site-keyed nop flags would double up.  On libm.c it turns
    floorf, __kernel_sinf, __kernel_cosf, __kernel_rem_pio2f and
    __ieee754_atan2f's whole nop skeleton from wrong to exact.
    """
    m = re.match(r'^\t([a-z0-9.]+)\t(.*)$', insn)
    if not m:
        return False
    mnem = m.group(1)
    ops = [o.strip() for o in m.group(2).split(',')]
    # Every operand of a compare is a source.  FP stores also read operand 0
    # (the FPR being written to memory); treating it as a destination hides
    # the classic gas pair hazard between mtc1 $f1 and swc1 $f0.
    sources = ops if (mnem.startswith('c.') or
                      mnem in ('swc1', 's.s', 'sdc1', 's.d')) else ops[1:]
    if not pair:
        return reg in sources
    want = fp_reg_num(reg)
    if want is None:
        return reg in sources
    return any(fp_reg_num(o) == want for o in sources)
RE_FPCOMPUTE = re.compile(
    r'^\t(c\.[a-z]+\.s|mul\.s|div\.s|add\.s|sub\.s|mov\.s|abs\.s|neg\.s'
    r'|sqrt\.s|trunc\.w\.s|cvt\.[a-z.]+)[ \t]')
RE_FP_STORE = re.compile(r'^\t(swc1|s\.s|sdc1|s\.d)[ \t]')
RE_FP_BRANCH_LIKELY = re.compile(r'^\tbc1[ft]l[ \t]')
# The R5900's sqrt.s takes its operand in the ft field; modern gas encodes
# it in fs even under -march=r5900, and no alternate syntax selects the
# R5900 form. Emit the word directly. (splat cannot disassemble it either
# -- it renders the original as a bare `c1 0xC0004`.)
RE_SQRT = re.compile(r'^\tsqrt\.s\t\$f(\d+),\$f(\d+)[ \t]*$')
# A leaf return: `j $31` or `jr $ra`/`jr $31`.
RE_RETURN = re.compile(r'^\t(j\t\$31|jr\t\$(ra|31))\b')
# A store gcc's own RTL scheduler (not gas) placed in a leaf return's
# delay slot -- e.g. `j $31 / sq $2,0($4)`. The original ee-as build
# instead keeps the store BEFORE the branch, with a genuine nop in the
# delay slot (same fingerprint as the TI-mode lq/sq near-misses and the
# swc1-after-mul cases: an independent trailing store keeps getting
# pulled forward). This is a pure position swap -- a delay-slot
# instruction executes unconditionally exactly once either way, so
# moving it immediately before the branch (with the branch's own delay
# slot now a nop) is semantically identical. Opt-in per file, since most
# functions legitimately want the store in the delay slot.
RE_RETURN_DELAY_STORE = re.compile(
    r'^\t(sw|sh|sb|sd|swc1|s\.s|sq)[ \t]')
# Stores that gas may steal into a leaf return's delay slot. Whether the
# original did that is function-specific, so this is opt-in per file via
# --barrier-return-store rather than a blanket rule: most functions here
# legitimately DO end with a store in the delay slot.
RE_STORE = re.compile(r'^\t(sw|sh|sb|sd|swc1|s\.s|sq)[ \t]')
# A return-value copy into $2 (originally `move $2,reg`, already rewritten
# to `daddu $2,reg,$0` by RE_MOVE above) immediately before a leaf return:
# gas's default reorder mode steals it into the jr delay slot, but the
# original ee-as build left it as three separate instructions (the copy,
# the branch, and a genuine nop). Same fingerprint/opt-in mechanism as
# RE_STORE -- confirmed on fabs (src/libm.c), where SET_HIGH_WORD's final
# `return x` computes the result into $4/$6 and needs an explicit copy to
# $2 that the original did not fold into the delay slot.
# Also matches an arbitrary-register argument-setup copy (e.g.
# `move $5,$4` i.e. `daddu $5,$4,$0`) immediately before a `jal` call:
# the same theft happens into a CALL's delay slot, not just a leaf
# return's -- confirmed on __ieee754_fmod's `return (x*y)/(x*y);`, where
# `a1 = a0;` before `dpdiv(...)` is left as a separate instruction (with
# a genuine nop in dpdiv's delay slot) in the original, but gas pulls it
# into the delay slot here.
RE_RETURN_MOVE = re.compile(r'^\tdaddu\t\$\w+,\$\w+,\$0$')
# A leaf return OR a call: both have a delay slot gas may steal a
# preceding independent move into. Used only by the RE_RETURN_MOVE
# barrier above -- RE_RETURN alone is used elsewhere for return-only
# fixes (la-before-return, mem-abs-before-return) that must not widen.
RE_RETURN_OR_CALL = re.compile(r'^\t(j\t\$31|jr\t\$(ra|31)|jal\t[A-Za-z_]\w*)\b')
# Any branch with a delay slot gas may steal a preceding move into:
# conditional branches and unconditional b, on top of returns/calls.
# Used by --barrier-branch-move, the conditional-branch analogue of
# --barrier-return-store's move case -- confirmed on the libgcc
# __fixunsdfdi/__fixunssfdi/__floatdidf conversions, where the original
# keeps `move a1,s1` above a bgez and leaves a genuine nop in the slot.
RE_ANY_BRANCH = re.compile(
    r'^\t(b\t|beqz?l?\t|bnez?l?\t|bgezl?\t|bltzl?\t|blezl?\t|bgtzl?\t|'
    r'j\t\$31|jr\t\$(ra|31)|jal\t[A-Za-z_]\w*)')
# Non-likely branches only: branch-likely slots annul when not taken, so
# gas never steals into them and they carry real content from gcc.
# Simple one-output ALU ops --barrier-branch-move also keeps out of a
# branch delay slot (a dsll32 before a bgez in __floatdidf, on top of
# the daddu moves).
RE_BARRIER_ALU = re.compile(
    r'^\t(daddu|addu|subu|dsll32|dsll|dsra32|dsrl32|sll|srl|sra|or|and|xor)'
    r'\t\$\w+,\$\w+(,(\$\w+|\d+))?$')
RE_PLAIN_BRANCH = re.compile(
    r'^\t(b\t|beqz?\t|bnez?\t|beq\t|bne\t|bgez\t|bltz\t|blez\t|bgtz\t|'
    r'j\t\$31|jr\t\$(ra|31)|jal\t[A-Za-z_]\w*)')


def synth_load32(reg, val):
    """Load an unsigned 32-bit value into `reg` with li, or lui[+ori]."""
    val &= 0xFFFFFFFF
    if val <= 0xFFFF:
        return [f"\tli\t{reg},{val}"]
    hi = (val >> 16) & 0xFFFF
    lo = val & 0xFFFF
    out = [f"\tlui\t{reg},{hi}"]
    if lo:
        out.append(f"\tori\t{reg},{reg},{lo}")
    return out


def synth_li_d(reg, bits, lit_pool):
    """Emit an instruction sequence loading the 64-bit pattern `bits` into
    `reg`, replacing an unassemblable `li.d`. See RE_LID comment above.

    `lit_pool` is a list the caller accumulates (label, bits) pairs into;
    the caller emits the backing `.rdata` once, at the end of the file."""
    if bits == 0:
        return [f"\tmove\t{reg},$0"]
    raw_tz = (bits & -bits & 0xFFFFFFFFFFFFFFFF).bit_length() - 1
    msb = bits.bit_length() - 1
    sig_width = msb - raw_tz + 1  # width of the fully-stripped significant part
    # The real li.d/dli macro only synthesizes bit-by-bit when the value's
    # ENTIRE significant content (from its highest set bit down to its
    # lowest set bit) fits in a 16-bit window -- i.e. "round" constants like
    # -1.0, 1.5, or 2.0**32 whose low ~48 bits are all zero. It is NOT a
    # generic "strip trailing zero bits, round to nearest halfword" rule
    # (an earlier version of this comment claimed that, based on only two
    # examples that happen to be consistent with both theories). Confirmed
    # against four real bit patterns from the original binary:
    #   -1.0             = 0xbff0000000000000 -> li $r,0xbff0 / dsll32 ,16
    #   -2147483648.0    = 0xc1e0000000000000 -> li $r,0xc1e0 / dsll32 ,16
    #   2.0**32          = 0x41f0000000000000 -> li $r,0x83e0 / dsll32 ,15
    #   1.5              = 0x3ff8000000000000 -> li $r,0xffe0 / dsll32 ,14
    # In every case the emitted 16-bit immediate has its OWN top bit (bit
    # 15) set -- i.e. it's a 16-bit window anchored at the value's highest
    # set bit (shift = msb - 15), not the widest all-zero-low-halfword
    # window. This only ever loses no precision when sig_width <= 16.
    if sig_width <= 16:
        shift = max(msb - 15, 0)
        sig = bits >> shift
        out = synth_load32(reg, sig)
        if shift:
            if shift < 32:
                out.append(f"\tdsll\t{reg},{reg},{shift}")
            else:
                out.append(f"\tdsll32\t{reg},{reg},{shift - 32}")
        return out
    # The value's significant content is wider than 16 bits (irrational-
    # looking constants, e.g. dtoa.c's 0.289529654602168): the original
    # does NOT synthesize these bit-by-bit at all -- confirmed on _dtoa_r,
    # whose quick-path constants are emitted as `lui $at,%hi(LCx)` /
    # `ld $a1,%lo(LCx)($at)`, loading from a table of 8-byte-strided rodata
    # slots (three consecutive constants sit at 0x4d79a0/0x4d79a8/
    # 0x4d79b0). This is the standard MIPS "big constant -> literal pool"
    # behavior: a table load is only 2 instructions, cheaper than
    # synthesizing hi32/lo32 halves and OR-ing them (5-6 instructions, and
    # was itself unverified against real bytes). Route through a
    # synthesized `.rdata` literal pool instead.
    label = f"$LID{len(lit_pool)}"
    lit_pool.append((label, bits))
    at = "$1" if reg != "$1" else "$2"
    return [f"\tlui\t{at},%hi({label})", f"\tld\t{reg},%lo({label})({at})"]


def wrap_mips1(text):
    return f"\t.set push\n\t.set mips1\n{text}\n\t.set pop"


def wrap_mips1_noreorder(text):
    # Same mips1 wrap, but also barriers reordering so the assembler cannot
    # steal the macro's final expanded instruction into a following return's
    # delay slot -- the original ee-as left a nop there instead.
    return f"\t.set push\n\t.set mips1\n\t.set noreorder\n{text}\n\t.set pop"


def next_insn(lines, idx):
    """Return the next non-blank, non-directive, non-label line."""
    for j in range(idx + 1, len(lines)):
        s = lines[j]
        if not s.strip() or s.lstrip().startswith(('.', '#')) or s.rstrip().endswith(':'):
            continue
        return s
    return ""


def previous_insn(lines, idx):
    """Return the previous non-blank, non-directive, non-label line."""
    for j in range(idx - 1, -1, -1):
        s = lines[j]
        if not s.strip() or s.lstrip().startswith(('.', '#')) or s.rstrip().endswith(':'):
            continue
        return s
    return ""


def function_at(lines):
    """Map line index -> enclosing function name.

    gcc emits `.ent NAME` / `.end NAME` around each function. Lines outside
    any function map to None. Used to scope a fix pass to specific functions:
    several passes are correct for one function and actively WRONG for
    another in the same translation unit (--hoist-return-store fixes
    xglLightIntensityAmbient but regresses nmlModel), so per-file scoping is
    too coarse.
    """
    owner = [None] * len(lines)
    cur = None
    for i, l in enumerate(lines):
        m = re.match(r'\s*\.ent\s+(\S+)', l)
        if m:
            cur = m.group(1)
        owner[i] = cur
        if re.match(r'\s*\.end\s+(\S+)', l):
            cur = None
    return owner


def in_scope(name, scope):
    """True if a pass scoped to `scope` applies to function `name`.

    Empty scope means "whole file" (backwards compatible with the old
    file-level boolean flags).
    """
    return not scope or name in scope

def hoist_return_delay_stores(lines, scope=()):
    """Swap an immediately-adjacent `j $31` / store pair so the store
    comes first and the delay slot is left as a plain nop. Only handles
    the case where the store is the literal next line after the branch
    (i.e. really is gcc's own delay-slot fill, not something separated
    by directives) -- that is the only shape gcc actually emits."""
    owner = function_at(lines)
    out = []
    i = 0
    n = len(lines)
    while i < n:
        line = lines[i]
        if (RE_RETURN.match(line) and i + 1 < n
                and RE_RETURN_DELAY_STORE.match(lines[i + 1])
                and in_scope(owner[i], scope)):
            out.append(lines[i + 1])
            out.append(line)
            out.append('\tnop')
            i += 2
            continue
        out.append(line)
        i += 1
    return out


def swap_registers(lines, sites):
    """Swap two raw register numbers throughout a named function's body.

    A site may also be scoped to an instruction RANGE, "FUNC:A-B:LO-HI",
    where LO and HI are inclusive 0-based instruction indices within FUNC
    (a bare "LO" means the single instruction LO, and several windows may
    be given comma-separated: "FUNC:16-20:91,98")
    (same counting convention as --swap-adjacent: directives, labels and
    #-comments are not counted). That is what a register tie-break
    confined to one window needs -- sceVif1PkRefLoadImage picks $a2 for
    an `n ^ qwc` compare temp where the original picks $a1, and swaps the
    two argument moves that bracket it, while every other $a1/$a2 in the
    function is an ordinary ABI argument that must NOT move.

    A pure register-numbering tie-break: for a tiny leaf function with
    only two independent pseudos and no other register pressure (e.g. an
    lq address temp and its loaded value), gcc2.96's allocator sometimes
    numbers the pair in the opposite order the original compiler did --
    no C-level reordering of the source (variable declaration order,
    temp-vs-inline expression, pointer-vs-value) was found to influence
    it (see JNT_getRootTrans/Rotate/Scale, where address and value swap
    $2<->$3 identically regardless of source shape). This is a blunt,
    whole-function-scoped rename: every whole-token $A becomes $B and
    vice versa, so it is only safe where A and B are truly the same kind
    of temp throughout the function (verified against the original
    binary at each call site, same as swap_adjacent/swap_into_slot).

    Both GPRs and FPRs are supported: a site's two halves are register
    TOKENS, "2-3" for $2/$3 and "f4-f6" for $f4/$f6. The rename is
    anchored ($%s\b straight after the dollar), so "2-3" never touches
    $20 or $f2.

    sites: {func: (tokA, tokB)} parsed from "FUNC:A-B" tokens.
    """
    if not sites:
        return lines
    owner = function_at(lines)
    out = []
    idx = 0
    prev_owner = None
    for i, line in enumerate(lines):
        if owner[i] != prev_owner:
            idx = 0
            prev_owner = owner[i]
        is_insn = bool(RE_INSN.match(line))
        entries = sites.get(owner[i]) or ()
        if entries and isinstance(entries[0], str):   # legacy (a, b) value
            entries = [tuple(entries) + (None, None)]
        for entry in entries:
            a, b, lo, hi = entry if len(entry) == 4 else (tuple(entry) + (None, None))
            if lo is not None and not (is_insn and lo <= idx <= hi):
                continue
            ra = re.compile(r'\$%s\b' % re.escape(a))
            rb = re.compile(r'\$%s\b' % re.escape(b))
            line = ra.sub('\x01', line)
            line = rb.sub('$%s' % a, line)
            line = line.replace('\x01', '$%s' % b)
        if is_insn:
            idx += 1
        out.append(line)
    return out


def swap_register_sources(lines, specs):
    """Swap two GPR tokens only in the source operands of named ALU sites.

    Specs are FUNC:N:A-B, with the ordinary 0-based instruction index. This
    covers allocator ties where two address pseudos use the opposite physical
    registers while their destination roles must remain fixed.  Only three-
    register addu/daddu instructions are accepted, and exactly one of A/B
    must occur in the two source operands at every site.
    """
    if not specs:
        return lines
    parsed = {}
    for spec in specs:
        try:
            func, idx_text, regs = spec.split(':')
            idx = int(idx_text)
            a, b = (x.lstrip('$') for x in regs.split('-'))
        except (ValueError, TypeError):
            raise SystemExit("--swap-reg-sources: bad spec %r "
                             "(want FUNC:N:A-B)" % spec)
        if idx < 0 or not all(re.match(r'^\d+$', x) for x in (a, b)):
            raise SystemExit("--swap-reg-sources: bad index/registers in %r" %
                             spec)
        parsed[(func, idx)] = (a, b)

    owner = function_at(lines)
    out = []
    idx = 0
    prev_owner = None
    applied = set()
    alu = re.compile(r'^(\t(?:addu|daddu)\t)(\$\d+),(.+)$')
    for i, line in enumerate(lines):
        if owner[i] != prev_owner:
            idx = 0
            prev_owner = owner[i]
        if RE_INSN.match(line):
            key = (owner[i], idx)
            if key in parsed:
                m = alu.match(line)
                if not m:
                    raise SystemExit("--swap-reg-sources: %s:%d is not an "
                                     "addu/daddu: %s" %
                                     (owner[i], idx, line))
                a, b = parsed[key]
                rest = m.group(3)
                hits = len(re.findall(r'\$(?:%s|%s)\b' %
                                      (re.escape(a), re.escape(b)), rest))
                if hits != 1:
                    raise SystemExit("--swap-reg-sources: %s:%d expected "
                                     "exactly one $%s/$%s source: %s" %
                                     (owner[i], idx, a, b, line))
                rest = re.sub(r'\$%s\b' % re.escape(a), '\x01', rest)
                rest = re.sub(r'\$%s\b' % re.escape(b), '$%s' % a, rest)
                rest = rest.replace('\x01', '$%s' % b)
                line = "%s%s,%s" % (m.group(1), m.group(2), rest)
                applied.add(key)
            idx += 1
        out.append(line)
    missing = set(parsed) - applied
    if missing:
        raise SystemExit("--swap-reg-sources: site(s) not found: %s" %
                         ",".join("%s:%d" % x for x in sorted(missing)))
    return out


def exchange_derived_results(lines, sites):
    """Exchange destinations of adjacent common-base addu/addiu results.

    FUNC:N names an `addu D1,BASE,INDEX` followed immediately by
    `addiu D2,BASE,IMM`, where D2 is BASE. Retail occasionally assigns the
    two derived values to D1/D2 in the opposite order from this compiler.
    The pass keeps both computations intact and emits
    `addiu D1,BASE,IMM; addu D2,BASE,INDEX`; named later-use sites can then
    receive the corresponding allocator rename via --swap-regs.
    """
    if not sites:
        return lines
    owner = function_at(lines)
    out = list(lines)
    idx = 0
    prev_owner = None
    applied = set()
    add = re.compile(r'^\taddu\t(\$\d+),(\$\d+),(\$\d+)(.*)$')
    addi = re.compile(
        r'^\t(addiu|addu)\t(\$\d+),(\$\d+),(-?\d+)(.*)$')
    for i, line in enumerate(lines):
        if owner[i] != prev_owner:
            idx = 0
            prev_owner = owner[i]
        if not RE_INSN.match(line):
            continue
        key = "%s:%d" % (owner[i], idx)
        if key in sites:
            if i + 1 >= len(lines) or not RE_INSN.match(lines[i + 1]):
                raise SystemExit("--exchange-derived-results: %s is not an "
                                 "adjacent instruction pair" % key)
            m1 = add.match(line)
            m2 = addi.match(lines[i + 1])
            if (not m1 or not m2 or m1.group(2) != m2.group(3)
                    or m2.group(2) != m2.group(3)
                    or m1.group(1) == m2.group(2)):
                raise SystemExit("--exchange-derived-results: %s does not "
                                 "match the common-base addu/addiu shape" %
                                 key)
            d1, base, index, suffix1 = m1.groups()
            imm_mnem, _d2, _base2, imm, suffix2 = m2.groups()
            out[i] = "\t%s\t%s,%s,%s%s" % (
                imm_mnem, d1, base, imm, suffix2)
            out[i + 1] = "\taddu\t%s,%s,%s%s" % (
                _d2, base, index, suffix1)
            applied.add(key)
        idx += 1
    missing = set(sites) - applied
    if missing:
        raise SystemExit("--exchange-derived-results: site(s) not found: %s" %
                         ",".join(sorted(missing)))
    return out


def retime_byte_copy_guards(lines, sites):
    """Recover the guarded first-byte copy used before a byte-copy loop.

    FUNC:N names `lb TEST,ADDR; branch TEST; lbu TMP,ADDR; move COPY,TMP;
    move WALK,BASE`. Retail schedules WALK between the signed test load and
    branch, loads directly into COPY in the delay slot, and leaves a nop in
    place of the eliminated TMP copy. The accepted register/address shape is
    exact and any deviation is fatal.
    """
    if not sites:
        return lines
    out = list(lines)
    for site in sorted(sites):
        try:
            func, idx_text = site.split(':')
            start = int(idx_text)
        except (ValueError, TypeError):
            raise SystemExit("--retime-byte-copy-guard: bad site %r "
                             "(want FUNC:N)" % site)
        owner = function_at(out)
        pos = [i for i, line in enumerate(out)
               if owner[i] == func and RE_INSN.match(line)]
        if start < 0 or start + 4 >= len(pos):
            raise SystemExit("--retime-byte-copy-guard: site not found: %s" %
                             site)
        i0, i1, i2, i3, i4 = pos[start:start + 5]
        m0 = re.match(r'^\tlb\t(\$\d+),([^()]+\(\$(\d+)\))(.*)$', out[i0])
        m2 = re.match(r'^\tlbu\t(\$\d+),([^()]+\(\$(\d+)\))(.*)$', out[i2])
        m3 = re.match(r'^\t(?:move\t(\$\d+),(\$\d+)|'
                      r'daddu\t(\$\d+),(\$\d+),\$0)$', out[i3])
        m4 = re.match(r'^\t(?:move\t(\$\d+),(\$\d+)|'
                      r'daddu\t(\$\d+),(\$\d+),\$0)$', out[i4])
        m3_dst, m3_src = ((m3.group(1), m3.group(2)) if m3 and m3.group(1)
                          else (m3.group(3), m3.group(4)) if m3
                          else (None, None))
        m4_dst, m4_src = ((m4.group(1), m4.group(2)) if m4 and m4.group(1)
                          else (m4.group(3), m4.group(4)) if m4
                          else (None, None))
        if (not m0 or not m2 or not m3 or not m4
                or not re.match(r'^\tb(?:eq|ne)\t%s,\$0,' %
                                re.escape(m0.group(1)), out[i1])
                or m0.group(2) != m2.group(2)
                or m2.group(1) != m3_src
                or m4_dst != m2.group(1)
                or m0.group(3) != m4_src.lstrip('$')):
            raise SystemExit("--retime-byte-copy-guard: %s shape changed: "
                             "%r" % (site, [out[i] for i in
                                             (i0, i1, i2, i3, i4)]))
        copy_reg = m3_dst
        out[i2] = "\tlbu\t%s,%s%s" % (copy_reg, m2.group(2), m2.group(4))
        out[i3] = "\tnop"
        walk = out[i4]
        del out[i4]
        out.insert(i0 + 1, walk)
    return out


def split_loop_byte_loads(lines, specs):
    """Split a loop-carried lbu+move into retail's lb test + lbu reload.

    Specs are FUNC:TEST:COPY instruction indices. TEST must be
    `lbu T,ADDR`; COPY must be `move C,T` after a branch that reads T.
    The result is `lb T,ADDR` and `lbu C,ADDR`, preserving byte semantics
    while reproducing the original signed test/unsigned copy lowering.
    """
    if not specs:
        return lines
    out = list(lines)
    for spec in specs:
        try:
            func, test_text, copy_text = spec.split(':')
            test_idx = int(test_text)
            copy_idx = int(copy_text)
        except (ValueError, TypeError):
            raise SystemExit("--split-loop-byte-load: bad spec %r "
                             "(want FUNC:TEST:COPY)" % spec)
        owner = function_at(out)
        pos = [i for i, line in enumerate(out)
               if owner[i] == func and RE_INSN.match(line)]
        if (test_idx < 0 or copy_idx <= test_idx or copy_idx >= len(pos)):
            raise SystemExit("--split-loop-byte-load: bad indices in %s" %
                             spec)
        ti, ci = pos[test_idx], pos[copy_idx]
        mt = re.match(r'^\tlbu\t(\$\d+),(.+)$', out[ti])
        mc = re.match(r'^\t(?:move\t(\$\d+),(\$\d+)|'
                      r'daddu\t(\$\d+),(\$\d+),\$0)$', out[ci])
        mc_dst, mc_src = ((mc.group(1), mc.group(2)) if mc and mc.group(1)
                          else (mc.group(3), mc.group(4)) if mc
                          else (None, None))
        if not mt or not mc or mc_src != mt.group(1):
            raise SystemExit("--split-loop-byte-load: %s shape changed" % spec)
        between = [out[j] for j in range(ti + 1, ci)
                   if RE_INSN.match(out[j])]
        if not any(RE_ANY_JUMP_OR_BRANCH.match(x)
                   and mt.group(1) in insn_regs(x)[1] for x in between):
            raise SystemExit("--split-loop-byte-load: %s has no branch test" %
                             spec)
        out[ti] = out[ti].replace("\tlbu\t", "\tlb\t", 1)
        out[ci] = "\tlbu\t%s,%s" % (mc_dst, mt.group(2))
    return out


def coalesce_symbol_addresses(lines, specs):
    """Coalesce a split %hi/%lo symbol address into one named GPR.

    Specs are FUNC:HI:LO:REG instruction indices. HI must be a `lui` of
    `%hi(SYM)` and LO an `addiu` using that result with `%lo(SYM)`.  The
    old GCC allocator occasionally keeps both halves in one GPR while a
    rebuild introduces a second destination.  This pass changes only those
    three register fields, rejects intervening uses of either GPR, and
    requires the two relocation symbols to agree.
    """
    if not specs:
        return lines
    out = list(lines)
    for spec in specs:
        try:
            func, hi_text, lo_text, reg_text = spec.split(':')
            hi_idx = int(hi_text)
            lo_idx = int(lo_text)
            new_reg = '$' + reg_text.lstrip('$')
        except (ValueError, TypeError):
            raise SystemExit("--coalesce-symbol-address: bad spec %r "
                             "(want FUNC:HI:LO:REG)" % spec)
        owner = function_at(out)
        pos = [i for i, line in enumerate(out)
               if owner[i] == func and RE_INSN.match(line)]
        if hi_idx < 0 or lo_idx <= hi_idx or lo_idx >= len(pos):
            raise SystemExit("--coalesce-symbol-address: bad indices in %s" %
                             spec)
        hi, lo = pos[hi_idx], pos[lo_idx]
        mh = re.match(r'^\tlui\t(\$\d+),%hi\(([^)]+)\)(.*)$', out[hi])
        ml = re.match(r'^\taddiu\t(\$\d+),(\$\d+),%lo\(([^)]+)\)(.*)$',
                      out[lo])
        if (not mh or not ml or ml.group(2) != mh.group(1)
                or ml.group(3) != mh.group(2) or new_reg == mh.group(1)):
            raise SystemExit("--coalesce-symbol-address: %s shape changed: "
                             "%r / %r" % (spec, out[hi], out[lo]))
        old_reg = mh.group(1)
        for i in pos[hi_idx + 1:lo_idx]:
            writes, reads = insn_regs(out[i])
            if old_reg in writes | reads or new_reg in writes | reads:
                raise SystemExit("--coalesce-symbol-address: %s has an "
                                 "intervening register use" % spec)
        out[hi] = "\tlui\t%s,%%hi(%s)%s" % (new_reg, mh.group(2),
                                              mh.group(3))
        out[lo] = "\taddiu\t%s,%s,%%lo(%s)%s" % (new_reg, new_reg,
                                                   mh.group(2), ml.group(4))
    return out


def swap_fp_commutative_operands(lines, sites):
    """Swap fs/ft at explicitly named add.s or mul.s instruction sites.

    Old GCC canonicalizes a commutative floating expression's two source
    operands independently of the source spelling.  A rebuilt instruction can
    consequently be semantically identical to retail while differing only in
    the fs/ft encoding.  Sites use the ordinary FUNC:N convention, where N is
    the 0-based instruction index and directives/labels do not count.

    This pass deliberately accepts only add.s and mul.s.  It never changes the
    destination, opcode, or instruction order, and it fails loudly if a named
    site is not one of those two exact three-FPR forms.
    """
    if not sites:
        return lines
    owner = function_at(lines)
    out = []
    idx = 0
    prev_owner = None
    applied = set()
    commutative = re.compile(
        r'^(\t(?:add|mul)\.s\t)(\$f\d+)\s*,\s*(\$f\d+)\s*,\s*'
        r'(\$f\d+)(.*)$')
    for i, line in enumerate(lines):
        if owner[i] != prev_owner:
            idx = 0
            prev_owner = owner[i]
        is_insn = bool(RE_INSN.match(line))
        if is_insn:
            key = "%s:%d" % (owner[i], idx)
            if key in sites:
                m = commutative.match(line)
                if not m:
                    raise SystemExit(
                        "--swap-fp-operands: %s is not a three-register "
                        "add.s or mul.s instruction: %s" % (key, line))
                prefix, fd, fs, ft, suffix = m.groups()
                line = "%s%s,%s,%s%s" % (prefix, fd, ft, fs, suffix)
                applied.add(key)
            idx += 1
        out.append(line)
    missing = set(sites) - applied
    if missing:
        raise SystemExit("--swap-fp-operands: site(s) not found: %s" %
                         ",".join(sorted(missing)))
    return out


def swap_int_commutative_operands(lines, sites):
    """Swap rs/rt at named three-GPR commutative instruction sites.

    This is the integer counterpart of --swap-fp-operands.  It accepts only
    addu/daddu/and/or/xor/nor, never changes the destination or opcode, and
    uses the same 0-based FUNC:N instruction indexing convention.
    """
    if not sites:
        return lines
    owner = function_at(lines)
    out = []
    idx = 0
    prev_owner = None
    applied = set()
    commutative = re.compile(
        r'^(\t(?:addu|daddu|and|or|xor|nor)\t)(\$\d+)\s*,\s*'
        r'(\$\d+)\s*,\s*(\$\d+)(.*)$')
    for i, line in enumerate(lines):
        if owner[i] != prev_owner:
            idx = 0
            prev_owner = owner[i]
        is_insn = bool(RE_INSN.match(line))
        if is_insn:
            key = "%s:%d" % (owner[i], idx)
            if key in sites:
                m = commutative.match(line)
                if not m:
                    raise SystemExit(
                        "--swap-int-operands: %s is not an accepted "
                        "three-register commutative instruction: %s" %
                        (key, line))
                prefix, rd, rs, rt, suffix = m.groups()
                line = "%s%s,%s,%s%s" % (prefix, rd, rt, rs, suffix)
                applied.add(key)
            idx += 1
        out.append(line)
    missing = set(sites) - applied
    if missing:
        raise SystemExit("--swap-int-operands: site(s) not found: %s" %
                         ",".join(sorted(missing)))
    return out


def rematerialize_call_constants(lines, specs):
    """Undo one over-aggressive constant live range across a call.

    GCC can assign a literal used on both sides of a call to a callee-saved
    register.  Retail occasionally rematerializes the literal after the call
    instead, avoiding that register's save/restore pair.  Each strict spec is
    FUNC:SAVED-CALLER-CARRY:VALUE.  The accepted shape has exactly one literal
    definition, two comparison uses separated by a jal, and one stack
    save/restore pair for SAVED.  CALLER's prior value at the first comparison
    is moved to CARRY before the new literal takes CALLER; the second
    comparison's other operand is moved to the first comparison's other
    register before a new `li CALLER,VALUE` is inserted.

    The pass also moves the return-address save/restore into the vacated stack
    slot and updates .mask.  Any deviation from this shape is a hard error, so
    a source edit cannot make the correction silently target unrelated code.
    """
    if not specs:
        return lines
    for spec in specs:
        try:
            func, regs, value_text = spec.split(':')
            saved_text, caller_text, carry_text = regs.split('-')
            saved = int(saved_text.lstrip('$'))
            caller = int(caller_text.lstrip('$'))
            carry = int(carry_text.lstrip('$'))
            value = int(value_text, 0)
        except (ValueError, TypeError):
            raise SystemExit(
                "--remat-call-constant: bad spec %r "
                "(want FUNC:SAVED-CALLER-CARRY:VALUE)" % spec)
        if not (16 <= saved <= 23 and 2 <= caller <= 15
                and 2 <= carry <= 15 and caller != carry):
            raise SystemExit(
                "--remat-call-constant: %s must name a callee-saved GPR "
                "and a caller-saved GPR" % spec)

        start = next((i for i, line in enumerate(lines)
                      if re.match(r'\s*\.ent\s+%s\s*$' % re.escape(func),
                                  line)), None)
        if start is None:
            raise SystemExit("--remat-call-constant: function not found: %s" %
                             func)
        end = next((i for i in range(start + 1, len(lines))
                    if re.match(r'\s*\.end\s+%s\s*$' % re.escape(func),
                                lines[i])), None)
        if end is None:
            raise SystemExit("--remat-call-constant: missing .end for %s" %
                             func)

        rs = r'\$%d\b' % saved
        rc = r'\$%d\b' % caller
        save_re = re.compile(r'^\tsd\t\$%d,(-?\d+)\(\$sp\)$' % saved)
        restore_re = re.compile(r'^\tld\t\$%d,(-?\d+)\(\$sp\)$' % saved)
        li_re = re.compile(r'^\tli\t\$%d,%d(?:\s|$)' % (saved, value))
        branch_re = re.compile(
            r'^\t(beq|bne|beql|bnel)\t(\$\d+),(\$\d+),(.+)$')

        save_sites = [(i, save_re.match(lines[i]))
                      for i in range(start, end) if save_re.match(lines[i])]
        restore_sites = [(i, restore_re.match(lines[i]))
                         for i in range(start, end)
                         if restore_re.match(lines[i])]
        li_sites = [i for i in range(start, end) if li_re.match(lines[i])]
        branch_sites = []
        for i in range(start, end):
            m = branch_re.match(lines[i])
            if m and re.search(rs, lines[i]):
                branch_sites.append((i, m))
        if (len(save_sites), len(restore_sites), len(li_sites),
                len(branch_sites)) != (1, 1, 1, 2):
            raise SystemExit(
                "--remat-call-constant: %s shape changed "
                "(save=%d restore=%d li=%d branches=%d)" %
                (func, len(save_sites), len(restore_sites), len(li_sites),
                 len(branch_sites)))

        save_i, save_m = save_sites[0]
        restore_i, restore_m = restore_sites[0]
        save_off = int(save_m.group(1))
        if int(restore_m.group(1)) != save_off:
            raise SystemExit("--remat-call-constant: %s save/restore offsets "
                             "differ" % func)
        first_i, first_m = branch_sites[0]
        second_i, second_m = branch_sites[1]
        calls = [i for i in range(first_i + 1, second_i)
                 if re.match(r'^\tjal\t', lines[i])]
        if len(calls) != 1:
            raise SystemExit("--remat-call-constant: %s expected exactly one "
                             "jal between comparisons" % func)

        first_regs = (first_m.group(2), first_m.group(3))
        saved_reg = "$%d" % saved
        caller_reg = "$%d" % caller
        carry_reg = "$%d" % carry
        if first_regs.count(saved_reg) != 1:
            raise SystemExit("--remat-call-constant: %s first comparison "
                             "does not use saved register once" % func)
        other_reg = first_regs[0] if first_regs[1] == saved_reg else first_regs[1]
        second_regs = (second_m.group(2), second_m.group(3))
        if (second_regs.count(saved_reg) != 1
                or second_regs.count(caller_reg) != 1):
            raise SystemExit("--remat-call-constant: %s second comparison "
                             "must use saved and caller registers" % func)

        defining = []
        dest_re = re.compile(r'^\t([a-z0-9.]+)\t\$%d,' % caller)
        for i in range(calls[0] + 1, second_i):
            if dest_re.match(lines[i]):
                defining.append(i)
        if not defining:
            raise SystemExit("--remat-call-constant: %s expected a post-call "
                             "definition of caller register" % func)
        # Earlier definitions may feed unrelated comparisons.  The final one
        # before the constant comparison is the value live at that branch.
        def_i = defining[-1]

        # CALLER already carries the first comparison's independent value.
        # Move its final pre-literal definition and the branch delay-slot use
        # together to CARRY before assigning the rematerialized constant.
        prior_defs = []
        prior_dest_re = re.compile(r'^\t([a-z0-9.]+)\t\$%d,' % caller)
        for i in range(start, li_sites[0]):
            if prior_dest_re.match(lines[i]):
                prior_defs.append(i)
        if not prior_defs:
            raise SystemExit("--remat-call-constant: %s expected a caller-"
                             "register definition before the literal" % func)
        prior_def_i = prior_defs[-1]
        first_slot_i = next((i for i in range(first_i + 1, end)
                             if RE_INSN.match(lines[i])), None)
        if first_slot_i is None or not re.search(rc, lines[first_slot_i]):
            raise SystemExit("--remat-call-constant: %s first comparison's "
                             "delay slot does not use caller register" % func)

        frame_m = next((re.match(r'^\t\.frame\t\$sp,(\d+),\$31', lines[i])
                        for i in range(start, end)
                        if re.match(r'^\t\.frame\t\$sp,(\d+),\$31', lines[i])),
                       None)
        mask_i = next((i for i in range(start, end)
                       if re.match(r'^\t\.mask\t0x[0-9A-Fa-f]+,-?\d+$',
                                   lines[i])), None)
        if frame_m is None or mask_i is None:
            raise SystemExit("--remat-call-constant: %s missing frame/mask" %
                             func)
        frame_size = int(frame_m.group(1))
        mask_m = re.match(r'^(\t\.mask\t)0x([0-9A-Fa-f]+),-?\d+$',
                          lines[mask_i])
        mask = int(mask_m.group(2), 16)
        if not (mask & (1 << saved)):
            raise SystemExit("--remat-call-constant: %s .mask does not save "
                             "$%d" % (func, saved))

        ra_save = [i for i in range(start, end)
                   if re.match(r'^\tsd\t\$31,-?\d+\(\$sp\)$', lines[i])]
        ra_restore = [i for i in range(start, end)
                      if re.match(r'^\tld\t\$31,-?\d+\(\$sp\)$', lines[i])]
        if len(ra_save) != 1 or len(ra_restore) != 1:
            raise SystemExit("--remat-call-constant: %s return-address stack "
                             "shape changed" % func)

        replacements = {}
        replacements[prior_def_i] = re.sub(rc, carry_reg,
                                            lines[prior_def_i], count=1)
        replacements[first_slot_i] = re.sub(rc, carry_reg,
                                             lines[first_slot_i])
        replacements[li_sites[0]] = re.sub(rs, caller_reg,
                                            lines[li_sites[0]])
        replacements[first_i] = re.sub(rs, caller_reg, lines[first_i])
        replacements[def_i] = re.sub(rc, other_reg, lines[def_i], count=1)
        second = lines[second_i]
        second = re.sub(rc, '\x01', second)
        second = re.sub(rs, caller_reg, second)
        second = second.replace('\x01', other_reg)
        replacements[second_i] = second
        replacements[ra_save[0]] = re.sub(r',-?\d+\(\$sp\)$',
                                           ',%d($sp)' % save_off,
                                           lines[ra_save[0]])
        replacements[ra_restore[0]] = re.sub(r',-?\d+\(\$sp\)$',
                                              ',%d($sp)' % save_off,
                                              lines[ra_restore[0]])
        mask &= ~(1 << saved)
        replacements[mask_i] = "%s0x%08X,%d" % (
            mask_m.group(1), mask, save_off - frame_size)

        rebuilt = []
        for i, line in enumerate(lines):
            if i in (save_i, restore_i):
                continue
            if i == second_i:
                rebuilt.append("\tli\t%s,%d" % (caller_reg, value))
            rebuilt.append(replacements.get(i, line))
        lines = rebuilt
    return lines


def unfill_gcc_slots(flat, scope, owner_of):
    """Rewrite gcc's own delay-slot fills back to hoisted-insn + nop.

    gcc -fdelayed-branch emits
        .set noreorder / .set nomacro / <branch> / <slot> / .set macro
        / .set reorder
    The original compiler build declines to fill some of these; with
    this pass the slot instruction is moved ABOVE the branch and a
    literal nop takes its place. Safe because gcc only fills a slot
    with an instruction that neither feeds the branch condition nor
    depends on it.

    Scope entries are either a bare FUNC (every gcc-filled slot in that
    function -- the original blanket behaviour) or a FUNC:N SITE, where
    N is the 0-based index of the gcc-filled slot within FUNC, counted
    over the emitted asm in order.

    Per-site is the useful mode and blanket is usually not: the original
    build FILLS most of these slots and declines only a handful, so
    unfilling all of them is far worse than unfilling none. _vfprintf_r
    is the standing example -- it needs exactly the isinf/isnan result
    tests unfilled and everything else left alone.

    The index is the dual of pin_slot_nops': that pass counts
    REORDER-mode branches (the ones gas fills), this one counts
    NOREORDER-mode branch blocks (the ones gcc already filled), so a
    branch is only ever countable by one of the two. Both are 0-based
    and in asm order, and because gcc wraps these blocks in
    `.set nomacro` no macro can expand inside one -- so for this pass
    asm order is also binary order, and a site index read off a
    disassembly is valid.
    """
    res = []
    i = 0
    counts = {}
    while i < len(flat):
        if flat[i].startswith("\t.ent\t"):
            counts[flat[i].split("\t")[-1]] = 0
        if (flat[i].strip() == ".set\tnoreorder" or
                flat[i].strip() == ".set noreorder") and i + 3 < len(flat):
            block = [x.strip().replace("\t", " ") for x in flat[i:i+4]]
            if (block[1].startswith(".set") and "nomacro" in block[1]
                    and RE_ANY_BRANCH.match(flat[i+2])
                    and flat[i+3].startswith("\t")
                    and not flat[i+3].strip().startswith(".")):
                owner = owner_of(i)
                n = counts.get(owner, 0)
                counts[owner] = n + 1
                if not scope or owner in scope or f"{owner}:{n}" in scope:
                    res.append(flat[i+3])      # hoisted slot insn
                    res.append(flat[i])        # .set noreorder
                    res.append(flat[i+1])      # .set nomacro
                    res.append(flat[i+2])      # branch
                    res.append("\tnop")
                    i += 4
                    continue
        res.append(flat[i])
        i += 1
    return res


RE_PIN_BRANCH = re.compile(
    r"^\t(b|beq|bne|beqz|bnez|blez|bgtz|bltz|bgez"
    r"|beql|bnel|beqzl|bnezl|blezl|bgtzl|bltzl|bgezl|j|jal)[ \t]")


def pin_slot_nops(flat, sites):
    """Pin a literal nop into an explicitly named branch's delay slot.

    The original build leaves some reorder-mode branch/call delay slots
    as literal nops where our gas pulls the preceding instruction in
    (e.g. _vfprintf_r's `ld $4,504($sp)` staying ABOVE `jal isnan`).
    Site-keyed like flip_branch_likely: FUNC:N names the Nth
    reorder-mode branch/jump of FUNC (0-based, asm order); the branch is
    wrapped in .set noreorder with an explicit nop so gas cannot fill
    the slot.  Branches already inside noreorder blocks (gcc-filled
    slots) are not counted and not eligible.
    """
    res = []
    cur = None
    idx = 0
    noreorder = False
    for line in flat:
        if line.startswith("\t.ent\t"):
            cur = line.split("\t")[-1]
            idx = 0
            noreorder = False
        s = line.strip().replace("\t", " ")
        if s == ".set noreorder":
            noreorder = True
        elif s in (".set reorder", ".set pop"):
            noreorder = False
        if not noreorder and RE_PIN_BRANCH.match(line):
            if f"{cur}:{idx}" in sites:
                res.append("\t.set\tnoreorder")
                res.append(line)
                res.append("\tnop")
                res.append("\t.set\treorder")
                idx += 1
                continue
            idx += 1
        res.append(line)
    return res


RE_WAR_ALU = re.compile(r"^\t(?:daddu|addu|move|or)\t(\$\w+),(.*)$")
RE_SP_RESTORE = re.compile(r"^\tld\t(\$\w+),\d+\(\$sp\)$")


def war_restore_swap(flat, scope):
    """Swap two adjacent epilogue register restores after a WAR hazard.

    The original build's second scheduler delays an epilogue `ld $A,
    off($sp)` by one slot when the immediately preceding instruction
    still READS $A (a write-after-read dependence, e.g. `move $v0,$s0`
    setting up the return value right before `ld $s0`), letting the next
    independent restore go first.  Our gcc keeps the prologue's source
    order instead.  Opt-in and function-scoped like the other passes:
    the pattern (reader of $A / ld $A,off($sp) / ld $B,off2($sp) with
    $B uninvolved) is swapped to (reader / ld $B / ld $A).  Seen in
    gcc_frame.c's __deregister_frame_info found-path epilogue.
    """
    res = []
    cur = None
    i = 0
    while i < len(flat):
        line = flat[i]
        if line.startswith("\t.ent\t"):
            cur = line.split("\t")[-1]
        if in_scope(cur, scope) and i + 2 < len(flat):
            m0 = RE_WAR_ALU.match(line)
            m1 = RE_SP_RESTORE.match(flat[i + 1])
            m2 = RE_SP_RESTORE.match(flat[i + 2])
            if (m0 and m1 and m2
                    and m1.group(1) != m2.group(1)
                    and m1.group(1) in m0.group(2)
                    and m2.group(1) not in line):
                res.append(line)
                res.append(flat[i + 2])
                res.append(flat[i + 1])
                i += 3
                continue
        res.append(line)
        i += 1
    return res


RE_INSN = re.compile(r'^\t[a-z]')
RE_ANY_JUMP_OR_BRANCH = re.compile(
    r'^\t(b|beq|bne|beqz|bnez|blez|bgtz|bltz|bgez'
    r'|beql|bnel|beqzl|bnezl|blezl|bgtzl|bltzl|bgezl'
    r'|bc1t|bc1f|bc1tl|bc1fl|j|jr|jal|jalr)[ \t]')
RE_MEMOP = re.compile(
    r'^\t(lw|lh|lb|lbu|lhu|lwu|ld|lq|lwc1|l\.s|ldc1'
    r'|sw|sh|sb|sd|sq|swc1|s\.s|sdc1)[ \t]')


def insn_regs(line):
    """(writes, reads) register sets for a simple instruction line.

    Conservative classifier used by the swap passes' independence check:
    stores and branches read every register operand; otherwise the first
    operand is the destination and the rest are sources. Register names
    are taken literally ($2 vs $v0 never mix within one gcc emission)."""
    m = re.match(r'^\t([a-z0-9.]+)\t(.*)$', line)
    if not m:
        return set(), set()
    mnem, rest = m.group(1), m.group(2)
    ops = [o.strip() for o in rest.split(',')]
    regs_of = lambda s: set(re.findall(r'\$\w+', s))
    if RE_STORE.match(line) or mnem in ('sq', 'sdc1') or mnem.startswith(('b', 'j')) or mnem.startswith('c.'):
        return set(), regs_of(rest)
    writes = regs_of(ops[0]) if ops else set()
    reads = regs_of(','.join(ops[1:]))
    # a load/store's base register inside (...) of operand 0 is a read
    mem = re.search(r'\(([^)]*)\)', ops[0]) if ops else None
    if mem:
        writes -= regs_of(mem.group(0))
        reads |= regs_of(mem.group(0))
    return writes, reads


def swap_ok(a, b):
    """True if adjacent instructions a/b may exchange positions.

    Conservative: both must be plain non-branch instructions, at most one
    may touch memory, and neither may write a register the other reads or
    writes (nor read one the other writes)."""
    if not (RE_INSN.match(a) and RE_INSN.match(b)):
        return False
    if RE_ANY_JUMP_OR_BRANCH.match(a) or RE_ANY_JUMP_OR_BRANCH.match(b):
        return False
    if RE_MEMOP.match(a) and RE_MEMOP.match(b):
        return False
    wa, ra = insn_regs(a)
    wb, rb = insn_regs(b)
    return not (wa & (rb | wb)) and not (wb & ra)



def _is_empty_asm_marker(line):
    """True for the marker/blank lines of an inline-asm block that emits
    nothing (`#APP`, `#NO_APP`, and the empty body between them)."""
    t = line.strip()
    return t in ("#APP", "#NO_APP", "")


def rotate_insns(flat, sites, allow_nop_marker=False):
    """Rotate a short window of instructions right by one.

    Site-keyed FUNC:N[:LEN] -- the window is the LEN instructions (LEN
    defaults to 3) starting at the Nth instruction of FUNC, 0-based in
    emission order, counted exactly like --swap-adjacent (directives,
    labels and #-comments are not instructions). The window becomes
    (last, first, ..., second-to-last).

    A NEGATIVE LEN rotates the |LEN|-instruction window the other way --
    (second, ..., last, first) -- which a right rotation cannot express
    with a single non-overlapping window (it would need |LEN|-1 of them,
    and the site index advances past the whole window once one fires).
    MenuTecL1R1Main:44:-4 is the motivating case: gcc gives the trailing
    `li 2` of a state store top scheduling priority (its consumer is the
    branch delay slot at the very end of the block) and issues it before
    the three constants the retail build put first.

    Why this exists: --swap-adjacent can only express independent
    2-element transpositions -- its sites are resolved against the
    pre-swap order and consumed two at a time, so a 3-element rotation
    (which needs two DEPENDENT swaps) is not expressible with it. The
    original build's scheduler produced exactly such rotations, e.g.
    tskUmnSimulationInfo wants an argument-setup addu hoisted above the
    lui/mtc1 pair that materializes a float constant. The rotated bytes
    are verified against the original binary, so the ordering is the
    original's by definition.

    The window must be contiguous instruction lines (no label or
    directive between them); otherwise the site is left untouched.
    """
    starts = {}
    for site in sites:
        parts = site.split(":")
        if len(parts) == 2:
            starts[(parts[0], int(parts[1]))] = 3
        elif len(parts) == 3:
            starts[(parts[0], int(parts[1]))] = int(parts[2])
    res = []
    cur = None
    idx = 0
    i = 0
    while i < len(flat):
        line = flat[i]
        if line.startswith("\t.ent\t"):
            cur = line.split("\t")[-1]
            idx = 0
        if RE_INSN.match(line):
            span = starts.get((cur, idx))
            if span and abs(span) >= 2:
                window, skipped, j = [], [], i
                while j < len(flat) and len(window) < abs(span):
                    w = flat[j]
                    if w.strip() == ".set push":
                        # A `.set push ... .set pop` block is ONE unit:
                        # fix_cc_asm wraps macro expansions (li.s, la, a
                        # float store to a far offset) in one to control
                        # how gas expands them, and the wrapper is part of
                        # the instruction's meaning -- rotating the
                        # instruction out of its wrapper would change the
                        # encoding. Before this, any window touching such
                        # a block was silently declined and the flag
                        # looked like it did nothing.
                        k = j
                        while k < len(flat) and flat[k].strip() != ".set pop":
                            k += 1
                        if k >= len(flat):
                            break
                        window.append("\n".join(flat[j:k + 1]))
                        j = k + 1
                        continue
                    if RE_INSN.match(w):
                        window.append(w)
                    elif (_is_empty_asm_marker(w)
                          or (allow_nop_marker and w.strip() == "#nop")):
                        # An empty #APP/#NO_APP block -- LAUNDER, LAUNDER_V,
                        # SCHED_NOP's siblings -- emits no bytes, so it is a
                        # textual boundary, not a real one. Carry it along
                        # instead of refusing the site. A block containing
                        # any instruction still stops the window.
                        skipped.append(w)
                    else:
                        break
                    j += 1
                if len(window) == abs(span):
                    if span < 0:
                        order = window[1:] + [window[0]]
                    else:
                        order = [window[-1]] + window[:-1]
                    for unit in order:
                        res.extend(unit.split("\n"))
                    res.extend(skipped)
                    idx += abs(span)
                    i = j
                    continue
            idx += 1
        res.append(line)
        i += 1
    return res


def short_loop_pads(flat, sites):
    """Set the nop count of a gcc R5900 short-loop pad.

    Site-keyed FUNC:N:COUNT -- N is the 0-based index of the pad block
    within FUNC (emission order), COUNT the number of nops it should
    contain.

    gcc pads short loops on the R5900 by emitting

        .set noreorder
        nop            (zero or more)
        .set reorder

    and it pads to a fixed TOTAL loop size, not a fixed nop count. This
    toolchain pads to 8; the original build padded to 10. No source
    shape moves it -- while/do-while/for/goto, char vs u_char, named
    temp vs direct compare were all swept, `-m5900` and
    `-falign-loops=` do nothing, and SCHED_NOP() lands before the
    closing load rather than between the load and the branch. It is a
    property of the compiler's target description, so the honest fix is
    to say what the original emitted.

    This pass is deliberately narrow: it only rewrites a noreorder block
    whose entire contents are nops, so it can never move, drop or
    reorder a real instruction. A block containing anything else is left
    exactly as it was.

    Functions blocked on this when it was written: FileSelectListReload,
    xglCdStreamOpen, xglMcSetMapName (~1KB together) and the last two
    words of __ieee754_pow.
    """
    if not sites:
        return flat
    want = {}
    for site in sites:
        parts = site.split(":")
        if len(parts) == 3:
            want[(parts[0], int(parts[1]))] = int(parts[2])
    if not want:
        return flat

    owner = function_at(flat)
    out = []
    i = 0
    idx = 0
    cur = None
    while i < len(flat):
        line = flat[i]
        if line.startswith("\t.ent\t"):
            cur = line.split("\t")[-1]
            idx = 0
        if line.strip() == ".set\tnoreorder" or line.strip() == ".set noreorder":
            # collect through the matching .set reorder
            j = i + 1
            body = []
            while j < len(flat):
                t = flat[j].strip()
                if t in (".set\treorder", ".set reorder"):
                    break
                body.append(flat[j])
                j += 1
            if j < len(flat) and all(b.strip() in ("nop", "") for b in body):
                n = want.get((owner[i], idx))
                idx += 1
                if n is not None:
                    out.append(line)
                    out.extend(["\tnop"] * n)
                    out.append(flat[j])
                    i = j + 1
                    continue
        out.append(line)
        i += 1
    return out


def byte_move_andi(flat, scope):
    """`daddu $A,$B,$0` right after `lbu $B,...`  ->  `andi $A,$B,0xff`.

    ee-gcc 2.96 sometimes widens a just-loaded byte with a plain register
    copy where the original build emitted the zero-extend explicitly.
    Both write the same bits: the operand is the destination of the
    immediately preceding `lbu`, so its upper 24 bits are zero by
    construction and masking them is a no-op. No source spelling reaches
    it -- a u_char temp, an int temp, `& 0xff`, a cast on the load and a
    cast on the compare all leave the copy (found on xglMcSetMapName,
    whose two EUC scan loops differ from each other in the original by
    exactly this: the first zero-extends, the second copies).

    Deliberately narrow: the rewrite fires only when the previous
    emitted instruction is an `lbu` into the copy's SOURCE register, so
    it can never mask a value whose high bits matter.
    """
    if scope is None:
        return flat
    whole = set()
    sites = set()
    for ent in scope:
        f, sep, n = ent.partition(":")
        if sep:
            sites.add((f, int(n)))
        else:
            whole.add(f)
    owner = function_at(flat)
    out = list(flat)
    prev = None
    seen = {}
    for i, line in enumerate(flat):
        stripped = line.strip()
        m = re.match(r'^\tdaddu\t(\$[0-9a-z]+),(\$[0-9a-z]+),\$0\s*$', line)
        if m and prev is not None:
            pm = re.match(r'^\tlbu\t(\$[0-9a-z]+),', prev)
            if pm and pm.group(1) == m.group(2):
                f = owner[i]
                n = seen.get(f, 0)
                seen[f] = n + 1
                if (not scope) or f in whole or (f, n) in sites:
                    out[i] = "\tandi\t%s,%s,0xff" % (m.group(1), m.group(2))
        if stripped and not stripped.startswith(('.', '#')) and not stripped.endswith(':'):
            prev = line
    return out


def zero_quad_stores(flat, scope):
    """`por $X,$0,$0` + `sq $X,d(b)`  ->  `nop` + `sq $0,d(b)`.

    gcc 2.9x always materializes a TI-mode zero into a register before
    storing it, because its TImode move pattern has no "store the zero
    register" alternative. The original build stored `$0` directly and
    padded with a nop, so the two differ by exactly two words in every
    quadword-clearing function (Java_xeno_Camera_setPointLightReset,
    Java_xeno_Camera_resetFog, the nmlModelGetGblPosition family).

    `$0` is hardwired zero, so substituting it for a register the `por`
    just zeroed is semantics-preserving by construction -- this pass
    cannot change what the code computes, only which encoding says it.

    Conservative: the scan follows $X only to the end of its live range
    -- the first instruction that WRITES $X starts a different value and
    stops the scan -- and rewrites the `por` only when every read of $X
    before that point is the source operand of an `sq`. Any other read
    (or no store at all) leaves the site alone. Reusing $X as an ordinary
    temp later in the function, which gcc does routinely, is therefore
    not a veto.
    """
    if not scope:
        return flat
    owner = function_at(flat)
    out = list(flat)
    # gcc emits these two with a SPACE after the mnemonic, not the tab it
    # uses for most instructions -- accept either.
    por = re.compile(r'^\t(por)[ \t](\$\w+),\$0,\$0$')
    for i, line in enumerate(flat):
        if not in_scope(owner[i], scope):
            continue
        m = por.match(line.rstrip())
        if not m:
            continue
        reg = m.group(2)
        ref = re.compile(r'%s\b' % re.escape(reg))
        stores = []
        ok = True
        for j in range(i + 1, len(flat)):
            if owner[j] != owner[i]:
                break
            nxt = flat[j]
            if not RE_INSN.match(nxt) or not ref.search(nxt):
                continue
            sq = re.match(r'^\t(sq)[ \t](\$\w+),(.*)$', nxt.rstrip())
            if sq and sq.group(2) == reg and not ref.search(sq.group(3)):
                stores.append(j)      # a use we can retarget to $0
                continue
            writes, reads = insn_regs(nxt.replace(' ', '\t', 1))
            if reg in writes and reg not in reads:
                break                 # live range ends; earlier reads stand
            ok = False                # a read we cannot retarget
            break
        if ok and stores:
            out[i] = "\tnop"
            for j in stores:
                out[j] = re.sub(r'^(\tsq[ \t])%s,' % re.escape(reg),
                                r'\g<1>$0,', out[j])
    return out


def rotate_insns_seq(flat, site_list, allow_nop_marker=False):
    """Apply --rotate sites ONE AT A TIME, re-resolving indices each time.

    rotate_insns() resolves every site against a single scan of the
    pre-rotation stream and steps the index past a window once it fires,
    so two windows that OVERLAP -- or a second window whose position only
    exists after the first has moved things -- cannot both be expressed.
    Chaining the rotations fixes that: each site is resolved against the
    output of the previous one, exactly as if the pass had been invoked
    once per site.

    The cost is that a site's index means "after the earlier sites in
    this list have been applied", so ORDER MATTERS and the list is not
    commutative. Use plain --rotate for independent windows; reach for
    this only when the windows overlap (JS_loadClass and
    Java_xeno_Chr_setPeer are the motivating cases -- both need two
    windows sharing instructions).
    """
    for site in site_list:
        flat = "\n".join(rotate_insns(
            flat, [site], allow_nop_marker=allow_nop_marker)).split("\n")
    return flat


def hoist_div_call_arg(flat, scope):
    """Hoist an independent fifth call argument above GCC's div-zero guard.

    GCC 2.96 schedules ``move $t0,$sp`` after the guarded remainder
    calculation used to form a following five-argument call.  The retail
    build schedules that call setup before the calculation.  The intervening
    branch/break/label directives prevent the ordinary short-window rotation
    pass from expressing the otherwise pure reorder.

    Keep this deliberately narrow: only the exact R5900 signed-remainder
    guard followed by ``addu $v0,$v0,$s0`` and ``move $t0,$sp`` is eligible,
    and callers must opt in by function name.  audit_swaps.py recompiles
    without the pass and verifies an identical instruction multiset.
    """
    if not scope:
        return flat
    owner = function_at(flat)
    out = list(flat)
    i = 7
    while i < len(out):
        if owner[i] not in scope or out[i].strip() != "daddu\t$8,$sp,$0":
            i += 1
            continue
        pattern = [
            r'^\taddu\t\$2,\$2,\$4$',
            r'^\t\.set\tnoreorder$',
            r'^\tbeql\t\$4,\$0,1f$',
            r'^\tbreak\t0,7$',
            r'^1:$',
            r'^\t\.set\treorder$',
            r'^\taddu\t\$2,\$2,\$16$',
        ]
        start = i - len(pattern)
        if all(re.match(rx, out[start + n])
               for n, rx in enumerate(pattern)):
            arg = out.pop(i)
            out.insert(start, arg)
            # Ownership is unchanged within the function; skip this site.
            i += 1
            continue
        i += 1
    return out


def retime_branch_slot(flat, scope):
    """Move a pre-branch store into a plain branch's filled delay slot.

    At an opted-in site GCC may emit ``store; bne / move`` while the retail
    scheduler emitted ``bne / store; lui; move``.  The move is call setup on
    the fall-through path, so executing it on the taken path is dead; the
    retail order both avoids that dead work and uses the independent store as
    the unconditional delay-slot instruction.

    This only accepts the exact GCC noreorder block, a preceding store, a
    register-copy slot, and a following ``lui`` immediately before another
    noreorder block.  audit_swaps.py proves the unmodified build has the same
    instruction multiset.
    """
    if not scope:
        return flat
    out = list(flat)
    i = 1
    while i + 7 < len(out):
        owner = function_at(out)
        if owner[i] not in scope:
            i += 1
            continue
        block = [x.strip().replace("\t", " ") for x in out[i:i + 6]]
        pre = out[i - 1]
        slot = out[i + 3]
        if not (block[0] == ".set noreorder"
                and block[1] == ".set nomacro"
                and re.match(r'^bne \$\w+,\$0,', block[2])
                and block[4] == ".set macro"
                and block[5] == ".set reorder"
                and RE_STORE.match(pre)
                and RE_RETURN_MOVE.match(slot)):
            i += 1
            continue
        j = i + 6
        while j < len(out) and not RE_INSN.match(out[j]):
            j += 1
        if (j >= len(out) or not out[j].startswith("\tlui\t")
                or j + 1 >= len(out)
                or out[j + 1].strip().replace("\t", " ") != ".set noreorder"):
            i += 1
            continue
        # Drop the pre-branch store, use it as the delay slot, and put the
        # displaced fall-through call setup immediately after the lui.
        del out[i - 1]
        i -= 1
        out[i + 3] = pre
        j -= 1
        out.insert(j + 1, slot)
        target = block[2].rsplit(',', 1)[-1]
        for k in range(j + 2, min(len(out), j + 20)):
            if out[k].strip() == target + ":" and out[k - 1] == slot:
                out[k - 1], out[k] = out[k], out[k - 1]
                break
        i = j + 2
    return out


def swap_adjacent_insns(flat, sites):
    """Swap the two instructions of an explicitly named adjacent pair.

    Site-keyed like flip_branch_likely/pin_slot_nops: FUNC:N names the
    pair whose FIRST instruction is the Nth instruction of FUNC (0-based,
    counting every instruction line in emission order -- directives,
    labels and #-comments are not counted). The two instruction lines
    must be literally adjacent (no label or directive between them) and
    pass swap_ok's independence check, otherwise the site is left
    untouched. The original build's second scheduler simply ordered the
    pair the other way (e.g. TMENU_addItem's li $7,-1 / li $5,1024
    argument setup); the swapped bytes are verified against the original
    binary, so semantics are the original's by definition."""
    res = []
    cur = None
    idx = 0
    i = 0
    while i < len(flat):
        line = flat[i]
        if line.startswith("\t.ent\t"):
            cur = line.split("\t")[-1]
            idx = 0
        if RE_INSN.match(line):
            # A site may be written FUNC:N! to force the swap past
            # swap_ok's independence check. The only use so far is two
            # adjacent stores, which swap_ok conservatively refuses
            # because it cannot prove they do not alias. Forcing is
            # defensible precisely there: the order being produced is
            # the ORIGINAL compiler's, so if the two stores could alias,
            # the original order is the correct one and ours is the
            # deviation. tools/audit_swaps.py still checks the result is
            # a pure reorder of the same instruction multiset.
            forced = f"{cur}:{idx}!" in sites
            if (forced or f"{cur}:{idx}" in sites) and i + 1 < len(flat) \
                    and (forced or swap_ok(line, flat[i + 1])) \
                    and RE_INSN.match(flat[i + 1]):
                res.append(flat[i + 1])
                res.append(line)
                idx += 2
                i += 2
                continue
            idx += 1
        res.append(line)
        i += 1
    return res


def swap_into_slot(flat, sites, allow_stack_mem=False):
    """Exchange a jal's filled delay-slot insn with the insn before it.

    FUNC:N names the Nth `jal` of FUNC (0-based, counting every jal line
    in emission order). The site must be a gcc-filled slot:

        <A>                      <B>
        .set noreorder           .set noreorder
        .set nomacro     -->     .set nomacro
        jal target               jal target
        <B>                      <A>
        .set macro               .set macro
        .set reorder             .set reorder

    Both orders execute A and B before the callee runs; only their
    mutual order swaps, so A/B must pass swap_ok and neither may touch
    $31. The original build filled the slot with the OTHER of the two
    argument-setup copies (PauseMenu's move $5,$16 / move $6,$2 around
    jal GameSnapShotSaveFile). Ineligible sites are left untouched."""
    res = []
    cur = None
    idx = 0
    i = 0
    while i < len(flat):
        line = flat[i]
        if line.startswith("\t.ent\t"):
            cur = line.split("\t")[-1]
            idx = 0
        if line.startswith("\tjal\t"):
            if (f"{cur}:{idx}" in sites and i >= 3 and i + 1 < len(flat)):
                a, s1, s2, slot = flat[i - 3], flat[i - 2], flat[i - 1], flat[i + 1]
                sets = [x.strip().replace("\t", " ") for x in (s1, s2)]
                eligible = swap_ok(a, slot)
                if (allow_stack_mem and not eligible
                        and RE_MEMOP.match(a) and RE_MEMOP.match(slot)):
                    wa, ra = insn_regs(a)
                    wb, rb = insn_regs(slot)
                    a_sp = "($sp)" in a
                    b_sp = "($sp)" in slot
                    eligible = (a_sp != b_sp
                                and not (wa & (rb | wb))
                                and not (wb & ra))
                if (sets == [".set noreorder", ".set nomacro"]
                        and eligible
                        and "$31" not in a + slot and "$ra" not in a + slot):
                    res[-3:] = [slot, s1, s2]
                    res.append(line)
                    res.append(a)
                    idx += 1
                    i += 2
                    continue
            idx += 1
        res.append(line)
        i += 1
    return res


def exchange_slot_with_prior(flat, specs):
    """Exchange a filled delay slot with an earlier independent instruction.

    Specs are FUNC:N:BACK. N counts gcc-filled branches/jumps in emission
    order, using the same convention as --unfill-gcc-slots; BACK selects the
    Nth preceding instruction before the branch.  Both instructions must be
    independent of each other, every instruction they cross, and the branch
    predicate. Branch-likely sites are rejected because their slots are not
    executed on the fall-through path.
    """
    if not specs:
        return flat
    parsed = {}
    for spec in specs:
        try:
            func, site_text, back_text = spec.split(':')
            site = int(site_text)
            back = int(back_text)
        except (ValueError, TypeError):
            raise SystemExit("--exchange-slot-prior: bad spec %r "
                             "(want FUNC:N:BACK)" % spec)
        if site < 0 or back < 1:
            raise SystemExit("--exchange-slot-prior: invalid site/back in "
                             "%r" % spec)
        parsed[(func, site)] = back

    out = list(flat)
    cur = None
    branch_idx = 0
    applied = set()
    likely = re.compile(
        r'^\t(?:beql|bnel|beqzl|bnezl|bgezl|bgtzl|blezl|bltzl|'
        r'bc1tl|bc1fl)[ \t]')
    for i, line in enumerate(out):
        if line.startswith("\t.ent\t"):
            cur = line.split("\t")[-1]
            branch_idx = 0
        if (not RE_ANY_JUMP_OR_BRANCH.match(line) or i < 2
                or i + 1 >= len(out)
                or out[i - 2].strip().replace("\t", " ") != ".set noreorder"
                or out[i - 1].strip().replace("\t", " ") != ".set nomacro"
                or not RE_INSN.match(out[i + 1])):
            continue
        key = (cur, branch_idx)
        branch_idx += 1
        if key not in parsed:
            continue
        if likely.match(line):
            raise SystemExit("--exchange-slot-prior: %s:%d is branch-likely" %
                             key)
        back = parsed[key]
        prior_i = i - 3
        seen = 0
        while prior_i >= 0:
            if out[prior_i].startswith("\t.ent\t") or out[prior_i].endswith(":"):
                prior_i = -1
                break
            if RE_INSN.match(out[prior_i]):
                seen += 1
                if seen == back:
                    break
            prior_i -= 1
        if prior_i < 0:
            raise SystemExit("--exchange-slot-prior: %s:%d has no BACK=%d "
                             "instruction" % (cur, key[1], back))
        prior = out[prior_i]
        slot = out[i + 1]
        mids = [out[j] for j in range(prior_i + 1, i)
                if RE_INSN.match(out[j])]
        branch_reads = insn_regs(line)[1]
        prior_writes = insn_regs(prior)[0]
        slot_writes = insn_regs(slot)[0]
        if (not swap_ok(prior, slot)
                or prior_writes & branch_reads
                or slot_writes & branch_reads
                or any(not swap_ok(prior, mid) for mid in mids)
                or any(not swap_ok(slot, mid) for mid in mids)):
            raise SystemExit("--exchange-slot-prior: %s:%d dependency check "
                             "failed" % key)
        out[prior_i], out[i + 1] = slot, prior
        applied.add(key)
    missing = set(parsed) - applied
    if missing:
        raise SystemExit("--exchange-slot-prior: site(s) not found: %s" %
                         ",".join("%s:%d" % x for x in sorted(missing)))
    return out


def retime_branch_to_call(flat, sites):
    """Move a pre-branch update into the following call's delay slot.

    At FUNC:N (N uses the gcc-filled branch/jump count), accept the strict
    shape `UPDATE; plain branch / ARG; jal / NEXT`. UPDATE moves into the
    jal delay slot and NEXT moves just after the call.  This is the retail
    schedule for loops where GCC hoists UPDATE over an exit test to fill a
    load-use gap, then uses NEXT as call-slot filler.
    """
    if not sites:
        return flat
    out = list(flat)
    applied = set()
    likely = re.compile(
        r'^\t(?:beql|bnel|beqzl|bnezl|bgezl|bgtzl|blezl|bltzl|'
        r'bc1tl|bc1fl)[ \t]')
    for spec in sites:
        try:
            func, site_text = spec.split(':')
            want = int(site_text)
        except (ValueError, TypeError):
            raise SystemExit("--retime-branch-call: bad site %r "
                             "(want FUNC:N)" % spec)
        cur = None
        branch_idx = 0
        branch_i = None
        for i, line in enumerate(out):
            if line.startswith("\t.ent\t"):
                cur = line.split("\t")[-1]
                branch_idx = 0
            if (not RE_ANY_JUMP_OR_BRANCH.match(line) or i < 2
                    or i + 1 >= len(out)
                    or out[i - 2].strip().replace("\t", " ") != ".set noreorder"
                    or out[i - 1].strip().replace("\t", " ") != ".set nomacro"
                    or not RE_INSN.match(out[i + 1])):
                continue
            if cur == func and branch_idx == want:
                branch_i = i
                break
            if cur == func:
                branch_idx += 1
        if branch_i is None:
            raise SystemExit("--retime-branch-call: site not found: %s" % spec)
        branch = out[branch_i]
        if likely.match(branch) or branch.startswith(("\tjal\t", "\tj\t")):
            raise SystemExit("--retime-branch-call: %s is not a plain "
                             "conditional branch" % spec)
        prior_i = branch_i - 3
        while prior_i >= 0 and not RE_INSN.match(out[prior_i]):
            if out[prior_i].endswith(":"):
                prior_i = -1
                break
            prior_i -= 1
        if prior_i < 0:
            raise SystemExit("--retime-branch-call: %s has no pre-branch "
                             "instruction" % spec)
        call_i = branch_i + 2
        while call_i < len(out) and not RE_INSN.match(out[call_i]):
            if out[call_i].endswith(":"):
                call_i = len(out)
                break
            call_i += 1
        if (call_i >= len(out) or not out[call_i].startswith("\tjal\t")
                or call_i + 1 >= len(out)
                or not RE_INSN.match(out[call_i + 1])):
            raise SystemExit("--retime-branch-call: %s is not followed by a "
                             "filled jal" % spec)
        update = out[prior_i]
        branch_slot = out[branch_i + 1]
        next_update = out[call_i + 1]
        branch_reads = insn_regs(branch)[1]
        if (not swap_ok(update, branch_slot)
                or not swap_ok(update, next_update)
                or insn_regs(update)[0] & branch_reads):
            raise SystemExit("--retime-branch-call: %s dependency check "
                             "failed" % spec)

        out[call_i + 1] = update
        del out[prior_i]
        call_i -= 1
        reorder_i = call_i + 2
        while reorder_i < len(out):
            if out[reorder_i].strip().replace("\t", " ") == ".set reorder":
                break
            if RE_INSN.match(out[reorder_i]):
                reorder_i = len(out)
                break
            reorder_i += 1
        if reorder_i >= len(out):
            raise SystemExit("--retime-branch-call: %s missing call reorder "
                             "boundary" % spec)
        out.insert(reorder_i + 1, next_update)
        applied.add(spec)
    missing = set(sites) - applied
    if missing:
        raise SystemExit("--retime-branch-call: site(s) not found: %s" %
                         ",".join(sorted(missing)))
    return out


def rebase_stack_memory(flat, specs):
    """Express a stack access through an already-computed stack pointer.

    Specs are FUNC:SITES:BASE:BIAS, where SITES is a comma-separated list of
    0-based instruction indices.  Each named load/store `OFF($sp)` becomes
    `OFF-BIAS($BASE)`.  The pass scans backward from every site and requires
    the most recent write to BASE to be exactly BASE = $sp + BIAS.  It fails
    on any other shape, making the address rewrite algebraically auditable.
    """
    if not specs:
        return flat
    parsed = {}
    for spec in specs:
        try:
            func, site_text, base_text, bias_text = spec.split(':')
            site_ids = [int(x) for x in site_text.split(',')]
            base = base_text.lstrip('$')
            bias = int(bias_text, 0)
        except (ValueError, TypeError):
            raise SystemExit("--rebase-stack-mem: bad spec %r "
                             "(want FUNC:SITES:BASE:BIAS)" % spec)
        if not re.match(r'^\d+$', base) or not site_ids:
            raise SystemExit("--rebase-stack-mem: bad base/sites in %r" %
                             spec)
        for site in site_ids:
            parsed[(func, site)] = (base, bias)

    owner = function_at(flat)
    out = list(flat)
    idx = 0
    prev_owner = None
    applied = set()
    mem_re = re.compile(
        r'^(\t[a-z0-9.]+[ \t]+\$\w+,)(-?\d+)\(\$sp\)(.*)$')
    for i, line in enumerate(flat):
        if owner[i] != prev_owner:
            idx = 0
            prev_owner = owner[i]
        if not RE_INSN.match(line):
            continue
        key = (owner[i], idx)
        if key in parsed:
            base, bias = parsed[key]
            m = mem_re.match(line)
            if not m or not RE_MEMOP.match(line):
                raise SystemExit("--rebase-stack-mem: %s:%d is not a "
                                 "stack load/store: %s" %
                                 (owner[i], idx, line))
            base_reg = "$%s" % base
            defining = None
            for j in range(i - 1, -1, -1):
                if owner[j] != owner[i]:
                    break
                if not RE_INSN.match(flat[j]):
                    continue
                writes, _reads = insn_regs(flat[j].replace(' ', '\t', 1))
                if base_reg in writes:
                    defining = flat[j]
                    break
            base_def = re.compile(
                r'^\t(?:addiu|addu|daddu)[ \t]+\$%s,\$sp,%d$' %
                (re.escape(base), bias))
            if defining is None or not base_def.match(defining):
                raise SystemExit("--rebase-stack-mem: %s:%d has no live "
                                 "$%s = $sp + %d definition" %
                                 (owner[i], idx, base, bias))
            off = int(m.group(2)) - bias
            out[i] = "%s%d($%s)%s" % (m.group(1), off, base, m.group(3))
            applied.add(key)
        idx += 1
    missing = set(parsed) - applied
    if missing:
        raise SystemExit("--rebase-stack-mem: site(s) not found: %s" %
                         ",".join("%s:%d" % x for x in sorted(missing)))
    return out


def swap_slot_target(flat, sites):
    """Exchange a filled branch delay slot with its target block's head.

    gcc's delayed-branch pass may fill a conditional branch's slot with
    an instruction hoisted from the branch TARGET block; the original
    build sometimes hoisted a DIFFERENT instruction from the same block
    (MenuShopModelDisp's beqz: gcc steals the lwc1 %gp_rel load, the
    original steals the move $4,$16 that follows it). FUNC:N names the
    Nth gcc-filled branch of FUNC (0-based, emission order, counting
    only branches inside .set noreorder/.set nomacro fill blocks -- the
    same shape unfill_gcc_slots recognizes). The slot instruction is
    exchanged with the first instruction after the branch's target
    label, guarded by swap_ok; on the taken path the two simply swap
    order, and the resulting bytes are verified against the original
    binary, so the swapped-in instruction's deadness on the fallthrough
    path is established by the byte match rather than analysis.
    Ineligible or unresolvable sites are left untouched."""
    # index labels per function
    res = list(flat)
    cur = None
    idx = 0
    for i, line in enumerate(res):
        if line.startswith("\t.ent\t"):
            cur = line.split("\t")[-1]
            idx = 0
        if line.startswith("\t.ent\t"):
            fn_start = i
        if (RE_PIN_BRANCH.match(line) and i >= 2 and i + 1 < len(res)
                and res[i - 2].strip().replace("\t", " ") == ".set noreorder"
                and res[i - 1].strip().replace("\t", " ") == ".set nomacro"
                and RE_INSN.match(res[i + 1])):
            if f"{cur}:{idx}" in sites:
                target = line.strip().split(",")[-1].strip()
                # find the target label within the same function
                lab = None
                for j in range(fn_start, len(res)):
                    if res[j] == target + ":":
                        lab = j
                        break
                    if res[j].startswith("\t.end\t"):
                        break
                if lab is not None:
                    k = lab + 1
                    while k < len(res) and not RE_INSN.match(res[k]):
                        if (res[k].rstrip().endswith(":")
                                or res[k].startswith("\t.end\t")):
                            k = None
                            break
                        k += 1
                    if k is not None and k < len(res) and swap_ok(res[i + 1], res[k]):
                        res[i + 1], res[k] = res[k], res[i + 1]
            idx += 1
    return res


def mtc1_hazard_nops(flat, sites):
    """Pin a literal nop after an explicitly named mtc1.

    The original ee-as padded some COP1-move stalls that no register/
    distance heuristic reproduces (see the chain-tracking NOTE in main);
    gcc sometimes marks the slot with a `#nop` comment modern gas
    ignores (nmlPacketAddTransMicrocodeInit), sometimes not at all
    (nmlModelFogPara's mtc1 $0,$f1 before a sub.s that reads neither).
    FUNC:N names the Nth mtc1 of FUNC (0-based, counting every mtc1
    line of the post-processed asm in emission order, so mtc1s
    synthesized from li.s expansions count too); a literal nop is
    appended after it. Bytes are verified against the original binary."""
    res = []
    cur = None
    idx = 0
    for line in flat:
        if line.startswith("\t.ent\t"):
            cur = line.split("\t")[-1]
            idx = 0
        res.append(line)
        if re.match(r'^\tmtc1[ \t]', line):
            if f"{cur}:{idx}" in sites:
                res.append("\tnop")
            idx += 1
    return res


RE_FLIP_COND_BRANCH = re.compile(
    r"^\t(beql?|bnel?|beqzl?|bnezl?|blezl?|bgtzl?|bltzl?|bgezl?"
    r"|bc1tl?|bc1fl?)([ \t])")


def split_hi_lo(flat, sites):
    """Rename one `lui %hi(SYM)`'s destination register, and rewrite the
    SOURCE (not destination) register of the matching `%lo(SYM)`
    instruction that folds it, leaving everything else untouched.

    Closes a hazard family found across xglHddMcCheckYourSaves,
    xglSoundLoadEffect/xglSoundLoadRequestSmd, and Judge_MakeNewFolder,
    each independently swept to exhaustion with no C source shape
    reaching it (see the TODOs at each site) and unreachable by
    --swap-regs, which renames symmetrically everywhere in its window.
    The original sometimes materialises a symbol's %hi into one
    register -- often otherwise dead at that point -- and folds %lo
    into a DIFFERENT register at the point of use:

        lui   $v0, %hi(sym)
        ...
        addiu $a0, $v0, %lo(sym)

    while ours computes both halves directly into the register actually
    used:

        lui   $a0, %hi(sym)
        ...
        addiu $a0, $a0, %lo(sym)

    No swap can express this: $a0's role as the addiu's DESTINATION must
    not change, only the lui's destination and the addiu's READ of it.

    Matching is done by SYMBOL TEXT (the identical %hi(EXPR)/%lo(EXPR)
    string), not by "next instruction reading this register number" --
    the first version of this pass used register liveness and silently
    mis-fired on xglHddMcCheckYourSaves, where an unrelated `move
    $s0,$a0` (staging the function's own incoming argument, nothing to
    do with the symbol) sits between the lui and its real %lo partner
    and also happens to read the same register number. A %lo for a
    DIFFERENT symbol, or any other instruction, is not a match even if
    it reads the same register.

    Site: FUNC:IDX:NEWREG, IDX 0-based (same insn-only counting
    convention as the other site-keyed passes: directives, labels and
    #-comments are not counted), NEWREG a bare register token ("2" for
    $2/$v0, matching --swap-regs's token convention). The instruction at
    IDX must be `lui $orig,%hi(EXPR)`; the pass scans forward for the
    first later instruction containing the literal text `%lo(EXPR)` for
    the SAME EXPR (stopping at the function's `.end`) and rewrites only
    that instruction's occurrence of $orig, leaving its own destination
    register untouched. A site that doesn't match either expectation is
    left untouched with a stderr warning -- silent no-ops here are
    exactly what let --short-loop-pad ship broken twice with passing
    unit tests (see that pass's history); this one is meant to fail
    loud on the CLI instead."""
    if not sites:
        return flat
    out = list(flat)
    cur = None
    idx = 0
    positions = []
    for i, line in enumerate(out):
        if line.startswith("\t.ent\t"):
            cur = line.split("\t")[-1]
            idx = 0
        if RE_INSN.match(line):
            positions.append((i, idx, cur))
            idx += 1
    for line_i, insn_idx, func in positions:
        for site_idx, newreg in sites.get(func, []):
            if site_idx != insn_idx:
                continue
            line = out[line_i]
            m = re.match(r'^\tlui\t\$(\w+),%hi\(([^)]*)\)(.*)$', line)
            if not m:
                sys.stderr.write("--split-hi-lo: %s:%d is not a "
                                 "`lui $reg,%%hi(EXPR)`, site ignored: "
                                 "%r\n" % (func, insn_idx, line))
                continue
            orig, expr = m.group(1), m.group(2)
            out[line_i] = "\tlui\t$%s,%%hi(%s)%s" % (newreg, expr, m.group(3))
            lo_needle = "%%lo(%s)" % expr
            found = False
            for j in range(line_i + 1, len(out)):
                l2 = out[j]
                if l2.startswith("\t.end\t"):
                    break
                if not RE_INSN.match(l2) or lo_needle not in l2:
                    continue
                m2 = re.match(r'^\t([a-z0-9.]+)\t(.*)$', l2)
                mnem, rest = m2.group(1), m2.group(2)
                ops = rest.split(',')
                ops[1:] = [re.sub(r'\$%s\b' % re.escape(orig),
                                  '$%s' % newreg, o) for o in ops[1:]]
                out[j] = "\t%s\t%s" % (mnem, ','.join(ops))
                found = True
                break
            if not found:
                sys.stderr.write("--split-hi-lo: %s:%d -- no later "
                                 "`%%lo(%s)` found before function end, "
                                 "site ignored\n" % (func, insn_idx, expr))
    return out


def flip_branch_likely(flat, likely_sites, plain_sites):
    """Flip the branch-likely (annul) bit on explicitly named sites.

    gcc 2.96's delayed-branch pass sometimes disagrees with the original
    build over WHICH filled branch gets the annulled (branch-likely)
    form -- same slot content, same target, only the l-bit differs.  No
    structural heuristic discriminates (the taken-path test matches both
    the flipped and the correctly-plain sites), so the sites are named
    explicitly: FUNC:N where N is the 0-based index of the conditional
    branch within the function, counted over the emitted asm in order
    (which is binary order).  --branch-likely makes the site the likely
    form (beqz -> beqzl); --branch-unlikely the reverse.  bltzal/bgezal
    are excluded from both the flip and the count (trailing-l ambiguity;
    gcc does not emit them here).  The resulting bytes reproduce the
    original binary at the site, so semantics are the original's by
    definition.
    """
    res = []
    cur = None
    count = 0
    for line in flat:
        if line.startswith("\t.ent\t"):
            cur = line.split("\t")[-1]
            count = 0
        m = RE_FLIP_COND_BRANCH.match(line)
        if m:
            mn = m.group(1)
            key = f"{cur}:{count}"
            if key in likely_sites and not mn.endswith("l"):
                line = "\t" + mn + "l" + line[1 + len(mn):]
            elif key in plain_sites and mn.endswith("l"):
                line = "\t" + mn[:-1] + line[1 + len(mn):]
            count += 1
        res.append(line)
    return res


def main(path, omitted_hazards, barrier_return_store=None,
         hoist_return_store=None, barrier_branch_move=None,
         no_fill_delay=None, expand_sym_loads=False,
         unfill_slots=None, barrier_lo_load=None,
         branch_likely=None, branch_unlikely=None,
         war_restore=None, pin_slot=None, lis_hazard_nop=None,
         swap_adjacent=None, swap_slot=None, mtc1_nop=None,
         swap_mem_slot=None,
         exchange_slot_prior=None,
         retime_branch_call=None,
         swap_slot_tgt=None, rotate=None, swap_regs=None,
         swap_reg_sources=None,
         exchange_derived=None,
         retime_byte_guard=None, split_loop_byte=None,
         coalesce_symbol_address=None,
         swap_fp_operands=None,
         swap_int_operands=None,
         remat_call_constants=None,
         rotate_seq=None, post_rotate_seq=None, hoist_div_arg=None,
         retime_branch=None,
         zero_quad_store=None, fp_pair_hazard=None,
         short_loop_pad=None, byte_move=None, split_hi_lo_raw=None,
         rebase_stack_mem=None):
    # Each flag is either None (off), an empty tuple (whole file), or a set
    # of function names to scope the pass to.
    with open(path) as f:
        lines = f.read().split('\n')

    if hoist_return_store is not None:
        lines = hoist_return_delay_stores(lines, hoist_return_store)

    if swap_regs is not None:
        spec = {}
        for tok in swap_regs:
            parts = tok.split(':')
            if len(parts) == 2:
                (fn, ab), rng = parts, None
            elif len(parts) == 3:
                fn, ab, rng = parts
            else:
                raise SystemExit("--swap-regs: bad site %r (want FUNC:A-B "
                                 "or FUNC:A-B:LO-HI)" % tok)
            a, b = ab.split('-')
            a, b = a.lstrip('$'), b.lstrip('$')
            for t in (a, b):
                if not re.match(r'^f?\d+$', t):
                    raise SystemExit("--swap-regs: bad register token %r "
                                     "(want 2, $2, f4 or $f4)" % t)
            if rng is None:
                spec.setdefault(fn, []).append((a, b, None, None))
            else:
                for part in rng.split(','):
                    if '-' in part:
                        lo, hi = (int(x) for x in part.split('-'))
                    else:
                        lo = hi = int(part)
                    spec.setdefault(fn, []).append((a, b, lo, hi))
        lines = swap_registers(lines, spec)

    if swap_reg_sources:
        lines = swap_register_sources(lines, swap_reg_sources)

    if exchange_derived:
        lines = exchange_derived_results(lines, exchange_derived)

    if retime_byte_guard:
        lines = retime_byte_copy_guards(lines, retime_byte_guard)

    if split_loop_byte:
        lines = split_loop_byte_loads(lines, split_loop_byte)

    if coalesce_symbol_address:
        lines = coalesce_symbol_addresses(lines, coalesce_symbol_address)

    if swap_fp_operands is not None:
        lines = swap_fp_commutative_operands(lines, swap_fp_operands)

    if swap_int_operands is not None:
        lines = swap_int_commutative_operands(lines, swap_int_operands)

    if remat_call_constants is not None:
        lines = rematerialize_call_constants(lines, remat_call_constants)

    owner = function_at(lines)

    # SOLVED -- see --fp-pair-hazard and reads_fp_reg().  A chain-tracking
    # extension was tried here first (accumulate pending mtc1 destinations
    # across intervening non-compute instructions) to close
    # floorf/__ieee754_atan2f's missing hazard nop, and it regressed
    # scalbnf and atanf.  It was reaching for the wrong invariant: the
    # rule is neither the register chain, nor the distance, nor whether
    # the destination is later consumed.  It is that the ORIGINAL
    # assembler compared FP registers with the low bit masked off both
    # sides -- $fN and $f(N^1) are one double -- so `mtc1 -> $f1` hazards
    # against a following `mul.s $f0,$f4,$f0` and `mtc1 -> $f3` does not.
    # atanf and scalbnf are exactly the cases where the pair does NOT
    # overlap, which is why they regressed under any register-identity
    # rule.  Opt in per file with --fp-pair-hazard.
    out = []
    lit_pool = []
    for i, line in enumerate(lines):
        line = RE_MOVE.sub(r'\tdaddu\t\1,\2,$0', line)
        m_brk = RE_BREAK.match(line)
        if m_brk:
            line = '\tbreak\t0,%s' % m_brk.group(1)

        following = next_insn(lines, i)
        omitted = any(following.startswith("\t" + op) for op in omitted_hazards)
        hazard_dest = fp_hazard_dest(line)
        pair_hazard = (fp_pair_hazard is not None
                       and in_scope(owner[i], fp_pair_hazard))
        needs_hazard_nop = (hazard_dest is not None
                            and (RE_FPCOMPUTE.match(following)
                                 or RE_FP_STORE.match(following))
                            and reads_fp_reg(following, hazard_dest, pair_hazard)
                            and not omitted
                            and not RE_FP_BRANCH_LIKELY.match(previous_insn(lines, i)))

        m_lid = RE_LID.match(line)
        if m_lid:
            reg, valstr = m_lid.groups()
            bits = struct.unpack('<Q', struct.pack('<d', float(valstr)))[0]
            synth = '\n'.join(synth_li_d(reg, bits, lit_pool))
            # gcc leaves the call in `.set reorder` whenever it did not fill
            # the delay slot itself; gas then steals the immediately
            # preceding instruction into it. When that preceding
            # instruction is the tail of a li.d expansion, the original
            # ee-as did NOT steal it -- li.d was a real macro there, and gas
            # never reorders across a macro expansion, so the call kept its
            # nop. Barrier the synthesized sequence in that case. (Same
            # class as the RE_CVT-before-call rule below.) Found on
            # _dtoa_r's `d.d *= 10.` at $L416.  The same holds for a plain
            # `b` to a local label -- gas steals the expansion tail into a
            # branch delay slot just as happily as into a call's, and the
            # original's macro boundary stopped both (libm.c's
            # __ieee754_acos, the `pi_o_2 - (x - pio2_lo)` return at
            # $L776) -- so the barrier covers b/$L targets too.
            if re.match(r'^\t(jal|j|b)\t[A-Za-z_$]', following):
                synth = "\t.set push\n\t.set noreorder\n" + synth + "\n\t.set pop"
            out.append(synth)
            continue

        m_sqrt = RE_SQRT.match(line)
        if m_sqrt:
            fd, fs = int(m_sqrt.group(1)), int(m_sqrt.group(2))
            out.append("\t.word 0x%08X\t/* sqrt.s $f%d,$f%d */" %
                       (0x46000004 | (fs << 16) | (fd << 6), fd, fs))
            continue

        # A numeric-absolute load/store macro immediately before a leaf
        # return: gas steals its expanded access into the jr delay slot,
        # where the original ee-as left a nop. Barrier just this case.
        if RE_MEM_ABS.match(line) and RE_RETURN.match(following):
            out.append(wrap_mips1_noreorder(line))
            continue

        m_big = RE_MEM_BIGOFF.match(line)
        if m_big and abs(int(m_big.group(3))) > 32767:
            op, reg, off, base = m_big.groups()
            if op in ("sd", "ld"):
                # .set mips1 fixes the addu-vs-daddu base computation for
                # sw/lw etc., but sd/ld are true 64-bit ops that don't
                # exist under mips1 -- gas silently splits them into a
                # pair of 32-bit sw/lw halves instead (wrong instruction
                # count and encoding). Expand the address macro by hand
                # instead, matching the original's lui+addu+sd/ld shape.
                offset = int(off)
                lo = offset & 0xFFFF
                if lo >= 0x8000:
                    lo -= 0x10000
                hi = ((offset - lo) >> 16) & 0xFFFF
                basereg = base[1:-1]
                out.append(f"\tlui\t$1,{hi}\n\taddu\t$1,$1,{basereg}"
                           f"\n\t{op}\t{reg},{lo}($1)")
            else:
                out.append(wrap_mips1(line))
            continue

        m_symreg = RE_MEM_SYM_REG.match(line)
        if not m_symreg and expand_sym_loads:
            m_symreg = RE_MEM_SYM_REG_LOAD.match(line)
        if m_symreg:
            op, reg, sym, basereg = m_symreg.groups()
            out.append(f"\tlui\t{reg},%hi({sym})\n"
                       f"\taddu\t{reg},{reg},{basereg}\n"
                       f"\t{op}\t{reg},%lo({sym})({reg})")
            continue

        # --barrier-lo-load: a %lo() memory load directly before a plain
        # branch/call must not be stolen into the delay slot (confirmed on
        # libm.c's atan/floor, where the .lit4 constant load sits above
        # the jal with a genuine nop in the slot).
        if (barrier_lo_load is not None
                and re.match(r"^\t(ld|lw|lwu|lh|lhu|lb|lbu)\t[^,]*,.*%lo", line)
                and re.match(r"^\t(jal|j|b)\t[A-Za-z_$]", following)
                and in_scope(owner[i], barrier_lo_load)):
            out.append("\t.set push\n\t.set noreorder\n" + line +
                       "\n\t.set pop")
            continue

        if (barrier_return_store is not None
                and ((RE_STORE.match(line) and RE_RETURN.match(following))
                     or (RE_RETURN_MOVE.match(line)
                         and RE_RETURN_OR_CALL.match(following)))
                and in_scope(owner[i], barrier_return_store)):
            out.append("\t.set push\n\t.set noreorder\n" + line +
                       "\n\t.set pop")
            continue


        # --no-fill-delay: emit every reorder-mode plain branch with an
        # explicit nop in a noreorder block, so gas can never steal the
        # preceding instruction into the slot. The original objects for
        # some TUs (libgcc float<->DI conversions) never have filled
        # slots at all -- a per-file property, opt-in like the barriers.
        if (no_fill_delay is not None
                and RE_PLAIN_BRANCH.match(line)
                and in_scope(owner[i], no_fill_delay)):
            out.append("\t.set push\n\t.set noreorder\n" + line +
                       "\n\tnop\n\t.set pop")
            continue

        if RE_LA.match(line) or RE_MEM.match(line) or RE_CVT.match(line):
            # When an `la reg, sym(idx)` macro is the last computation before
            # a leaf return, the modern assembler pulls its final addu into
            # the jr delay slot; the original ee-as left a nop. Barrier it.
            if RE_LA.match(line) and RE_RETURN.match(following):
                out.append(wrap_mips1_noreorder(line))
                continue
            # --barrier-lo-load also pins an la macro above a call the
            # same way (la-before-jal, mirroring la-before-return).
            if (barrier_lo_load is not None and RE_LA.match(line)
                    and re.match(r"^\t(jal|j)\t[A-Za-z_]", following)
                    and in_scope(owner[i], barrier_lo_load)):
                out.append(wrap_mips1_noreorder(line))
                continue
            # gas also steals a conversion into a following CALL's delay
            # slot; the original ee-as left the nop there.
            if RE_CVT.match(line) and re.match(r'^\t(jal|j)\t[A-Za-z_]', following):
                out.append(wrap_mips1_noreorder(line))
                continue
            # A conversion immediately before a leaf return is a case gas's
            # own delay-slot-fill (active whenever the block isn't
            # `.set noreorder`) can steal into the jr's delay slot, bumping
            # the compiler's real delay-slot instruction (e.g. mul.s using
            # the converted value) out of place. Barrier it with noreorder
            # instead of padding a literal nop after it: sefRandf has this
            # exact mtc1->cvt->jr/mul.s shape with NO extra nop in the
            # original, so the fix must be "stop gas reordering", not "add
            # a byte to compensate for a steal that noreorder would have
            # prevented for free".
            if RE_CVT.match(line) and RE_RETURN.match(following):
                out.append(wrap_mips1_noreorder(line))
                continue
            m_mem = RE_MEM.match(line)
            # A BARE symbol operand (`sw $3,D_004DC6B4`, no `($reg)` index)
            # never needs the mips1 wrap at all: gas resolves it to either
            # a single gp-relative instruction or a plain lui+%lo pair with
            # no addu/daddu in either case, so there is no encoding this
            # wrap could fix. Wrapping it anyway is actively harmful: gas's
            # mips1 load-delay hazard check looks at the immediately
            # PRECEDING instruction regardless of what mode IT was
            # assembled under, so if that previous instruction happens to
            # be an ordinary (unwrapped) load feeding this one, gas
            # inserts a genuine interlock nop the R5900/original ee-as
            # never had -- and `.set noreorder` does NOT suppress it (this
            # is a mandatory-hazard insertion, not a reorder-fill).
            # Verified with a minimal .s: `lw $3,0($5)` / (wrapped)
            # `sw $3,sym` reproducibly gains an extra nop only when
            # wrapped; dropping the wrap for the bare-symbol case removes
            # it while the encoding (gp-rel reloc or lui/%lo pair) is
            # identical either way. Only the INDEXED form (`sym($reg)`)
            # actually needs the wrap, to get `addu` instead of `daddu` in
            # the address computation. Found via
            # Java_xeno_util_Layout_getManager__I.
            if m_mem and '(' not in m_mem.group(3):
                wrapped = line
                if needs_hazard_nop:
                    wrapped += "\n\tnop"
                out.append(wrapped)
                continue
            if m_mem and m_mem.group(1) in LOAD_OPS:
                wrapped = wrap_mips1_noreorder(line)
            else:
                wrapped = wrap_mips1(line)
            if needs_hazard_nop:
                wrapped += "\n\tnop"
            out.append(wrapped)
            continue

        m_lis = RE_LIS.match(line)
        if m_lis:
            # ee-as synthesized li.s inline (lui $at / mtc1) whenever the
            # value's significant bits fit a 16-bit window anchored at the
            # top set bit -- same rule as the li.d macro above.  Modern gas
            # sends every li.s to .lit4 instead; expand the synthesizable
            # ones ourselves.  (A/B experiment: xglAtan2's `+ 1.0f`.)
            reg, valstr = [s.strip() for s in m_lis.group(1).split(",", 1)]
            fbits = struct.unpack("<I", struct.pack("<f", float(valstr)))[0]
            if fbits and (fbits & 0xFFFF) == 0:
                wrapped = ("\tlui\t$1,%d\n\tmtc1\t$1,%s"
                           % ((fbits >> 16) & 0xFFFF, reg))
                # ee-as put a hazard nop after the expansion's mtc1 when
                # the next instruction is a COP1 compute -- even one that
                # does not read the destination (xglAtan2's poly tail,
                # where the li.s sits in a gp-relative lwc1 cluster).
                # -G0 sites with a preceding integer-synth li.s show NO
                # nop (libm atanf), so this is opt-in per function via
                # XENO_LIS_HAZARD_FUNCS (proposed flag: --lis-hazard-nop).
                prev = previous_insn(lines, i)
                prev_fp_load = re.match(r'^\t(lwc1|li\.s)[ \t]', prev)
                if (lis_hazard_nop is not None
                        and RE_FPCOMPUTE.match(following) and prev_fp_load
                        and in_scope(owner[i], lis_hazard_nop)
                        and not needs_hazard_nop):
                    wrapped += "\n\tnop"
            elif re.match(r'^\t(j\t\$31|jr\t\$(ra|31))\b', following):
                # ee-as never let the li.s expansion's lwc1 slide into a
                # following return's delay slot; modern gas does.  Barrier
                # the expansion so the jr keeps its genuine nop.
                wrapped = wrap_mips1_noreorder(line)
            else:
                wrapped = wrap_mips1(line)
            if needs_hazard_nop:
                wrapped += "\n\tnop"
            out.append(wrapped)
            continue

        if needs_hazard_nop:
            out.append(line + "\n\tnop")
            continue

        out.append(line)

    if lit_pool:
        # Backing store for the li.d table-load fallback above. Section
        # choice/placement doesn't need to match the original -- only
        # .text bytes are verified, and the lui %hi/ld %lo relocations
        # this data is addressed through are masked immediates in
        # verify.py's comparison either way.
        out.append("\t.rdata")
        out.append("\t.align 3")
        for label, bits in lit_pool:
            lo32 = bits & 0xFFFFFFFF
            hi32 = (bits >> 32) & 0xFFFFFFFF
            out.append(f"{label}:")
            out.append(f"\t.word {lo32}")
            out.append(f"\t.word {hi32}")


    if unfill_slots is not None:
        flat = "\n".join(out).split("\n")
        # function ownership for post-pass lines: track .ent markers
        owners = []
        cur = None
        for line in flat:
            if line.startswith("\t.ent\t"):
                cur = line.split("\t")[-1]
            owners.append(cur)
        out = unfill_gcc_slots(flat, unfill_slots, lambda i: owners[i])

    if pin_slot:
        flat = "\n".join(out).split("\n")
        out = pin_slot_nops(flat, pin_slot)

    if hoist_div_arg:
        flat = "\n".join(out).split("\n")
        out = hoist_div_call_arg(flat, hoist_div_arg)

    if rebase_stack_mem:
        flat = "\n".join(out).split("\n")
        out = rebase_stack_memory(flat, rebase_stack_mem)

    if swap_adjacent:
        flat = "\n".join(out).split("\n")
        out = swap_adjacent_insns(flat, swap_adjacent)

    if rotate:
        flat = "\n".join(out).split("\n")
        out = rotate_insns(flat, rotate)

    if rotate_seq:
        flat = "\n".join(out).split("\n")
        out = rotate_insns_seq(flat, rotate_seq)

    if retime_branch:
        flat = "\n".join(out).split("\n")
        out = retime_branch_slot(flat, retime_branch)

    if zero_quad_store:
        flat = "\n".join(out).split("\n")
        out = zero_quad_stores(flat, zero_quad_store)

    if short_loop_pad:
        flat = "\n".join(out).split("\n")
        out = short_loop_pads(flat, short_loop_pad)

    if byte_move is not None:
        flat = "\n".join(out).split("\n")
        out = byte_move_andi(flat, byte_move)

    if split_hi_lo_raw:
        spec = {}
        for tok in split_hi_lo_raw:
            parts = tok.split(':')
            if len(parts) != 3:
                raise SystemExit("--split-hi-lo: bad site %r (want "
                                 "FUNC:IDX:NEWREG)" % tok)
            fn, idx_s, reg = parts
            if not idx_s.isdigit():
                raise SystemExit("--split-hi-lo: bad site %r (IDX must be "
                                 "a plain integer)" % tok)
            reg = reg.lstrip('$')
            if not re.match(r'^f?\d+$', reg):
                raise SystemExit("--split-hi-lo: bad register token %r in "
                                 "%r (want 2, $2, f4 or $f4)" % (reg, tok))
            spec.setdefault(fn, []).append((int(idx_s), reg))
        flat = "\n".join(out).split("\n")
        out = split_hi_lo(flat, spec)

    if swap_slot:
        flat = "\n".join(out).split("\n")
        out = swap_into_slot(flat, swap_slot)

    if swap_mem_slot:
        flat = "\n".join(out).split("\n")
        out = swap_into_slot(flat, swap_mem_slot, allow_stack_mem=True)

    if post_rotate_seq:
        flat = "\n".join(out).split("\n")
        out = rotate_insns_seq(flat, post_rotate_seq,
                               allow_nop_marker=True)

    if exchange_slot_prior:
        flat = "\n".join(out).split("\n")
        out = exchange_slot_with_prior(flat, exchange_slot_prior)

    if retime_branch_call:
        flat = "\n".join(out).split("\n")
        out = retime_branch_to_call(flat, retime_branch_call)

    if swap_slot_tgt:
        flat = "\n".join(out).split("\n")
        out = swap_slot_target(flat, swap_slot_tgt)

    if mtc1_nop:
        flat = "\n".join(out).split("\n")
        out = mtc1_hazard_nops(flat, mtc1_nop)

    if barrier_branch_move is not None:
        # Post-pass so it also covers lines synthesized by earlier passes
        # (e.g. the li.d expansion's trailing dsll32): keep a simple ALU
        # op or register copy out of a following plain branch's delay
        # slot by pinning it behind a reorder barrier.
        flat = "\n".join(out).split("\n")
        res = []
        for i, line in enumerate(flat):
            nxt = flat[i + 1] if i + 1 < len(flat) else ""
            prev = flat[i - 1] if i else ""
            if (RE_STORE.match(line)
                    and re.match(r'^\tjal\t', next_insn(flat, i) or "")
                    and "noreorder" not in prev
                    and "volatile" in nxt):
                # A (volatile MMIO) store left in reorder mode right
                # before a call: ee-as left the call's slot as a nop
                # instead of stealing the store (xglMovieClose).
                res.append("\t.set push\n\t.set noreorder\n" + line +
                           "\n\t.set pop")
            elif ((RE_RETURN_MOVE.match(line) or RE_BARRIER_ALU.match(line))
                    and RE_ANY_BRANCH.match(nxt)
                    and "noreorder" not in prev):
                res.append("\t.set push\n\t.set noreorder\n" + line +
                           "\n\t.set pop")
            else:
                res.append(line)
        out = res

    if war_restore is not None:
        flat = "\n".join(out).split("\n")
        out = war_restore_swap(flat, war_restore)

    if branch_likely or branch_unlikely:
        flat = "\n".join(out).split("\n")
        out = flip_branch_likely(flat, branch_likely or frozenset(),
                                 branch_unlikely or frozenset())

    with open(path, 'w') as f:
        f.write('\n'.join(out))


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("path")
    parser.add_argument("--omit-hazard", action="append", default=[])
    parser.add_argument("--barrier-return-store", nargs="?", const="",
                        default=None, metavar="FUNCS",
                        help="keep a trailing store out of a leaf return delay "
                             "slot; optionally a comma-separated list of "
                             "function names to scope the pass to")
    parser.add_argument("--barrier-branch-move", nargs="?", const="",
                        default=None, metavar="FUNCS",
                        help="keep a preceding move out of ANY branch delay "
                             "slot (incl. conditional branches); optionally "
                             "a comma-separated function-name list")
    parser.add_argument("--rotate", nargs="?", const="",
                        default=None, metavar="SITES",
                        help="rotate a window right by one: FUNC:N[:LEN] "
                             "(LEN defaults to 3); expresses the dependent "
                             "swaps --swap-adjacent cannot")
    parser.add_argument("--rotate-seq", default=None, metavar="SITES",
                        help="like --rotate, but the comma-separated sites "
                             "are applied ONE AT A TIME with indices "
                             "re-resolved after each, so overlapping "
                             "windows can both fire (order matters)")
    parser.add_argument("--post-rotate-seq", default=None, metavar="SITES",
                        help="apply sequential instruction rotations after "
                             "--swap-into-slot; for schedules whose call "
                             "delay-slot exchange must happen first")
    parser.add_argument("--hoist-div-arg", nargs="?", const="",
                        default=None, metavar="FUNCS",
                        help="hoist an independent $t0 fifth-call argument "
                             "above the R5900 signed-divide zero guard")
    parser.add_argument("--retime-branch-slot", nargs="?", const="",
                        default=None, metavar="FUNCS",
                        help="move a pre-branch store into a filled plain "
                             "branch slot and sink its dead call-arg move")
    parser.add_argument("--short-loop-pad", default=None, metavar="SITES",
                        help="set a gcc R5900 short-loop pad's nop count: "
                             "FUNC:N:COUNT, N being the 0-based pad index "
                             "in FUNC. gcc pads to a fixed total (8 here, "
                             "10 in the original build) and no source shape "
                             "moves it. Only rewrites blocks containing "
                             "nothing but nops.")
    parser.add_argument("--byte-move-andi", nargs="?", const="",
                        default=None, metavar="FUNCS",
                        help="rewrite a register copy of a just-lbu'd byte "
                             "to the explicit `andi $x,$y,0xff` zero-extend "
                             "the original build emitted; optionally a "
                             "comma-separated function-name list")
    parser.add_argument("--split-hi-lo", action="append", default=None,
                        metavar="FUNC:IDX:NEWREG",
                        help="rename one `lui`'s destination register and "
                             "the SOURCE (not destination) occurrence in "
                             "the next instruction reading it -- for a "
                             "%%hi materialised into a different register "
                             "than the %%lo fold uses, which --swap-regs "
                             "cannot express since it renames symmetrically. "
                             "IDX is the 0-based instruction index of the "
                             "`lui` (insn-only counting convention); "
                             "repeatable for multiple sites")
    parser.add_argument("--zero-quad-store", nargs="?", const="",
                        default=None, metavar="FUNCS",
                        help="rewrite gcc's `por $X,$0,$0` + `sq $X` "
                             "quadword-zero idiom to `nop` + `sq $0`, the "
                             "form the original build emitted")
    parser.add_argument("--fp-pair-hazard", nargs="?", const="",
                        default=None, metavar="FUNCS",
                        help="an mtc1 destination hazards its whole "
                             "even-aligned FP register PAIR, the way the "
                             "original ee-as's insn_uses_reg() did "
                             "((op & ~1) == (dest & ~1)); optionally a "
                             "comma-separated function-name list")
    parser.add_argument("--lis-hazard-nop", nargs="?", const="",
                        default=None, metavar="FUNCS",
                        help="add the ee-as hazard nop after a synthesized "
                             "li.s mtc1 when a COP1 compute follows")
    parser.add_argument("--barrier-lo-load", nargs="?", const="",
                        default=None, metavar="FUNCS",
                        help="keep %%lo() loads and la macros out of a "
                             "following call/branch delay slot")
    parser.add_argument("--unfill-gcc-slots", nargs="?", const="",
                        default=None, metavar="FUNCS_OR_SITES",
                        help="hoist gcc's own branch-delay-slot fills back "
                             "above the branch, leaving a literal nop. "
                             "Entries are FUNC (all of that function's "
                             "gcc-filled slots) or FUNC:N (just the Nth, "
                             "0-based, asm order). Per-site is normally "
                             "what you want -- the original fills most of "
                             "these slots and declines only a few")
    parser.add_argument("--pin-slot-nop", default=None, metavar="SITES",
                        help="comma-separated FUNC:N sites whose Nth "
                             "reorder-mode branch/jump (0-based, asm order) "
                             "gets a literal nop pinned into its delay slot")
    parser.add_argument("--war-restore-swap", nargs="?", const="",
                        default=None, metavar="FUNCS",
                        help="swap two adjacent epilogue $sp restores when "
                             "the preceding insn still reads the first one's "
                             "register (original sched2's WAR delay)")
    parser.add_argument("--branch-likely", default=None, metavar="SITES",
                        help="comma-separated FUNC:N sites whose Nth "
                             "conditional branch (0-based, asm order) is "
                             "flipped to the branch-likely (annulled) form")
    parser.add_argument("--branch-unlikely", default=None, metavar="SITES",
                        help="comma-separated FUNC:N sites flipped from the "
                             "branch-likely form back to the plain form")
    parser.add_argument("--swap-adjacent", default=None, metavar="SITES",
                        help="comma-separated FUNC:N sites: swap the adjacent "
                             "independent instruction pair whose FIRST insn is "
                             "the Nth instruction of FUNC (0-based, counting "
                             "every instruction line in emission order; "
                             "directives/labels/#-comments not counted). "
                             "Ineligible sites (branches, mutual register "
                             "dependence, two memory ops, non-adjacent lines) "
                             "are left untouched")
    parser.add_argument("--swap-into-slot", default=None, metavar="SITES",
                        help="comma-separated FUNC:N sites: at the Nth jal of "
                             "FUNC (0-based, emission order), exchange the "
                             "gcc-filled delay-slot insn with the insn "
                             "immediately before the jal's noreorder block")
    parser.add_argument("--swap-mem-into-slot", default=None,
                        metavar="SITES",
                        help="like --swap-into-slot, but also accept an "
                             "independent stack load/non-stack memory op pair")
    parser.add_argument("--exchange-slot-prior", default=None,
                        metavar="SPECS",
                        help="exchange a gcc-filled delay slot with its "
                             "BACK-th preceding independent instruction; "
                             "comma-separated FUNC:N:BACK specs")
    parser.add_argument("--retime-branch-call", default=None,
                        metavar="SITES",
                        help="retime UPDATE; branch/ARG; jal/NEXT to "
                             "branch/ARG; jal/UPDATE; NEXT at named "
                             "gcc-filled branch sites")
    parser.add_argument("--rebase-stack-mem", action="append", default=None,
                        metavar="FUNC:SITES:BASE:BIAS",
                        help="rewrite named OFF($sp) memory sites through an "
                             "already-live BASE=$sp+BIAS pointer, validating "
                             "that definition before every site")
    parser.add_argument("--swap-slot-target", default=None, metavar="SITES",
                        help="comma-separated FUNC:N sites: at the Nth "
                             "gcc-filled branch of FUNC (0-based, emission "
                             "order, counting only noreorder/nomacro fill "
                             "blocks), exchange the delay-slot insn with the "
                             "first insn after the branch's target label "
                             "(the original chose a different target-block "
                             "insn to hoist)")
    parser.add_argument("--mtc1-nop", default=None, metavar="SITES",
                        help="comma-separated FUNC:N sites: append a literal "
                             "nop after the Nth mtc1 of FUNC (0-based, "
                             "emission order in the post-processed asm) -- "
                             "the ee-as COP1-move stall pad no heuristic "
                             "reproduces")
    parser.add_argument("--swap-regs", action="append", default=None,
                        metavar="FUNC:A-B",
                        help="whole-function (or FUNC:A-B:LO-HI instruction-range) register-number swap: every "
                             "$A becomes $B and vice versa within FUNC "
                             "(raw register tokens, e.g. 2-3 for $v0/$v1 "
                             "or f4-f6 for $f4/$f6). An allocator naming "
                             "tie-break, "
                             "distinct from the scheduling fixers above; "
                             "repeatable for multiple FUNC:A-B sites")
    parser.add_argument("--swap-reg-sources", default=None,
                        metavar="SPECS",
                        help="swap two GPRs only in the source operands of "
                             "named addu/daddu sites; comma-separated "
                             "FUNC:N:A-B specs")
    parser.add_argument("--exchange-derived-results", default=None,
                        metavar="SITES",
                        help="exchange the destinations of an adjacent "
                             "common-base addu/addiu derived-value pair")
    parser.add_argument("--retime-byte-copy-guard", default=None,
                        metavar="SITES",
                        help="comma-separated FUNC:N guarded byte-copy "
                             "sites whose loop walker is initialized before "
                             "the guard and whose first byte loads directly "
                             "into the copy register")
    parser.add_argument("--split-loop-byte-load", default=None,
                        metavar="SPECS",
                        help="comma-separated FUNC:TEST:COPY sites where a "
                             "signed loop-test byte and unsigned copied byte "
                             "must be loaded separately")
    parser.add_argument("--coalesce-symbol-address", default=None,
                        metavar="SPECS",
                        help="comma-separated FUNC:HI:LO:REG sites whose "
                             "%hi/%lo symbol address must use one GPR")
    parser.add_argument("--swap-fp-operands", default=None, metavar="SITES",
                        help="comma-separated FUNC:N sites whose three-FPR "
                             "add.s or mul.s has its two commutative source "
                             "operands exchanged; destination and opcode are "
                             "unchanged")
    parser.add_argument("--swap-int-operands", default=None, metavar="SITES",
                        help="comma-separated FUNC:N sites whose accepted "
                             "three-GPR commutative instruction has its two "
                             "source operands exchanged; destination and "
                             "opcode are unchanged")
    parser.add_argument("--remat-call-constant", action="append", default=None,
                        metavar="FUNC:SAVED-CALLER-CARRY:VALUE",
                        help="strictly replace one callee-saved integer "
                             "literal live range spanning a call with caller-"
                             "saved rematerialization; validates the complete "
                             "save/compare/call/compare/restore shape")
    parser.add_argument("--expand-sym-loads", action="store_true",
                        help="also manually expand integer loads with "
                             "symbol(+off)(reg) addresses (see "
                             "RE_MEM_SYM_REG_LOAD)")
    parser.add_argument("--no-fill-delay", nargs="?", const="",
                        default=None, metavar="FUNCS",
                        help="pin an explicit nop into every plain branch's "
                             "delay slot (no gas filling at all); optionally "
                             "a comma-separated function-name list")
    parser.add_argument("--hoist-return-store", nargs="?", const="",
                        default=None, metavar="FUNCS",
                        help="move a store gcc placed in a leaf return's "
                             "delay slot to before the branch instead")
    parser.add_argument("--as-g0", action="store_true",
                        help="file is assembled with `as -G0` (see "
                             "configure.py asflags_for) -- li.s float "
                             "literals never land in .lit4, so they always "
                             "carry the mtc1 COP1 hazard")
    # A flag that stores (rather than appends) keeps only its LAST
    # occurrence, so repeating it silently drops the earlier sites --
    # that regressed three matched functions once. Reject it loudly.
    # Genuine append-flags (--omit-hazard, --swap-regs, ...) repeat fine.
    _appendable = set()
    for _act in parser._actions:
        if _act.__class__.__name__ == "_AppendAction":
            _appendable.update(_act.option_strings)
    _seen = set()
    for _a in sys.argv[1:]:
        if _a.startswith("--"):
            _name = _a.split("=")[0]
            if _name in _appendable:
                continue
            if _name in _seen:
                parser.error(
                    "duplicate flag %s: it stores rather than appends, so "
                    "only the last occurrence would survive and the earlier "
                    "sites would be silently dropped. Put every site for "
                    "this pass in one comma-separated value." % _name)
            _seen.add(_name)
    args = parser.parse_args()
    def scope(v):
        if v is None:
            return None
        return frozenset(f for f in v.split(',') if f)

    ASSUME_NO_LIT4 = args.as_g0
    main(args.path, set(args.omit_hazard), scope(args.barrier_return_store),
         scope(args.hoist_return_store), scope(args.barrier_branch_move),
         scope(args.no_fill_delay), args.expand_sym_loads,
         scope(args.unfill_gcc_slots), scope(args.barrier_lo_load),
         scope(args.branch_likely), scope(args.branch_unlikely),
         scope(args.war_restore_swap), scope(args.pin_slot_nop),
         scope(args.lis_hazard_nop), scope(args.swap_adjacent),
         scope(args.swap_into_slot), scope(args.mtc1_nop),
         scope(args.swap_mem_into_slot),
         [t for t in (args.exchange_slot_prior or "").split(',') if t],
         scope(args.retime_branch_call),
         scope(args.swap_slot_target), scope(args.rotate),
         args.swap_regs,
         [t for t in (args.swap_reg_sources or "").split(',') if t],
         scope(args.exchange_derived_results),
         scope(args.retime_byte_copy_guard),
         [t for t in (args.split_loop_byte_load or "").split(',') if t],
         [t for t in (args.coalesce_symbol_address or "").split(',') if t],
         scope(args.swap_fp_operands),
         scope(args.swap_int_operands),
         args.remat_call_constant,
         [t for t in (args.rotate_seq or "").split(',') if t],
         [t for t in (args.post_rotate_seq or "").split(',') if t],
         scope(args.hoist_div_arg),
         scope(args.retime_branch_slot),
         scope(args.zero_quad_store), scope(args.fp_pair_hazard),
         [t for t in (args.short_loop_pad or "").split(',') if t],
         scope(args.byte_move_andi), args.split_hi_lo,
         args.rebase_stack_mem)
