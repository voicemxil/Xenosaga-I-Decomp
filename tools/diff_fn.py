#!/usr/bin/env python3
"""Show the masked word-level diff for one decompiled function.

    python3 tools/diff_fn.py FUNCTION_NAME
    python3 tools/diff_fn.py FUNCTION_NAME --all      # print every word, not
                                                      # just the differing ones

Uses the same masking rules as verify.py (jal/j targets, gp_rel/hi/lo
immediates) and prints original vs built mnemonics around each diff, then
a one-line triage verdict (register tie-break / scheduling / nop count /
immediate / real logic difference).
"""
import os
import re
import subprocess
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from decomplib import Repo, parse_decompiled, ELF  # noqa: E402
from triagelib import triage  # noqa: E402

OBJDUMP = "/usr/local/ps2dev/ee/bin/mips64r5900el-ps2-elf-objdump"


def disasm_lines(cmd, count):
    """Disassembly text, one entry per instruction word.

    -z (disassemble-zeroes) is essential: without it objdump collapses runs
    of zero words into `...` and the text no longer lines up with the words.
    """
    result = subprocess.run(cmd, capture_output=True, text=True)
    lines = []
    for line in result.stdout.split('\n'):
        if re.match(r'\s+[0-9a-f]+:\s+([0-9a-f]{8})\b', line):
            lines.append(line.strip())
    if len(lines) < count:
        lines += [""] * (count - len(lines))
    return lines


def main():
    args = [a for a in sys.argv[1:] if not a.startswith("-")]
    show_all = "--all" in sys.argv
    if not args:
        sys.exit(__doc__)
    name = args[0]

    entries, hardware = parse_decompiled()
    entry = next((e for e in entries + hardware if e.name == name), None)
    if entry is None:
        sys.exit(f"{name} not found in config/decompiled.txt")

    repo = Repo()
    r = repo.compare(name, entry.addr, entry.size)
    if r["status"] == "SKIP":
        sys.exit(f"{name}: {r['reason']}")

    om, bm = r["masked_orig"], r["masked_built"]
    diffs = r["diffs"]
    n = max(len(om), len(bm))
    orig_lines = disasm_lines(
        [OBJDUMP, "-d", "-z", "-j", ".text",
         f"--start-address={hex(entry.addr)}",
         f"--stop-address={hex(entry.addr + entry.size)}", ELF], n)
    built_lines = disasm_lines(
        [OBJDUMP, "-d", "-z", "-r", f"--start-address={hex(r['offset'])}",
         f"--stop-address={hex(r['offset'] + entry.size)}", r["obj"]], n)

    if len(om) != len(bm):
        print(f"length mismatch: orig {len(om)} words, built {len(bm)} words")
    if not diffs and len(om) == len(bm):
        print(f"{name}: MATCH ({len(om)} words)")
        return
    print(f"{name}: {len(diffs)} differing word(s) of {len(om)}")
    show = range(min(len(om), len(bm))) if show_all else diffs
    for i in show:
        mark = "*" if (show_all and i in diffs) else " "
        print(f" {mark}[{i:3}] orig:  {orig_lines[i]}")
        print(f"        built: {built_lines[i]}")
    label, detail = triage(om, bm, diffs)
    print(f"\n  triage: {label} -- {detail}")


if __name__ == "__main__":
    main()
