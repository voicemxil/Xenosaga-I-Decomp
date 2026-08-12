#!/usr/bin/env python3
"""Verify a DATA symbol's bytes, the way diff_fn.py verifies a function.

    python3 tools/diff_data.py SYMBOL [--dump]

verify.py only ever checked .text, which is why static tables (VIF/GS
packet payloads, lookup tables, string tables) could not be reconstructed
as C initializers -- there was no way to check the result. This closes
that gap.

Original bytes come from config/SLUS_204.69.rom (the flat image splat
splits), whose file offset is vaddr - ROM_BASE. Built bytes come from
whichever build/src/*.o defines the symbol.

--dump prints the original bytes as .word rows so an initializer can be
written from them without hand-transcribing the disassembly.
"""
import glob
import re
import subprocess
import sys

ROM = "config/SLUS_204.69.rom"
ROM_BASE = 0x200000
SYMS = "config/symbol_addrs.txt"
BUILT = "build/src"
OBJDUMP = "/usr/local/ps2dev/ee/bin/mips64r5900el-ps2-elf-objdump"
OBJCOPY = "/usr/local/ps2dev/ee/bin/mips64r5900el-ps2-elf-objcopy"


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
        out = subprocess.run([OBJDUMP, "-t", obj],
                             capture_output=True, text=True).stdout
        for line in out.split("\n"):
            # e.g. "00000000 g     O .data  00000010 TestPrim"
            if not line.endswith(" " + name):
                continue
            parts = line.split()
            if len(parts) < 5 or " O " not in line:
                continue
            offset = int(parts[0], 16)
            section = parts[3]
            size = int(parts[4], 16)
            raw = subprocess.run(
                [OBJCOPY, "-O", "binary", f"--only-section={section}",
                 obj, "/dev/stdout"], capture_output=True).stdout
            return raw[offset:offset + size], size, obj
    return None, 0, None


def main():
    if len(sys.argv) < 2:
        sys.exit(__doc__)
    name = sys.argv[1]
    dump = "--dump" in sys.argv
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
        if not dump:
            print(f"{name}: no definition of {built_name!r} in "
                  f"{only_obj or BUILT + '/*.o'} "
                  f"(inferred size {size:#x}; use --dump to see the bytes)")
    orig = original_bytes(addr, size)

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
