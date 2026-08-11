#!/usr/bin/env python3
"""Show the masked word-level diff for one decompiled function.

Usage: python3 tools/diff_fn.py FUNCTION_NAME

Uses the same masking rules as verify.py (jal/j targets, gp_rel/hi/lo
immediates) and prints original vs built mnemonics around each diff.
"""
import subprocess, re, sys, glob

ELF = "elf/SLUS_204.69"
BUILT = "build/src"
OBJDUMP = "/usr/local/ps2dev/ee/bin/mips64r5900el-ps2-elf-objdump"


def get_lines_words_relocs(cmd):
    result = subprocess.run(cmd, capture_output=True, text=True)
    words, lines, relocs = [], [], {}
    for line in result.stdout.split('\n'):
        m = re.match(r'\s+[0-9a-f]+:\s+([0-9a-f]{8})\b', line)
        if m:
            words.append(m.group(1))
            lines.append(line.strip())
            continue
        r = re.match(r'\s+[0-9a-f]+:\s+R_MIPS_(\w+)', line)
        if r and words:
            relocs[len(words) - 1] = r.group(1)
    return words, lines, relocs


def mask_word(w):
    val = int(w, 16)
    if (val >> 26) == 0x03:
        return f"{val & 0xFC000000:08x}"
    if (val & 0xFC00003F) == 0x0000000D:
        return "BREAK"
    return w


def main():
    name = sys.argv[1]
    entry = None
    for line in open("config/decompiled.txt"):
        m = re.match(rf'{re.escape(name)}\s*=\s*(0x[0-9A-Fa-f]+),\s*(0x[0-9A-Fa-f]+);', line.strip())
        if m:
            entry = (int(m.group(1), 16), int(m.group(2), 16))
            break
    if not entry:
        sys.exit(f"{name} not found in config/decompiled.txt")
    addr, size = entry

    obj = None
    func_addr = None
    for o in sorted(glob.glob(f"{BUILT}/*.o")):
        nm = subprocess.run([OBJDUMP, "-t", o], capture_output=True, text=True)
        for nm_line in nm.stdout.split('\n'):
            if nm_line.endswith(f" {name}") and " F .text" in nm_line:
                obj = o
                func_addr = int(nm_line.split()[0], 16)
                break
        if obj:
            break
    if not obj:
        sys.exit(f"no defined {name} in {BUILT}/*.o")

    orig, orig_lines, _ = get_lines_words_relocs(
        [OBJDUMP, "-d", "-j", ".text", f"--start-address={hex(addr)}",
         f"--stop-address={hex(addr + size)}", ELF])
    built, built_lines, relocs = get_lines_words_relocs(
        [OBJDUMP, "-d", "-r", f"--start-address={hex(func_addr)}",
         f"--stop-address={hex(func_addr + size)}", obj])

    om = [mask_word(w) for w in orig]
    bm = [mask_word(w) for w in built]
    for i, rtype in relocs.items():
        if i >= len(om) or i >= len(bm):
            continue
        if rtype in ("GPREL16", "HI16", "LO16", "16", "LITERAL"):
            om[i] = om[i][:4] + "0000"
            bm[i] = bm[i][:4] + "0000"
        elif rtype == "26":
            om[i] = f"{int(om[i], 16) & 0xFC000000:08x}"
            bm[i] = f"{int(bm[i], 16) & 0xFC000000:08x}"

    if len(om) != len(bm):
        print(f"length mismatch: orig {len(om)} words, built {len(bm)} words")
    diffs = [i for i in range(min(len(om), len(bm))) if om[i] != bm[i]]
    if not diffs:
        print(f"{name}: MATCH ({len(om)} words)")
        return
    print(f"{name}: {len(diffs)} differing word(s) of {len(om)}")
    for i in diffs:
        print(f"  [{i:3}] orig:  {orig_lines[i]}")
        print(f"        built: {built_lines[i]}")


if __name__ == "__main__":
    main()
