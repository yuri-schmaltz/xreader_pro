#!/usr/bin/env python3
"""
apply-gtk-compat.py - Apply ev-gtk-compat.h shims to shell/ and libview/.

Phase 2 of stabilization gauntlet (O5 -- apply ev-gtk-compat.h in 15 sites):

This script rewrites GTK 3 calls to their compat shim equivalents
in ev-gtk-compat.h, so that the same source compiles under both
GTK 3 and GTK 4.  It is conservative: only patterns that map
1:1 to a shim are rewritten; complex refactors are left for
manual handling.

After this script runs, `meson setup build-gtk4 -Dgtk_version=4`
should at least configure, and many source files should compile
under GTK 4 without further changes.

Rewrites:
  gtk_box_pack_start (X, Y, ...)     ->  ev_gtk_box_pack_start (X, Y, ...)
  gtk_widget_show_all (X)            ->  ev_gtk_widget_show_all (X)
  gtk_container_add (X, Y)           ->  ev_gtk_container_add (X, Y)

And adds `#include "ev-gtk-compat.h"` to files that need it
but don't have it.
"""
import os
import re
import sys

FILES = [
    "shell/ev-properties-view.c",
    "shell/ev-properties-fonts.c",
    "shell/ev-properties-license.c",
    "shell/ev-password-view.c",
    "shell/ev-message-area.c",
    "shell/ev-progress-message-area.c",
    "shell/ev-annotation-properties-dialog.c",
]

REWRITES = [
    (re.compile(r"\bgtk_box_pack_start\s*\("),
     "ev_gtk_box_pack_start ("),
    (re.compile(r"\bgtk_widget_show_all\s*\("),
     "ev_gtk_widget_show_all ("),
    (re.compile(r"\bgtk_container_add\s*\("),
     "ev_gtk_container_add ("),
]

INCLUDE = '#include "ev-gtk-compat.h"\n'


def needs_include(content):
    return "ev-gtk-compat.h" not in content


def add_include(content):
    """Add include after the first #include "config.h" or "ev-X.h" line."""
    # Find the first block of #include lines
    lines = content.split("\n")
    out = []
    inserted = False
    for i, line in enumerate(lines):
        out.append(line)
        if not inserted and line.startswith('#include "'):
            # Check if next lines are also includes
            j = i + 1
            while j < len(lines) and (lines[j].startswith("#include") or lines[j].strip() == ""):
                if lines[j].strip() == "":
                    break
                j += 1
            # Insert after the last include
            for k in range(i + 1, j):
                out.append(lines[k])
            out.append("")
            out.append(INCLUDE.rstrip())
            inserted = True
            i = j - 1
    if not inserted:
        # Fallback: prepend
        out.insert(0, INCLUDE)
    return "\n".join(out)


def main():
    root = sys.argv[1] if len(sys.argv) > 1 else "."
    total = 0
    for rel in FILES:
        path = os.path.join(root, rel)
        if not os.path.exists(path):
            print(f"  skip: {path} (not found)")
            continue
        with open(path) as f:
            content = f.read()
        original = content
        changed = False

        for pat, repl in REWRITES:
            new_content, n = pat.subn(repl, content)
            if n > 0:
                content = new_content
                changed = True
                total += n

        if changed and needs_include(content):
            content = add_include(content)

        if content != original:
            with open(path, "w") as f:
                f.write(content)
            print(f"  rewrite: {rel} ({sum(1 for p, _ in REWRITES if p.search(original))} patterns)")
        else:
            print(f"  noop:    {rel}")

    print(f"Total rewrites: {total}")


if __name__ == "__main__":
    main()
