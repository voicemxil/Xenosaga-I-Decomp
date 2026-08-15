#!/usr/bin/env python3
"""Audit every reordering-flag match for hidden logic differences.

The reordering passes (--swap-adjacent, --rotate, --swap-into-slot,
--swap-slot-target) are only legitimate when the compiler emitted the
RIGHT INSTRUCTIONS IN THE WRONG ORDER: instruction scheduling is not
expressible in C, so a pure reorder is a genuine toolchain gap. They are
NOT legitimate as a way to paper over a C body that computes something
different -- that would silently encode wrong logic behind right bytes.

The two cases are machine-distinguishable. Recompile the file WITHOUT
its reordering flags and compare the instruction MULTISET against the
original:

    identical multiset  -> only the order differed. Genuine.
    different multiset  -> different instructions/registers were being
                           emitted, and the reorder is masking it. Suspect.

This audits every function named in a reordering flag and reports any
that fail the test, plus how many sites each needed (a function needing
many sites is worth a second look even when it passes -- it often means
the source shape is leading the scheduler somewhere the original's did
not go).

    python3 tools/audit_swaps.py            # audit all
    python3 tools/audit_swaps.py Menu.c     # one file
"""
import collections
import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.getcwd())
import ccpipe  # noqa: E402
from decomplib import Repo, masked_compare  # noqa: E402
from elflib import Elf32  # noqa: E402

def _identity(word):
    """Instruction identity with relocatable operands removed."""
    op = word >> 26
    if op == 0 or op == 16 or op == 17 or op == 18:
        return word                      # R-type / coprocessor: keep whole
    if op in (2, 3):
        return op << 26                  # j / jal: target is a reloc
    return word & 0xFFFF0000             # I-type: drop the immediate


REORDER_FLAGS = ("--swap-adjacent", "--rotate", "--swap-into-slot",
                 "--swap-slot-target")


def sources_by_basename():
    out = {}
    for root, _dirs, files in os.walk("src"):
        for f in files:
            if f.endswith(".c"):
                out[f] = os.path.join(root, f)
    return out


def main():
    only = sys.argv[1] if len(sys.argv) > 1 else None
    import configure
    srcs = sources_by_basename()
    repo = Repo()
    problems = []
    audited = 0
    for base, flagstr in sorted(configure.FILE_FIX_FLAGS.items()):
        if only and base != only:
            continue
        if not any(f in flagstr for f in REORDER_FLAGS):
            continue
        path = srcs.get(base)
        if not path:
            continue
        cc, cflags, fixflags, asflags = ccpipe.file_settings(base)
        fixflags = list(fixflags)
        # Strip the reordering flags and their site lists.
        stripped, sites = [], collections.defaultdict(list)
        i = 0
        while i < len(fixflags):
            if fixflags[i] in REORDER_FLAGS:
                for site in fixflags[i + 1].split(","):
                    fn = site.split(":")[0]
                    sites[fn].append(fixflags[i])
                i += 2
                continue
            stripped.append(fixflags[i])
            i += 1
        ok, log = ccpipe.compile_c(path, "/tmp/_audit.o", cc=cc,
                                   cflags=cflags, fixflags=tuple(stripped),
                                   asflags=asflags)
        if not ok:
            problems.append((base, "<file>", "does not compile without "
                                             "reordering flags"))
            continue
        elf = Elf32("/tmp/_audit.o")
        syms = elf.func_symbols_sized()
        for fn, used in sorted(sites.items()):
            loc = repo.orig_functions.get(fn)
            if fn not in syms or not loc:
                continue
            audited += 1
            off, _ = syms[fn]
            vaddr, size = loc
            orig = repo.orig.words_at_vaddr(vaddr, size)
            built = elf.words_at_offset(off, size)
            om, bm, diffs = masked_compare(orig, built, elf.relocs(), off)
            # Compare multisets on instruction IDENTITY, not raw words:
            # masked_compare blanks relocated immediates BY POSITION, so a
            # moved instruction gets masked at the wrong index and a pure
            # reorder would look like a changed multiset. Zero the immediate
            # of I-type ops (they are what carry %hi/%lo relocs) and the
            # target of J-type; R-type words are kept whole.
            same = (collections.Counter(map(_identity, orig)) ==
                    collections.Counter(map(_identity, built)))
            tag = "OK  " if same else "BAD "
            print(f"  {tag}{fn:38} {len(diffs):3} pre-fix diffs, "
                  f"{len(used)} site(s), identical multiset={same}")
            if not same:
                problems.append((base, fn,
                                 "reordering is masking a different "
                                 "instruction multiset"))
    print(f"\naudited {audited} reordering-flag functions")
    if problems:
        print("\nSUSPECT -- these are not pure reorders:")
        for base, fn, why in problems:
            print(f"  {base}: {fn}: {why}")
        return 1
    print("all reordering flags are pure instruction reorders "
          "(same instructions, different order)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
