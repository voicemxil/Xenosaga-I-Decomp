#!/usr/bin/env python3
"""Verify decompiled C functions match the original ELF."""
import subprocess, sys, re, glob

ELF = "elf/SLUS_204.69"
BUILT = "build/src"
OBJDUMP = "/usr/local/ps2dev/ee/bin/mips64r5900el-ps2-elf-objdump"
CC_OBJDUMP = "/usr/local/ps2dev/ee/bin/mips64r5900el-ps2-elf-objdump"


def get_words(cmd):
    """Extract instruction words from objdump output."""
    words, _ = get_words_relocs(cmd)
    return words


def get_words_relocs(cmd):
    """Extract instruction words and relocation indices from objdump output.

    Returns (words, relocs) where relocs maps instruction index ->
    relocation type (e.g. "GPREL16") for instructions that carry a
    relocation in the object file. Requires -r in cmd to see relocs.
    """
    result = subprocess.run(cmd, capture_output=True, text=True)
    words = []
    relocs = {}
    for line in result.stdout.split('\n'):
        # Match lines like: "  20b1d8:    8c820000    lw v0,0(a0)"
        m = re.match(r'\s+[0-9a-f]+:\s+([0-9a-f]{8})\b', line)
        if m:
            words.append(m.group(1))
            continue
        # Relocation lines interleaved by objdump -dr:
        # "      14: R_MIPS_GPREL16    s_nScriptTalkLock"
        r = re.match(r'\s+[0-9a-f]+:\s+R_MIPS_(\w+)', line)
        if r and words:
            relocs[len(words) - 1] = r.group(1)
    return words, relocs


# Read decompiled.txt
entries = []
hardware = []
with open("config/decompiled.txt") as f:
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
                hardware.append(name)
            else:
                entries.append((name, addr, size))

passed = 0
failed = 0

# Scan every object's symbol table once up front: name -> (obj, offset).
# Only defined function symbols count — other objects may reference the
# same name as *UND* (callers) at address 0.
symbol_map = {}
for o in sorted(glob.glob(f"{BUILT}/*.o")):
    nm = subprocess.run(
        [CC_OBJDUMP, "-t", o], capture_output=True, text=True
    )
    for nm_line in nm.stdout.split('\n'):
        if " F .text" in nm_line:
            parts = nm_line.split()
            if parts:
                symbol_map.setdefault(parts[-1], (o, int(parts[0], 16)))

for name, addr, size in entries:
    obj, sym_offset = symbol_map.get(name, (None, None))

    if not obj:
        print(f"  SKIP {name} - no object file found")
        continue

    end = addr + size

    orig = get_words([OBJDUMP, "-d", "-j", ".text",
                      f"--start-address=0x{addr:x}",
                      f"--stop-address=0x{end:x}", ELF])

    func_addr = sym_offset

    func_end = func_addr + size
    built, built_relocs = get_words_relocs([CC_OBJDUMP, "-d", "-r",
                                            f"--start-address=0x{func_addr:x}",
                                            f"--stop-address=0x{func_end:x}", obj])

    if not orig:
        print(f"  SKIP {name} - no original bytes found")
        continue

    def mask_word(w):
        """Mask jal targets and break encodings for comparison."""
        val = int(w, 16)
        # jal: opcode 0x0C (top 6 bits = 000011), mask lower 26 bits
        if (val >> 26) == 0x03:
            return f"{val & 0xFC000000:08x}"
        # break: both 0x0007000d and 0x000001cd are break instructions
        # SPECIAL opcode (0) with funct=0x0d (break)
        if (val & 0xFC00003F) == 0x0000000D:
            return "BREAK"
        return w

    orig_masked = [mask_word(w) for w in orig]
    built_masked = [mask_word(w) for w in built]

    # Mask 16-bit immediates resolved at link time (gp-relative and
    # hi/lo address pairs) — unresolved in the .o, like jal targets.
    for i, rtype in built_relocs.items():
        if i >= len(orig_masked) or i >= len(built_masked):
            continue
        if rtype in ("GPREL16", "HI16", "LO16", "16", "LITERAL"):
            orig_masked[i] = orig_masked[i][:4] + "0000"
            built_masked[i] = built_masked[i][:4] + "0000"
        elif rtype == "26":
            # j/jal targets: mask the 26-bit field (jal is also handled
            # opcode-wise by mask_word, but plain j tail calls are not)
            orig_masked[i] = f"{int(orig_masked[i], 16) & 0xFC000000:08x}"
            built_masked[i] = f"{int(built_masked[i], 16) & 0xFC000000:08x}"

    if orig_masked == built_masked:
        print(f"  OK   {name}")
        passed += 1
    else:
        # Find first difference for display
        print(f"  FAIL {name}")
        print(f"    orig:  {' '.join(orig[:8])}")
        print(f"    built: {' '.join(built[:8])}")
        failed += 1

for name in hardware:
    print(f"  HW   {name}")

print(f"\n{passed} passed, {failed} failed, {len(hardware)} hardware, {len(entries) + len(hardware)} total")
