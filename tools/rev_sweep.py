#!/usr/bin/env python3
"""Build a TU against several candidate REVISIONS of a vendored source file.

    python3 tools/rev_sweep.py src/libc/newlib_vfprintf.c vfprintf.c \
        /tmp/revs/vfprintf_*.c

Asking "am I on the right source revision?" beats asking "what flag fixes
this?" for every file in src/libc and src/libgcc, because the originals
are public. It has now dissolved four walls that had been triaged as
register allocation and swept for flags for a long time:

  dpmul             117d -> 0  (fp-bit.c: the tree had a gcc-3.4.0 hand
                                transcription, the rest of libgcc is the
                                1999-12-29 rev, and multiply was
                                rewritten in between)
  find_fde           20d -> 0  (frame.c 78a0d70cdf55, 2000-02-01)
  end_fde_sort      138d -> 0  (frame.c 89d7f003d32b, 2000-06-08)
  frame_init         90d -> 0  (same commit, via inlined start_fde_sort)

Getting the candidates -- no local clone needed, and the commit subjects
alone often name the function you are stuck on:

    gh api "repos/gcc-mirror/gcc/commits?path=gcc/frame.c\
&until=2000-11-01T00:00:00Z&per_page=30" \
      --jq '.[] | "\\(.sha[0:12]) \\(.commit.author.date[0:10]) \\
\\(.commit.message | split("\\n")[0])"'

    gh api "repos/mirror/newlib-cygwin/contents/newlib/libc/stdio/vfprintf.c\
?ref=SHA" --jq '.content' | base64 -d > /tmp/revs/vfprintf_SHA.c

Bound the search by DATE. The ee-gcc 2.96 snapshot is 2000-10-03 and the
newlib base is 2000-08-16, so a commit after that is not in the shipped
library -- which is exactly how the vfprintf.c question got settled in
the negative rather than left open.

Each candidate is copied into a scratch include directory that is
PREPENDED to the include path, so the real newlib/ tree is never
modified. That matters: eleven agents build out of this checkout at once
and swapping a vendored header in place would hand one of them a
corrupt object.

A negative result is worth as much as a positive one -- record it in the
TU's header so the next person does not re-bisect.
"""
import argparse
import os
import shutil
import sys
import tempfile

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(ROOT, "tools"))
sys.path.insert(0, ROOT)
import ccpipe  # noqa: E402
from decomplib import Repo, parse_decompiled, masked_compare  # noqa: E402
from elflib import Elf32  # noqa: E402


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("source", help="the src/*.c wrapper TU")
    ap.add_argument("included", help="basename the wrapper #includes, "
                                     "e.g. vfprintf.c")
    ap.add_argument("candidates", nargs="+")
    args = ap.parse_args()

    base = os.path.basename(args.source)
    cc, cflags, fix, asflags = ccpipe.file_settings(base)
    repo = Repo()
    ents, hw = parse_decompiled()
    reg = {e.name: (e.addr, e.size) for e in ents + hw}

    incdir = tempfile.mkdtemp(prefix="revsweep_inc_")
    work = tempfile.mkdtemp(prefix="revsweep_")
    try:
        for cand in args.candidates:
            shutil.copy(cand, os.path.join(incdir, args.included))
            obj = os.path.join(work, "o.o")
            ok, log = ccpipe.compile_c(args.source, obj, cc=cc,
                                       cflags="-I" + incdir + " " + cflags,
                                       fixflags=fix, asflags=asflags)
            tag = os.path.basename(cand)
            if not ok:
                tail = " | ".join(log.strip().split("\n")[-2:])[:110]
                print(f"{tag:36s} COMPILE FAILED: {tail}", flush=True)
                continue
            e = Elf32(obj)
            counts = {}
            for name, off in e.func_symbols().items():
                loc = reg.get(name) or repo.orig_functions.get(name)
                if not loc:
                    continue
                addr, size = loc
                orig = repo.orig.words_at_vaddr(addr, size)
                built = e.words_at_offset(off, size)
                _, _, diffs = masked_compare(orig, built, e.relocs(), off)
                counts[name] = len(diffs)
            total = sum(counts.values())
            detail = " ".join(f"{k}={v}" for k, v in sorted(counts.items()))
            print(f"{tag:36s} total={total:6d}  {detail}", flush=True)
    finally:
        shutil.rmtree(incdir, ignore_errors=True)
        shutil.rmtree(work, ignore_errors=True)
    return 0


if __name__ == "__main__":
    sys.exit(main())
