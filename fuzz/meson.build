# fuzz/meson.build - libFuzzer harnesses
#
# Build the document-load fuzzer with:
#
#   CC=clang meson setup build-fuzz \
#     -Db_sanitize=none \
#     -Dfuzz=enabled
#
# The harness itself is linked with -fsanitize=fuzzer (which brings
# clang's ASan coverage instrumentation for the harness); the project
# shared libraries are NOT sanitized so the -Wl,--no-undefined shared
# library links keep working under clang.
#
# Or use the ASan/UBSan-only variant (no fuzzer, no corpus) for CI:
#
#   meson setup build -Db_sanitize=address,undefined
#   meson test -C build fuzz-document-load
#
# The default build leaves the fuzz harness out; it's only built when
# the user opts in.  This keeps the default `ninja -C build` cheap.

if get_option('fuzz').enabled()
  fuzz_deps = [
    glib,
    gio,
    libdocument_dep,
  ]

  if cc.has_multi_arguments(['-fsanitize=fuzzer-no-link'])
    fuzz_link_args = ['-fsanitize=fuzzer']
  else
    fuzz_link_args = []
    warning('libFuzzer not available; fuzz harness will not be built')
  endif

  executable(
    'fuzz-document-load',
    'fuzz-document-load.c',
    include_directories: include_dirs,
    dependencies: fuzz_deps,
    c_args: [
      '-DG_LOG_DOMAIN="fuzz-document-load"',
    ],
    link_args: fuzz_link_args,
    build_by_default: false,
  )

  # Complements fuzz-document-load: same input, but writes the
  # bytes to 14 different file extensions so the MIME dispatcher
  # is forced to take the extension-based fast path for every
  # backend (poppler, djvu, dvi, gxps, epub, tiff, comics, odf,
  # ...).  See fuzz/fuzz-document-load-all.c for the full list.
  executable(
    'fuzz-document-load-all',
    'fuzz-document-load-all.c',
    include_directories: include_dirs,
    dependencies: fuzz_deps,
    c_args: [
      '-DG_LOG_DOMAIN="fuzz-document-load-all"',
    ],
    link_args: fuzz_link_args,
    build_by_default: false,
  )
endif
