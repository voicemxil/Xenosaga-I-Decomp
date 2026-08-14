#!/usr/bin/env python3
"""Compile one src/*.c and diff EVERY function in it against the original.

    python3 tools/checkfile.py src/ssd.c            # compile fresh, check all
    python3 tools/checkfile.py src/ssd.c --built    # use build/src/ssd.o
    python3 tools/checkfile.py src/ssd.c --new      # only functions that are
                                                    # not yet in decompiled.txt
    python3 tools/checkfile.py --all --built        # sweep every source file

Compiles through the exact ninja pipeline (compiler, flags and fix_cc_asm
flags come from configure.py) into a temp dir -- it never touches build/,
so it is safe to run while other agents are building.

Function bounds come from config/decompiled.txt when the function is
registered there, otherwise from the original ELF's own symbol table, so
functions that have never been registered are checked too. Matching
functions that are not yet registered are printed as ready-to-paste
config/decompiled.txt lines.
"""
import argparse
import glob
import os
import sys
import tempfile

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from decomplib import Repo, parse_decompiled, masked_compare  # noqa: E402
from elflib import Elf32  # noqa: E402
from triagelib import triage  # noqa: E402
import ccpipe  # noqa: E402

NOP = 0


def check_object(repo, objpath, srcname, registered, only_new=False,
                 verbose=False):
    e = Elf32(objpath)
    funcs = e.func_symbols_sized()
    relocs = e.relocs()
    results = []
    for name, (off, bsize) in sorted(funcs.items(), key=lambda kv: kv[1][0]):
        if bsize == 0:
            continue
        reg = registered.get(name)
        if only_new and reg:
            continue
        # The original ELF kept its symbol table, so prefer its bounds:
        # they are authoritative and catch a wrong size in decompiled.txt
        # (a truncated size makes verify.py compare a prefix and pass).
        loc = repo.orig_functions.get(name)
        regnote = ""
        if loc:
            addr, osize = loc
            if reg and reg[0] != addr:
                regnote = (f"  [!] decompiled.txt says {reg[0]:#010x}, the ELF "
                           f"symbol is at {addr:#010x}")
            elif reg and reg[1] != osize:
                regnote = (f"  [!] decompiled.txt size {reg[1]:#x} vs ELF "
                           f"symbol size {osize:#x}")
        elif reg:
            addr, osize = reg
        else:
            results.append((name, "ABSENT", "not in the original ELF", None))
            continue
        n = min(osize, bsize)
        orig = repo.orig.words_at_vaddr(addr, n)
        built = e.words_at_offset(off, n)
        om, bm, diffs = masked_compare(orig, built, relocs, off)
        note = ""
        # A size difference that is only trailing nops is alignment padding
        # (the original ELF symbol and the compiled symbol disagree about
        # whether the pad belongs to the function); not a real diff.
        if osize != bsize:
            if osize > bsize:
                tail = repo.orig.words_at_vaddr(addr + bsize, osize - bsize)
                where = "original"
            else:
                tail = e.words_at_offset(off + osize, bsize - osize)
                where = "built"
            if all(w == NOP for w in tail):
                note = f" (+{len(tail)} trailing pad nop(s) in the {where})"
            else:
                om = om + [w for w in (tail if where == "original" else [])]
                bm = bm + [w for w in (tail if where == "built" else [])]
                diffs = diffs + list(range(n // 4, max(osize, bsize) // 4))
        if not diffs:
            results.append((name, "MATCH", f"{n // 4} words{note}{regnote}",
                            (addr, min(osize, bsize))))
        else:
            label, detail = triage(om, bm, [d for d in diffs if d < len(bm)])
            results.append((name, f"{len(diffs)} diffs",
                            f"{label} -- {detail}{note}{regnote}", None))
    return results


def main():
    ap = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("source", nargs="?")
    ap.add_argument("--all", action="store_true", help="every src/*.c")
    ap.add_argument("--built", action="store_true",
                    help="use the existing build/src/*.o instead of compiling")
    ap.add_argument("--new", action="store_true",
                    help="only functions not yet in config/decompiled.txt")
    ap.add_argument("-q", "--quiet", action="store_true",
                    help="hide matching functions")
    args = ap.parse_args()

    if not args.source and not args.all:
        sys.exit(__doc__)

    entries, hardware = parse_decompiled()
    registered = {e.name: (e.addr, e.size) for e in entries + hardware}
    repo = Repo()

    sources = sorted(glob.glob("src/*.c")) if args.all else [args.source]
    tmpdir = tempfile.mkdtemp(prefix="checkfile_")
    grand = [0, 0, 0]
    suggestions = []

    for src in sources:
        base = os.path.basename(src)
        if args.built:
            hits = glob.glob(os.path.join("build/src", "**", base.replace(".c", ".o")),
                             recursive=True)
            objpath = hits[0] if hits else os.path.join(
                "build/src", base.replace(".c", ".o"))
            if not os.path.exists(objpath):
                print(f"{src}: no {objpath} (run ninja, or drop --built)")
                continue
        else:
            cc, cflags, fix, asflags = ccpipe.file_settings(base)
            objpath = os.path.join(tmpdir, base.replace(".c", ".o"))
            ok, log = ccpipe.compile_c(src, objpath, cc=cc, cflags=cflags,
                                       fixflags=fix, asflags=asflags)
            if not ok:
                print(f"{src}: COMPILE FAILED\n{log}")
                grand[2] += 1
                continue

        results = check_object(repo, objpath, base, registered, args.new)
        n_ok = sum(1 for r in results if r[1] == "MATCH")
        n_bad = len(results) - n_ok
        grand[0] += n_ok
        grand[1] += n_bad
        print(f"\n=== {src}  ({n_ok} match, {n_bad} not) ===")
        for name, status, detail, loc in results:
            if status == "MATCH":
                if not args.quiet:
                    print(f"  MATCH  {name}  {detail}")
                if name not in registered and loc:
                    suggestions.append(
                        f"{name} = 0x{loc[0]:08X}, 0x{loc[1]:X}; // {base}")
            else:
                print(f"  {status:>7}  {name}  {detail}")

    if suggestions:
        print("\nmatching but not registered -- paste into config/decompiled.txt:")
        for s in suggestions:
            print(f"  {s}")
    print(f"\n{grand[0]} match, {grand[1]} not matching"
          + (f", {grand[2]} files failed to compile" if grand[2] else ""))
    return 0 if grand[1] == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
