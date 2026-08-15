#!/bin/bash
# Run after every `splat split` to patch generated files.
#
# NOTE: this script no longer copies the pinned linker script over Splat's
# generated one. The build links directly against
# config/SLUS_204.69.pinned.ld (see configure.py / docs/LINKING.md), so a
# split run that skipped this script can no longer silently downgrade the
# link to a script with no symbol INCLUDEs. config/SLUS_204.69.ld is now an
# untracked Splat artifact that nothing consumes; it is kept only so drift
# can be inspected.

set -e
cd "$(dirname "$0")/.."

# Strip comments from symbol_addrs.txt (the linker cannot parse //)
python3 - <<'PY'
import re, pathlib
p = pathlib.Path("config/symbol_addrs.txt")
p.write_text(re.sub(r"//.*", "", p.read_text()))
PY

# Warn if Splat's generated script gained placements the pinned one lacks.
if [ -f config/SLUS_204.69.ld ]; then
  python3 - <<'PY'
import pathlib, re
gen = pathlib.Path("config/SLUS_204.69.ld").read_text().splitlines()
pin = pathlib.Path("config/SLUS_204.69.pinned.ld").read_text()
missing = [l.strip() for l in gen
           if re.match(r"\s*build/\S+\.o\(", l) and l.strip() not in pin]
if missing:
    print("WARNING: config/SLUS_204.69.pinned.ld is missing input sections "
          "that Splat generated. Add them by hand:")
    for m in missing:
        print("   ", m)
else:
    print("Pinned linker script covers every generated input section.")
PY
fi

echo "Post-split patches applied."
