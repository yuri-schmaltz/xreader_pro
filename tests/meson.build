# tests/meson.build - GLib Testing unit tests for pure helpers in libdocument.
#
# Run via `meson test -C build` (or `meson test -C build test-file-helpers`).
#
# Each test executable is built from a small selection of C source files
# from libdocument that don't pull in a heavy backend (no poppler, no
# webkit, no gtk).  This keeps the test harness fast and lets the CI
# sanitizer job cover the same code paths as the runtime.

test_env = environment()
test_env.set('GSETTINGS_BACKEND', 'memory')
test_env.set('GSETTINGS_SCHEMA_DIR', join_paths(meson.project_build_root(), 'data'))
test_env.set('XREADER_BACKEND_DIR', join_paths(meson.project_build_root(), 'backend'))
test_env.set('G_TEST_SRCDIR', meson.current_source_dir())
test_env.set('G_TEST_BUILDDIR', meson.current_build_dir())

test_file_helpers = executable(
    'test-file-helpers',
    'test-file-helpers.c',
    '../libdocument/ev-file-helpers.c',
    include_directories: include_dirs,
    dependencies: [
        glib,
        gio,
        zlib,
    ],
    c_args: [
        '-DG_LOG_DOMAIN="test-file-helpers"',
        '-DEV_COMPILATION=1',
    ],
)

test(
    'test-file-helpers',
    test_file_helpers,
    protocol: 'gtest',
    env: test_env,
)

test_mime = executable(
    'test-mime',
    'test-mime.c',
    '../libdocument/ev-file-helpers.c',
    include_directories: include_dirs,
    dependencies: [
        glib,
        gio,
        zlib,
    ],
    c_args: [
        '-DG_LOG_DOMAIN="test-mime"',
        '-DEV_COMPILATION=1',
    ],
)

test(
    'test-mime',
    test_mime,
    protocol: 'gtest',
    env: test_env,
)

test_document_factory = executable(
    'test-document-factory',
    'test-document-factory.c',
    include_directories: include_dirs,
    dependencies: [
        glib,
        gio,
        gtk,
        zlib,
        libdocument_dep,
    ],
    c_args: [
        '-DG_LOG_DOMAIN="test-document-factory"',
        '-DEV_COMPILATION=1',
    ],
)

test(
    'test-document-factory',
    test_document_factory,
    protocol: 'gtest',
    env: test_env,
)

test_image = executable(
    'test-image',
    'test-image.c',
    include_directories: include_dirs,
    dependencies: [
        glib,
        gio,
        gdk_pixbuf,
        gtk,
        libdocument_dep,
    ],
    c_args: [
        '-DG_LOG_DOMAIN="test-image"',
        '-DEV_COMPILATION=1',
    ],
)

test(
    'test-image',
    test_image,
    protocol: 'gtest',
    env: test_env,
)

test_page_render = executable(
    'test-page-render',
    'test-page-render.c',
    include_directories: include_dirs,
    dependencies: [
        glib,
        gio,
        cairo,
        libdocument_dep,
    ],
    c_args: [
        '-DG_LOG_DOMAIN="test-page-render"',
        '-DEV_COMPILATION=1',
    ],
)

test(
    'test-page-render',
    test_page_render,
    protocol: 'gtest',
    env: test_env,
)

test_form_field = executable(
    'test-form-field',
    'test-form-field.c',
    include_directories: include_dirs,
    dependencies: [
        glib,
        gio,
        cairo,
        libdocument_dep,
    ],
    c_args: [
        '-DG_LOG_DOMAIN="test-form-field"',
        '-DEV_COMPILATION=1',
    ],
)

test(
    'test-form-field',
    test_form_field,
    protocol: 'gtest',
    env: test_env,
)

test_rect_intersect = executable(
    'test-rect-intersect',
    'test-rect-intersect.c',
    include_directories: include_dirs,
    dependencies: [
        glib,
        gobject,
        cairo,
        libdocument_dep,
    ],
    c_args: [
        '-DG_LOG_DOMAIN="test-rect-intersect"',
        '-DEV_COMPILATION=1',
    ],
)

test(
    'test-rect-intersect',
    test_rect_intersect,
    protocol: 'gtest',
    env: test_env,
)

test_tab_manager = executable(
    'test-tab-manager',
    'test-tab-manager.c',
    '../shell/ev-tab.c',
    '../shell/ev-tab-manager.c',
    include_directories: [include_dirs, include_directories('../shell')],
    dependencies: [
        glib,
        gobject,
        gio,
        gtk,
        libdocument_dep,
        libview_dep,
    ],
    c_args: [
        '-DG_LOG_DOMAIN="test-tab-manager"',
        '-DXREADER_COMPILATION=1',
    ],
)

test(
    'test-tab-manager',
    test_tab_manager,
    protocol: 'gtest',
    env: test_env,
)

test_reopen_stack = executable(
    'test-reopen-stack',
    'test-reopen-stack.c',
    '../shell/ev-tab.c',
    '../shell/ev-tab-manager.c',
    include_directories: [include_dirs, include_directories('../shell')],
    dependencies: [
        glib,
        gobject,
        gio,
        gtk,
        libdocument_dep,
        libview_dep,
    ],
    c_args: [
        '-DG_LOG_DOMAIN="test-reopen-stack"',
        '-DXREADER_COMPILATION=1',
    ],
)

test(
    'test-reopen-stack',
    test_reopen_stack,
    protocol: 'gtest',
    env: test_env,
)

test_tabbed_integration = executable(
    'test-tabbed-integration',
    'test-tabbed-integration.c',
    '../shell/ev-tabbed-window.c',
    '../shell/ev-tab.c',
    '../shell/ev-tab-manager.c',
    include_directories: [include_dirs, include_directories('../shell')],
    dependencies: [
        glib,
        gobject,
        gio,
        gtk,
        libdocument_dep,
        libview_dep,
    ],
    c_args: [
        '-DG_LOG_DOMAIN="test-tabbed-integration"',
        '-DXREADER_COMPILATION=1',
    ],
)

test(
    'test-tabbed-integration',
    test_tabbed_integration,
    protocol: 'gtest',
    env: test_env,
)

test_session_roundtrip = executable(
    'test-session-roundtrip',
    'test-session-roundtrip.c',
    include_directories: include_dirs,
    dependencies: [
        glib,
    ],
    c_args: [
        '-DG_LOG_DOMAIN="test-session-roundtrip"',
    ],
)

test(
    'test-session-roundtrip',
    test_session_roundtrip,
    protocol: 'gtest',
    env: test_env,
)

test_signatures = executable(
    'test-signatures',
    'test-signatures.c',
    include_directories: include_dirs,
    dependencies: [
        glib,
        gobject,
        libdocument_dep,
    ],
    c_args: [
        '-DG_LOG_DOMAIN="test-signatures"',
        '-DEV_COMPILATION=1',
    ],
)

test(
    'test-signatures',
    test_signatures,
    protocol: 'gtest',
    env: test_env,
)

test_ocr = executable(
    'test-ocr',
    'test-ocr.c',
    include_directories: include_dirs,
    dependencies: [
        glib,
        gobject,
        cairo,
        libdocument_dep,
    ],
    c_args: [
        '-DG_LOG_DOMAIN="test-ocr"',
        '-DEV_COMPILATION=1',
    ],
)

test(
    'test-ocr',
    test_ocr,
    protocol: 'gtest',
    env: test_env,
)

test_multi_tab_documents = executable(
    'test-multi-tab-documents',
    'test-multi-tab-documents.c',
    '../shell/ev-tabbed-window.c',
    '../shell/ev-tab.c',
    '../shell/ev-tab-manager.c',
    include_directories: [include_dirs, include_directories('../shell')],
    dependencies: [
        glib,
        gobject,
        gio,
        gtk,
        libdocument_dep,
        libview_dep,
    ],
    c_args: [
        '-DG_LOG_DOMAIN="test-multi-tab-documents"',
        '-DXREADER_COMPILATION=1',
    ],
)

test(
    'test-multi-tab-documents',
    test_multi_tab_documents,
    protocol: 'gtest',
    env: test_env,
)
