# Xreader Roadmap

This document tracks the long-term work in the xreader fork.
Each item has:

  - A short code (A1, A2, B1, ...).
  - A release target (the version in which the work is
    expected to land).
  - A scope estimate (Bounded / Multi-PR / Multi-month).
  - A status (Done / In progress / Planned / Blocked).

Items are grouped by category (A = Security, B = Features,
C = Long-term, D = Build/CI/Dev tools, E = Documentation).

## A -- Security

  - **A1** Security audit (cmd injection + buffer overflow)
    -- 4.7.0 -- Bounded -- **DONE** (PRs #1, #2, #10, #18;
    also PRs #11, #25 fixed at the same time).
  - **A2** Ongoing security hardening -- Indefinite --
    Bounded -- **IN PROGRESS** (per-PR: NULL guards,
    bounds checks, g_uri_unescape_string NULL returns,
    FORTIFY_SOURCE=3, etc.).

## B -- Features

  - **B1** GtkAction -> GAction migration -- 4.8.0 to 4.13.0
    -- Multi-PR -- **IN PROGRESS** (PRs #76, #77, #89
    started; see `docs/GACTION_MIGRATION.md`).
  - **B2** Pdf form support -- 4.8.0 (infrastructure
    already in `libdocument/ev-form-field.{c,h}`; see
    PRs #78, #79 for the form-field helpers + tests)
    -- Multi-PR -- **IN PROGRESS**.
  - **B3** Tabs (single window, multiple documents) --
    4.14.0 -- Multi-month -- Planned (depends on B1).
  - **B4** Search improvements (regex, case-insensitive,
    multi-file) -- 4.10.0 -- Bounded -- Planned.
  - **B5** Annotation export to PDF -- 4.9.0 -- Bounded
    -- Planned.
  - **B6** PDF form-filling UI -- 4.9.0 -- Multi-PR
    -- Planned.

## C -- Long-term / platform

  - **C1** Upstream rebase on Evince master -- 4.15.0 --
    Multi-month -- Planned.
  - **C2** GTK 4 port -- 4.16.0 -- Multi-month -- Planned
    (depends on C1 + B1).
  - **C3** Tabbed view -- 4.8.0 (foundation) / 4.9.0 (full
    integration) -- Multi-month -- **IN PROGRESS** (PRs #96-#102
    in 4.8.0: EvTab + EvTabManager + EvTabbedWindow + CLI flag
    + GSettings + keyboard shortcuts + drag-and-drop; PRs in
    4.9.0 will add the shared chrome + page navigation +
    session save/restore).
  - **C4** Wayland-native window management -- 4.13.0 --
    Multi-month -- Planned (depends on GTK 4 partial).
  - **C5** XApp sidebar / libxapp integration -- 4.12.0
    -- Multi-PR -- Planned (libxapp 2.5+ needed, currently
    on 2.4.2 in Debian 12).

## D -- Build / CI / dev tools

  - **D1** meson build system -- 4.6.0 -- Bounded --
    **DONE** (PR from upstream).
  - **D2** Strict-warnings CI job -- 4.7.0 -- Bounded --
    **DONE** (PR #8).
  - **D3** libFuzzer harness -- 4.7.0 -- Bounded --
    **DONE** (PR #19).
  - **D4** ASan + UBSan CI -- 4.7.0 -- Bounded --
    **DONE** (PR #8).
  - **D5** Unit tests (5 test executables, 52 cases) --
    4.7.0 -- Bounded -- **DONE** (PRs #25, #33, #41,
    #45, #60).
  - **D6** `.mailmap` cleanup (96 -> 75 authors) -- 4.7.0
    -- Bounded -- **DONE** (PR #38).
  - **D7** `tools/check-source.sh` -- 4.7.0 -- Bounded --
    **DONE** (PR #17).
  - **D8** `tools/lint-commits.sh` -- 4.7.0 -- Bounded --
    **DONE** (PR #34).
  - **D9** `tools/release.sh` -- 4.7.0 -- Bounded --
    **DONE** (PR #35 + #55 + #84).
  - **D10** `tools/mailmap-cleanup.sh` -- 4.7.0 -- Bounded
    -- **DONE** (PR #36 + #72).
  - **D11** CI: cppcheck, source-style, coverage, iwyu,
    valgrind, clang-tidy, coverity-scan, clang, arm64,
    multi-distro -- 4.7.0 to 4.8.0 -- Bounded --
    **DONE** (PRs #42, #46, #53, #67, #68, #69, #70,
    #71, #80; multi-distro PR #37).
  - **D12** `tools/lint-branches.sh` -- 4.8.0 -- Bounded
    -- **DONE** (PR #85).
  - **D13** `tests/meson.build` + `fuzz/meson.build`
    cleanups -- 4.7.0 -- Bounded -- **DONE** (PRs #25,
    #33, #41, #45, #60, #79, #88).
  - **D14** Snap manifest -- 4.8.0 -- Bounded --
    **DONE** (PR #75).
  - **D16** `_FORTIFY_SOURCE=3` + `-fstack-protector-strong`
    -- 4.8.0 -- Bounded -- **DONE** (PR #81).

## E -- Documentation

  - **E1** README + INSTALL + HACKING + SECURITY + NEWS --
    4.7.0 -- Bounded -- **DONE** (initial set + PRs
    #82, #83 for expansion).
  - **E2** debian/changelog -- Ongoing -- Bounded --
    **IN PROGRESS** (one entry per release).
  - **E3** gtk-doc blocks on public headers -- 4.7.0 to
    4.8.0 -- Bounded -- **IN PROGRESS** (9 of ~30
    headers done in PRs #52, #56-#60).
  - **E4** CONTRIBUTING.md -- 4.8.0 -- Bounded --
    **DONE** (PR #91).
  - **E5** CODE_OF_CONDUCT.md -- 4.8.0 -- Bounded --
    **DONE** (PR #92).
  - **E6** SUPPORT.md -- 4.8.0 -- Bounded -- **DONE**
    (PR #93).
  - **E7** ARCHITECTURE.md -- 4.8.0 -- Bounded --
    **DONE** (PR #94).
  - **E8** ROADMAP.md -- 4.8.0 -- Bounded -- **DONE**
    (this file, PR #95).
  - **E9** GACTION_MIGRATION.md -- 4.8.0 -- Bounded --
    **DONE** (PR #90).

## Release cadence

  - 4.7.0 -- released 2026-07-28 (31 self-PRs)
  - 4.8.0 -- target Q4 2026 (24+ self-PRs)
  - 4.9.0 -- target Q1 2027 (PDF form UI, B5)
  - 4.10.0 -- target Q2 2027 (search improvements, B4)
  - 4.11.0 - 4.13.0 -- 2027 (GAction + XApp + Wayland)
  - 4.14.0 -- Q4 2027 (tabbed view, C3)
  - 4.15.0 -- 2028 (upstream rebase, C1)
  - 4.16.0 -- 2028+ (GTK 4, C2)

## How items are added / updated

To add a new item, edit this file and open a PR.
Items are discussed in the issue tracker before being
added (no surprises).  The maintainer (yuri-schmaltz)
is the final arbiter of priorities.

## How to contribute

See `CONTRIBUTING.md` and `HACKING.md`.  The short
version: open a PR against `master` with a branch
named `<category>/<short-description>`.  The CI will
run automatically; the maintainer reviews within
a few days.
