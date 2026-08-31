epub_sources = [
    'epub-document.c',
    'epub-document.h',
]

# EPUB backend has K&R style functions without explicit
# 'void' arg lists -- suppress -Wstrict-prototypes.
epub_c_args = [
    '-Wno-strict-prototypes',
    '-Wno-missing-prototypes',
]

epub_deps = [
    cairo,
    glib,
    gtk,
    xml,
    libzip_dep,
]

shared_module(
    'epubdocument',
    epub_sources,
    c_args: epub_c_args,
    link_with: [libdocument],
    link_args: ['-Wl,-Bsymbolic', '-Wl,-z,relro', '-Wl,-z,now'],
    include_directories: include_dirs,
    dependencies: epub_deps,
    install: true,
    install_dir: backendsdir,
)

custom_target(
    'epub_backend',
    input: 'epubdocument.xreader-backend.in',
    output: 'epubdocument.xreader-backend',
    command: [intltool_merge, '-d', '-u', po_dir, '@INPUT@', '@OUTPUT@'],
    install: true,
    install_dir: backendsdir,
)
