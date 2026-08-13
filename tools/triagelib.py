#!/usr/bin/env python3
"""Classify a near-miss diff: register tie-break, scheduling, nop count,
immediate/relocation, or a genuine logic difference.

Agents were doing this by eye off diff_fn.py output and occasionally got
it wrong (e.g. discarding a match over cvt.w.s vs trunc.w.s, which is the
same encoding). The classification here is done on ENCODINGS, using the
same masking verify.py uses.
"""

SPECIAL = 0x00
REGIMM = 0x01
COP1 = 0x11

# opcodes whose operands are (rs, rt, imm16)
I_TYPE = set(range(0x04, 0x10)) | set(range(0x20, 0x30)) | {
    0x18, 0x19, 0x1A, 0x1B, 0x31, 0x35, 0x39, 0x3D}


def fields(w):
    return {
        "op": (w >> 26) & 0x3F,
        "rs": (w >> 21) & 0x1F,
        "rt": (w >> 16) & 0x1F,
        "rd": (w >> 11) & 0x1F,
        "sa": (w >> 6) & 0x1F,
        "funct": w & 0x3F,
        "imm": w & 0xFFFF,
        "target": w & 0x03FFFFFF,
    }


def same_shape(a, b):
    """True if two words are the same instruction modulo register numbers."""
    fa, fb = fields(a), fields(b)
    if fa["op"] != fb["op"]:
        return False
    if fa["op"] in (SPECIAL, COP1):
        return fa["funct"] == fb["funct"] and fa["sa"] == fb["sa"]
    if fa["op"] == REGIMM:
        return fa["rt"] == fb["rt"]
    return fa["imm"] == fb["imm"]


def same_regs(a, b):
    fa, fb = fields(a), fields(b)
    return (fa["rs"], fa["rt"], fa["rd"]) == (fb["rs"], fb["rt"], fb["rd"])


NOP = 0x00000000


def classify_word(a, b):
    """Per-instruction class for a differing pair."""
    fa, fb = fields(a), fields(b)
    if fa["op"] != fb["op"]:
        return "opcode"
    if fa["op"] in (SPECIAL, COP1) and fa["funct"] != fb["funct"]:
        return "opcode"
    if same_shape(a, b) and not same_regs(a, b):
        return "register"
    if same_regs(a, b) and not same_shape(a, b):
        return "immediate"
    return "operands"


def register_swaps(orig, built, diffs):
    """Which register numbers were substituted, as (orig, built) pairs."""
    out = []
    for i in diffs:
        fa, fb = fields(orig[i]), fields(built[i])
        for f in ("rs", "rt", "rd"):
            if fa[f] != fb[f]:
                out.append((fa[f], fb[f]))
    return out


REG_NAMES = ["zero", "at", "v0", "v1", "a0", "a1", "a2", "a3",
             "t0", "t1", "t2", "t3", "t4", "t5", "t6", "t7",
             "s0", "s1", "s2", "s3", "s4", "s5", "s6", "s7",
             "t8", "t9", "k0", "k1", "gp", "sp", "fp", "ra"]


def triage(masked_orig, masked_built, diffs=None):
    """Classify a whole function. Returns (label, detail-string)."""
    om, bm = masked_orig, masked_built
    if diffs is None:
        n = min(len(om), len(bm))
        diffs = [i for i in range(n) if om[i] != bm[i]]

    if len(om) == len(bm) and not diffs:
        return "MATCH", "identical after masking"

    if len(om) != len(bm):
        no_nop_o = [w for w in om if w != NOP]
        no_nop_b = [w for w in bm if w != NOP]
        if no_nop_o == no_nop_b:
            return ("NOP-COUNT",
                    f"same instructions, {om.count(NOP)} nops in original vs "
                    f"{bm.count(NOP)} built ({len(om)} vs {len(bm)} words)")
        if sorted(no_nop_o) == sorted(no_nop_b):
            return ("SCHEDULING+NOP",
                    f"same instruction multiset reordered, plus a nop-count "
                    f"difference ({len(om)} vs {len(bm)} words)")
        return ("LENGTH",
                f"different instruction counts ({len(om)} orig vs {len(bm)} built)")

    if sorted(om) == sorted(bm):
        moved = len(diffs)
        return ("SCHEDULING",
                f"identical instruction multiset in a different order "
                f"({moved} positions differ)")

    # Nop-count difference that happens to keep the length (padding moved).
    kinds = [classify_word(om[i], bm[i]) for i in diffs]
    uniq = set(kinds)

    if uniq == {"register"}:
        swaps = register_swaps(om, bm, diffs)
        pretty = ", ".join(sorted({f"${REG_NAMES[a]}->${REG_NAMES[b]}"
                                   for a, b in swaps}))
        return ("REGISTER", f"{len(diffs)} instruction(s), registers only: {pretty}")

    if uniq == {"immediate"}:
        return ("IMMEDIATE",
                f"{len(diffs)} instruction(s) differ only in an immediate/offset "
                f"(check for an unmasked relocation or a wrong struct offset)")

    if uniq <= {"register", "immediate", "operands"}:
        return ("OPERANDS",
                f"{len(diffs)} instruction(s), same opcodes but different "
                f"operands ({'/'.join(sorted(uniq))})")

    if NOP in (om[i] for i in diffs) or NOP in (bm[i] for i in diffs):
        if sorted(w for w in om if w != NOP) == sorted(w for w in bm if w != NOP):
            return ("NOP-PLACEMENT",
                    f"same instructions, {len(diffs)} nop position(s) differ")

    n_op = kinds.count("opcode")
    return ("LOGIC",
            f"{len(diffs)} differing instruction(s), {n_op} with a different "
            f"opcode -- a real code difference, not a tie-break")
