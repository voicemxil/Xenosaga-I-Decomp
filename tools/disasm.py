#!/usr/bin/env python3
"""Disassemble a function from the ORIGINAL ELF, by name.

    python3 tools/disasm.py FUNCTION_NAME
    python3 tools/disasm.py FUNCTION_NAME --raw     # bare hex words
    python3 tools/disasm.py --at 0xA023A8 --size 0xAC

Use this instead of grepping asm/cod/000000.s. Two reasons:

1. Overlay functions live in a separate ELF load segment, so reading them
   directly from the ELF avoids depending on a generated assembly reference.
   (The ov02 reference is now generated from the real ELF section rather than
   the old zero-filled logical-ROM placeholder.)
2. The asm comment format is `/* FILEOFF VADDR */`, and reading the first
   column has cost this project twice -- once, 63 functions all wrong by
   0x200000. Here you pass a name and never see an address.

Bounds come from the ELF's own symbol table, which survived unstripped.
"""
import os
import subprocess
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from decomplib import Repo, ELF  # noqa: E402

OBJDUMP = "/usr/local/ps2dev/ee/bin/mips64r5900el-ps2-elf-objdump"


def main():
    args = [a for a in sys.argv[1:] if not a.startswith("--")]
    raw = "--raw" in sys.argv
    repo = Repo()

    if "--at" in sys.argv:
        addr = int(sys.argv[sys.argv.index("--at") + 1], 0)
        size = int(sys.argv[sys.argv.index("--size") + 1], 0) if "--size" in sys.argv else 0x80
        name = "0x%08X" % addr
    else:
        if not args:
            sys.exit(__doc__)
        name = args[0]
        fn = repo.orig_functions.get(name)
        if not fn:
            sys.exit("%s not found in the ELF symbol table" % name)
        addr, size = fn

    out = subprocess.run(
        [OBJDUMP, "-d", "-z", f"--start-address={hex(addr)}",
         f"--stop-address={hex(addr + size)}", ELF],
        capture_output=True, text=True).stdout

    print("/* %s @ %#010x, %#x bytes (%d instructions) */" % (name, addr, size, size // 4))
    for line in out.split("\n"):
        s = line.strip()
        if not s or ":" not in s:
            continue
        parts = s.split("\t")
        if len(parts) < 2:
            continue
        if raw:
            print(parts[1].strip())
        else:
            print("    " + "\t".join(p.strip() for p in parts[1:]))


if __name__ == "__main__":
    main()
