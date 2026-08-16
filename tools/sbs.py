#!/usr/bin/env python3
"""Side-by-side original vs freshly-compiled disassembly for ONE function,
even when it is not yet in config/decompiled.txt.

    python3 tools/sbs.py src/game/File.c FileObjectJpegDecChange
"""
import os, subprocess, sys, tempfile
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from decomplib import Repo, ELF
from elflib import Elf32
import ccpipe

OBJDUMP = "/usr/local/ps2dev/ee/bin/mips64r5900el-ps2-elf-objdump"

def dis(path, start, stop, elf=True):
    out = subprocess.run([OBJDUMP, "-d", "-z", f"--start-address={hex(start)}",
                          f"--stop-address={hex(stop)}", path],
                         capture_output=True, text=True).stdout
    rows = []
    for line in out.split("\n"):
        s = line.strip()
        if ":" not in s:
            continue
        parts = s.split("\t")
        if len(parts) < 3:
            if len(parts) == 2 and len(parts[1].strip()) == 8:
                rows.append("")
            continue
        rows.append("\t".join(parts[2:]).strip())
    return rows

def main():
    src, fn = sys.argv[1], sys.argv[2]
    repo = Repo()
    addr, osize = repo.orig_functions[fn]
    tmp = tempfile.mkdtemp()
    obj = os.path.join(tmp, "o.o")
    cc, cflags, fix, asflags = ccpipe.file_settings(os.path.basename(src))
    ok, log = ccpipe.compile_c(src, obj, cc=cc, cflags=cflags, fixflags=fix,
                               asflags=asflags)
    if not ok:
        sys.exit(log)
    e = Elf32(obj)
    off, bsize = e.func_symbols_sized()[fn]
    a = dis(ELF, addr, addr + osize)
    b = dis(obj, off, off + bsize)
    print(f"{fn}: orig {osize//4} words, built {bsize//4} words")
    for i in range(max(len(a), len(b))):
        x = a[i] if i < len(a) else ""
        y = b[i] if i < len(b) else ""
        print("%-40s %s %s" % (x, " " if x == y else "|", y))

main()
