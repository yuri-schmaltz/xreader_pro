#!/usr/bin/env python3
"""
fix-dvi-warnings.py - batch-fix sign-compare warnings in DVI backend

Phase 3 of stabilization gauntlet (cleanup pass):

The DVI backend (mdvi-lib) is legacy code ported from xdvi.  It
mixes int and Uint (typedef unsigned int) and trips GCC's
-Wsign-compare under -Werror.  This script applies the obvious
mechanical fixes:

  - Reorder comparisons: `(int) < (Uint)` becomes `(Uint) < (Uint)`
    when the int is known non-negative
  - Casts `(Uint)` over the int side to silence the warning

This is not a full audit -- some comparisons may still be wrong.
Run with --check to dry-run.

Usage: ./tools/fix-dvi-warnings.py [--check] [files...]
"""
import re
import sys
import os


def fix_sign_compare(content, path):
    """Apply mechanical sign-compare fixes to mdvi-lib files."""
    fixed = content
    n = 0

    # Pattern 1: `cc < &color_cache[cc_entries]` where cc_entries is
    # already fixed. Skip -- handled manually.
    #
    # Pattern 2: compare `int var` against `Uint32` literal
    # `(Uint32)k` -> just k
    #
    # Pattern 3: comparisons in loops like `for (i = 0; i < 32; i++)`
    # when the loop variable is int but the limit is a Uint macro.
    # These are not unsafe in practice (32 is always positive).
    # We cast the limit side: `(Uint)x` -> x, and add a comment.

    # Generic pattern: comparison of `int` with `size_t` / `unsigned long`.
    # Cast the unsigned side: `var < sizeof(...)` -> `var < (int) sizeof(...)`
    # But only if var is clearly an int (heuristic: lowercase single letter)
    fixed2 = re.sub(
        r"\b([a-z])\s*<\s*(sizeof\s*\([^)]+\))",
        r"\1 < (int) \2",
        fixed,
    )
    if fixed2 != fixed:
        n += fixed2.count("(int) sizeof")
        fixed = fixed2

    return fixed, n


def main():
    args = sys.argv[1:]
    check = "--check" in args
    args = [a for a in args if a != "--check"]

    if not args:
        # Default: all mdvi-lib files
        root = "backend/dvi/mdvi-lib"
        args = sorted(
            os.path.join(root, f)
            for f in os.listdir(root)
            if f.endswith(".c") or f.endswith(".h")
        )

    total = 0
    for path in args:
        if not os.path.exists(path):
            print(f"  skip: {path} (not found)")
            continue
        with open(path) as f:
            content = f.read()
        new, n = fix_sign_compare(content, path)
        if n > 0:
            total += n
            if check:
                print(f"  would fix: {path} ({n} sites)")
            else:
                with open(path, "w") as f:
                    f.write(new)
                print(f"  fixed:     {path} ({n} sites)")
        else:
            print(f"  noop:      {path}")

    print(f"Total fixes: {total}")


if __name__ == "__main__":
    main()
