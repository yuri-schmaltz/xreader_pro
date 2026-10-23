# Contributing to Xreader

Thanks for your interest in Xreader!  This document is the
contributor guide.  It is intentionally short -- most of the
detail lives in `HACKING.md` (build + test + CI) and
`SECURITY.md` (responsible disclosure).

## Code of conduct

Xreader follows the [Contributor Covenant 2.1](https://www.contributor-covenant.org/version/2/1/code_of_conduct/).
The full text is in `CODE_OF_CONDUCT.md`.

## How to contribute

The most useful contributions are:

  1. **Bug reports** -- if you can reproduce a crash, file
     a GitHub issue with the steps to reproduce, the xreader
     version, and the document file (if sharable).
  2. **Test cases** -- PRs that add a failing test for an
     existing bug are extremely welcome, even without the
     fix.  The test acts as a regression guard.
  3. **Documentation** -- the docs in `HACKING.md` and
     `SECURITY.md` are not exhaustive; PRs that close
     documentation gaps are welcome.
  4. **Bug fixes** -- if you've fixed a bug, the PR description
     should include a "Test plan" section describing how the
     reviewer can verify the fix manually.
  5. **Security fixes** -- see `SECURITY.md`.  Do NOT open a
     public issue for a security report.

## Workflow

  1. Fork [yuri-schmaltz/xreader](https://github.com/yuri-schmaltz/xreader)
     and create a topic branch off `master`:
     `git checkout -b fix/something`.
  2. Make the change.  Run `meson test -C builddir` to make
     sure the test suite is still green.
  3. Add an entry to `NEWS` if the change is user-visible.
  4. Push the branch: `git push origin fix/something`.
  5. Open a pull request against `master`.

For security reports, see `SECURITY.md` -- do NOT open a
public GitHub issue.

## Branch naming

Branch names follow the rules in `tools/lint-branches.sh`:

  - Must start with a category prefix
    (`fix/`, `feat/`, `chore/`, `refactor/`, `docs/`, `test/`,
    `ci/`, `tools/`, `build/`, `libdocument/`, `libview/`,
    `shell/`, `backend/`, `thumbnailer/`, `previewer/`,
    `data/`, `debian/`, `fuzz/`, `meson/`, `cut-n-paste/`,
    `snap/`).
  - Must have a kebab-case description (lowercase letters,
    digits, hyphens, dots -- dots are allowed for
    `release-X.Y.Z` branches).
  - Must not contain more than one slash.
  - Must not start with a dot, underscore, or digit.
  - Must be at most 80 characters.

## Commit messages

Commit messages follow the rules in `tools/lint-commits.sh`
(run by CI on every PR):

  - First line: `<component>: ...` (or a conventional-commits
    prefix: `chore:`, `fix:`, `feat:`, `refactor:`, `docs:`,
    `test:`, `ci:`).  At most 72 characters.  Imperative mood.
  - Body: wrapped at 80 columns or less, with a blank line
    separating from the subject.

## Coding style

See the "Code style" section in `HACKING.md` for the full
list.  The short version:

  - 8-space tabs for indentation, K&R braces.
  - Public functions should have `G_GNUC_NONNULL` / `G_GNUC_PURE`
    / `G_GNUC_MALLOC` / `G_GNUC_WARN_UNUSED_RESULT` /
    `G_GNUC_PRINTF` annotations as appropriate.
  - Public functions should have a `/** ... */` gtk-doc block.
  - Use `g_new` / `g_free` (not `g_slice_*`, deprecated).
  - Use `g_get_real_time` (not `g_get_current_time` + `GTimeVal`,
    deprecated).
  - Use `g_ascii_strcasecmp` (not `strcasecmp`).

## Testing

The project has 7 unit-test executables (52 tests) and 2
libFuzzer harnesses.  Run them with:

```
meson setup builddir
meson test -C builddir
```

See `HACKING.md` for the full test recipe.

## Review process

PRs are reviewed by the maintainer.  The review focuses on:

  - Does the PR fix the bug it claims to fix?  The
    "Test plan" section in the PR description should make
    this easy to verify.
  - Are the new tests comprehensive?  The test should fail
    before the fix and pass after.
  - Does the PR follow the coding style?
  - Does the PR introduce new warnings under `-Wall -Wextra
    -Wformat=2 -Wformat-security -Wnull-dereference
    -Wmissing-prototypes -Wshadow -Wpointer-arith -Wcast-align
    -Wstrict-prototypes`?

PRs that don't follow the style are not rejected -- the
maintainer will help fix the style issues.  PRs that don't
have a "Test plan" are usually sent back to the author
with a request to add one.

## Recognition

Contributors are credited in the `debian/changelog` for the
release that includes their change.  Run
`./tools/mailmap-cleanup.sh` to see the canonical author
list (currently 75 distinct contributors, down from 96
pre-mailmap).
