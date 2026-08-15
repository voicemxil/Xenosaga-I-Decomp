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


def reads_fp_reg(insn, reg):
    """True if `insn` reads `reg` as a source operand."""
    m = re.match(r'^\t([a-z0-9.]+)\t(.*)$', insn)
    if not m:
        return False
    mnem = m.group(1)
    ops = [o.strip() for o in m.group(2).split(',')]
    # every operand of a compare is a source; otherwise operand 0 is the dest
    sources = ops if mnem.startswith('c.') else ops[1:]
    return reg in sources
RE_FPCOMPUTE = re.compile(
    r'^\t(c\.[a-z]+\.s|mul\.s|div\.s|add\.s|sub\.s|mov\.s|abs\.s|neg\.s'
    r'|sqrt\.s|trunc\.w\.s|cvt\.[a-z.]+)[ \t]')
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


def unfill_gcc_slots(flat, scope, owner_of):
    """Rewrite gcc's own delay-slot fills back to hoisted-insn + nop.

    gcc -fdelayed-branch emits
        .set noreorder / .set nomacro / <branch> / <slot> / .set macro
        / .set reorder
    The original compiler build declines to fill some of these; with
    this pass the slot instruction is moved ABOVE the branch and a
    literal nop takes its place. Safe because gcc only fills a slot
    with an instruction that neither feeds the branch condition nor
    depends on it. Opt-in per function like the barrier passes.
    """
    res = []
    i = 0
    while i < len(flat):
        if (flat[i].strip() == ".set	noreorder" or
                flat[i].strip() == ".set noreorder") and i + 3 < len(flat):
            block = [x.strip().replace("\t", " ") for x in flat[i:i+4]]
            if (block[1].startswith(".set") and "nomacro" in block[1]
                    and RE_ANY_BRANCH.match(flat[i+2])
                    and flat[i+3].startswith("\t")
                    and not flat[i+3].strip().startswith(".")
                    and in_scope(owner_of(i), scope)):
                res.append(flat[i+3])          # hoisted slot insn
                res.append(flat[i])            # .set noreorder
                res.append(flat[i+1])          # .set nomacro
                res.append(flat[i+2])          # branch
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


def rotate_insns(flat, sites):
    """Rotate a short window of instructions right by one.

    Site-keyed FUNC:N[:LEN] -- the window is the LEN instructions (LEN
    defaults to 3) starting at the Nth instruction of FUNC, 0-based in
    emission order, counted exactly like --swap-adjacent (directives,
    labels and #-comments are not instructions). The window becomes
    (last, first, ..., second-to-last).

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
            if span and span >= 2 and i + span <= len(flat):
                window = flat[i:i + span]
                if all(RE_INSN.match(w) for w in window):
                    res.append(window[-1])
                    res.extend(window[:-1])
                    idx += span
                    i += span
                    continue
            idx += 1
        res.append(line)
        i += 1
    return res


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
            if (f"{cur}:{idx}" in sites and i + 1 < len(flat)
                    and swap_ok(line, flat[i + 1])):
                res.append(flat[i + 1])
                res.append(line)
                idx += 2
                i += 2
                continue
            idx += 1
        res.append(line)
        i += 1
    return res


def swap_into_slot(flat, sites):
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
                if (sets == [".set noreorder", ".set nomacro"]
                        and swap_ok(a, slot)
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
         swap_slot_tgt=None, rotate=None):
    # Each flag is either None (off), an empty tuple (whole file), or a set
    # of function names to scope the pass to.
    with open(path) as f:
        lines = f.read().split('\n')

    if hoist_return_store is not None:
        lines = hoist_return_delay_stores(lines, hoist_return_store)

    owner = function_at(lines)

    # NOTE: a chain-tracking extension was tried here (accumulate pending
    # mtc1 destinations across intervening non-compute instructions, under
    # ASSUME_NO_LIT4) to close floorf/__ieee754_atan2f's missing hazard
    # nop between two back-to-back mtc1s. It reproduced those two but
    # regressed scalbnf and atanf (real, previously-matching functions
    # gained spurious nops), and a closer look shows the true rule is NOT
    # simply "any pending mtc1 register read by a later compute": atanf
    # has the identical shape (mtc1->A, mtc1->B, compute reading A) with
    # NO nop in the original, while floorf/atan2f need one. The
    # distinguishing factor is not the register, the instruction count
    # between them, or whether B is itself later consumed -- all of which
    # were tried and ruled out. Left unsolved; see the resume doc.
    out = []
    lit_pool = []
    for i, line in enumerate(lines):
        line = RE_MOVE.sub(r'\tdaddu\t\1,\2,$0', line)

        following = next_insn(lines, i)
        omitted = any(following.startswith("\t" + op) for op in omitted_hazards)
        hazard_dest = fp_hazard_dest(line)
        needs_hazard_nop = (hazard_dest is not None
                            and RE_FPCOMPUTE.match(following)
                            and reads_fp_reg(following, hazard_dest)
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
            # _dtoa_r's `d.d *= 10.` at $L416.
            if re.match(r'^\t(jal|j)\t[A-Za-z_]', following):
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

    if swap_adjacent:
        flat = "\n".join(out).split("\n")
        out = swap_adjacent_insns(flat, swap_adjacent)

    if rotate:
        flat = "\n".join(out).split("\n")
        out = rotate_insns(flat, rotate)

    if swap_slot:
        flat = "\n".join(out).split("\n")
        out = swap_into_slot(flat, swap_slot)

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
    parser.add_argument("--lis-hazard-nop", nargs="?", const="",
                        default=None, metavar="FUNCS",
                        help="add the ee-as hazard nop after a synthesized "
                             "li.s mtc1 when a COP1 compute follows")
    parser.add_argument("--barrier-lo-load", nargs="?", const="",
                        default=None, metavar="FUNCS",
                        help="keep %%lo() loads and la macros out of a "
                             "following call/branch delay slot")
    parser.add_argument("--unfill-gcc-slots", nargs="?", const="",
                        default=None, metavar="FUNCS",
                        help="hoist gcc's own branch-delay-slot fills back "
                             "above the branch, leaving a literal nop")
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
         scope(args.swap_slot_target), scope(args.rotate))
