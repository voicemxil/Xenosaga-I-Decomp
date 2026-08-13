#!/usr/bin/env python3
"""Verify a DATA symbol's bytes, the way diff_fn.py verifies a function.

    python3 tools/diff_data.py SYMBOL              # verify a reconstruction
    python3 tools/diff_data.py SYMBOL --dump       # original bytes as .word rows
    python3 tools/diff_data.py SYMBOL --c          # ready-to-compile C array
    python3 tools/diff_data.py SYMBOL --c --bytes  # ...as unsigned char[]

verify.py only ever checked .text, which is why static tables (VIF/GS
packet payloads, lookup tables, string tables) could not be reconstructed
as C initializers -- there was no way to check the result. This closes
that gap.

Original bytes come from config/SLUS_204.69.rom (the flat image splat
splits), whose file offset is vaddr - ROM_BASE. Built bytes come from
whichever build/src/*.o defines the symbol.

--dump prints the original bytes as .word rows; --c goes one step further
and prints a compilable `unsigned int NAME[] = { ... }` (or `unsigned
char[]` with --bytes) that can be pasted straight into a src/*.c, built,
and then verified with a plain re-run of this tool.
"""
import glob
import re
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from elflib import Elf32  # noqa: E402

ROM = "config/SLUS_204.69.rom"
ROM_BASE = 0x200000
SYMS = "config/symbol_addrs.txt"
BUILT = "build/src"


def symbol_table():
    """name -> (address, size) for every dlabel in the split data.

    Sourced from the asm rather than symbol_addrs.txt: splat mints names
    for unnamed literals (TestPrim.0_00362790 and friends) that never
    appear in the ELF symbol table, and those are exactly the ones we
    need to reconstruct. Size is the gap to the next label.
    """
    entries = []
    for path in sorted(glob.glob("asm/data/cod/*.s")):
        pending = None
        for line in open(path):
            m = re.match(r'dlabel (\S+)', line)
            if m:
                pending = m.group(1)
                continue
            m = re.match(r'\s*/\* [0-9A-Fa-f]+ ([0-9A-Fa-f]{8})', line)
            if m and pending:
                entries.append((int(m.group(1), 16), pending))
                pending = None
    entries.sort()
    table = {}
    for i, (addr, name) in enumerate(entries):
        end = entries[i + 1][0] if i + 1 < len(entries) else addr + 0x40
        table[name] = (addr, end - addr)
    return table


def original_bytes(addr, size):
    with open(ROM, "rb") as f:
        f.seek(addr - ROM_BASE)
        return f.read(size)


def built_bytes(name, only_obj=None):
    """Find the object defining `name` and return (bytes, size, obj)."""
    for obj in ([only_obj] if only_obj else sorted(glob.glob(f"{BUILT}/*.o"))):
        try:
            e = Elf32(obj)
        except (ValueError, OSError):
            continue
        loc = e.object_symbols().get(name)
        if loc is None:
            continue
        section, offset, size = loc
        raw = e.section_bytes(section)
        return raw[offset:offset + size], size, obj
    return None, 0, None


def emit_c(name, orig, addr, kind):
    """Print a compilable initializer for the original bytes."""
    ident = re.sub(r'[^A-Za-z0-9_]', '_', name)
    print(f"/* {name} @ {addr:#010x}, {len(orig)} bytes -- reconstructed "
          f"from the original image */")
    if kind == "byte":
        print(f"unsigned char {ident}[{len(orig)}] = {{")
        for i in range(0, len(orig), 12):
            row = ", ".join(f"0x{b:02X}" for b in orig[i:i + 12])
            print(f"    {row},")
        print("};")
        return
    pad = (-len(orig)) % 4
    words = orig + b"\x00" * pad
    n = len(words) // 4
    print(f"unsigned int {ident}[{n}] = {{")
    for i in range(0, len(words), 16):
        row = ", ".join(
            f"0x{int.from_bytes(words[j:j + 4], 'little'):08X}"
            for j in range(i, min(i + 16, len(words)), 4))
        print(f"    /* +{i:#06x} */ {row},")
    print("};")
    if pad:
        print(f"/* note: {pad} byte(s) of zero padding added to reach a "
              f"word boundary */")


def main():
    if len(sys.argv) < 2:
        sys.exit(__doc__)
    name = sys.argv[1]
    dump = "--dump" in sys.argv
    emit = ("byte" if "--bytes" in sys.argv else "word") \
        if ("--c" in sys.argv or "--bytes" in sys.argv) else None
    only_obj = None
    if "--obj" in sys.argv:
        only_obj = sys.argv[sys.argv.index("--obj") + 1]
    # splat mints names like TestPrim.0_00362790 for file-local statics;
    # a C identifier cannot contain a dot, so allow naming the C symbol.
    built_name = name
    if "--as" in sys.argv:
        built_name = sys.argv[sys.argv.index("--as") + 1]
    elif "." in name:
        built_name = name.replace(".", "_")

    table = symbol_table()
    if name not in table:
        sys.exit(f"{name} not found among data labels in asm/data/cod/*.s")
    addr, span = table[name]

    built, size, obj = built_bytes(built_name, only_obj)
    if built is None:
        # Nothing defines it yet -- infer size from the next symbol so the
        # bytes can still be dumped as a starting point.
        size = span
        if not dump and not emit:
            print(f"{name}: no definition of {built_name!r} in "
                  f"{only_obj or BUILT + '/*.o'} "
                  f"(inferred size {size:#x}; use --dump to see the bytes)")
    orig = original_bytes(addr, size)

    if emit:
        emit_c(name, orig, addr, emit)
        return

    if dump:
        print(f"/* {name} @ {addr:#010x}, {size:#x} bytes */")
        for i in range(0, len(orig), 4):
            word = int.from_bytes(orig[i:i + 4], "little")
            print(f"    /* +{i:#06x} */ 0x{word:08X},")
        return

    if built is None:
        return
    if built == orig:
        print(f"{name}: MATCH ({size} bytes, from {obj})")
        return
    diffs = [i for i in range(min(len(orig), len(built))) if orig[i] != built[i]]
    print(f"{name}: {len(diffs)} differing byte(s) of {size} (from {obj})")
    for i in diffs[:16]:
        print(f"  +{i:#06x}  orig {orig[i]:#04x}  built {built[i]:#04x}")


if __name__ == "__main__":
    main()
