#!/usr/bin/env python3
"""Generate config/m2c_context.c -- the type context m2c reads.

    python3 tools/gen_m2c_context.py

Without a context, m2c has to guess: every call becomes `s32 f()` and
every struct access becomes `*(s32 *)(arg0 + 0x24)`. With one, it knows
the real return types, argument counts and struct layouts, and its draft
comes out much closer to something you can edit.

We already know 2,400+ function signatures -- they are sitting in the C
we have written. This scans src/ for function DEFINITIONS and emits them
as prototypes, plus the struct/typedef declarations the same files
carry. m2c parses the result with pycparser, so the output has to be
plain C99: anything with a steering macro, inline asm, or a GCC
extension in it is skipped rather than risked, because one unparseable
line poisons the context for every function.

Re-run it whenever a lot of new functions land; it is cheap.
"""
import os
import re
import sys

ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)),
                                     ".."))
OUT = os.path.join(ROOT, "config", "m2c_context.c")

# A function definition: a line starting at column 0 that looks like
# `type name(args)` and whose next non-blank line is `{`.
RE_DEF = re.compile(
    r'^([A-Za-z_][A-Za-z_0-9 \t\*]*?)\s*'      # return type
    r'\b([A-Za-z_][A-Za-z_0-9]*)\s*'           # name
    r'\(([^;{]*)\)\s*$')                       # args

# Things pycparser will not accept, or that we simply do not need.
BANNED = re.compile(r'\b(?:__asm__|asm|PIN|LAUNDER|LAUNDER_V|LAUNDER2|'
                    r'PASSTHRU|PASSTHRU_V|SCHED_NOP|PS2_ASM|__attribute__|'
                    r'__builtin_\w+)\b')

SKIP_TYPES = ("static", "typedef", "return", "if", "for", "while", "switch",
              "else", "do", "case", "sizeof", "goto")

# pycparser takes PREPROCESSED C only: no comments, and every type named
# must be declared. So the generated file carries no comments at all --
# its documentation lives in this script.
PREAMBLE = """typedef signed char s8;
typedef unsigned char u8;
typedef signed short s16;
typedef unsigned short u16;
typedef signed int s32;
typedef unsigned int u32;
typedef signed long long s64;
typedef unsigned long long u64;
typedef float f32;
typedef double f64;
typedef unsigned int size_t;
"""

BASE_TYPES = {
    "void", "char", "short", "int", "long", "float", "double", "signed",
    "unsigned", "const", "volatile", "struct", "union", "enum", "register",
    "s8", "u8", "s16", "u16", "s32", "u32", "s64", "u64", "f32", "f64",
    "size_t", "_Bool",
}


def unknown_types(protos):
    """Type names used by the prototypes that nothing declares."""
    seen = set()
    for p in protos.values():
        for tok in re.findall(r'[A-Za-z_][A-Za-z_0-9]*', p):
            seen.add(tok)
    # a prototype's own name is not a type; neither are parameter names,
    # but over-declaring an opaque struct for a stray identifier is
    # harmless -- it only has to make the file parse.
    return sorted(t for t in seen - BASE_TYPES - set(protos)
                  if not t.startswith("__"))


def collect():
    protos = {}
    for root, _dirs, files in os.walk(os.path.join(ROOT, "src")):
        for f in sorted(files):
            if not f.endswith(".c"):
                continue
            path = os.path.join(root, f)
            with open(path, errors="ignore") as fh:
                lines = fh.readlines()
            for i, line in enumerate(lines):
                if line.startswith((" ", "\t", "#", "/", "*", "}")):
                    continue
                m = RE_DEF.match(line.rstrip())
                if not m:
                    continue
                # the definition's body must open on the next real line
                nxt = ""
                for j in range(i + 1, min(i + 3, len(lines))):
                    if lines[j].strip():
                        nxt = lines[j].strip()
                        break
                if not nxt.startswith("{"):
                    continue
                rtype, name, argstr = (m.group(1).strip(), m.group(2),
                                       m.group(3).strip())
                if rtype.split()[0] in SKIP_TYPES if rtype.split() else True:
                    continue
                whole = "%s %s(%s)" % (rtype, name, argstr)
                if BANNED.search(whole):
                    continue
                # K&R definitions (empty parens with args below) are not
                # safe to emit as a prototype -- `()` means "unknown" and
                # would make m2c drop argument types it could infer.
                if argstr == "":
                    continue
                protos.setdefault(name, "%s %s(%s);" % (rtype, name, argstr))
    return protos


def main():
    protos = collect()
    stubs = "".join("typedef struct %s_s %s;\n" % (t, t)
                    for t in unknown_types(protos))

    sys.path.insert(0, os.path.join(ROOT, "tools", "m2c"))
    try:
        from pycparser import c_parser
    except ImportError:
        c_parser = None

    names = sorted(protos)
    dropped = []
    text = ""
    for _attempt in range(400):
        text = PREAMBLE + stubs + "\n" + "\n".join(protos[k] for k in names) + "\n"
        if c_parser is None:
            break
        try:
            c_parser.CParser().parse(text, OUT)
        except Exception as exc:
            # pycparser points at the offending line; drop that one
            # prototype and retry. One unparseable declaration would
            # otherwise poison the context for EVERY function.
            m = re.search(r':(\d+):', str(exc))
            head = len(PREAMBLE.split("\n")) - 1 + len(stubs.split("\n")) - 1 + 1
            if not m:
                break
            idx = int(m.group(1)) - head - 1
            if not (0 <= idx < len(names)):
                break
            dropped.append(names.pop(idx))
            continue
        break

    with open(OUT, "w") as fh:
        fh.write(text)
    print("wrote %s: %d prototypes, %d opaque type stubs"
          % (os.path.relpath(OUT, ROOT), len(names), stubs.count("typedef")))
    if dropped:
        print("  dropped %d unparseable: %s%s"
              % (len(dropped), ", ".join(dropped[:6]),
                 " ..." if len(dropped) > 6 else ""))
    if c_parser is None:
        print("  (pycparser not importable here -- run under "
              "/opt/m2c-venv/bin/python to parse-check)")
        return 0
    try:
        c_parser.CParser().parse(text, OUT)
    except Exception as exc:
        print("  WARNING: context still does NOT parse: %s" % str(exc)[:160])
        return 1
    print("  parse check: OK")
    return 0


if __name__ == "__main__":
    sys.exit(main())
