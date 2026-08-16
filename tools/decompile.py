#!/usr/bin/env python3
"""First-draft C for one function, straight from the original assembly.

    python3 tools/decompile.py Get_Rnd
    python3 tools/decompile.py svSaveDataInit --no-context
    python3 tools/decompile.py xglFontLoad --passes 2

Runs m2c (github.com/ethteck/m2c) over the splat-generated assembly. The
output is a STARTING POINT, never a finished match: m2c reconstructs
control flow and expressions faithfully but knows nothing about this
game's struct layouts, so it emits `*(s32 *)(arg0 + 0x24)` where the
real source said `pWork->nIndex`. Expect to rewrite types, names and
loop shapes. What it saves is the tedious part -- working out the
control-flow graph and which temporaries feed which expression.

Two things this wrapper handles that raw m2c does not:

  1. splat's asm carries `.include "macro.inc"` and `nonmatching NAME,
     SIZE` macro lines that m2c rejects outright ("unsupported non-nop
     instruction outside of function"). We slice out just the requested
     function, between its `glabel` and `endlabel`.
  2. The EE is little-endian MIPS built by gcc, so the target is
     mipsel-gcc-c, not m2c's ido default.

CAVEAT for this target: m2c was built for N64 (IRIX cc / mips-gcc) and
does not know the R5900's 128-bit MMI instructions or VU0 macro-mode
ops. A function using those will decompile partially or not at all --
that is expected, and those are the functions that need PS2_ASM anyway
(see include/matching.h). Ordinary integer and float code, which is most
of the game, comes through fine.
"""
import argparse
import os
import re
import subprocess
import sys

ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)),
                                     ".."))
M2C = os.path.join(ROOT, "tools", "m2c", "m2c.py")
# m2c needs pycparser 2.x; the build venv has 3.x, which dropped
# pycparser.plyparser. Keep them apart rather than downgrading the venv
# the whole build depends on.
M2C_PY = "/opt/m2c-venv/bin/python"
CONTEXT = os.path.join(ROOT, "config", "m2c_context.c")
ASM_DIRS = ["asm/cod", "asm/ov02"]


def find_function(name):
    """Return (path, lines) for the block between glabel/endlabel."""
    want = re.compile(r'^\s*glabel\s+%s\s*$' % re.escape(name))
    end = re.compile(r'^\s*endlabel\s+%s\s*$' % re.escape(name))
    for d in ASM_DIRS:
        full = os.path.join(ROOT, d)
        if not os.path.isdir(full):
            continue
        for fn in sorted(os.listdir(full)):
            if not fn.endswith(".s"):
                continue
            path = os.path.join(full, fn)
            with open(path, errors="ignore") as fh:
                lines = fh.readlines()
            for i, line in enumerate(lines):
                if want.match(line):
                    for j in range(i + 1, len(lines)):
                        if end.match(lines[j]):
                            return path, lines[i:j + 1]
                    # No endlabel: run to the next glabel instead.
                    for j in range(i + 1, len(lines)):
                        if re.match(r'^\s*glabel\s', lines[j]):
                            return path, lines[i:j]
                    return path, lines[i:]
    return None, None


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("function")
    ap.add_argument("--no-context", action="store_true",
                    help="skip config/m2c_context.c (use if it fails to parse)")
    ap.add_argument("--passes", "-P", default=None,
                    help="m2c translation passes (2 often untangles loops)")
    ap.add_argument("--keep-asm", action="store_true",
                    help="print the sliced assembly too")
    args = ap.parse_args()

    path, block = find_function(args.function)
    if not block:
        sys.exit("no `glabel %s` in %s -- check the name with "
                 "tools/disasm.py" % (args.function, "/".join(ASM_DIRS)))

    # `endlabel` is a splat macro; m2c does not know it.
    body = [l for l in block if not re.match(r'^\s*endlabel\s', l)]
    scratch = os.environ.get("TMPDIR", "/tmp")
    asm_path = os.path.join(scratch, "_m2c_%s.s" % args.function)
    with open(asm_path, "w") as fh:
        fh.write(".set noat\n.set noreorder\n\n")
        fh.write(".section .text, \"ax\"\n\n")
        fh.writelines(body)
        fh.write("\n")

    if args.keep_asm:
        print("/* sliced from %s */" % os.path.relpath(path, ROOT))

    cmd = [M2C_PY, M2C, "--target", "mipsel-gcc-c",
           "--function", args.function]
    if args.passes:
        cmd += ["--passes", str(args.passes)]
    if not args.no_context and os.path.exists(CONTEXT):
        cmd += ["--context", CONTEXT]
    cmd.append(asm_path)

    out = subprocess.run(cmd, capture_output=True, text=True, cwd=ROOT)
    text = out.stdout
    if not text.strip():
        sys.stderr.write(out.stderr)
        sys.exit("m2c produced nothing for %s" % args.function)
    if "Decompilation failure" in text and not args.no_context:
        # A bad context file poisons every function; retry without it so
        # the caller still gets a draft, and say why.
        retry = [c for c in cmd if c != "--context" and c != CONTEXT]
        out2 = subprocess.run(retry, capture_output=True, text=True, cwd=ROOT)
        if "Decompilation failure" not in out2.stdout:
            print("/* NOTE: config/m2c_context.c did not apply here; this "
                  "draft is untyped. */")
            text = out2.stdout
    print(text)
    return 0


if __name__ == "__main__":
    sys.exit(main())
