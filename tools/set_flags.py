#!/usr/bin/env python3
"""Set one configure.py flag entry, atomically, without touching any other.

configure.py is shared by every agent working the tree at once, and
hand-editing it has clobbered other people's entries repeatedly: an
editor that reflows or re-indents the whole dict silently drops whatever
landed between the read and the write. This tool makes that impossible.

    python3 tools/set_flags.py --cflags scePad.c "-O2 -G0"
    python3 tools/set_flags.py --fix Menu.c "--swap-adjacent MenuFoo:12"
    python3 tools/set_flags.py --asflags tsk.c "-G0"
    python3 tools/set_flags.py --show Menu.c
    python3 tools/set_flags.py --unset-fix Menu.c

Guarantees, in order:

  1. The file is locked for the whole read-modify-write, so two agents
     running this at the same moment serialize instead of racing.
  2. Only the named key's value is rewritten; the edit is a surgical
     splice of that value's source span, so comments, formatting and
     line breaks elsewhere are untouched.
  3. Afterwards the new text is re-parsed and EVERY OTHER KEY of the
     dict is compared against its old value. If anything else moved, the
     write is rolled back and the tool exits nonzero. A clobber is
     therefore a loud failure, not a silent loss.

--fix REPLACES the entry. To add a site to a pass that is already there,
read the current value with --show first and pass the full new string:
duplicate flags are rejected by fix_cc_asm.py anyway (a store-action
flag keeps only its last occurrence, which once silently dropped three
matched functions).
"""
import argparse
import ast
import fcntl
import os
import sys

CONFIGURE = os.path.join(os.path.dirname(os.path.abspath(__file__)),
                         "..", "configure.py")
DICTS = {"cflags": "FILE_CFLAGS_OVERRIDE", "fix": "FILE_FIX_FLAGS",
         "asflags": "FILE_ASFLAGS_OVERRIDE"}


def find_dict(tree, name):
    for node in ast.walk(tree):
        if isinstance(node, ast.Assign):
            for t in node.targets:
                if isinstance(t, ast.Name) and t.id == name:
                    if not isinstance(node.value, ast.Dict):
                        sys.exit("%s is not a dict literal" % name)
                    return node.value
    sys.exit("no dict named %s in configure.py" % name)


def dict_items(node):
    """{key: (value_source_span, literal_value)} for a dict literal."""
    out = {}
    for k, v in zip(node.keys, node.values):
        if not isinstance(k, ast.Constant) or not isinstance(k.value, str):
            continue
        out[k.value] = (k, v)
    return out


def span(src_lines, node):
    """(start_offset, end_offset) of a node within the whole source."""
    starts = [0]
    for line in src_lines:
        starts.append(starts[-1] + len(line) + 1)
    return (starts[node.lineno - 1] + node.col_offset,
            starts[node.end_lineno - 1] + node.end_col_offset)


def literals(text, name):
    """Every key -> literal value of dict `name`, for the clobber check."""
    node = find_dict(ast.parse(text), name)
    out = {}
    for k, v in zip(node.keys, node.values):
        if isinstance(k, ast.Constant):
            out[k.value] = ast.literal_eval(v)
    return out


def q(value):
    """Double-quoted like the rest of configure.py, unless that needs escapes."""
    if '"' not in value and "\\" not in value:
        return '"%s"' % value
    return repr(value)


def main():
    ap = argparse.ArgumentParser(description=__doc__,
                                 formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--cflags", nargs=2, metavar=("FILE.c", "FLAGS"))
    ap.add_argument("--fix", nargs=2, metavar=("FILE.c", "FLAGS"))
    ap.add_argument("--asflags", nargs=2, metavar=("FILE.c", "FLAGS"),
                    help="assembler-only flags (see configure.py asflags_for);\n`-G0' is what stops gas pooling a li.s float literal in .lit4")
    ap.add_argument("--replace", action="store_true",
                    help="allow --fix to DROP flags the entry already has")
    ap.add_argument("--unset-cflags", metavar="FILE.c")
    ap.add_argument("--unset-fix", metavar="FILE.c")
    ap.add_argument("--unset-asflags", metavar="FILE.c")
    ap.add_argument("--show", metavar="FILE.c",
                    help="print both entries for FILE.c and exit")
    args = ap.parse_args()

    path = os.path.normpath(CONFIGURE)
    with open(path, "r+") as fh:
        fcntl.flock(fh, fcntl.LOCK_EX)
        text = fh.read()

        if args.show:
            for kind, dname in DICTS.items():
                cur = literals(text, dname)
                print("%-22s %s" % (dname + ":",
                                    cur.get(args.show, "<unset>")))
            return 0

        ops = []
        if args.cflags:
            ops.append(("cflags", args.cflags[0], args.cflags[1]))
        if args.fix:
            ops.append(("fix", args.fix[0], args.fix[1]))
        if args.asflags:
            ops.append(("asflags", args.asflags[0], args.asflags[1]))
        if args.unset_cflags:
            ops.append(("cflags", args.unset_cflags, None))
        if args.unset_fix:
            ops.append(("fix", args.unset_fix, None))
        if args.unset_asflags:
            ops.append(("asflags", args.unset_asflags, None))
        if not ops:
            ap.error("nothing to do -- pass --cflags/--fix/--asflags/"
                     "--unset-*/--show")

        # --fix REPLACES, so passing a partial string silently drops the
        # flags already there -- and a dropped flag does not fail, it just
        # un-matches functions nobody is looking at. Refuse to lose one
        # unless the caller says so.
        for kind, key, value in ops:
            if kind == "fix" and value:
                cur = literals(text, DICTS["fix"]).get(key, "")
                lost = [t for t in cur.split() if t.startswith("--")
                        and t not in value.split()]
                if lost and not args.replace:
                    sys.exit(
                        "REFUSED: --fix %s would drop %d flag(s) already "
                        "set: %s\n  current: %s\n  yours  : %s\n"
                        "Pass the full string (read it with --show), or "
                        "--replace if dropping them is intended."
                        % (key, len(lost), " ".join(lost), cur, value))

        for kind, key, value in ops:
            if value is not None and not value.strip():
                # An empty --cflags SETS the empty string, so the file
                # compiles with no -O2 at all and every function in it
                # goes LENGTH-mismatch. That has bitten once already.
                ap.error("empty value for --%s %s: this would SET an empty "
                         "flag string, not clear the entry (the file would "
                         "then build with no -O2 and every function in it "
                         "would mismatch). Use --unset-%s %s to clear it."
                         % (kind, key, kind, key))

        for kind, key, value in ops:
            dname = DICTS[kind]
            before = literals(text, dname)
            node = find_dict(ast.parse(text), dname)
            items = dict_items(node)
            lines = text.split("\n")

            if key in items:
                knode, vnode = items[key]
                if value is None:                     # delete the whole entry
                    ks, _ = span(lines, knode)
                    _, ve = span(lines, vnode)
                    start = text.rfind("\n", 0, ks) + 1
                    end = ve
                    while end < len(text) and text[end] in ", ":
                        end += 1
                    if end < len(text) and text[end] == "\n":
                        end += 1
                    text = text[:start] + text[end:]
                else:                                 # replace just the value
                    vs, ve = span(lines, vnode)
                    text = text[:vs] + q(value) + text[ve:]
            else:
                if value is None:
                    print("%s: %s was not set" % (dname, key))
                    continue
                brace = text.index("{", text.index(dname))
                eol = text.index("\n", brace) + 1
                text = text[:eol] + "    %s: %s,\n" % (q(key), q(value)) + text[eol:]

            # Clobber check: nothing but `key` may have changed.
            after = literals(text, dname)
            moved = [k for k in set(before) | set(after)
                     if k != key and before.get(k) != after.get(k)]
            if moved:
                sys.exit("REFUSED: editing %s would have changed %d other "
                         "entr%s (%s). configure.py NOT written."
                         % (key, len(moved), "y" if len(moved) == 1 else "ies",
                            ", ".join(sorted(moved)[:5])))
            print("%s[%r] = %r" % (dname, key, after.get(key)))

        compile(text, "configure.py", "exec")   # never write a broken file
        fh.seek(0)
        fh.write(text)
        fh.truncate()
    return 0


if __name__ == "__main__":
    sys.exit(main())
