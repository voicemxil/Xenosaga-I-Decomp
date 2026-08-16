#!/usr/bin/env python3
"""Find source files that span more than one ORIGINAL translation unit.

    python3 tools/tu_boundaries.py            # files that span a boundary
    python3 tools/tu_boundaries.py --all      # every file, with its TU count
    python3 tools/tu_boundaries.py --map      # the raw boundary list

The original build left 415 `__gnu_compiled_c` markers in the ELF, one
per translation unit. Our tree has far fewer .c files, so several of
ours are amalgamations of two or more original TUs.

That matters because per-file COMPILER FLAGS are the thing you cannot
reach with any source-level lever. Two TUs merged into one of our files
must share one flag set, and if the original built them differently,
every function in the minority half is unmatchable no matter how the C
is written. It has bitten twice:

  kernel.c   built with the wrong compiler entirely; invisible because
             its 151 frameless syscall stubs matched anyway, and only a
             framed function exposed the 16-vs-8 byte register-save
             stride.
  mpeg.c     hid the MPEG2 picture/slice decoder, which was built
             WITHOUT -fno-schedule-insns. Splitting it out into
             sceMpegDec.c turned four near-misses into matches with NO
             SOURCE CHANGE, and took one function from 21 differing
             words to zero.

The fingerprint from the second case, worth knowing: address
materialisation interleaved AROUND an intervening store is something
-fno-schedule-insns can never emit. A whole file showing a consistent
scheduling difference is a TU-boundary suspicion, not a lever hunt.

A file spanning a boundary is not automatically wrong -- the original
may well have used the same flags for both halves. This only tells you
where to look when a whole file is behaving oddly.
"""
import argparse
import collections
import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from elflib import Elf32  # noqa: E402

ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)),
                                     ".."))
ELF = os.path.join(ROOT, "elf", "SLUS_204.69")
LEDGER = os.path.join(ROOT, "config", "decompiled.txt")


def boundaries():
    elf = Elf32(ELF)
    out = sorted({v for n, v, _s, _t, _sec in elf.symbols()
                  if n and "gnu_compiled" in n})
    return out


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--all", action="store_true")
    ap.add_argument("--map", action="store_true")
    args = ap.parse_args()

    marks = boundaries()
    if args.map:
        print("%d translation-unit markers" % len(marks))
        for v in marks:
            print("  0x%08X" % v)
        return 0

    def tu_of(addr):
        """Index of the TU containing addr (markers are TU starts)."""
        lo, hi = 0, len(marks)
        while lo < hi:
            mid = (lo + hi) // 2
            if marks[mid] <= addr:
                lo = mid + 1
            else:
                hi = mid
        return lo - 1

    # source file -> {tu index -> [(name, addr, size)]}
    byfile = collections.defaultdict(lambda: collections.defaultdict(list))
    for line in open(LEDGER):
        m = re.match(r'\s*(\w+)\s*=\s*(0x[0-9A-Fa-f]+)\s*,\s*(0x[0-9A-Fa-f]+)',
                     line)
        if not m:
            continue
        c = line.find("//")
        src = line[c + 2:].strip().split()[0] if c >= 0 else ""
        if not src:
            continue
        addr, size = int(m.group(2), 16), int(m.group(3), 16)
        byfile[src][tu_of(addr)].append((m.group(1), addr, size))

    rows = []
    for src, tus in byfile.items():
        real = {k: v for k, v in tus.items() if k >= 0}
        if len(real) > 1 or args.all:
            rows.append((len(real), src, real))
    rows.sort(reverse=True)

    if not rows:
        print("no source file spans more than one original TU")
        return 0

    print("%d source file(s) span more than one ORIGINAL translation unit."
          % len([r for r in rows if r[0] > 1]))
    print("Per-file compiler flags cannot differ within one of our files, so")
    print("if the original built the halves differently, the minority half is")
    print("unmatchable by any source change. See mpeg.c -> sceMpegDec.c.\n")
    for count, src, tus in rows:
        if count <= 1 and not args.all:
            continue
        print("%-24s %d TUs" % (src, count))
        for tu in sorted(tus):
            fns = sorted(tus[tu], key=lambda f: f[1])
            span = "0x%08X-0x%08X" % (fns[0][1], fns[-1][1] + fns[-1][2])
            names = ", ".join(f[0] for f in fns[:4])
            if len(fns) > 4:
                names += ", +%d more" % (len(fns) - 4)
            print("    TU %-4d %-23s %2d fn  %s" % (tu, span, len(fns), names))
    return 0


if __name__ == "__main__":
    sys.exit(main())
