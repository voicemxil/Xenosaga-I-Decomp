#!/usr/bin/env python3
"""Compile a C file exactly the way the ninja `cc` rule does.

cc -S  ->  tools/fix_cc_asm.py  ->  as.  Every tool that compiles game
code (trycc, the permuter, checkfile) must go through the same path or it
will disagree with the real build over delay slots and macro expansion.

Per-file compiler/flags are read straight out of configure.py so there is
one source of truth.
"""
import os
import subprocess
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
TOOLS = os.path.join(ROOT, "tools")

# Set XENO_FIX_CC_ASM to A/B a modified post-processor without touching the
# shared tools/fix_cc_asm.py that other agents are building against.
FIX_CC_ASM = os.environ.get("XENO_FIX_CC_ASM",
                            os.path.join(TOOLS, "fix_cc_asm.py"))

CC96 = "/opt/ee-gcc2.96/bin/ee-gcc"          # game code
CC29 = "/usr/local/ps2dev/ee-gcc/bin/ee-gcc"  # SDK (sce*) code
AS = "/usr/local/ps2dev/ee/bin/mips64r5900el-ps2-elf-as"
ASFLAGS = ["-march=r5900", "-mabi=eabi", f"-I{ROOT}/include", "--no-warn"]

CFLAGS96 = "-O2 -G8"
CFLAGS29 = "-O2 -G0 -fno-schedule-insns"


def file_settings(basename):
    """(cc, cflags, fixflags, asflags) for a source file, per configure.py."""
    # Game code is the default; only Sony SDK translation units use 2.9.
    # This used to fall back to configure.CC (the SDK compiler) for any file
    # not yet listed, so checking a NEW file -- the tool's whole purpose --
    # compiled it with the wrong compiler and reported a byte-perfect
    # function as "a real code difference".
    cc, cflags = (CC29, CFLAGS29) if basename.startswith("sce") else (CC96, CFLAGS96)
    fix = []
    asflags = ""
    try:
        sys.path.insert(0, ROOT)
        import configure  # noqa: E402
        cc = configure.cc_for(basename)
        cflags = configure.cflags_for(basename)
        fix = configure.FILE_FIX_FLAGS.get(basename, "").split()
        asflags = configure.asflags_for(basename)
    except Exception:
        pass
    return cc, cflags, fix, asflags


def compile_c(src, obj, cc=CC96, cflags=CFLAGS96, fixflags=(), keep_asm=None,
              extra=(), asflags=""):
    """Compile `src` to `obj`. Returns (ok, log). `keep_asm` keeps the .s."""
    asm = keep_asm or obj + ".s"
    r = subprocess.run([cc] + cflags.split() + ["-Iinclude"] + list(extra) + ["-S", "-o", asm, src],
                       capture_output=True, text=True)
    if r.returncode != 0:
        return False, (r.stderr or r.stdout)
    log = r.stderr
    r = subprocess.run([sys.executable, FIX_CC_ASM, asm] + list(fixflags),
                       capture_output=True, text=True)
    if r.returncode != 0:
        return False, log + (r.stderr or r.stdout)
    # The assembler has its own small-data (-G) threshold, independent of
    # what was passed to the compiler -- gas defaults to -G8 regardless of
    # gcc's -G value, which silently keeps li.s/li.d float/double literals
    # pooled in %gp_rel .lit4/.lit8 even when gcc was told -G0. Any
    # per-file override must reach the assembler too. See configure.py's
    # asflags_for / FILE_ASFLAGS_OVERRIDE (found on libm.c).
    r = subprocess.run([AS] + ASFLAGS + (asflags.split() if asflags else []) +
                       ["-o", obj, asm],
                       capture_output=True, text=True)
    if r.returncode != 0:
        return False, log + (r.stderr or r.stdout)
    if keep_asm is None and os.path.exists(asm):
        os.unlink(asm)
    return True, log
