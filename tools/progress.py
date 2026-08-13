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


def main():
    entries, hardware_entries = parse_decompiled()
    repo = Repo()

    matched_bytes = matched_count = 0
    unmatched_bytes = unmatched_count = 0
    hardware_bytes = sum(e.size for e in hardware_entries)
    hardware_count = len(hardware_entries)
    per_file = {}

    for e in entries:
        ok = repo.compare(e.name, e.addr, e.size)["status"] == "OK"
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


if __name__ == "__main__":
    main()
