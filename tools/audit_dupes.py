#!/usr/bin/env python3
"""Report functions defined by more than one source object.

    python3 tools/audit_dupes.py
    python3 tools/audit_dupes.py --quiet    # print only if there are dupes

This has cost real time twice, in two different disguises:

1. `libc.c` carried dead hand-written copies of _dtoa_r, quorem,
   _pow5mult and five more. libc.o sorts before newlib_*.o under
   --allow-multiple-definition, so the LINKER shipped the mismatching
   copies while verify.py checked the byte-exact ones next door and said
   OK. The image was wrong and every per-function check was green.

2. sefSetLightFlag / sefSetHitSignal / sefSetSeSignal existed as
   non-matching near-misses in sef.c AND as byte-exact versions in
   sefIs.c. `checkfile.py src/ssd/sef.c` diffed the STALE copies against
   the registered entries and reported three registered-and-failing
   functions that had in fact never been broken.

Both times the symptom pointed somewhere other than the cause. The rule
that falls out, for anyone writing a new function: GREP THE WHOLE
TERRITORY for the name first, not just the file you expect it in.

asm/ objects are excluded: the splat reference defines every function in
the game by design, so it collides with everything and means nothing.

A duplicate is not automatically a bug -- `frame_init` legitimately
names two different functions (EW.c's and libgcc's), which is why
gcc_frame.c registers its copy by address. What matters is that you know
they are there and which one the linker takes.
"""
import argparse
import collections
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from elflib import Elf32  # noqa: E402

ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)),
                                     ".."))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("-q", "--quiet", action="store_true",
                    help="print nothing when there are no duplicates")
    args = ap.parse_args()

    build = os.path.join(ROOT, "build")
    if not os.path.isdir(build):
        return 0
    defs = collections.defaultdict(set)
    for root, _dirs, files in os.walk(build):
        for f in files:
            if not f.endswith(".o"):
                continue
            path = os.path.join(root, f)
            rel = os.path.relpath(path, build)
            if rel.startswith("asm" + os.sep):
                continue                      # the splat reference: by design
            try:
                elf = Elf32(path)
            except Exception:
                continue
            for name, (_off, size) in elf.func_symbols_sized().items():
                if size:
                    defs[name].add(rel)

    multi = {k: sorted(v) for k, v in defs.items() if len(v) > 1}
    if not multi:
        if not args.quiet:
            print("no function is defined by more than one source object")
        return 0

    print("WARNING: %d function(s) defined by more than one source object."
          % len(multi))
    print("  The linker picks one; verify.py may be checking the other.")
    for name, objs in sorted(multi.items()):
        print("    %-28s %s" % (name, ", ".join(objs)))
    return 0


if __name__ == "__main__":
    sys.exit(main())
