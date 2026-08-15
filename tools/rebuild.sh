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
ninja
exec python3 tools/verify.py "$@"
