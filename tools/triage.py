#!/usr/bin/env python3
"""Classify near-miss diffs automatically.

    python3 tools/triage.py FUNCTION_NAME     # one function
    python3 tools/triage.py --all             # every non-matching entry
    python3 tools/triage.py --only Ssd        # a family
    python3 tools/triage.py --all -v          # also print the differing words

Classes:
  MATCH           already byte-identical after masking
  REGISTER        same instructions, different register numbers (allocator
                  tie-break -- permuter / minimal pin territory)
  SCHEDULING      identical instruction multiset in a different order
                  (statement reordering -- try tools/permute.py)
  NOP-COUNT       same instructions, different padding (often an R5900
                  erratum / assembler artifact, not source-controllable)
  NOP-PLACEMENT   same instructions, nops sit in different slots
  IMMEDIATE       same opcodes+registers, different immediate/offset
                  (wrong struct offset, or a relocation that isn't masked)
  OPERANDS        same opcodes, different operands
  LENGTH / LOGIC  a real code difference -- rewrite the C
"""
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from decomplib import Repo, parse_decompiled, matches_filter  # noqa: E402
from triagelib import triage  # noqa: E402


def main():
    argv = sys.argv[1:]
    verbose = "-v" in argv
    do_all = "--all" in argv
    only = []
    if "--only" in argv:
        only = [p for p in argv[argv.index("--only") + 1].replace(",", " ").split()]
    names = [a for a in argv if not a.startswith("-")]
    if only:
        names = [n for n in names if n != argv[argv.index("--only") + 1]]
    if not (do_all or only or names):
        sys.exit(__doc__)

    entries, hardware = parse_decompiled()
    repo = Repo()
    if names:
        sel = [e for e in entries + hardware if e.name in names]
        missing = set(names) - {e.name for e in sel}
        for m in missing:
            print(f"{m}: not in config/decompiled.txt")
    else:
        sel = [e for e in entries if matches_filter(e, only, None, repo)]

    counts = {}
    for e in sel:
        r = repo.compare(e.name, e.addr, e.size)
        if r["status"] == "SKIP":
            print(f"{e.name}: SKIP ({r['reason']})")
            continue
        label, detail = triage(r["masked_orig"], r["masked_built"], r["diffs"])
        if do_all and label == "MATCH":
            counts[label] = counts.get(label, 0) + 1
            continue
        counts[label] = counts.get(label, 0) + 1
        print(f"{e.name}: {label} -- {detail}")
        if verbose and r["diffs"]:
            for i in r["diffs"][:24]:
                o = r["orig"][i] if i < len(r["orig"]) else 0
                b = r["built"][i] if i < len(r["built"]) else 0
                print(f"    [{i:3}] orig {o:08x}  built {b:08x}")
    if len(sel) > 1:
        print("\n  " + ", ".join(f"{k}={v}" for k, v in sorted(counts.items())))


if __name__ == "__main__":
    main()
