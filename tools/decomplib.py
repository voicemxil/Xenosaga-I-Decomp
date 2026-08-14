#!/usr/bin/env python3
"""Shared plumbing for the decomp verification tools.

Holds the three things every tool here re-implemented: parsing
config/decompiled.txt, finding which build/src/*.o defines a symbol, and
the masked original-vs-built word comparison (jal/j targets and
link-time-resolved immediates are unresolved in a .o, so they are masked).

Reads ELF files directly (tools/elflib.py) instead of shelling out to
objdump, which is what makes a full verify take ~1s instead of ~75s.
"""
import glob
import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from elflib import Elf32  # noqa: E402

ELF = "elf/SLUS_204.69"
BUILT = "build/src"
DECOMP = "config/decompiled.txt"

# Relocation types whose 16-bit immediate is only filled in at link time.
MASKED_IMM16 = ("GPREL16", "HI16", "LO16", "16", "LITERAL")

# Sentinel for masked `break` encodings: outside the 32-bit word range so
# it can never collide with a real instruction.
BREAK = 1 << 32

ENTRY_RE = re.compile(
    r'(\w+)\s*=\s*(0x[0-9A-Fa-f]+),\s*(0x[0-9A-Fa-f]+);')


class Entry:
    __slots__ = ("name", "addr", "size", "hardware", "source", "line")

    def __init__(self, name, addr, size, hardware, source, line):
        self.name = name
        self.addr = addr
        self.size = size
        self.hardware = hardware
        self.source = source
        self.line = line

    def __repr__(self):
        return f"<Entry {self.name} {self.addr:#x} {self.size:#x}>"


def parse_decompiled(path=DECOMP):
    """Return (entries, hardware) as lists of Entry, skipping comments."""
    entries, hardware = [], []
    with open(path) as f:
        for line in f:
            line = line.strip()
            if not line or line.startswith('#'):
                continue
            m = ENTRY_RE.match(line)
            if not m:
                continue
            src = ""
            c = line.find("//")
            if c >= 0:
                src = line[c + 2:].strip().split()[0] if line[c + 2:].strip() else ""
            e = Entry(m.group(1), int(m.group(2), 16), int(m.group(3), 16),
                      'HARDWARE' in line, src, line)
            (hardware if e.hardware else entries).append(e)
    return entries, hardware


def mask_word(w):
    """Mask jal targets and break encodings (same rule as the old tools)."""
    if (w >> 26) == 0x03:
        return w & 0xFC000000
    if (w & 0xFC00003F) == 0x0000000D:
        return BREAK  # both break encodings compare equal
    return w


def masked_compare(orig, built, relocs, offset=0):
    """Mask both word lists the way verify.py does and return (om, bm, diffs).

    `relocs` is {byte offset -> type name} for the BUILT object; `offset`
    is where the function starts inside that object's .text.
    """
    om = [mask_word(w) for w in orig]
    bm = [mask_word(w) for w in built]
    n = min(len(om), len(bm))
    for i in range(n):
        rtype = relocs.get(offset + i * 4)
        if rtype is None:
            continue
        if rtype in MASKED_IMM16:
            om[i] &= 0xFFFF0000
            bm[i] &= 0xFFFF0000
        elif rtype == "26":
            om[i] &= 0xFC000000
            bm[i] &= 0xFC000000
    return om, bm, [i for i in range(n) if om[i] != bm[i]]


class Repo:
    """Original ELF + the built objects, with a symbol index."""

    def __init__(self, elf=ELF, built=BUILT):
        self.orig = Elf32(elf)
        self.built_dir = built
        self._objs = {}
        self._symbol_map = None
        self._orig_funcs = None

    def obj(self, path):
        e = self._objs.get(path)
        if e is None:
            e = self._objs[path] = Elf32(path)
        return e

    @property
    def symbol_map(self):
        """function name -> (object path, offset in that object's .text).

        Matches the old objdump -t scan: sorted object order, first
        defined `F .text` symbol wins.
        """
        if self._symbol_map is None:
            self._symbol_map = {}
            for path in sorted(glob.glob(f"{self.built_dir}/**/*.o", recursive=True)):
                try:
                    funcs = self.obj(path).func_symbols()
                except (ValueError, IndexError, OSError):
                    continue
                for name, off in funcs.items():
                    self._symbol_map.setdefault(name, (path, off))
        return self._symbol_map

    @property
    def orig_functions(self):
        """name -> (vaddr, size) for every function in the original ELF.

        The shipped ELF kept its full symbol table, so any function can be
        looked up by name without going through config/decompiled.txt.
        """
        if self._orig_funcs is None:
            self._orig_funcs = {}
            for name, value, size, typ, sec in self.orig.symbols():
                if typ == 2 and name and sec:
                    self._orig_funcs.setdefault(name, (value, size))
        return self._orig_funcs

    def compare(self, name, addr, size, source=None):
        """Compare one function. Returns a dict:

        status: 'OK' | 'FAIL' | 'SKIP'
        reason: set when SKIP
        obj, offset, orig, built (raw word lists), diffs (indices)

        `source` is the decompiled.txt source-file base name; when given
        and that object defines the symbol, it takes precedence over the
        global first-object-wins symbol map (two TUs may define the same
        function, e.g. libc.c's WIP _dtoa_r helpers vs the vendored
        newlib copies).
        """
        loc = None
        if source:
            hits = glob.glob(f"{self.built_dir}/**/{source}.o", recursive=True)
            for path in sorted(hits):
                try:
                    off = self.obj(path).func_symbols().get(name)
                except (ValueError, IndexError, OSError):
                    continue
                if off is not None:
                    loc = (path, off)
                    break
        if loc is None:
            loc = self.symbol_map.get(name)
        if loc is None:
            return {"status": "SKIP", "reason": "no object file found",
                    "name": name}
        path, offset = loc
        orig = self.orig.words_at_vaddr(addr, size)
        if not orig:
            return {"status": "SKIP", "reason": "no original bytes found",
                    "name": name, "obj": path, "offset": offset}
        obj = self.obj(path)
        built = obj.words_at_offset(offset, size)
        relocs = obj.relocs()

        om, bm, diffs = masked_compare(orig, built, relocs, offset)
        ok = (len(om) == len(bm)) and not diffs
        return {"status": "OK" if ok else "FAIL", "name": name,
                "obj": path, "offset": offset, "orig": orig, "built": built,
                "masked_orig": om, "masked_built": bm, "diffs": diffs,
                "relocs": {i: relocs.get(offset + i * 4)
                           for i in range(min(len(orig), len(built)))
                           if relocs.get(offset + i * 4)}}


def matches_filter(entry, only=None, files=None, repo=None):
    """--only PREFIX[,PREFIX...] / --file NAME[,NAME...] filtering."""
    if only:
        if not any(entry.name.startswith(p) or p in entry.name for p in only):
            return False
    if files:
        src = entry.source or ""
        obj = ""
        if repo is not None:
            loc = repo.symbol_map.get(entry.name)
            if loc:
                obj = os.path.basename(loc[0])
        if not any(f in src or f.replace(".c", ".o") == obj or f == obj
                   for f in files):
            return False
    return True
