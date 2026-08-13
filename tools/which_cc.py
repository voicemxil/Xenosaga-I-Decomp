#!/usr/bin/env python3
"""Scan every function in the original ELF for the 2.9-ee SDK-compiler
prologue fingerprint (16 bytes reserved per saved callee-saved register,
instead of 2.96's 8 bytes).

Usage:
    python3 tools/which_cc.py                # full sweep, summary + clusters
    python3 tools/which_cc.py -v              # also list every hit
    python3 tools/which_cc.py NAME [NAME...]  # inspect specific functions

Method: decode the prologue of every function directly from
Repo.orig_functions (name -> (vaddr, size)) in the ORIGINAL, already-linked
ELF. Find the initial `addiu/daddiu sp,sp,-N` frame allocation, then walk
forward collecting `sd`/`sw` stores of callee-saved integer registers
($16-$23 = s0-s7, $30 = s8/fp, $31 = ra) to `offset(sp)`, stopping at the
first control-transfer instruction (jal/j/jr/branch). If there are at least
two such stores, the constant stride between their (sorted) offsets tells
you the per-register reservation: 8 bytes/reg is 2.96 (o32 EABI), 16
bytes/reg is the 2.9-ee SDK compiler. Functions with 0 or 1 saved register
can't be classified this way (no stride to measure) and are skipped as
"insufficient data", not counted either way.

This is a fingerprint, not proof. A hit should still be confirmed by
actually rebuilding the file with the SDK compiler and checking that the
diff drops (see XENOSAGA_RESUME_PROMPT.md).
"""
import os
import sys
from collections import Counter, defaultdict

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from decomplib import Repo  # noqa: E402

CALLEE_SAVED = set(range(16, 24)) | {30, 31}  # s0-s7, s8/fp, ra
SP = 29


def decode_prologue(words):
    """Return (frame_size_or_None, [(reg, offset), ...] saved-reg stores)."""
    frame = None
    saves = []
    start = 0
    # Locate the initial sp adjustment (addiu/daddiu sp,sp,-N). Usually the
    # first instruction, but a leading arg-register move can precede it.
    for i, w in enumerate(words[:4]):
        opcode = w >> 26
        rs = (w >> 21) & 0x1f
        rt = (w >> 16) & 0x1f
        imm = w & 0xffff
        if opcode in (0x09, 0x19) and rs == SP and rt == SP:  # addiu/daddiu
            if imm >= 0x8000:
                imm -= 0x10000
            if imm < 0:
                frame = -imm
                start = i + 1
                break
    if frame is None:
        return None, []

    seen_regs = set()
    for w in words[start:start + 40]:
        opcode = w >> 26
        rs = (w >> 21) & 0x1f
        rt = (w >> 16) & 0x1f
        imm = w & 0xffff
        funct = w & 0x3f

        if opcode in (0x2b, 0x3f):  # sw, sd
            if rs == SP and rt in CALLEE_SAVED:
                if rt in seen_regs:
                    # A repeat store of an already-saved register means
                    # we've drifted out of the prologue into the body
                    # (e.g. a loop spilling through the same stack slot).
                    break
                off = imm - 0x10000 if imm >= 0x8000 else imm
                saves.append((rt, off))
                seen_regs.add(rt)
            continue
        if opcode == 0x00 and funct in (0x21, 0x2d):  # addu/daddu = move pseudo
            continue  # argument-register renames don't end the prologue
        if opcode == 0x00 and funct in (0x08, 0x09):  # jr/jalr
            break
        if opcode in (0x02, 0x03):  # j, jal
            break
        # branches: opcodes 0x01,0x04-0x07,0x14-0x17
        if opcode in (0x01, 0x04, 0x05, 0x06, 0x07, 0x14, 0x15, 0x16, 0x17):
            break
    return frame, saves


def classify(words):
    """Return 'sdk' | '296' | None (insufficient data), plus the stride."""
    frame, saves = decode_prologue(words)
    if frame is None or len(saves) < 2:
        return None, None, frame, saves
    offs = sorted(set(o for _, o in saves))
    if len(offs) < 2:
        return None, None, frame, saves
    strides = [offs[i + 1] - offs[i] for i in range(len(offs) - 1)]
    if all(s == 8 for s in strides):
        return "296", 8, frame, saves
    # Multiples of 16 only (16, 32, 48, ...) are still consistent with a
    # 16-bytes-per-register reservation where some slots are skipped
    # (e.g. an interspersed FP callee-save we don't track, or a
    # non-contiguous register choice). A genuine 8 mixed in with 16s is
    # the truly ambiguous case.
    if all(s % 16 == 0 for s in strides):
        return "sdk", strides, frame, saves
    return "mixed", strides, frame, saves


REG_NAMES = {16: "s0", 17: "s1", 18: "s2", 19: "s3", 20: "s4", 21: "s5",
             22: "s6", 23: "s7", 30: "s8", 31: "ra"}


def main():
    repo = Repo()
    args = sys.argv[1:]
    verbose = "-v" in args
    names = [a for a in args if not a.startswith("-")]

    if names:
        for name in names:
            fn = repo.orig_functions.get(name)
            if not fn:
                print(f"{name}: not found in ELF symbol table")
                continue
            addr, size = fn
            words = repo.orig.words_at_vaddr(addr, size)
            verdict, stride, frame, saves = classify(words)
            regs = ", ".join(f"{REG_NAMES.get(r, r)}@{o}" for r, o in saves)
            print(f"{name} @ {addr:#010x} size={size:#x} frame={frame} "
                  f"saves=[{regs}] verdict={verdict} stride={stride}")
        return

    sdk_hits = []
    normal = 0
    mixed_hits = []
    insufficient = 0

    for name, (addr, size) in sorted(repo.orig_functions.items(),
                                      key=lambda kv: kv[1][0]):
        if size <= 0:
            continue
        words = repo.orig.words_at_vaddr(addr, size)
        if not words:
            continue
        verdict, stride, frame, saves = classify(words)
        if verdict == "sdk":
            sdk_hits.append((name, addr, size, frame, saves))
        elif verdict == "296":
            normal += 1
        elif verdict == "mixed":
            mixed_hits.append((name, addr, size, frame, saves, stride))
        else:
            insufficient += 1

    print(f"Scanned {len(repo.orig_functions)} ELF functions.")
    print(f"SDK-fingerprint (16 bytes/saved-reg): {len(sdk_hits)}")
    print(f"Normal 2.96 (8 bytes/saved-reg):       {normal}")
    print(f"Mixed stride (ambiguous):               {len(mixed_hits)}")
    print(f"Insufficient data (<2 saved regs):      {insufficient}")
    print()

    # Cluster by name prefix (strip trailing digits/camel tail heuristics:
    # just use the leading run of non-uppercase-transition chars up to
    # first underscore, or first 3-6 chars, whichever is more informative).
    def prefix_of(name):
        if "_" in name:
            return name.split("_")[0] + "_*"
        # camelCase / PascalCase leading token
        i = 1
        while i < len(name) and not (name[i].isupper() and name[i - 1].islower()):
            i += 1
        return name[:i] if i < len(name) else name

    by_prefix = Counter(prefix_of(n) for n, *_ in sdk_hits)
    print("SDK-fingerprint hits clustered by name prefix:")
    for prefix, count in by_prefix.most_common(60):
        print(f"  {count:4d}  {prefix}")

    if verbose:
        print()
        print("All SDK-fingerprint hits:")
        for name, addr, size, frame, saves in sdk_hits:
            regs = ", ".join(f"{REG_NAMES.get(r, r)}@{o}" for r, o in saves)
            print(f"  {name} @ {addr:#010x} size={size:#x} frame={frame} [{regs}]")

    if mixed_hits:
        print()
        print(f"Mixed-stride functions ({len(mixed_hits)}, worth eyeballing):")
        for name, addr, size, frame, saves, stride in mixed_hits[:40]:
            regs = ", ".join(f"{REG_NAMES.get(r, r)}@{o}" for r, o in saves)
            print(f"  {name} @ {addr:#010x} frame={frame} strides={stride} [{regs}]")


if __name__ == "__main__":
    main()
