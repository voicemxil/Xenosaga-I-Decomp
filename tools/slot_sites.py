#!/usr/bin/env python3
"""List the delay-slot sites of a function, with the index each site-keyed
fixer flag uses.

    python3 tools/slot_sites.py src/libc/newlib_vfprintf.c _vfprintf_r
    python3 tools/slot_sites.py src/libc/libm.c floorf --kind cond

Site-keyed passes in fix_cc_asm.py all say "FUNC:N, 0-based, asm order",
but they count DIFFERENT things, and guessing N by eye off the .s is
wrong often enough to waste a sweep -- macros (li/la/ld of a symbol)
expand to several instructions, so an index counted from .s lines does
not agree with one read off a disassembly. This prints all three
countings from the real compile, side by side with the word offset each
site lands at in the emitted binary, so a flag can be aimed at a site
seen in a scratch_diff.py listing.

  gccfill  --unfill-gcc-slots FUNC:N   slots gcc already filled
                                       (.set noreorder/nomacro blocks)
  gasfill  --pin-slot-nop FUNC:N       reorder-mode branches, the ones
                                       gas fills; disjoint from gccfill
  cond     --branch-likely FUNC:N      conditional branches, annulled or
           --branch-unlikely FUNC:N    not; counts both modes

Word offsets come from objdump of the assembled object, so they are
directly comparable with tools/scratch_diff.py output.
"""
import argparse
import os
import re
import subprocess
import sys
import tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(ROOT, "tools"))
sys.path.insert(0, ROOT)
import ccpipe  # noqa: E402

OBJDUMP = "/usr/local/ps2dev/ee/bin/mips64r5900el-ps2-elf-objdump"

RE_ANY_BRANCH = re.compile(r"^\t(b|beq|bne|beqz|bnez|blez|bgtz|bltz|bgez"
                           r"|beql|bnel|beqzl|bnezl|blezl|bgtzl|bltzl|bgezl"
                           r"|bc1t|bc1f|bc1tl|bc1fl|j|jal|jr|jalr)[ \t]")
RE_COND = re.compile(r"^\t(beql?|bnel?|beqzl?|bnezl?|blezl?|bgtzl?|bltzl?"
                     r"|bgezl?|bc1tl?|bc1fl?)([ \t])")


def emitted_words(src, fn, extra):
    """[(word index, text)] for one function of the assembled object."""
    base = os.path.basename(src)
    cc, cflags, fix, asflags = ccpipe.file_settings(base)
    d = tempfile.mkdtemp(prefix="slotsites_")
    asm = os.path.join(d, base.replace(".c", ".s"))
    obj = os.path.join(d, base.replace(".c", ".o"))
    ok, log = ccpipe.compile_c(src, obj, cc=cc, cflags=cflags,
                               fixflags=list(fix) + extra, keep_asm=asm,
                               asflags=asflags)
    if not ok:
        sys.exit("compile failed:\n" + log)
    txt = subprocess.run([OBJDUMP, "-d", "--no-show-raw-insn", obj],
                         capture_output=True, text=True).stdout
    words, on, first = [], False, None
    for line in txt.split("\n"):
        if line.endswith(">:"):
            on = line.split("<")[1][:-2] == fn
            continue
        if not on or ":\t" not in line:
            continue
        addr = int(line.split(":")[0].strip(), 16)
        if first is None:
            first = addr
        words.append(((addr - first) // 4, line.split("\t", 1)[1].strip()))
    return asm, words


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("source")
    ap.add_argument("func")
    ap.add_argument("--kind", choices=["all", "gccfill", "gasfill", "cond"],
                    default="all")
    ap.add_argument("--extra", default="", help="extra fixer flags")
    args = ap.parse_args()

    asm, words = emitted_words(args.source, args.func, args.extra.split())
    lines = open(asm).read().split("\n")

    # Walk the .s for the function and classify each branch.
    sites, cur, noreorder = [], None, False
    gcc_n = gas_n = cond_n = 0
    for i, line in enumerate(lines):
        if line.startswith("\t.ent\t"):
            cur = line.split("\t")[-1]
            if cur == args.func:
                gcc_n = gas_n = cond_n = 0
            noreorder = False
        s = line.strip().replace("\t", " ")
        if s == ".set noreorder":
            noreorder = True
        elif s in (".set reorder", ".set pop"):
            noreorder = False
        if cur != args.func or not RE_ANY_BRANCH.match(line):
            continue
        kinds = []
        if noreorder:
            slot = lines[i + 1] if i + 1 < len(lines) else ""
            filled = slot.startswith("\t") and not slot.strip().startswith(".")
            if filled:
                kinds.append(("gccfill", gcc_n))
                gcc_n += 1
        else:
            kinds.append(("gasfill", gas_n))
            gas_n += 1
        if RE_COND.match(line):
            kinds.append(("cond", cond_n))
            cond_n += 1
        if kinds:
            sites.append((line.strip().replace("\t", " "), kinds))

    # Pair asm branches with emitted words in order.
    br_words = [(w, t) for w, t in words
                if re.match(r"^(b|j)\w*\s", t) or t.split()[0] in ("b", "j")]
    if len(br_words) != len(sites):
        print(f"note: {len(sites)} branches in the .s vs {len(br_words)} in "
              f"the object -- word offsets below may be misaligned "
              f"(a macro probably expanded to a branch)\n")

    print(f"{args.func}: {len(sites)} branch site(s)\n")
    print(f"{'word':>6}  {'gccfill':>7} {'gasfill':>7} {'cond':>5}   insn")
    for k, (text, kinds) in enumerate(sites):
        d = dict(kinds)
        if args.kind != "all" and args.kind not in d:
            continue
        w = br_words[k][0] if k < len(br_words) else -1
        def col(name):
            return str(d[name]) if name in d else "-"
        print(f"{w:>6}  {col('gccfill'):>7} {col('gasfill'):>7} "
              f"{col('cond'):>5}   {text}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
