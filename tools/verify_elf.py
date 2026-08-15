#!/usr/bin/env python3
"""Whole-image verification: built ELF vs. the original ROM image.

tools/verify.py answers "does this function match?" one function at a time.
This answers the other question: "how much of the final executable image is
byte-correct?" -- which is what actually has to reach 100% for the build to
be a byte-matching reproduction, and what has to be close enough to boot.

    python3 tools/verify_elf.py            # summary + first mismatch ranges
    python3 tools/verify_elf.py -n 50      # show 50 ranges instead of 20
    python3 tools/verify_elf.py --per-section
    python3 tools/verify_elf.py -q         # one summary line (CI / ninja)
    python3 tools/verify_elf.py --save build/SLUS_204.69.bin

How it works: `objcopy -O binary --gap-fill=0x00` flattens the built ELF by
LOAD ADDRESS (the AT(...) expressions in config/SLUS_204.69.pinned.ld), which
is exactly the layout Splat calls the "ROM image". config/SLUS_204.69.rom is
the same transform applied to elf/SLUS_204.69, so the two are directly
comparable byte for byte. See docs/LINKING.md.

Exit status is 0 unless --strict is given and the images differ.
"""
import argparse
import os
import subprocess
import sys
import tempfile
from bisect import bisect_right

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
OBJCOPY = "/usr/local/ps2dev/ee/bin/mips64r5900el-ps2-elf-objcopy"
READELF = "/usr/local/ps2dev/ee/bin/mips64r5900el-ps2-elf-readelf"
BUILT_ELF = os.path.join(ROOT, "build", "SLUS_204.69.elf")
ORIG_ROM = os.path.join(ROOT, "config", "SLUS_204.69.rom")
ORIG_ELF = os.path.join(ROOT, "elf", "SLUS_204.69")


def die(msg):
    print(f"verify_elf: {msg}", file=sys.stderr)
    sys.exit(2)


def tool(name, path):
    if not os.path.exists(path):
        die(f"{name} not found at {path}; run inside the dev container "
            f"(see SETUP.md)")
    return path


# Splat's logical ROM image is the two loaded regions of the retail ELF laid
# end to end: file offsets 0x1000 (start of .text) through 0x2F18EC (end of
# the ov02 overlay). NOTE: `objcopy -O binary` on the RETAIL elf does *not*
# reproduce this -- the retail ov02 LOAD segment has PhysAddr 0xA5B8F8, so
# objcopy leaves a ~5.7 MB hole and emits an 8.4 MB file. (The built ELF is
# different: config/SLUS_204.69.pinned.ld gives it AT() load addresses that
# are already the ROM offsets, so objcopy on the BUILT ELF is correct.)
ROM_FILE_START = 0x1000
ROM_FILE_END = 0x2F18EC


def make_rom(out_path):
    if not os.path.exists(ORIG_ELF):
        die(f"{ORIG_ELF} not found")
    with open(ORIG_ELF, "rb") as f:
        data = f.read()
    if len(data) < ROM_FILE_END:
        die(f"{ORIG_ELF} is only {len(data)} bytes; expected at least "
            f"0x{ROM_FILE_END:X}")
    with open(out_path, "wb") as f:
        f.write(data[ROM_FILE_START:ROM_FILE_END])
    print(f"wrote {out_path} ({ROM_FILE_END - ROM_FILE_START} bytes) from "
          f"{ORIG_ELF}[0x{ROM_FILE_START:X}:0x{ROM_FILE_END:X}]")


def flatten(elf_path, out_path):
    """objcopy -O binary: ELF -> raw image ordered by load address."""
    subprocess.run(
        [tool("objcopy", OBJCOPY), "-O", "binary", "--gap-fill=0x00",
         elf_path, out_path],
        check=True)
    with open(out_path, "rb") as f:
        return f.read()


# --------------------------------------------------------------------------
# Mapping ROM offsets back to something a human can act on.
# --------------------------------------------------------------------------

def load_sections(elf_path):
    """[(name, lma, size)] for allocated PROGBITS sections, by load address.

    readelf -S prints the virtual address, not the load address, so the LMA
    comes from the program headers: each LOAD segment gives VirtAddr ->
    PhysAddr, and PhysAddr is what objcopy -O binary lays the bytes out by.
    """
    out = subprocess.run([tool("readelf", READELF), "-lS", "-W", elf_path],
                         check=True, capture_output=True, text=True).stdout

    segments = []   # (vaddr, paddr, filesz)
    sections = []   # (name, vaddr, size)
    for line in out.splitlines():
        f = line.split()
        if len(f) >= 6 and f[0] == "LOAD":
            try:
                segments.append((int(f[2], 16), int(f[3], 16), int(f[4], 16)))
            except ValueError:
                pass
        elif line.lstrip().startswith("[") and "PROGBITS" in line:
            # "  [ 1] .cod   PROGBITS   00200000 001000 138600 00  AX ..."
            body = line.split("]", 1)[1].split()
            try:
                name, vaddr, size = body[0], int(body[2], 16), int(body[4], 16)
            except (IndexError, ValueError):
                continue
            sections.append((name, vaddr, size))

    result = []
    for name, vaddr, size in sections:
        if size == 0:
            continue
        for svaddr, spaddr, sfilesz in segments:
            if sfilesz and svaddr <= vaddr < svaddr + sfilesz:
                result.append((name, vaddr - svaddr + spaddr, size))
                break
    result.sort(key=lambda s: s[1])
    return result


def load_symbols(elf_path):
    """[(vaddr, size, name)] for sized FUNC/OBJECT symbols."""
    out = subprocess.run([tool("readelf", READELF), "-sW", elf_path],
                         check=True, capture_output=True, text=True).stdout
    syms = []
    for line in out.splitlines():
        f = line.split()
        # "  12: 00200008  1234 FUNC  GLOBAL DEFAULT  1 name"
        if len(f) < 8 or not f[0].endswith(":"):
            continue
        try:
            value, size = int(f[1], 16), int(f[2], 0)
        except ValueError:
            continue
        if f[3] not in ("FUNC", "OBJECT") or size <= 0:
            continue
        syms.append((value, size, f[7]))
    return syms


class RomMap:
    """Translates ROM offsets to section + symbol names."""

    def __init__(self, elf_path):
        self.sections = load_sections(elf_path)
        # vaddr -> lma delta per section, so symbol vaddrs can be mapped too.
        self._vmap = []
        out = subprocess.run([tool("readelf", READELF), "-SW", elf_path],
                             check=True, capture_output=True, text=True).stdout
        vaddrs = {}
        for line in out.splitlines():
            if line.lstrip().startswith("[") and "PROGBITS" in line:
                body = line.split("]", 1)[1].split()
                try:
                    vaddrs[body[0]] = int(body[2], 16)
                except (IndexError, ValueError):
                    pass
        for name, lma, size in self.sections:
            v = vaddrs.get(name)
            if v is not None:
                self._vmap.append((v, v + size, lma - v))

        self._starts = [s[1] for s in self.sections]
        self.symbols = []
        for value, size, name in load_symbols(elf_path):
            for lo, hi, delta in self._vmap:
                if lo <= value < hi:
                    self.symbols.append((value + delta, size, name))
                    break
        self.symbols.sort()
        self._symstarts = [s[0] for s in self.symbols]

    def section_of(self, off):
        i = bisect_right(self._starts, off) - 1
        if i < 0:
            return "?"
        name, lma, size = self.sections[i]
        return name if off < lma + size else f"(gap after {name})"

    def symbol_of(self, off):
        i = bisect_right(self._symstarts, off) - 1
        if i < 0:
            return None
        start, size, name = self.symbols[i]
        if off < start + size:
            return f"{name}+0x{off - start:X}"
        return None


def diff_ranges(a, b, limit=None):
    """Yield (start, end) half-open runs where a and b differ."""
    n = min(len(a), len(b))
    i = 0
    found = 0
    while i < n:
        if a[i] == b[i]:
            i += 1
            continue
        start = i
        while i < n and a[i] != b[i]:
            i += 1
        yield (start, i)
        found += 1
        if limit is not None and found >= limit:
            return


def main():
    ap = argparse.ArgumentParser(
        description=__doc__,
        formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--elf", default=BUILT_ELF, help="built ELF to check")
    ap.add_argument("--rom", default=ORIG_ROM, help="reference ROM image")
    ap.add_argument("-n", "--ranges", type=int, default=20,
                    help="how many mismatch ranges to print (0 = none)")
    ap.add_argument("--per-section", action="store_true",
                    help="also print a per-section match breakdown")
    ap.add_argument("-q", "--quiet", action="store_true",
                    help="print only the one-line summary")
    ap.add_argument("--save", metavar="PATH",
                    help="keep the flattened image at PATH")
    ap.add_argument("--strict", action="store_true",
                    help="exit non-zero unless the images are identical")
    ap.add_argument("--make-rom", action="store_true",
                    help="(re)generate the reference ROM from elf/SLUS_204.69 "
                         "and exit")
    args = ap.parse_args()

    if args.make_rom:
        make_rom(args.rom)
        return 0

    if not os.path.exists(args.elf):
        die(f"{args.elf} not found -- run `ninja` first")
    if not os.path.exists(args.rom):
        die(f"{args.rom} not found -- regenerate it with:\n"
            f"  python3 tools/verify_elf.py --make-rom")

    with open(args.rom, "rb") as f:
        want = f.read()

    tmp = args.save or os.path.join(tempfile.gettempdir(),
                                    "verify_elf_built.bin")
    got = flatten(args.elf, tmp)
    if not args.save:
        try:
            os.unlink(tmp)
        except OSError:
            pass

    n = min(len(got), len(want))
    same = sum(1 for i in range(n) if got[i] == want[i])
    pct = 100.0 * same / len(want) if want else 0.0

    if args.quiet:
        print(f"{same}/{len(want)} bytes match ({pct:.4f}%), "
              f"built image {len(got)} bytes")
        return 1 if (args.strict and got != want) else 0

    print(f"reference : {args.rom}  {len(want)} bytes")
    print(f"built     : {args.elf} -> raw image {len(got)} bytes")
    if len(got) != len(want):
        print(f"SIZE DIFF : built image is {len(got) - len(want):+d} bytes")
    print(f"match     : {same}/{len(want)} bytes  ({pct:.4f}%)")
    print(f"mismatch  : {n - same} bytes in the overlapping region")

    rmap = RomMap(args.elf)

    if args.per_section:
        print("\nper-section:")
        for name, lma, size in rmap.sections:
            hi = min(lma + size, n)
            if hi <= lma:
                print(f"  {name:<14} 0x{lma:08X} +0x{size:06X}  "
                      f"(beyond reference image)")
                continue
            ok = sum(1 for i in range(lma, hi) if got[i] == want[i])
            span = hi - lma
            print(f"  {name:<14} 0x{lma:08X} +0x{size:06X}  "
                  f"{ok}/{span} ({100.0 * ok / span:.4f}%)")

    if args.ranges:
        print(f"\nfirst {args.ranges} mismatch ranges:")
        shown = 0
        for start, end in diff_ranges(got, want, args.ranges):
            sym = rmap.symbol_of(start) or rmap.section_of(start)
            print(f"  0x{start:08X}-0x{end:08X}  ({end - start:6d} bytes)  {sym}")
            shown += 1
        if shown == 0:
            print("  none -- the overlapping region is byte-identical")
        else:
            total = sum(1 for _ in diff_ranges(got, want))
            if total > shown:
                print(f"  ... {total - shown} more ranges")

    return 1 if (args.strict and got != want) else 0


if __name__ == "__main__":
    sys.exit(main())
