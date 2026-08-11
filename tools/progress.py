#!/usr/bin/env python3
"""Calculate decompilation progress and optionally update README."""
import re
import sys
import subprocess
import glob

DECOMP = "config/decompiled.txt"
ELF = "elf/SLUS_204.69"
BUILT = "build/src"
OBJDUMP = "/usr/local/ps2dev/ee/bin/mips64r5900el-ps2-elf-objdump"
TOTAL_TEXT_SIZE = 1279344


def get_words(cmd):
    words, _ = get_words_relocs(cmd)
    return words


def get_words_relocs(cmd):
    """Extract instruction words and relocation indices (needs -r for relocs)."""
    result = subprocess.run(cmd, capture_output=True, text=True)
    words = []
    relocs = {}
    for line in result.stdout.split('\n'):
        m = re.match(r'\s+[0-9a-f]+:\s+([0-9a-f]{8})\b', line)
        if m:
            words.append(m.group(1))
            continue
        r = re.match(r'\s+[0-9a-f]+:\s+R_MIPS_(\w+)', line)
        if r and words:
            relocs[len(words) - 1] = r.group(1)
    return words, relocs


def mask_word(w):
    val = int(w, 16)
    if (val >> 26) == 0x03:
        return f"{val & 0xFC000000:08x}"
    if (val & 0xFC00003F) == 0x0000000D:
        return "BREAK"
    return w


_symbol_map = None


def get_symbol_map():
    """Scan every object's symbol table once: name -> (obj, offset)."""
    global _symbol_map
    if _symbol_map is None:
        _symbol_map = {}
        for o in sorted(glob.glob(f"{BUILT}/*.o")):
            nm = subprocess.run([OBJDUMP, "-t", o], capture_output=True, text=True)
            for nm_line in nm.stdout.split('\n'):
                if " F .text" in nm_line:
                    parts = nm_line.split()
                    if parts:
                        _symbol_map.setdefault(parts[-1], (o, int(parts[0], 16)))
    return _symbol_map


def verify_function(name, addr, size):
    """Returns True if function matches."""
    obj, func_addr = get_symbol_map().get(name, (None, None))
    if not obj:
        return False

    end = addr + size
    orig = get_words([OBJDUMP, "-d", "-j", ".text",
                      f"--start-address=0x{addr:x}",
                      f"--stop-address=0x{end:x}", ELF])

    func_end = func_addr + size
    built, built_relocs = get_words_relocs([OBJDUMP, "-d", "-r",
                                            f"--start-address=0x{func_addr:x}",
                                            f"--stop-address=0x{func_end:x}", obj])

    if not orig:
        return False

    orig_masked = [mask_word(w) for w in orig]
    built_masked = [mask_word(w) for w in built]

    # Mask link-time-resolved immediates, same as verify.py.
    for i, rtype in built_relocs.items():
        if i >= len(orig_masked) or i >= len(built_masked):
            continue
        if rtype in ("GPREL16", "HI16", "LO16", "16", "LITERAL"):
            orig_masked[i] = orig_masked[i][:4] + "0000"
            built_masked[i] = built_masked[i][:4] + "0000"
        elif rtype == "26":
            orig_masked[i] = f"{int(orig_masked[i], 16) & 0xFC000000:08x}"
            built_masked[i] = f"{int(built_masked[i], 16) & 0xFC000000:08x}"

    return orig_masked == built_masked


# Read decompiled.txt
entries = []
hardware_entries = []
with open(DECOMP) as f:
    for line in f:
        line = line.strip()
        if not line or line.startswith('#'):
            continue
        m = re.match(r'(\w+)\s*=\s*(0x[0-9A-Fa-f]+),\s*(0x[0-9A-Fa-f]+);', line)
        if m:
            name = m.group(1)
            addr = int(m.group(2), 16)
            size = int(m.group(3), 16)
            if 'HARDWARE' in line:
                hardware_entries.append((name, size))
            else:
                entries.append((name, addr, size))

# Verify each function
matched_bytes = 0
matched_count = 0
unmatched_bytes = 0
unmatched_count = 0
hardware_bytes = sum(s for _, s in hardware_entries)
hardware_count = len(hardware_entries)

for name, addr, size in entries:
    if verify_function(name, addr, size):
        matched_bytes += size
        matched_count += 1
    else:
        unmatched_bytes += size
        unmatched_count += 1

decompiled_bytes = matched_bytes + unmatched_bytes + hardware_bytes
decompiled_count = matched_count + unmatched_count + hardware_count
pct = (matched_bytes / TOTAL_TEXT_SIZE) * 100
decomp_pct = (decompiled_bytes / TOTAL_TEXT_SIZE) * 100

print(f"Decompilation progress: {decompiled_bytes}/{TOTAL_TEXT_SIZE} bytes ({decomp_pct:.3f}%)")
print(f"  Verified match: {matched_count} functions ({matched_bytes} bytes, {pct:.3f}%)")
print(f"  In progress:    {unmatched_count} functions ({unmatched_bytes} bytes)")
print(f"  Hardware:       {hardware_count} functions ({hardware_bytes} bytes)")
print(f"  Total:          {decompiled_count} functions ({decompiled_bytes} bytes)")

if "--update-readme" in sys.argv:
    table = f"""### Decompilation Progress
| Category | Functions | Bytes | % of .text |
|----------|-----------|-------|------------|
| Verified match | {matched_count} | {matched_bytes} | {pct:.3f}% |
| In progress | {unmatched_count} | {unmatched_bytes} | |
| Hardware | {hardware_count} | {hardware_bytes} | |
| **Total** | **{decompiled_count}** | **{decompiled_bytes}** | **{decomp_pct:.3f}%** |
*Auto-updated on each push. Run `python3 tools/progress.py` locally for current stats.*"""
    with open("README.md") as f:
        readme = f.read()
    updated = re.sub(
        r'### Decompilation Progress.*?\*Auto-updated.*?\*',
        table,
        readme,
        flags=re.DOTALL
    )
    if updated != readme:
        with open("README.md", "w") as f:
            f.write(updated)
        print("README.md updated.")
    else:
        print("README.md already up to date.")
