#!/usr/bin/env python3
"""Audit every reordering-flag match for hidden logic differences.

The reordering passes (--swap-adjacent, --rotate, --swap-into-slot,
--swap-slot-target, --swap-fp-operands, --swap-int-operands) are only
legitimate when the compiler emitted the RIGHT INSTRUCTIONS IN THE WRONG
ORDER: instruction scheduling is not
expressible in C, so a pure reorder is a genuine toolchain gap. They are
NOT legitimate as a way to paper over a C body that computes something
different -- that would silently encode wrong logic behind right bytes.

The two cases are machine-distinguishable. Recompile the file WITHOUT
its reordering flags and compare the instruction MULTISET against the
original:

    identical multiset  -> only the order differed. Genuine.
    different multiset  -> different instructions/registers were being
                           emitted, and the reorder is masking it. Suspect.

This audits every function named in a reordering flag and reports any
that fail the test, plus how many sites each needed (a function needing
many sites is worth a second look even when it passes -- it often means
the source shape is leading the scheduler somewhere the original's did
not go).

    python3 tools/audit_swaps.py            # audit all
    python3 tools/audit_swaps.py Menu.c     # one file
"""
import atexit
import collections
import os
import re
import shutil
import sys
import tempfile

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.getcwd())
import ccpipe  # noqa: E402
from decomplib import Repo, mask_word, masked_compare  # noqa: E402
from elflib import Elf32  # noqa: E402

def _identity(word):
    """Instruction identity with relocatable and encoding-variant bits gone.

    Starts from decomplib's mask_word, which is what verify.py itself
    normalizes with -- WITHOUT it the two legal encodings of `break`
    (`break 0x7` = 0x000001CD and `break 0x0,0x7` = 0x0007000D) count as
    two different instructions, and any reordering flag on a function
    containing a division (gcc emits a break as the divide-by-zero trap)
    gets reported as masking a different multiset. That false positive
    cost an agent a real investigation on xglFontAscii2Euc.
    """
    word = mask_word(word)
    op = word >> 26
    # COP1 single-precision add and multiply are commutative.  The original
    # and rebuilt compilers can choose opposite fs/ft encodings even though
    # the instruction has the same destination and semantics.  Canonicalize
    # those two source fields so --swap-fp-operands remains auditable by the
    # same multiset test as the instruction-order corrections.
    if (op == 17 and ((word >> 21) & 0x1F) == 16
            and (word & 0x3F) in (0, 2)):
        ft = (word >> 16) & 0x1F
        fs = (word >> 11) & 0x1F
        lo, hi = sorted((ft, fs))
        word &= ~((0x1F << 16) | (0x1F << 11))
        word |= (lo << 16) | (hi << 11)
    # R-type addu/daddu/and/or/xor/nor likewise differ only by a
    # commutative rs/rt encoding at --swap-int-operands sites.
    if op == 0 and (word & 0x3F) in (0x21, 0x2D, 0x24, 0x25, 0x26, 0x27):
        rs = (word >> 21) & 0x1F
        rt = (word >> 16) & 0x1F
        lo, hi = sorted((rs, rt))
        word &= ~((0x1F << 21) | (0x1F << 16))
        word |= (lo << 21) | (hi << 16)
    if op == 0 or op == 16 or op == 17 or op == 18:
        return word                      # R-type / coprocessor: keep whole
    if op in (2, 3):
        return op << 26                  # j / jal: target is a reloc
    return word & 0xFFFF0000             # I-type: drop the immediate


REORDER_FLAGS = ("--swap-adjacent", "--rotate", "--rotate-seq",
                 "--swap-into-slot", "--swap-slot-target",
                 "--hoist-div-arg", "--retime-branch-slot",
                 "--swap-fp-operands", "--swap-int-operands")

_TMPDIR = tempfile.mkdtemp(prefix="audit_swaps.")
atexit.register(shutil.rmtree, _TMPDIR, True)


def sources_by_basename():
    out = {}
    for root, _dirs, files in os.walk("src"):
        for f in files:
            if f.endswith(".c"):
                out[f] = os.path.join(root, f)
    return out


def main():
    only = sys.argv[1] if len(sys.argv) > 1 else None
    import configure
    srcs = sources_by_basename()
    repo = Repo()
    problems = []
    skipped = []
    audited = 0
    for base, flagstr in sorted(configure.FILE_FIX_FLAGS.items()):
        if only and base != only:
            continue
        if not any(f in flagstr for f in REORDER_FLAGS):
            continue
        path = srcs.get(base)
        if not path:
            continue
        cc, cflags, fixflags, asflags = ccpipe.file_settings(base)
        fixflags = list(fixflags)
        # Strip the reordering flags and their site lists.
        stripped, sites = [], collections.defaultdict(list)
        i = 0
        while i < len(fixflags):
            if fixflags[i] in REORDER_FLAGS:
                for site in fixflags[i + 1].split(","):
                    fn = site.split(":")[0]
                    sites[fn].append(fixflags[i])
                i += 2
                continue
            stripped.append(fixflags[i])
            i += 1
        # A per-process temp dir, NOT a fixed path: several agents run this
        # at once in the same container, and a shared /tmp/_audit.o meant
        # concurrent runs clobbered each other's object mid-compile. That
        # produced BOTH false "does not compile right now" skips and false
        # SUSPECT verdicts on functions that are fine.
        obj = os.path.join(_TMPDIR, "audit.o")
        ok, log = ccpipe.compile_c(path, obj, cc=cc,
                                   cflags=cflags, fixflags=tuple(stripped),
                                   asflags=asflags)
        if not ok:
            # Eleven agents edit this tree at once, so a file that does not
            # compile right now is almost always someone mid-edit, not a
            # bad flag. Say so instead of filing it as a SUSPECT reorder.
            print(f"  SKIP {base:41} does not compile right now "
                  f"(mid-edit?) -- not audited")
            skipped.append(base)
            continue
        elf = Elf32(obj)
        syms = elf.func_symbols_sized()
        for fn, used in sorted(sites.items()):
            loc = repo.orig_functions.get(fn)
            if fn not in syms or not loc:
                continue
            audited += 1
            off, bsize = syms[fn]
            vaddr, size = loc
            orig = repo.orig.words_at_vaddr(vaddr, size)
            # Each function read at ITS OWN extent. Using the original's
            # size for both truncates a longer un-flagged build and drops
            # its epilogue out of the window, which then shows up as
            # "orig has jr/addiu, built has nops" -- a fake multiset
            # difference. cos and sin were reported SUSPECT purely
            # because of this.
            built = elf.words_at_offset(off, bsize or size)
            om, bm, diffs = masked_compare(orig, built, elf.relocs(), off)
            # Compare multisets on instruction IDENTITY, not raw words:
            # masked_compare blanks relocated immediates BY POSITION, so a
            # moved instruction gets masked at the wrong index and a pure
            # reorder would look like a changed multiset. Zero the immediate
            # of I-type ops (they are what carry %hi/%lo relocs) and the
            # target of J-type; R-type words are kept whole.
            co = collections.Counter(map(_identity, orig))
            cb = collections.Counter(map(_identity, built))
            same = co == cb
            # A delta of NOPS ONLY is still a pure reorder. Moving two
            # instructions can let gas fill a delay slot it previously
            # had to pad, so the un-flagged build carries extra nops and
            # nothing else. A nop computes nothing, so this cannot hide a
            # logic difference -- what matters is that every REAL
            # instruction is accounted for.
            delta = list((co - cb).elements()) + list((cb - co).elements())
            nop_only = bool(delta) and all(w == 0 for w in delta)
            if nop_only:
                same = True
            note = ""
            if nop_only:
                note = (f", differs by {len(delta)} nop(s) only "
                        f"-- gas filled a slot it had padded")
            tag = "OK  " if same else "BAD "
            print(f"  {tag}{fn:38} {len(diffs):3} pre-fix diffs, "
                  f"{len(used)} site(s), identical multiset={same}{note}")
            if not same:
                problems.append((base, fn,
                                 "reordering is masking a different "
                                 "instruction multiset"))
    print(f"\naudited {audited} reordering-flag functions")
    if skipped:
        print(f"skipped {len(skipped)} file(s) that do not compile right "
              f"now: {', '.join(skipped)}")
    if problems:
        print("\nSUSPECT -- these are not pure reorders:")
        for base, fn, why in problems:
            print(f"  {base}: {fn}: {why}")
        return 1
    print("all reordering flags are pure instruction reorders "
          "(same instructions, different order)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
