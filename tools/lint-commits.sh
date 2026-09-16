#!/usr/bin/env bash
#
# tools/lint-commits.sh - enforce the xreader commit-message style
#
# Scans the commits in the range given on the command line
# (default: origin/master..HEAD, i.e. everything the current
# branch has on top of master) and exits non-zero on the first
# violation of the house style:
#
#   1. The first line of the message (the "subject") is at most
#      72 characters (the canonical git convention).
#   2. The subject starts with a "<component>: " prefix, where
#      <component> is one of the well-known sub-directories of
#      the repository (libview, libdocument, shell, backend,
#      thumbnailer, previewer, tests, fuzz, tools, data,
#      cut-n-paste, debian, meson, build, ci, ...) or a small
#      allowlist of top-level names (NEWS, README, HACKING,
#      RELEASE, INSTALL, CONTRIBUTING, ...) or a "chore:" /
#      "fix:" / "feat:" / "refactor:" / "docs:" / "test:" /
#      "ci:" conventional-commits prefix.
#   3. Lines in the body of the message are wrapped at 80
#      columns or less.
#   4. A blank line separates the subject from the body.
#   5. The body is written in the imperative mood ("Fix the
#      bug", not "Fixed the bug" / "This fixes the bug").
#
# Run from the repository root:
#
#   ./tools/lint-commits.sh                    # check HEAD vs origin/master
#   ./tools/lint-commits.sh HEAD~5..HEAD       # check the last 5 commits
#   ./tools/lint-commits.sh --staged           # check the staged-but-not-committed
#                                               # message (uses git commit --verbose)
#
# Exit codes:
#   0  -- every commit message in the range is clean.
#   1  -- at least one commit message has a style violation.
#   2  -- usage error (e.g. bad revision range).

set -euo pipefail

repo_root="$(cd "$(dirname "$0")/.." && pwd)"
cd "$repo_root"

red()   { printf '\033[31m%s\033[0m\n' "$*"; }
green() { printf '\033[32m%s\033[0m\n' "$*"; }
bold()  { printf '\033[1m%s\033[0m\n'  "$*"; }

err_count=0

# ---------------------------------------------------------------------------
# Configuration: known components, conventional prefixes, top-level files.
# ---------------------------------------------------------------------------

# Sub-directories that may appear as a <component>: prefix.
known_subdirs=(
    libview libdocument shell backend thumbnailer previewer
    tests fuzz tools data cut-n-paste debian po help
    dvi
)

# Top-level files / docs that may appear as a <component>: prefix.
known_top_files=(
    NEWS README HACKING RELEASE INSTALL CONTRIBUTING
    AUTHORS COPYING
    meson.build meson_options.txt configure
    .editorconfig .gitignore .gitattributes .mailmap
    .clang-format SECURITY.md
)

# Conventional-commits prefixes accepted as alternatives to the
# <component>: prefix.
conventional_prefixes=(
    chore: fix: feat: refactor: docs: test: ci: build: perf:
)

# Allowlist of historical commits that pre-date the style and should
# not be flagged by the linter.  Add a SHA here when backporting a
# third-party change whose message you cannot rewrite.
allowlist=(
)

is_in_allowlist() {
    local sha="$1"
    for allowed in "${allowlist[@]}"; do
        if [ "$allowed" = "$sha" ]; then
            return 0
        fi
    done
    return 1
}

# ---------------------------------------------------------------------------
# Argument parsing.
# ---------------------------------------------------------------------------

range=""
mode="range"

while [ $# -gt 0 ]; do
    case "$1" in
        --staged)
            mode="staged"
            shift
            ;;
        -h|--help)
            sed -n '2,/^set -euo/p' "$0" | sed 's/^# \{0,1\}//'
            exit 0
            ;;
        *)
            range="$1"
            shift
            ;;
    esac
done

case "$mode" in
    staged)
        # `git log -1 --pretty=%B` of the staged message is what
        # `git commit --verbose` would write, but the staged msg
        # is only in MERGE_MSG / COMMIT_EDITMSG.  Easiest: read
        # the file directly.
        if [ ! -s .git/COMMIT_EDITMSG ]; then
            red "No staged commit message (is .git/COMMIT_EDITMSG non-empty?)"
            exit 2
        fi
        subject=$(head -1 .git/COMMIT_EDITMSG)
        body=$(tail -n +2 .git/COMMIT_EDITMSG)
        shas=("staged")
        ;;
    range)
        if [ -z "$range" ]; then
            # Default: the current branch's commits on top of origin/master.
            if ! git rev-parse --verify origin/master >/dev/null 2>&1; then
                red "origin/master not found; pass an explicit range on the command line"
                exit 2
            fi
            range="origin/master..HEAD"
        fi
        mapfile -t shas < <(git rev-list --reverse "$range")
        if [ "${#shas[@]}" -eq 0 ]; then
            green "No commits in range '$range' -- nothing to lint."
            exit 0
        fi
        ;;
esac

bold "Linting ${#shas[@]} commit(s) in range '$range'"

# ---------------------------------------------------------------------------
# Per-commit checks.
# ---------------------------------------------------------------------------

check_one() {
    local sha="$1"
    local full_msg subject rest body
    if [ "$sha" = "staged" ]; then
        full_msg=$(cat .git/COMMIT_EDITMSG)
    else
        full_msg=$(git log -1 --pretty=%B "$sha")
    fi

    subject=$(printf '%s\n' "$full_msg" | head -1)
    rest=$(printf '%s\n' "$full_msg" | tail -n +2)

    if [ -n "$rest" ] && [ "$(printf '%s' "$rest" | head -1 | tr -d '[:space:]')" != "" ]; then
        err "  $sha: missing blank line between subject and body"
    fi

    # 1. Subject length.
    if [ "${#subject}" -gt 72 ]; then
        err "  $sha: subject is ${#subject} chars (max 72): $subject"
    fi

    # 2. Subject prefix.
    local prefix="" rest_of_subject="$subject"
    if [[ "$subject" == *": "* ]]; then
        prefix="${subject%%:*}:"
        rest_of_subject="${subject#*: }"
    fi

    local prefix_ok=0
    if [ -n "$prefix" ]; then
        for known in "${conventional_prefixes[@]}"; do
            if [ "$prefix" = "$known" ]; then
                prefix_ok=1
                break
            fi
        done
        if [ "$prefix_ok" -eq 0 ]; then
            for known in "${known_subdirs[@]}"; do
                if [ "$prefix" = "$known:" ]; then
                    prefix_ok=1
                    break
                fi
            done
        fi
        if [ "$prefix_ok" -eq 0 ]; then
            for known in "${known_top_files[@]}"; do
                if [ "$prefix" = "$known:" ]; then
                    prefix_ok=1
                    break
                fi
            done
        fi
        if [ "$prefix_ok" -eq 0 ]; then
            err "  $sha: subject prefix '$prefix' is not a known component or conventional prefix"
        fi
    else
        err "  $sha: subject has no '<component>: ' or 'conventional:' prefix: $subject"
    fi

    # 3. Body wrap at 80 columns.
    if [ -n "$rest" ]; then
        while IFS= read -r line; do
            # Skip blank lines and list bullets (they can be long
            # because of an indented sub-clause).
            if [ -z "$line" ] || [[ "$line" =~ ^[[:space:]]*[\*\-\`] ]] || [[ "$line" =~ ^[[:space:]]*$ ]]; then
                continue
            fi
            if [ "${#line}" -gt 80 ]; then
                err "  $sha: body line is ${#line} chars (max 80): $line"
            fi
        done <<< "$rest"
    fi

    # 4. Imperative mood in the first word of the subject.
    # Heuristic: skip if the subject starts with "Revert", "Merge",
    # "Initial", "Update" (which is a valid imperative in this project).
    local first_word="${rest_of_subject%% *}"
    case "$first_word" in
        Revert|Merge|Initial|""|Update|Release|Drop|Use|Switch|Add|Replace|\
        Remove|Rename|Build|Fix|Move|Allow|Test|Refactor|Set|Make|Print|Clear|\
        Run|Read|Write|Open|Close|Take|Copy|Configure|Install|Reinstate|\
        Forward-port|Cover|Follow|Format|Hook|Set|Update|Tighten|Tune|Bump|\
        Land|Accept|Backport|Squash|Source|Mention|Refresh|Mention|Whitelist|\
        Blacklist|Cleanup|Clean-up|Pin|Promote|Reject|Reword|Resurrect|\
        Squash|Sync|Tag|Tighten|Unbreak|Unify|Use)
            : # OK
            ;;
        *)
            # The "ed" / "ing" check.
            if [[ "$first_word" == *ed && "${#first_word}" -gt 4 ]] || \
               [[ "$first_word" == *ing && "${#first_word}" -gt 5 ]]; then
                err "  $sha: subject first word '$first_word' may not be in imperative mood: $subject"
            fi
            ;;
    esac

    # 5. No trailing whitespace in the subject or any body line.
    if [[ "$subject" =~ [[:space:]]$ ]]; then
        err "  $sha: subject has trailing whitespace: $subject"
    fi
    if [ -n "$rest" ]; then
        while IFS= read -r line; do
            if [[ "$line" =~ [[:space:]]$ ]]; then
                err "  $sha: body line has trailing whitespace: $line"
            fi
        done <<< "$rest"
    fi

    # 6. No tab characters in the subject or body (commits use
    # spaces; tabs sneak in from copy-pasting from a terminal).
    if [[ "$subject" == *$'\t'* ]]; then
        err "  $sha: subject contains a tab character: $subject"
    fi
    if [ -n "$rest" ]; then
        while IFS= read -r line; do
            if [[ "$line" == *$'\t'* ]]; then
                err "  $sha: body line contains a tab character: $line"
            fi
        done <<< "$rest"
    fi
}

err() {
    red   "    $*" >&2
    err_count=$((err_count + 1))
}

for sha in "${shas[@]}"; do
    if is_in_allowlist "$sha"; then
        green "  (allowlisted) $sha"
        continue
    fi
    if check_one "$sha"; then
        :
    fi
done

# ---------------------------------------------------------------------------
# Summary
# ---------------------------------------------------------------------------

echo
if [ "$err_count" -eq 0 ]; then
    green "All ${#shas[@]} commit message(s) are clean."
    exit 0
else
    red "$err_count commit message(s) failed the linter."
    exit 1
fi
