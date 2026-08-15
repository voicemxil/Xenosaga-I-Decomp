#!/usr/bin/env python3
"""Add or remove one fixer site in ONE configure.py FILE_FIX_FLAGS entry.

    python3 tools/menu_flag.py Menu.c add    --rotate MenuCfTaikiPop:10:5
    python3 tools/menu_flag.py Menu.c remove --rotate MenuCfTaikiPop:10:2

Deliberately surgical: it re-reads configure.py from disk, rewrites only the
one physical line whose key matches, and replaces the file atomically. It
never reformats the dict and never carries a stale in-memory copy across a
long-running command, so a concurrent edit to another entry cannot be lost.
"""
import os
import re
import sys


def main():
    key, action, flag, site = sys.argv[1:5]
    path = os.path.join(os.path.dirname(os.path.dirname(
        os.path.abspath(__file__))), "configure.py")
    lines = open(path).read().split("\n")
    hits = [i for i, l in enumerate(lines)
            if l.lstrip().startswith('"%s":' % key)]
    if len(hits) != 1:
        sys.exit("expected exactly one %r entry line, found %d" % (key, len(hits)))
    i = hits[0]
    line = lines[i]
    m = re.search(re.escape(flag) + r"\s+([^\s'\"]+)", line)
    if action == "add":
        if not m:
            sys.exit("%s not present in the %s entry; add it by hand first" % (flag, key))
        sites = m.group(1).split(",")
        if site in sites:
            sys.exit("site already present")
        line = line[:m.start(1)] + ",".join(sites + [site]) + line[m.end(1):]
    elif action == "remove":
        if not m:
            sys.exit("%s not present" % flag)
        sites = [s for s in m.group(1).split(",") if s != site]
        if not sites:
            sys.exit("refusing to leave %s with no sites" % flag)
        line = line[:m.start(1)] + ",".join(sites) + line[m.end(1):]
    else:
        sys.exit("action must be add or remove")
    lines[i] = line
    tmp = path + ".tmp%d" % os.getpid()
    with open(tmp, "w") as f:
        f.write("\n".join(lines))
    os.replace(tmp, path)
    print(line.strip()[:200])


if __name__ == "__main__":
    main()
