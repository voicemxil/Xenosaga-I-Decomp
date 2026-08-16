#!/usr/bin/env python3
"""Standalone diff for one function in one file, without needing a full
ninja build or config/decompiled.txt registration. Compiles the file fresh
via the same pipeline as checkfile.py, then prints a diff_fn.py-style
word-level diff for one named function using the ELF's own symbol bounds.

    python3 tools/scratch_diff.py src/xglSound.c xglSoundSendSwd
"""
import os
import re
import subprocess
import sys
import tempfile

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from decomplib import Repo, masked_compare  # noqa: E402
from triagelib import triage  # noqa: E402
import ccpipe  # noqa: E402

OBJDUMP = "/usr/local/ps2dev/ee/bin/mips64r5900el-ps2-elf-objdump"
ELF = "elf/SLUS_204.69"


def disasm_lines(cmd, count):
    result = subprocess.run(cmd, capture_output=True, text=True)
    lines = []
    for line in result.stdout.split('\n'):
        if re.match(r'\s+[0-9a-f]+:\s+([0-9a-f]{8})\b', line):
            lines.append(line.strip())
    if len(lines) < count:
        lines += [""] * (count - len(lines))
    return lines


def main():
    src, name = sys.argv[1], sys.argv[2]
    show_all = "--all" in sys.argv
    base = os.path.basename(src)
    repo = Repo()
    loc = repo.orig_functions.get(name)
    if not loc:
        sys.exit(f"{name} not in original ELF symbol table")
    addr, size = loc

    cc, cflags, fix, asflags = ccpipe.file_settings(base)
    tmpdir = tempfile.mkdtemp(prefix="scratchdiff_")
    objpath = os.path.join(tmpdir, base.replace(".c", ".o"))
    ok, log = ccpipe.compile_c(src, objpath, cc=cc, cflags=cflags, fixflags=fix,
                               asflags=asflags)
    if not ok:
        sys.exit(f"COMPILE FAILED\n{log}")

    from elflib import Elf32
    e = Elf32(objpath)
    funcs = e.func_symbols_sized()
    if name not in funcs:
        sys.exit(f"{name} not found in compiled {objpath}")
    off, bsize = funcs[name]
    relocs = e.relocs()
    n = min(size, bsize)
    orig = repo.orig.words_at_vaddr(addr, n)
    built = e.words_at_offset(off, n)
    om, bm, diffs = masked_compare(orig, built, relocs, off)

    # Overlay functions (ov02 and friends) live in their OWN ELF section,
    # not .text -- hard-coding `-j .text` printed an empty original side
    # for every one of them and made a length mismatch look like a total
    # rewrite. Ask the ELF which section actually contains the address.
    sec = ".text"
    for s in repo.orig.sections:
        if s["addr"] and s["addr"] <= addr < s["addr"] + s["size"]:
            sec = s["name"]
            break
    orig_lines = disasm_lines(
        [OBJDUMP, "-d", "-z", "-j", sec,
         f"--start-address={hex(addr)}",
         f"--stop-address={hex(addr + n)}", ELF], n // 4)
    built_lines = disasm_lines(
        [OBJDUMP, "-d", "-z", "-r", f"--start-address={hex(off)}",
         f"--stop-address={hex(off + n)}", objpath], n // 4)

    if size != bsize:
        print(f"length mismatch: orig {size} bytes, built {bsize} bytes")
    if not diffs and size == bsize:
        print(f"{name}: MATCH ({n // 4} words)")
        return
    print(f"{name}: {len(diffs)} differing word(s) of {n // 4}")
    show = range(min(len(om), len(bm))) if show_all else diffs
    for i in show:
        mark = "*" if (show_all and i in diffs) else " "
        print(f" {mark}[{i:3}] orig:  {orig_lines[i] if i < len(orig_lines) else ''}")
        print(f"        built: {built_lines[i] if i < len(built_lines) else ''}")
    if diffs:
        label, detail = triage(om, bm, [d for d in diffs if d < len(bm)])
        print(f"\n  triage: {label} -- {detail}")


if __name__ == "__main__":
    main()
