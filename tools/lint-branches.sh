#!/usr/bin/env bash
#
# tools/lint-branches.sh - enforce the xreader branch-name style
#
# Scans the local branch names and exits non-zero on the first
# violation of the house style:
#
#   1. The branch name starts with a category prefix (one of
#      "fix/", "feat/", "chore/", "refactor/", "docs/", "test/",
#      "ci/", "tools/", "build/", "libdocument/", "libview/",
#      "shell/", "backend/", "thumbnailer/", "previewer/", "data/",
#      "debian/", "fuzz/", "meson/").
#   2. The category prefix is followed by a short kebab-case
#      description (lowercase letters, digits, and hyphens only).
#   3. The branch name does not contain a slash other than the
#      category separator.
#   4. The branch name does not start with a dot, an underscore,
#      or a digit (these are reserved by git for refs that should
#      not be visible in normal listings).
#   5. The branch name is at most 80 characters (a soft limit
#      imposed to make the name fit in CI logs).
#
# Run from the repository root:
#
#   ./tools/lint-branches.sh              # check all local branches
#   ./tools/lint-branches.sh --all        # check all local + remote branches
#   ./tools/lint-branches.sh fix/foo      # check one specific branch
#
# Exit codes:
#   0  -- every branch name is clean.
#   1  -- at least one branch name has a style violation.
#   2  -- usage error.

set -euo pipefail

repo_root="$(cd "$(dirname "$0")/.." && pwd)"
cd "$repo_root"

red()   { printf '\033[31m%s\033[0m\n' "$*"; }
green() { printf '\033[32m%s\033[0m\n' "$*"; }
bold()  { printf '\033[1m%s\033[0m\n'  "$*"; }

err_count=0

# ---------------------------------------------------------------------------
# Allowed category prefixes
# ---------------------------------------------------------------------------

KNOWN_PREFIXES='^(fix|feat|chore|refactor|docs|test|ci|tools|build|libdocument|libview|shell|backend|thumbnailer|previewer|data|debian|fuzz|meson|cut-n-paste|snap)\/'

# ---------------------------------------------------------------------------
# Lint one branch name
# ---------------------------------------------------------------------------

lint_branch() {
    local branch="$1"

    # Skip git's internal refs (HEAD, FETCH_HEAD, etc.)
    if [ "$branch" = "HEAD" ] || [[ "$branch" == *"/HEAD" ]]; then
        return 0
    fi

    # Rule 5: branch length
    if [ "${#branch}" -gt 80 ]; then
        red "FAIL  $branch  (longer than 80 chars: ${#branch})"
        err_count=$((err_count + 1))
        return 1
    fi

    # Rule 4: leading dot / underscore / digit
    if [[ "$branch" =~ ^[._0-9] ]]; then
        red "FAIL  $branch  (starts with a dot, underscore, or digit)"
        err_count=$((err_count + 1))
        return 1
    fi

    # Rule 3: more than one slash
    local slash_count="${branch//[^\/]/}"
    if [ "${#slash_count}" -gt 1 ]; then
        red "FAIL  $branch  (more than one slash)"
        err_count=$((err_count + 1))
        return 1
    fi

    # Rule 1: known prefix (or special: master, a maintenance branch
    # like 4.6-maintenance, or a remote-tracking branch like origin/master)
    if [ "$branch" = "master" ] || [[ "$branch" == *-maintenance ]] || [[ "$branch" == origin/* ]]; then
        green "OK    $branch (special: master/maintenance/remote-tracking)"
        return 0
    fi
    if ! [[ "$branch" =~ $KNOWN_PREFIXES ]]; then
        red "FAIL  $branch  (does not start with a known category prefix)"
        err_count=$((err_count + 1))
        return 1
    fi

    # Rule 2: kebab-case description (after the slash, no uppercase,
    # no underscores, no other punctuation than hyphens)
    local description="${branch#*/}"
    if ! [[ "$description" =~ ^[a-z0-9.-]+$ ]]; then
        red "FAIL  $branch  (description '$description' is not kebab-case-ish)"
        err_count=$((err_count + 1))
        return 1
    fi

    green "OK    $branch"
    return 0
}

# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

if [ $# -eq 0 ]; then
    bold "==> Checking all local branches"
    while IFS= read -r branch; do
        lint_branch "${branch#refs/heads/}"
    done < <(git for-each-ref --format='%(refname)' refs/heads/)
elif [ "$1" = "--all" ]; then
    bold "==> Checking all local + remote branches"
    while IFS= read -r branch; do
        lint_branch "${branch#refs/heads/}"
    done < <(git for-each-ref --format='%(refname)' refs/heads/ refs/remotes/)
else
    bold "==> Checking $1"
    lint_branch "$1"
fi

echo
if [ "$err_count" -gt 0 ]; then
    red "==> $err_count branch name(s) failed"
    exit 1
fi

green "==> All branch names OK"
exit 0
