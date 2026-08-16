#!/bin/sh
# ninja, serialized against every other agent's ninja.
#
# Ninja is not designed for two concurrent invocations against the same
# build directory, and this pipeline makes that actively destructive:
# fix_cc_asm.py writes each object's intermediate assembly to
# `<out>.o.s` IN PLACE, so two runs building the same target interleave
# their writes and leave a corrupted .s behind. It then fails as
# `junk at end of line` or `.size expression does not evaluate to a
# constant` -- in a file you never touched, which reads exactly like
# someone else broke the tree.
#
# With eleven agents sharing one build/, this is not hypothetical: it
# has been hit on newlib_mallocr.o.s, fpbit_df.o.s and gcc_frame.o.s.
#
# Use this instead of bare `ninja`. It takes an exclusive lock, so a
# second agent waits rather than corrupting the first one's output.
#
# Note tools/checkfile.py never needs this -- it compiles into its own
# temp directory and is immune. When ninja and checkfile disagree,
# checkfile is the authority.
set -e
mkdir -p build
exec flock build/.ninja.lock ninja "$@"
