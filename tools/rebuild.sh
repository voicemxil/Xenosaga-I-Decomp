#!/bin/sh
# Force-fresh build + verify.
#
# Why this exists: the repo is edited from the host and built inside the
# dev container, and both checkfile.py --built and verify.py read the
# objects in build/ rather than compiling. If ninja has not run since
# your last edit -- or skipped a file because its mtime did not advance
# past the object's (host/container clock skew, virtiofs mtime lag) --
# they report the PREVIOUS build's results, which looks like "my change
# did nothing". Touching the sources first makes the skip impossible.
#
# Usage: tools/rebuild.sh [verify args...]
set -e
find src -name '*.c' -exec touch {} +
# Serialized: concurrent ninja runs corrupt the shared intermediate
# .o.s files. See tools/build.sh.
sh tools/build.sh

# verify.py alone cannot catch a whole class of breakage: it checks each
# function against the object that DEFINES it, but the linker picks which
# definition actually ships. Duplicate definitions under
# --allow-multiple-definition once meant the image carried mismatching
# copies of _dtoa_r, quorem and six more while verify.py checked the
# byte-exact ones next door and reported OK. Comparing the linked image
# to retail is the only check that sees what actually shipped.
# Non-fatal: with several agents editing at once the link may be
# transiently broken, and that must not mask the per-function results.
# Functions defined by two source objects: the linker picks one and
# verify.py may be checking the other. This has produced both a wrong
# shipped image and three phantom "registered-and-failing" reports.
python3 tools/audit_dupes.py --quiet 2>/dev/null || true

if ! python3 tools/verify_elf.py --strict -q 2>/dev/null; then
    echo "WARNING: linked image does NOT match retail byte-for-byte."
    echo "  Run: python3 tools/verify_elf.py --strict   (see docs/LINKING.md)"
    echo "  If the link itself failed, this is probably another agent mid-edit."
fi

exec python3 tools/verify.py "$@"
