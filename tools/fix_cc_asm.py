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
# sequence, then shift back into place with dsll/dsll32. When the low 32
# bits are not all zero (irrational-looking constants with few trailing
# zero bits), fall back to building hi32 and lo32 halves in two registers
# and OR-ing them together -- unverified against real bytes for that path,
# but it is the standard MIPS 64-bit constant idiom.
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


def fp_hazard_dest(line):
    """FP register written by an mtc1-class instruction, or None.

    The COP1 move hazard is REGISTER-dependent, not opcode-dependent: the
    original pads only when the next FP instruction READS the register the
    mtc1 just wrote. (CheckSlope has `mtc1 $zero,$f5` followed directly by
    `mul.s $f1,$f1,$f0` with no nop; SEQ_motion has `mtc1 $v1,$f1` + nop +
    `mul.s $f1,$f1,$f20`.) A li.s whose constant has non-zero low 16 bits
    is expanded by gas as a .lit4 LOAD, not lui+mtc1, so it carries no
    hazard at all.
    """
    m = re.match(r'^\tmtc1\t\$\w+,(\$f\d+)', line)
    if m:
        return m.group(1)
    m = re.match(r'^\tli\.s\t(\$f\d+),\s*(\S+)', line)
    if m:
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


def synth_li_d(reg, bits):
    """Emit an instruction sequence loading the 64-bit pattern `bits` into
    `reg`, replacing an unassemblable `li.d`. See RE_LID comment above."""
    if bits == 0:
        return [f"\tmove\t{reg},$0"]
    tz = (bits & -bits & 0xFFFFFFFFFFFFFFFF).bit_length() - 1
    sig = bits >> tz
    if sig.bit_length() <= 32:
        out = synth_load32(reg, sig)
    else:
        # Low 32 bits are not all zero: build hi32/lo32 halves separately
        # and merge with an `or`, using $at as scratch.
        hi32 = (bits >> 32) & 0xFFFFFFFF
        lo32 = bits & 0xFFFFFFFF
        out = synth_load32(reg, hi32)
        out.append(f"\tdsll32\t{reg},{reg},0")
        if lo32:
            at = "$1" if reg != "$1" else "$2"
            out += synth_load32(at, lo32)
            out.append(f"\tdsll32\t{at},{at},0")
            out.append(f"\tdsrl32\t{at},{at},0")
            out.append(f"\tor\t{reg},{reg},{at}")
        return out
    if tz:
        if tz < 32:
            out.append(f"\tdsll\t{reg},{reg},{tz}")
        else:
            out.append(f"\tdsll32\t{reg},{reg},{tz - 32}")
    return out


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


def main(path, omitted_hazards, barrier_return_store=None,
         hoist_return_store=None):
    # Each flag is either None (off), an empty tuple (whole file), or a set
    # of function names to scope the pass to.
    with open(path) as f:
        lines = f.read().split('\n')

    if hoist_return_store is not None:
        lines = hoist_return_delay_stores(lines, hoist_return_store)

    owner = function_at(lines)

    out = []
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
            out.append('\n'.join(synth_li_d(reg, bits)))
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

        if (barrier_return_store is not None
                and ((RE_STORE.match(line) and RE_RETURN.match(following))
                     or (RE_RETURN_MOVE.match(line)
                         and RE_RETURN_OR_CALL.match(following)))
                and in_scope(owner[i], barrier_return_store)):
            out.append("\t.set push\n\t.set noreorder\n" + line +
                       "\n\t.set pop")
            continue

        if RE_LA.match(line) or RE_MEM.match(line) or RE_CVT.match(line):
            # When an `la reg, sym(idx)` macro is the last computation before
            # a leaf return, the modern assembler pulls its final addu into
            # the jr delay slot; the original ee-as left a nop. Barrier it.
            if RE_LA.match(line) and RE_RETURN.match(following):
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

        if RE_LIS.match(line):
            wrapped = wrap_mips1(line)
            if needs_hazard_nop:
                wrapped += "\n\tnop"
            out.append(wrapped)
            continue

        if needs_hazard_nop:
            out.append(line + "\n\tnop")
            continue

        out.append(line)

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
    parser.add_argument("--hoist-return-store", nargs="?", const="",
                        default=None, metavar="FUNCS",
                        help="move a store gcc placed in a leaf return's "
                             "delay slot to before the branch instead")
    args = parser.parse_args()
    def scope(v):
        if v is None:
            return None
        return frozenset(f for f in v.split(',') if f)

    main(args.path, set(args.omit_hazard), scope(args.barrier_return_store),
         scope(args.hoist_return_store))
