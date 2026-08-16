#!/usr/bin/env python3
"""Register every newly-matching function of one source file, atomically.

    python3 tools/register.py src/xgl/xglSound.c
    python3 tools/register.py src/xgl/xglSound.c --dry-run

Runs checkfile.py on the file, takes the functions it reports as matching
but not yet registered, and appends them to config/decompiled.txt under
an exclusive lock. Symbols already present are skipped, so re-running is
harmless.

Why not just paste checkfile's output: config/decompiled.txt is shared by
every agent working the tree at once. Two agents doing read-append-write
by hand at the same moment lose one of the two sets of lines, and the
loss is invisible -- the functions simply stop being verified, and the
next full run reports a lower count with no failure to point at.

IMPORTANT: run `sh tools/rebuild.sh` before trusting any count. checkfile
compiles fresh, but verify.py reads build/ objects, and a stale or
partial build both invents failures and hides real ones.
"""
import argparse
import fcntl
import os
import re
import subprocess
import sys

ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)),
                                     ".."))
LEDGER = os.path.join(ROOT, "config", "decompiled.txt")
RE_ENTRY = re.compile(r'^\s*(\w+)\s*=\s*0x[0-9A-Fa-f]+\s*,')


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("source")
    ap.add_argument("--dry-run", action="store_true")
    args = ap.parse_args()

    out = subprocess.run([sys.executable, "tools/checkfile.py", args.source,
                          "--new"], cwd=ROOT, capture_output=True, text=True)
    # checkfile exits nonzero whenever anything in the file is still
    # unmatched, which is the normal case -- only a compile failure is an
    # error here.
    if "failed to compile" in out.stdout or not out.stdout.strip():
        sys.stderr.write(out.stdout + out.stderr)
        sys.exit("checkfile.py could not build %s" % args.source)
    found = [l.strip() for l in out.stdout.split("\n") if RE_ENTRY.match(l)]
    if not found:
        print("nothing new to register in %s" % args.source)
        return 0

    with open(LEDGER, "r+") as fh:
        fcntl.flock(fh, fcntl.LOCK_EX)
        text = fh.read()
        have = set()
        for line in text.split("\n"):
            m = RE_ENTRY.match(line)
            if m:
                have.add(m.group(1))
        add = [l for l in found if RE_ENTRY.match(l).group(1) not in have]
        if not add:
            print("all %d already registered" % len(found))
            return 0
        for l in add:
            print("  + " + l)
        if args.dry_run:
            print("(dry run -- nothing written)")
            return 0
        fh.seek(0)
        fh.write(text.rstrip("\n") + "\n" + "\n".join(add) + "\n")
        fh.truncate()
    print("registered %d function(s) from %s" % (len(add), args.source))
    return 0


if __name__ == "__main__":
    sys.exit(main())
