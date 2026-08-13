#!/usr/bin/env python3
"""Brute-force the statement orderings gcc 2.96 collapses into a schedule.

gcc 2.96 permutes runs of consecutive stores, so the source order of a
store group is observable but NOT guessable -- and for N statements only
a handful of distinct machine schedules exist. This compiles every
ordering of the marked statements, dedupes the emitted instruction
sequences, and tells you which ordering (if any) reproduces the original.

Mark the reorderable statements in the C file:

    /* PERM_BEGIN */
    p->x = a;
    p->y = b;
    p->z = c;
    /* PERM_END */

Multiple blocks are allowed; every combination of per-block orderings is
tried. Statements may span several lines (brace/paren depth is tracked),
and a comment line sticks to the statement below it.

    python3 tools/permute.py work.c --fn xglFooSet
    python3 tools/permute.py work.c --fn Foo --cc29        # SDK compiler
    python3 tools/permute.py work.c --fn Foo --addr 0x2011a0 --size 0x40
    python3 tools/permute.py work.c --fn Foo --jobs 8 --max 5040
    python3 tools/permute.py work.c --fn Foo --keep /tmp/best.c

Exit status is 0 when a matching ordering was found, 1 otherwise.
"""
import argparse
import itertools
import multiprocessing
import os
import sys
import tempfile

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from decomplib import Repo, masked_compare  # noqa: E402
from elflib import Elf32  # noqa: E402
from triagelib import triage  # noqa: E402
import ccpipe  # noqa: E402

BEGIN = "PERM_BEGIN"
END = "PERM_END"


def split_statements(lines):
    """Split block lines into statement units, tracking nesting."""
    units, cur, depth = [], [], 0
    for line in lines:
        cur.append(line)
        code = line.split("//")[0]
        depth += code.count("(") - code.count(")")
        depth += code.count("{") - code.count("}")
        stripped = code.strip()
        if depth <= 0 and (stripped.endswith(";") or stripped.endswith("}")):
            units.append(cur)
            cur = []
            depth = 0
    if cur and any(l.strip() for l in cur):
        units.append(cur)
    return units


def parse_blocks(path):
    """Return (prefix_chunks, blocks) so the file can be reassembled."""
    lines = open(path).read().split("\n")
    chunks, blocks, i = [], [], 0
    plain = []
    while i < len(lines):
        if BEGIN in lines[i]:
            chunks.append(("text", plain))
            plain = []
            body, i = [], i + 1
            while i < len(lines) and END not in lines[i]:
                body.append(lines[i])
                i += 1
            i += 1  # skip END
            units = split_statements(body)
            blocks.append(units)
            chunks.append(("block", len(blocks) - 1))
            continue
        plain.append(lines[i])
        i += 1
    chunks.append(("text", plain))
    return chunks, blocks


def render(chunks, blocks, orders):
    out = []
    for kind, val in chunks:
        if kind == "text":
            out.extend(val)
        else:
            units = blocks[val]
            for idx in orders[val]:
                out.extend(units[idx])
    return "\n".join(out)


_ctx = {}


def _init(ctx):
    _ctx.update(ctx)


def _run(job):
    n, orders = job
    src = render(_ctx["chunks"], _ctx["blocks"], orders)
    d = _ctx["tmpdir"]
    cpath = os.path.join(d, f"p{n}.c")
    opath = os.path.join(d, f"p{n}.o")
    with open(cpath, "w") as f:
        f.write(src)
    ok, log = ccpipe.compile_c(cpath, opath, cc=_ctx["cc"],
                               cflags=_ctx["cflags"], fixflags=_ctx["fixflags"],
                               asflags=_ctx.get("asflags", ""))
    if not ok:
        os.unlink(cpath)
        return orders, None, None, log.strip().split("\n")[0][:160]
    try:
        e = Elf32(opath)
        off = e.func_symbols().get(_ctx["fn"])
        if off is None:
            return orders, None, None, f"no function {_ctx['fn']} in output"
        built = e.words_at_offset(off, _ctx["size"])
        relocs = e.relocs()
    finally:
        for p in (cpath, opath):
            if os.path.exists(p):
                os.unlink(p)
    om, bm, diffs = masked_compare(_ctx["target"], built, relocs, off)
    score = len(diffs) + 4 * abs(len(_ctx["target"]) - len(built))
    return orders, score, (tuple(bm), tuple(om)), None


def describe(blocks, orders):
    parts = []
    for b, order in enumerate(orders):
        names = []
        for idx in order:
            txt = " ".join(l.strip() for l in blocks[b][idx] if l.strip())
            names.append(txt[:40])
        parts.append(f"    block {b}: " + "  |  ".join(names))
    return "\n".join(parts)


def main():
    ap = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("source")
    ap.add_argument("--fn", required=True, help="function to compare")
    ap.add_argument("--target", help="name in the original ELF, if different")
    ap.add_argument("--addr", help="original address (else looked up by name)")
    ap.add_argument("--size", help="original size in bytes")
    ap.add_argument("--cc29", action="store_true",
                    help="use the SDK compiler (2.9-ee) instead of 2.96")
    ap.add_argument("--cflags")
    ap.add_argument("--fix", action="append", default=[],
                    help="extra tools/fix_cc_asm.py flag, e.g. --fix '--omit-hazard mov.s'")
    ap.add_argument("--jobs", type=int, default=max(1, (os.cpu_count() or 2)))
    ap.add_argument("--max", type=int, default=5040,
                    help="refuse to sweep more orderings than this")
    ap.add_argument("--keep", help="write the best-scoring source here")
    ap.add_argument("--asflags", default="",
                    help="extra assembler flags, e.g. --asflags -G0 "
                         "(gas has its own -G small-data threshold, "
                         "independent of the compiler's -G)")
    ap.add_argument("--all-scores", action="store_true",
                    help="list every distinct schedule, not just the best")
    args = ap.parse_args()

    chunks, blocks = parse_blocks(args.source)
    if not blocks:
        sys.exit(f"no /* {BEGIN} */ ... /* {END} */ block in {args.source}")

    repo = Repo()
    name = args.target or args.fn
    if args.addr and args.size:
        addr, size = int(args.addr, 0), int(args.size, 0)
    else:
        loc = repo.orig_functions.get(name)
        if not loc:
            sys.exit(f"{name} not found in the original ELF symbol table; "
                     f"pass --addr/--size")
        addr, size = loc
    target = repo.orig.words_at_vaddr(addr, size)
    if not target:
        sys.exit(f"no original bytes at {addr:#x}")

    cc = ccpipe.CC29 if args.cc29 else ccpipe.CC96
    cflags = args.cflags or (ccpipe.CFLAGS29 if args.cc29 else ccpipe.CFLAGS96)
    fixflags = [tok for f in args.fix for tok in f.split()]

    counts = [len(b) for b in blocks]
    total = 1
    for c in counts:
        for k in range(2, c + 1):
            total *= k
    sizes = " x ".join(f"{c}!" for c in counts)
    print(f"{name}: {len(target)} words at {addr:#x}")
    print(f"blocks: {counts} -> {sizes} = {total} orderings, {args.jobs} jobs")
    if total > args.max:
        sys.exit(f"refusing to sweep {total} orderings (--max {args.max}); "
                 f"split the block or raise --max")

    perms = [list(itertools.permutations(range(c))) for c in counts]
    jobs = [(i, o) for i, o in enumerate(itertools.product(*perms))]

    tmpdir = tempfile.mkdtemp(prefix="permute_")
    ctx = {"chunks": chunks, "blocks": blocks, "tmpdir": tmpdir, "cc": cc,
           "cflags": cflags, "fixflags": fixflags, "fn": args.fn, "size": size,
           "target": target, "asflags": args.asflags}

    results = []
    errors = []
    if args.jobs > 1:
        with multiprocessing.Pool(args.jobs, _init, (ctx,)) as pool:
            for r in pool.imap_unordered(_run, jobs, chunksize=1):
                results.append(r)
    else:
        _init(ctx)
        results = [_run(j) for j in jobs]

    schedules = {}
    best = None
    for orders, score, key, err in results:
        if err:
            errors.append(err)
            continue
        sig = key[0]
        slot = schedules.setdefault(sig, [score, orders, 0])
        slot[2] += 1
        if best is None or score < best[0]:
            best = (score, orders)

    if errors:
        print(f"\n{len(errors)} ordering(s) failed to compile, e.g.: {errors[0]}")
    if not schedules:
        sys.exit("nothing compiled")

    print(f"\ncompiled {sum(s[2] for s in schedules.values())} orderings -> "
          f"{len(schedules)} distinct instruction sequence(s)")
    ranked = sorted(schedules.values(), key=lambda s: s[0])
    for score, orders, n in (ranked if args.all_scores else ranked[:3]):
        tag = "MATCH" if score == 0 else f"{score} diffs"
        print(f"\n  [{tag}] reached by {n} ordering(s); first one:")
        print(describe(blocks, orders))

    if best and best[0] == 0:
        print(f"\nMATCH -- use this ordering.")
    else:
        # explain what the best one is still missing
        _, orders = best
        src = render(chunks, blocks, orders)
        path = args.keep or os.path.join(tmpdir, "best.c")
        with open(path, "w") as f:
            f.write(src)
        obj = os.path.join(tmpdir, "best.o")
        ok, _ = ccpipe.compile_c(path, obj, cc=cc, cflags=cflags,
                                 fixflags=fixflags, asflags=args.asflags)
        if ok:
            e = Elf32(obj)
            off = e.func_symbols()[args.fn]
            built = e.words_at_offset(off, size)
            om, bm, diffs = masked_compare(target, built, e.relocs(), off)
            label, detail = triage(om, bm, diffs)
            print(f"\nno ordering matches. best is {best[0]}: {label} -- {detail}")
        print(f"best source written to {path}")
    if args.keep and best[0] == 0:
        with open(args.keep, "w") as f:
            f.write(render(chunks, blocks, best[1]))
        print(f"matching source written to {args.keep}")
    return 0 if best and best[0] == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
