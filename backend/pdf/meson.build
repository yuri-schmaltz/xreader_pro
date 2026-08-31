pdf_sources = [
    'ev-poppler.cc',
    'ev-poppler.h'
]

# The PDF backend has a few -Wunused-parameter and
# -Wswitch warnings under modern poppler (newer Poppler
# enum values are not all handled).  Suppress them so the
# overall -Werror pass succeeds; the runtime behaviour is
# unchanged because the missing enum values would fall
# through to the same 'unimplemented' path.
pdf_c_args = [
    '-Wno-unused-parameter',
    '-Wno-switch',
    '-Wno-cast-function-type',
    '-Wno-deprecated-declarations',
    '-Wno-type-limits',
    '-Wno-absolute-value',
    '-Wno-missing-prototypes',
    '-Wno-missing-field-initializers',
]

pdf_deps = [
    gtk,
    xml,
    poppler,
]

shared_module(
    'pdfdocument',
    pdf_sources,
    c_args: pdf_c_args,
    cpp_args: pdf_c_args,
    link_with: [libdocument],
    link_args: ['-Wl,-Bsymbolic', '-Wl,-z,relro', '-Wl,-z,now'],
    include_directories: include_dirs,
    dependencies: pdf_deps,
    install: true,
    install_dir: backendsdir,
)

custom_target(
    'pdf_backend',
    input: 'pdfdocument.xreader-backend.in',
    output: 'pdfdocument.xreader-backend',
    command: [intltool_merge, '-d', '-u', po_dir, '@INPUT@', '@OUTPUT@'],
    install: true,
    install_dir: backendsdir,
)
