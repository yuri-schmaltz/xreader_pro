#!/usr/bin/env bash
#
# tools/check-source.sh - quick source-quality checks for xreader
#
# Runs the following checks and exits non-zero on the first failure:
#
#   1. No trailing whitespace in any tracked *.c / *.h / *.build file.
#   2. No tab characters in files outside *.c, *.h, and the
#      Meson / Automake / Makefile flavors (the project uses
#      tabs for those by convention).
#   3. No CRLF line endings.
#   4. No BOMS at the start of any text file.
#   5. clang-format --dry-run --Werror, if clang-format is
#      installed and the .clang-format file is present.
#   6. Shellcheck on this script, if shellcheck is installed.
#
# Run from the repository root:  ./tools/check-source.sh

set -euo pipefail

repo_root="$(cd "$(dirname "$0")/.." && pwd)"
cd "$repo_root"

red()   { printf '\033[31m%s\033[0m\n' "$*"; }
green() { printf '\033[32m%s\033[0m\n' "$*"; }
bold()  { printf '\033[1m%s\033[0m\n' "$*"; }

fail_count=0

step() {
	printf '\n'
	green "==> $*"
}

err() {
	red   "    $*" >&2
	fail_count=$((fail_count + 1))
}

# ---------------------------------------------------------------------------
# 1. No trailing whitespace in source files.
# ---------------------------------------------------------------------------
step "1. No trailing whitespace in C / H / build files"
if git -C "$repo_root" ls-files '*.c' '*.h' '*.build' 'meson.build' \
	| grep -v '/.worktrees/' \
	| grep -v 'help/reference/' \
	| grep -v 'po/' \
	| grep -v 'ChangeLog' \
	| grep -v 'AUTHORS' \
	| grep -v 'cut-n-paste/' \
	| xargs -r grep -En ' +$' \
	| head -20 ; then
	err "trailing whitespace found (see above)"
else
	green "    ok"
fi

# ---------------------------------------------------------------------------
# 2. No tabs outside the tab-allowed file types.
# ---------------------------------------------------------------------------
step "2. No tabs in non-source files (md, json, xml, ui, ...)"
if git -C "$repo_root" ls-files '*.md' '*.json' '*.xml' '*.ui' '*.yml' '*.yaml' \
	| grep -v '/.worktrees/' \
	| grep -v 'help/' \
	| xargs -r grep -Eln $'\t' \
	| head -20 ; then
	err "tabs found in non-source files (see above)"
else
	green "    ok"
fi

# ---------------------------------------------------------------------------
# 3. No CRLF line endings.
# ---------------------------------------------------------------------------
step "3. No CRLF line endings in C / H / build files"
if git -C "$repo_root" ls-files '*.c' '*.h' '*.build' 'meson.build' '*.md' '*.json' '*.xml' \
	| grep -v '/.worktrees/' \
	| xargs -r grep -El $'\r' \
	| head -20 ; then
	err "CRLF line endings found (see above)"
else
	green "    ok"
fi

# ---------------------------------------------------------------------------
# 4. No BOMs.
# ---------------------------------------------------------------------------
step "4. No UTF-8 BOMs in tracked text files"
if git -C "$repo_root" ls-files -z \
	| xargs -0 -I{} sh -c 'head -c 3 "{}" | grep -lP "\\xEF\\xBB\\xBF" "{}" 2>/dev/null' \
	2>/dev/null | head -20 ; then
	err "UTF-8 BOM found (see above)"
else
	green "    ok"
fi

# ---------------------------------------------------------------------------
# 5. clang-format --dry-run, if available.
# ---------------------------------------------------------------------------
step "5. clang-format --dry-run (if installed)"
if command -v clang-format >/dev/null 2>&1 && [ -f .clang-format ]; then
	if git -C "$repo_root" ls-files -z -- '*.c' '*.h' \
		| grep -zv 'cut-n-paste/smclient/' \
		| xargs -0 clang-format --dry-run --Werror --ferror-limit=0 2>&1 \
		| head -40 ; then
		err "clang-format reported differences (see above)"
	else
		green "    ok"
	fi
else
	green "    skipped (clang-format not installed)"
fi

# ---------------------------------------------------------------------------
# 6. shellcheck, if available.
# ---------------------------------------------------------------------------
step "6. shellcheck on tools/ (if installed)"
if command -v shellcheck >/dev/null 2>&1; then
	if shellcheck -x tools/*.sh 2>&1 | head -20 ; then
		err "shellcheck reported warnings (see above)"
	else
		green "    ok"
	fi
else
	green "    skipped (shellcheck not installed)"
fi

# ---------------------------------------------------------------------------
# Summary
# ---------------------------------------------------------------------------
echo
if [ "$fail_count" -eq 0 ]; then
	green "All checks passed."
	exit 0
else
	red "$fail_count check(s) failed."
	exit 1
fi
