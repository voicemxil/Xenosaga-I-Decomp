#!/usr/bin/env python3
"""Calculate decompilation progress and optionally update README.

    python3 tools/progress.py
    python3 tools/progress.py --update-readme
    python3 tools/progress.py --by-file       # per-source-file breakdown

Verification is real (same masked comparison as verify.py), now done by
reading the ELF and objects directly instead of spawning objdump per
function -- a full run is ~1s instead of ~75s.
"""
import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from decomplib import Repo, parse_decompiled  # noqa: E402

TOTAL_TEXT_SIZE = 1279344



def portability_debt():
    """Count matching-only steering constructs and PS2 hardware asm.

    Steering constructs (PIN/LAUNDER/PASSTHRU/SCHED_NOP) emit no code
    and vanish in a non-MATCHING build, so they cost portability
    nothing -- but they are also where a wrong C body can hide behind a
    forced shape, so the count is worth watching. PS2_ASM and raw
    __asm__ blocks DO carry work and must be hand-ported.
    """
    import glob as _glob
    import re as _re
    steer = hw = raw = asmfn = 0
    steer_re = _re.compile(r'\b(PIN|LAUNDER|LAUNDER2|LAUNDER_V|PASSTHRU|PASSTHRU_V|SCHED_NOP)\s*\(')
    for path in _glob.glob("src/**/*.c", recursive=True):
        if "/libgcc/" in path:
            continue  # vendored gcc/newlib sources are upstream verbatim
        text = open(path, errors="ignore").read()
        steer += len(steer_re.findall(text))
        hw += len(_re.findall(r'\bPS2_ASM\s*\(', text))
        raw += len(_re.findall(r'__asm__\s*(?:__volatile__|volatile)?\s*\(\s*"', text))
        # Whole functions transcribed as file-scope asm (a .ent directive
        # inside an __asm__ block). These match bytes but document no
        # logic and must be rewritten wholesale by any port.
        asmfn += len(_re.findall(r'\.ent\s', text))
    return steer, hw, raw, asmfn


def main():
    entries, hardware_entries = parse_decompiled()
    repo = Repo()

    matched_bytes = matched_count = 0
    unmatched_bytes = unmatched_count = 0
    hardware_bytes = sum(e.size for e in hardware_entries)
    hardware_count = len(hardware_entries)
    per_file = {}

    for e in entries:
        ok = repo.compare(e.name, e.addr, e.size, e.source)["status"] == "OK"
        if ok:
            matched_bytes += e.size
            matched_count += 1
        else:
            unmatched_bytes += e.size
            unmatched_count += 1
        slot = per_file.setdefault(e.source or "?", [0, 0, 0, 0])
        slot[0 if ok else 2] += 1
        slot[1 if ok else 3] += e.size

    decompiled_bytes = matched_bytes + unmatched_bytes + hardware_bytes
    decompiled_count = matched_count + unmatched_count + hardware_count
    pct = (matched_bytes / TOTAL_TEXT_SIZE) * 100
    decomp_pct = (decompiled_bytes / TOTAL_TEXT_SIZE) * 100

    print(f"Decompilation progress: {decompiled_bytes}/{TOTAL_TEXT_SIZE} bytes ({decomp_pct:.3f}%)")
    print(f"  Verified match: {matched_count} functions ({matched_bytes} bytes, {pct:.3f}%)")
    print(f"  In progress:    {unmatched_count} functions ({unmatched_bytes} bytes)")
    print(f"  Hardware:       {hardware_count} functions ({hardware_bytes} bytes)")
    print(f"  Total:          {decompiled_count} functions ({decompiled_bytes} bytes)")
    _print_debt()

    if "--by-file" in sys.argv:
        print("\n  per source file (matched / in-progress):")
        for name in sorted(per_file, key=lambda k: -per_file[k][1]):
            ok_n, ok_b, bad_n, bad_b = per_file[name]
            flag = "" if not bad_n else f"   <-- {bad_n} not matching ({bad_b} bytes)"
            print(f"    {name:<24} {ok_n:>4} fn  {ok_b:>7} bytes{flag}")

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
            table, readme, flags=re.DOTALL)
        if updated != readme:
            with open("README.md", "w") as f:
                f.write(updated)
            print("README.md updated.")
        else:
            print("README.md already up to date.")


def _print_debt():
    steer, hw, raw, asmfn = portability_debt()
    print(f"  Portability:    {steer} steering constructs (vanish in a "
          f"non-MATCHING build), {hw} PS2_ASM + {raw - hw} raw asm blocks "
          f"needing hand-porting")
    if asmfn:
        print(f"  Transcribed:    {asmfn} whole functions written as "
              f"file-scope asm (bytes match, but they document no logic "
              f"and a port must rewrite each one)")


if __name__ == "__main__":
    main()
