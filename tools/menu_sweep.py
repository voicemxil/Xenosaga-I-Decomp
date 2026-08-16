#!/usr/bin/env python3
"""Sweep a site-indexed fixer flag over N=lo..hi for one function.

    python3 tools/menu_sweep.py src/game/Menu.c FUNC --branch-likely 0 40
    python3 tools/menu_sweep.py src/game/Menu.c FUNC --rotate 0 40 '{f}:{n}:5'

The site template may use {n1}/{n2}/{n3} for n+1/n+2/n+3, which is what
--rotate-seq needs when a second, overlapping window has to fire after the
first one has shifted the indices.

The candidate site is passed to checkfile through $XENO_EXTRA_FIX_FLAGS,
which ccpipe merges into the file's configure.py flags IN MEMORY. This tool
never writes configure.py: an earlier version snapshotted it and restored
the snapshot when the sweep finished, which silently discarded flag entries
other agents had added during the (multi-minute) sweep and regressed their
matched functions. Once a site is confirmed here, add it to configure.py by
hand -- editing only your own FILE_FIX_FLAGS entry.

Prints only matches unless $SWEEP_VERBOSE is set; stops at the first match.
"""
import os
import re
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))


def main():
    src, func, flag = sys.argv[1], sys.argv[2], sys.argv[3]
    lo = int(sys.argv[4]) if len(sys.argv) > 4 else 0
    hi = int(sys.argv[5]) if len(sys.argv) > 5 else 40
    tpl = sys.argv[6] if len(sys.argv) > 6 else "{f}:{n}"
    base = os.path.basename(src)

    for n in range(lo, hi + 1):
        site = tpl.format(f=func, n=n, n1=n + 1, n2=n + 2, n3=n + 3)
        env = dict(os.environ)
        env["XENO_EXTRA_FIX_FLAGS"] = "%s=%s %s" % (base, flag, site)
        out = subprocess.run(
            ["docker", "exec", "-w", "/work",
             "-e", "XENO_EXTRA_FIX_FLAGS=" + env["XENO_EXTRA_FIX_FLAGS"],
             "xenosaga-dev", "bash", "-c",
             "export PATH=$PATH:/usr/local/ps2dev/ee/bin:"
             "/usr/local/ps2dev/ee-gcc/bin;"
             " source /opt/venv/bin/activate;"
             " python3 tools/checkfile.py %s" % src],
            capture_output=True, text=True, cwd=ROOT).stdout
        line = [l for l in out.splitlines()
                if re.search(r'\s%s\s' % re.escape(func), l)]
        txt = line[0].strip() if line else "??"
        if "MATCH" in txt or os.environ.get("SWEEP_VERBOSE"):
            print("%-3d %s" % (n, txt), flush=True)
        if "MATCH" in txt:
            print("*** MATCH at %s %s" % (flag, site), flush=True)
            return


if __name__ == "__main__":
    main()
