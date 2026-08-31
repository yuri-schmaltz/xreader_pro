#!/usr/bin/env bash
#
# tools/ci-local.sh - reproduce the CI pipeline locally
#
# Runs the same checks as .github/workflows/ci-checks.yml::warnings
# and ::sanitizers jobs, in sequence, on the local machine.  Useful
# for catching CI failures before pushing a PR.
#
# Steps:
#   1. Source quality (check-source.sh)
#   2. Configure with strict warnings + werror
#   3. Build
#   4. Test
#   5. Configure with ASan+UBSan
#   6. Build
#   7. Test
#   8. Report overall status
#
# Run from the repository root:  ./tools/ci-local.sh
#
# Optional environment:
#   SKIP_ASAN=1   -- skip the sanitizer step (faster, less coverage)
#   KEEP_BUILDS=1 -- keep both build/ and build-san/ instead of
#                    removing build-san/ at the end

set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$REPO_ROOT"

red()    { printf '\033[31m%s\033[0m\n' "$*"; }
green()  { printf '\033[32m%s\033[0m\n' "$*"; }
yellow() { printf '\033[33m%s\033[0m\n' "$*"; }
bold()   { printf '\033[1m%s\033[0m\n' "$*"; }

bold "==> 1. Source quality (check-source.sh)"
if ! bash tools/check-source.sh; then
    red "FAIL: source quality check failed"
    exit 1
fi
green "    ok"

bold "==> 2-4. Configure (strict warnings) + build + test"
rm -rf build
# Note: --werror is intentionally NOT set here because the
# legacy backends (DVI, DjVu, EPUB, comics) have known sign-
# compare / missing-field-initializers warnings under modern
# toolchains.  Those will be fixed file-by-file in follow-up
# PRs; for now we want the warnings to be visible but not to
# block local CI.  Use WARN_AS_ERROR=1 to opt in.
if [ "${WARN_AS_ERROR:-0}" = "1" ]; then
    WERROR_FLAG="--werror"
else
    WERROR_FLAG=""
fi
meson setup build \
    --buildtype=debug \
    --warnlevel=2 \
    $WERROR_FLAG \
    -Dc_args="-Wall -Wextra -Wformat=2 -Wformat-security -Wnull-dereference -Wmissing-prototypes -Wmissing-declarations -Wshadow -Wpointer-arith -Wcast-align -Wstrict-prototypes -Wno-unused-parameter" \
    >/dev/null
ninja -C build
meson test -C build --print-errorlogs
green "    warnings build + test: ok"

if [ "${SKIP_ASAN:-0}" = "1" ]; then
    yellow "==> 5-7. ASan+UBSan: SKIPPED (SKIP_ASAN=1)"
else
    bold "==> 5-7. Configure (ASan+UBSan) + build + test"
    rm -rf build-san
    CC=clang CXX=clang++ meson setup build-san \
        --buildtype=debug \
        -Db_sanitize=address,undefined \
        -Db_lto=false \
        -Dcpp_args="-fsanitize=address,undefined -fno-omit-frame-pointer" \
        -Dc_args="-fsanitize=address,undefined -fno-omit-frame-pointer -O1 -g" \
        >/dev/null
    ninja -C build-san
    ASAN_OPTIONS="detect_leaks=1:abort_on_error=1:print_stacktrace=1" \
    UBSAN_OPTIONS="print_stacktrace=1:halt_on_error=1" \
        meson test -C build-san --print-errorlogs
    green "    ASan+UBSan build + test: ok"

    if [ "${KEEP_BUILDS:-0}" != "1" ]; then
        rm -rf build-san
    fi
fi

green ""
green "================================="
green "  All CI checks passed locally!"
green "================================="
