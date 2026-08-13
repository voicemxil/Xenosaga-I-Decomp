#!/usr/bin/env python3
"""Verify decompiled C functions match the original ELF.

    python3 tools/verify.py                 # everything (default)
    python3 tools/verify.py --only Ssd      # just one family (substring/prefix)
    python3 tools/verify.py --file ssd.c    # just the functions from one TU
    python3 tools/verify.py -q              # failures + summary only

Reads the ELF and the built objects directly rather than spawning
objdump twice per function, so a full run is ~1s instead of ~75s.
Output format (OK/FAIL/SKIP/HW lines + summary) is unchanged.
"""
import argparse
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from decomplib import Repo, parse_decompiled, matches_filter  # noqa: E402


def split_list(v):
    return [x for x in (v or "").replace(",", " ").split() if x]


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--only", action="append", default=[],
                    help="only functions whose name contains/starts with this "
                         "(comma-separated or repeated)")
    ap.add_argument("--file", action="append", default=[],
                    help="only functions from this source file, e.g. ssd.c")
    ap.add_argument("-q", "--quiet", action="store_true",
                    help="print failures and the summary only")
    ap.add_argument("--fail-names", action="store_true",
                    help="print just the failing names, one per line")
    args = ap.parse_args()

    only = [p for v in args.only for p in split_list(v)]
    files = [p for v in args.file for p in split_list(v)]

    entries, hardware = parse_decompiled()
    repo = Repo()
    if only or files:
        entries = [e for e in entries if matches_filter(e, only, files, repo)]
        hardware = [e for e in hardware if matches_filter(e, only, files, repo)]

    passed = failed = 0
    fail_names = []
    for e in entries:
        r = repo.compare(e.name, e.addr, e.size)
        if r["status"] == "SKIP":
            if not args.fail_names:
                print(f"  SKIP {e.name} - {r['reason']}")
            continue
        if r["status"] == "OK":
            passed += 1
            if not args.quiet and not args.fail_names:
                print(f"  OK   {e.name}")
        else:
            failed += 1
            fail_names.append(e.name)
            if args.fail_names:
                print(e.name)
                continue
            print(f"  FAIL {e.name}")
            print(f"    orig:  {' '.join(f'{w:08x}' for w in r['orig'][:8])}")
            print(f"    built: {' '.join(f'{w:08x}' for w in r['built'][:8])}")

    if not args.fail_names:
        for e in hardware:
            if not args.quiet:
                print(f"  HW   {e.name}")
        print(f"\n{passed} passed, {failed} failed, {len(hardware)} hardware, "
              f"{len(entries) + len(hardware)} total")
    return 0


if __name__ == "__main__":
    sys.exit(main())
