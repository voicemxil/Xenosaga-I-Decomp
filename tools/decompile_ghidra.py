#!/usr/bin/env python3
"""First-draft C for one function, via Ghidra headless + EmotionEngineReloaded.

    python3 tools/decompile_ghidra.py Get_Rnd
    python3 tools/decompile_ghidra.py svSaveDataInit --output /tmp/draft.c

Alternative to tools/decompile.py's m2c path. m2c was built for N64
(IRIX cc / mips-gcc) and does not know the R5900's 128-bit MMI
instructions or VU0 macro-mode ops, and produces garbage or nothing for
functions that use them. Ghidra with the EmotionEngineReloaded plugin
targets the R5900 directly and generally gives a usable draft where m2c
gives up -- but it is still only a STARTING POINT: Ghidra knows nothing
of this game's struct layouts either, so expect `*(undefined4 *)(a0 +
0x24)` in place of `pWork->nIndex`, and expect to rewrite types, names
and loop shapes same as with the m2c path.

Runs the existing xenosaga-ee-vm Ghidra project
(/Users/teed/Claude/GhidraProjects/xenosaga-ee-vm.gpr, program SLUS_204.69,
already imported and analyzed) headless, via the ExportR5900Function.java
script in /Users/teed/Claude/GhidraScripts. That project may be in
concurrent use by another session -- this wrapper opens it with
-process (never -import, never -deleteProject, never -overwrite), so it
attaches to the existing analyzed program rather than re-importing or
mutating project state.
"""
import argparse
import os
import re
import subprocess
import sys

ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)),
                                     ".."))
GHIDRA_HOME = "/Users/teed/Claude/.tools/ghidra-ee/ghidra_12.1.2_PUBLIC"
ANALYZE_HEADLESS = os.path.join(GHIDRA_HOME, "support", "analyzeHeadless")
GHIDRA_PROJECT_DIR = "/Users/teed/Claude/GhidraProjects"
GHIDRA_PROJECT_NAME = "xenosaga-ee-vm"
GHIDRA_PROGRAM = "SLUS_204.69"
SCRIPT_PATH = "/Users/teed/Claude/GhidraScripts"
SCRIPT_NAME = "ExportR5900Function.java"
SYMBOL_ADDRS = os.path.join(ROOT, "config", "symbol_addrs.txt")


def find_symbol(name):
    """Return (addr, size) for NAME from config/symbol_addrs.txt.

    That file only carries an address, not a size (splat convention), so
    the size is taken from the next distinct address after this one --
    good enough for a decompile pass, which just needs to not run past
    the next function.
    """
    entries = []
    pat = re.compile(r'^\s*(\w+)\s*=\s*(0x[0-9A-Fa-f]+)\s*;')
    with open(SYMBOL_ADDRS) as fh:
        for line in fh:
            m = pat.match(line)
            if m:
                entries.append((m.group(1), int(m.group(2), 16)))
    addrs = sorted(set(a for _n, a in entries))
    for n, a in entries:
        if n == name:
            later = [x for x in addrs if x > a]
            size = (later[0] - a) if later else 0x1000
            return a, size
    return None, None


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("function")
    ap.add_argument("--addr", help="hex start address (skip symbol_addrs.txt lookup)")
    ap.add_argument("--size", help="hex byte length (skip symbol_addrs.txt lookup)")
    ap.add_argument("--output", "-o", help="write the generated C draft to this file")
    args = ap.parse_args()

    if args.addr and args.size:
        addr, size = int(args.addr, 16), int(args.size, 16)
    else:
        addr, size = find_symbol(args.function)
        if addr is None:
            sys.exit("no `%s` in config/symbol_addrs.txt -- pass --addr/--size "
                     "directly" % args.function)

    scratch = os.environ.get("TMPDIR", "/tmp")
    out_path = os.path.join(scratch, "_ghidra_%s.c" % args.function)

    cmd = [
        ANALYZE_HEADLESS,
        GHIDRA_PROJECT_DIR, GHIDRA_PROJECT_NAME,
        "-process", GHIDRA_PROGRAM,
        "-noanalysis",
        "-readOnly",
        "-scriptPath", SCRIPT_PATH,
        "-postScript", SCRIPT_NAME,
        "0x%X" % addr, "0x%X" % size, args.function, out_path,
    ]

    result = subprocess.run(cmd, capture_output=True, text=True)
    if not os.path.exists(out_path):
        sys.stderr.write(result.stdout[-4000:])
        sys.stderr.write(result.stderr[-4000:])
        sys.exit("Ghidra produced no output for %s" % args.function)

    with open(out_path) as fh:
        text = fh.read()
    if not text.strip():
        sys.exit("Ghidra wrote an empty file for %s" % args.function)

    if args.output:
        with open(args.output, "w") as fh:
            fh.write(text)
        print("wrote %s" % args.output)
    else:
        print(text)
    return 0


if __name__ == "__main__":
    sys.exit(main())
