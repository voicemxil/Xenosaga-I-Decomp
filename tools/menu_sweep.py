#!/usr/bin/env python3
"""Sweep a site-indexed fixer flag over N=0..MAX for one function.

    python3 tools/menu_sweep.py src/game/Menu.c FUNC --branch-likely 0 40

Temporarily appends "FLAG FUNC:N" to the file's configure.py FILE_FIX_FLAGS
entry, runs checkfile, and reports the diff count for FUNC at each N.
Restores configure.py on exit.
"""
import os
import re
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))


def entry_span(text, base):
    m = re.search(r'^    "%s": ' % re.escape(base), text, re.M)
    if not m:
        return None
    i = m.end()
    # value is either a plain string or a parenthesised concatenation
    j = i
    depth = 0
    while j < len(text):
        if text[j] == '(':
            depth += 1
        elif text[j] == ')':
            depth -= 1
            if depth == 0:
                j += 1
                break
        elif text[j] == ',' and depth == 0:
            break
        elif text[j] == '"' and depth == 0:
            j += 1
            while text[j] != '"':
                j += 1
            j += 1
            break
        j += 1
    return i, j


def main():
    src, func, flag = sys.argv[1], sys.argv[2], sys.argv[3]
    lo = int(sys.argv[4]) if len(sys.argv) > 4 else 0
    hi = int(sys.argv[5]) if len(sys.argv) > 5 else 40
    extra_tpl = sys.argv[6] if len(sys.argv) > 6 else "{f}:{n}"
    base = os.path.basename(src)
    cfg = os.path.join(ROOT, "configure.py")
    orig = open(cfg).read()
    span = entry_span(orig, base)
    try:
        for n in range(lo, hi + 1):
            extra = extra_tpl.format(f=func, n=n)
            add = '%s %s' % (flag, extra)
            if span:
                i, j = span
                val = orig[i:j]
                # fix_cc_asm.py uses store (not append) semantics and now
                # ERRORS on a duplicate flag, so merge into the existing
                # comma-separated site list rather than adding a second flag.
                lit = eval(val)
                if flag + " " in lit + " ":
                    k = lit.index(flag + " ") + len(flag) + 1
                    end = lit.find(" ", k)
                    end = len(lit) if end < 0 else end
                    lit = lit[:end] + "," + extra + lit[end:]
                else:
                    lit = lit + " " + add
                new = orig[:i] + repr(lit) + orig[j:]
            else:
                m = re.search(r'^FILE_FIX_FLAGS = \{\n', orig, re.M)
                new = orig[:m.end()] + '    "%s": "%s",\n' % (base, add) + orig[m.end():]
            open(cfg, "w").write(new)
            out = subprocess.run(
                ["docker", "exec", "-w", "/work", "xenosaga-dev", "bash", "-c",
                 "export PATH=$PATH:/usr/local/ps2dev/ee/bin:/usr/local/ps2dev/ee-gcc/bin;"
                 " source /opt/venv/bin/activate;"
                 " python3 tools/checkfile.py %s" % src],
                capture_output=True, text=True, cwd=ROOT).stdout
            line = [l for l in out.splitlines()
                    if re.search(r'\s%s\s' % re.escape(func), l)]
            txt = line[0].strip() if line else "??"
            if "MATCH" in txt or os.environ.get("SWEEP_VERBOSE"):
                print("%-3d %s" % (n, txt), flush=True)
            if "MATCH" in txt:
                print("*** MATCH at %s" % extra, flush=True)
                return
    finally:
        open(cfg, "w").write(orig)


if __name__ == "__main__":
    main()
