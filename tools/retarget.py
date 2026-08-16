#!/usr/bin/env python3
"""Repoint the `// source` comment of existing config/decompiled.txt entries.

    python3 tools/retarget.py fpbit_df dpadd dpsub dpmul
    python3 tools/retarget.py fpbit_df --dry-run dpadd

register.py appends new matches; set_flags.py edits configure.py. Neither
can say "this function now lives in a different translation unit", which
is what happens whenever a hand-written TU is replaced by a vendored one.
Doing that by hand is exactly the read-modify-write that has silently
destroyed other agents' entries, so it gets the same treatment as the
other two editors:

  1. config/decompiled.txt is flock'd for the whole read-modify-write.
  2. Only the trailing `// source` comment of the NAMED symbols changes;
     the edit is a splice of that comment's span, so every other line is
     byte-identical.
  3. Afterwards the new text is re-parsed and every entry's
     (name, addr, size, hardware) tuple is compared against its old
     value. If anything else moved -- or if a name was not found, or was
     found more than once -- the write is rolled back and the tool exits
     nonzero.

Pass the source WITHOUT the .c suffix. decomplib resolves the hint by
globbing `build/**/<source>.o`, so a `// foo.c` comment looks for
`foo.c.o`, never matches, and silently falls back to the global
first-object-wins symbol map -- which is the wrong object as soon as two
TUs define the same symbol.
"""
import argparse
import fcntl
import os
import re
import sys

ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)),
                                     ".."))
LEDGER = os.path.join(ROOT, "config", "decompiled.txt")
RE_ENTRY = re.compile(r'^(\s*(\w+)\s*=\s*(0x[0-9A-Fa-f]+)\s*,\s*(0x[0-9A-Fa-f]+)\s*;)'
                      r'(.*)$')


def index(text):
    """[(name, addr, size, hardware)] over the uncommented entries, in order.

    A list rather than a dict on purpose: the ledger currently has a
    duplicate name in it, and an order-sensitive list is the stronger
    invariant anyway -- it catches a reordering as well as a value change.
    """
    out = []
    for line in text.split("\n"):
        m = RE_ENTRY.match(line)
        if not m or line.lstrip().startswith("//"):
            continue
        out.append((m.group(2), int(m.group(3), 16), int(m.group(4), 16),
                    "HARDWARE" in m.group(5)))
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("source", help="new source basename, no .c suffix; "
                                   "'-' with --drop-todo")
    ap.add_argument("names", nargs="+")
    ap.add_argument("--drop-todo", action="store_true",
                    help="instead of retargeting, delete the COMMENTED-OUT "
                         "`// name = ...` TODO line for each name (what a "
                         "wall leaves behind once it falls)")
    ap.add_argument("--add", metavar="ADDR,SIZE",
                    help="append a NEW entry at an explicit address instead "
                         "of retargeting. For a function whose name cannot be "
                         "looked up in the original ELF symbol table -- either "
                         "it has no symbol there, or the name collides with an "
                         "unrelated function. register.py cannot do this: it "
                         "only takes addresses from that same by-name lookup. "
                         "Exactly one name may be given.")
    ap.add_argument("--dup-ok", action="store_true",
                    help="with --add, allow a name that is already "
                         "registered. Only safe when BOTH entries carry a "
                         "source hint that resolves (a hint without the .c "
                         "suffix), because that is the only thing that tells "
                         "decomplib which object each address belongs to.")
    ap.add_argument("--dry-run", action="store_true")
    args = ap.parse_args()
    src = args.source[:-2] if args.source.endswith(".c") else args.source
    want = set(args.names)

    with open(LEDGER, "r+") as fh:
        fcntl.flock(fh, fcntl.LOCK_EX)
        text = fh.read()
        before = index(text)
        have = set(e[0] for e in before)

        if args.add:
            if len(args.names) != 1:
                raise SystemExit("retarget: --add takes exactly one name")
            name = args.names[0]
            if name in have and not args.dup_ok:
                raise SystemExit(
                    "retarget: %s is already registered. If this is a genuine "
                    "name collision between two different functions in the "
                    "image, re-run with --dup-ok, and make sure the EXISTING "
                    "entry's source hint has no .c suffix or it will not "
                    "resolve." % name)
            a, _, sz = args.add.partition(",")
            line = "%s = %s, %s; // %s" % (name, a.strip(), sz.strip(), src)
            new = text.rstrip("\n") + "\n" + line + "\n"
            if index(new)[:len(before)] != before:
                raise SystemExit("retarget: an unrelated entry moved -- aborted")
            print("  + " + line)
            if args.dry_run:
                print("(dry run -- nothing written)")
                return 0
            fh.seek(0)
            fh.write(new)
            fh.truncate()
            print("added 1 entry")
            return 0

        missing = sorted(n for n in want if n not in have)
        if args.drop_todo:
            missing = []
        if missing:
            raise SystemExit("retarget: not registered: " + " ".join(missing))

        if args.drop_todo:
            out, hit = [], set()
            for line in text.split("\n"):
                m = RE_ENTRY.match(line.lstrip()[2:]) if \
                    line.lstrip().startswith("//") else None
                if m and m.group(2) in want:
                    hit.add(m.group(2))
                    continue
                out.append(line)
            new = "\n".join(out)
            if index(new) != before:
                raise SystemExit("retarget: an unrelated entry moved -- aborted")
            for n in sorted(hit):
                print("  -  dropped stale TODO for %s" % n)
            if args.dry_run:
                print("(dry run -- nothing written)")
                return 0
            fh.seek(0)
            fh.write(new)
            fh.truncate()
            print("dropped %d stale TODO line(s)" % len(hit))
            return 0

        out, hit = [], set()
        for line in text.split("\n"):
            m = RE_ENTRY.match(line)
            if m and not line.lstrip().startswith("//") and m.group(2) in want:
                name, rest = m.group(2), m.group(5)
                # Keep any trailing annotation (HARDWARE, notes) after the
                # source word; only the source word itself is replaced.
                c = rest.find("//")
                tail = ""
                if c >= 0:
                    words = rest[c + 2:].strip().split(None, 1)
                    tail = (" " + words[1]) if len(words) > 1 else ""
                line = "%s // %s%s" % (m.group(1), src, tail)
                hit.add(name)
            out.append(line)
        new = "\n".join(out)

        if index(new) != before:
            raise SystemExit("retarget: an unrelated entry moved -- aborted")
        for n in sorted(hit):
            print("  -> %s // %s" % (n, src))
        if args.dry_run:
            print("(dry run -- nothing written)")
            return 0
        fh.seek(0)
        fh.write(new)
        fh.truncate()
    print("retargeted %d entr%s to %s" % (len(hit), "y" if len(hit) == 1 else "ies", src))
    return 0


if __name__ == "__main__":
    sys.exit(main())
