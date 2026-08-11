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

MEM_OPS = ("sw", "sh", "sb", "swc1", "lw", "lh", "lb", "lbu", "lhu",
           "lwc1", "s.s", "l.s")

RE_MOVE = re.compile(r'\tmove\t(\$[0-9a-z]+),(\$[0-9a-z]+)')
RE_LA = re.compile(r'^\tla[ \t](.*)$')
RE_MEM = re.compile(r'^\t(' + '|'.join(re.escape(op) for op in MEM_OPS) +
                    r')[ \t]([^,]*),([A-Za-z_.].*)$')
RE_LIS = re.compile(r'^\tli\.s[ \t](.*)$')
RE_CVT = re.compile(r'^\t(cvt\.[a-z.]+|trunc\.w\.s)[ \t](.*)$')
# FP loads whose result feeds a COP1 compute need the hazard nop the
# original ee-as inserted (modern gas does not).
RE_FPLOAD = re.compile(r'^\t(li\.s|l\.s|lwc1|mtc1)[ \t]')
RE_FPCOMPUTE = re.compile(
    r'^\t(c\.[a-z]+\.s|mul\.s|div\.s|add\.s|sub\.s|mov\.s|abs\.s|neg\.s'
    r'|sqrt\.s|trunc\.w\.s|cvt\.[a-z.]+)[ \t]')
RE_FP_BRANCH_LIKELY = re.compile(r'^\tbc1[ft]l[ \t]')
# A leaf return: `j $31` or `jr $ra`/`jr $31`.
RE_RETURN = re.compile(r'^\t(j\t\$31|jr\t\$(ra|31))\b')


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


def main(path, omitted_hazards):
    with open(path) as f:
        lines = f.read().split('\n')

    out = []
    for i, line in enumerate(lines):
        line = RE_MOVE.sub(r'\tdaddu\t\1,\2,$0', line)

        following = next_insn(lines, i)
        omitted = any(following.startswith("\t" + op) for op in omitted_hazards)
        needs_hazard_nop = (RE_FPLOAD.match(line)
                            and RE_FPCOMPUTE.match(following)
                            and not omitted
                            and not RE_FP_BRANCH_LIKELY.match(previous_insn(lines, i)))

        if RE_LA.match(line) or RE_MEM.match(line) or RE_CVT.match(line):
            # When an `la reg, sym(idx)` macro is the last computation before
            # a leaf return, the modern assembler pulls its final addu into
            # the jr delay slot; the original ee-as left a nop. Barrier it.
            if RE_LA.match(line) and RE_RETURN.match(following):
                out.append(wrap_mips1_noreorder(line))
                continue
            wrapped = wrap_mips1(line)
            if needs_hazard_nop:
                wrapped += "\n\tnop"
            # Modern gas otherwise moves a conversion fed by mtc1 into a
            # following return delay slot; the original assembler kept it
            # in place after the mtc1 hazard nop.
            if RE_CVT.match(line) and re.match(r'^\tmtc1[ \t]', previous_insn(lines, i)):
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
    args = parser.parse_args()
    main(args.path, set(args.omit_hazard))
