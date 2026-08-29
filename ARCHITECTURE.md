# Xreader Architecture

This document is a high-level overview of the Xreader code
structure.  It is intended for new contributors who want to
understand where to make a change before diving into the
specific files.

## High-level layers

Xreader is organized in 4 layers, from bottom to top:

  1. **Backends** (`backend/`) -- one per document format.
     Each backend implements the `EvDocument` interface
     (defined in `libdocument/ev-document.h`) and provides
     the format-specific code to load, render, and search
     a document.
  2. **libdocument** (`libdocument/`) -- the document
     abstraction layer.  Defines the `EvDocument` interface
     and the helper classes (annotation, form field, link,
     etc.) that the backends implement.
  3. **libview** (`libview/`) -- the rendering layer.
     Implements the GTK widgets that display a document
     (`EvView`), the thumbnail strip, the find bar, and
     the page cache.
  4. **shell** (`shell/`) -- the GTK application.  The
     `EvApplication` (registered via `g_application_run`),
     the `EvWindow` (the per-document window), the
     `EvPropertiesView` (the file properties dialog), etc.

Supporting directories:

  - `libmisc/` -- small utility classes (file helpers,
    uri helpers, etc.) that don't fit the document model.
  - `thumbnailer/` -- a small headless binary that
    generates thumbnail images for the file manager.
  - `previewer/` -- a small headless binary that previews
    a single page (used by the file manager's preview
    pane).
  - `cut-n-paste/` -- third-party code that has been
    vendored in (e.g. `eggdesktopfile`).
  - `tests/` -- unit tests (52 cases, 7 executables).
  - `fuzz/` -- libFuzzer harnesses (2 harnesses, 11 seeds).
  - `tools/` -- developer scripts (`check-source.sh`,
    `lint-commits.sh`, `lint-branches.sh`, `mailmap-cleanup.sh`,
    `release.sh`).
  - `data/` -- desktop files, icons, AppData.
  - `debian/` -- packaging.
  - `po/` -- translations.
  - `makepot/` -- the script that generates the .pot file.
  - `help/` -- the user documentation (Mallard XML).

## The EvDocument interface

The `EvDocument` interface (in `libdocument/ev-document.h`)
is the central abstraction.  It defines the methods that
all backends must implement:

  - Load / close a document.
  - Get the number of pages.
  - Get a page (an `EvPage` object).
  - Render a page to a cairo surface.
  - Get the document info (title, author, ...).
  - Get the document security (encryption status,
    permissions).
  - Get the document links (a list of `EvLink`).
  - Get the document annotations (a list of
    `EvAnnotation`).
  - Get the document attachments (a list of
    `EvAttachment`).
  - Get the document metadata.
  - Get the document fonts (a list of `EvDocumentFont`).
  - Get the document layers (optional, for PDF).
  - Get the document forms (optional, for PDF).
  - Get the document outline (a tree of `EvDocumentOutline`).
  - Get the document transitions (a list of
    `EvDocumentTransition`).
  - Get the document print settings.
  - Get the document text (for the find-in-document
    feature).
  - Search the document for a string.

Each backend (`backend/pdf/`, `backend/djvu/`, `backend/xps/`,
`backend/dvi/`, `backend/tiff/`, `backend/comics/`,
`backend/ps/`, `backend/impress/`, `backend/pixbuf/`)
implements this interface on top of the format's library
(poppler for PDF, djvulibre for DjVu, etc.).

## The GObject type system

Xreader uses GObject throughout.  The public types are:

  - `EvDocument` (interface, in `libdocument/`)
  - `EvBackendInfo` (boxed type, in `libdocument/`)
  - `EvRectangle` (boxed type, in `libdocument/`)
  - `EvPoint` (boxed type, in `libdocument/`)
  - `EvMapping` (boxed type, in `libdocument/`)
  - `EvSourceLink` (boxed type, in `libdocument/`)
  - `EvPage` (GObject, in `libdocument/`)
  - `EvDocumentInfo` (boxed type, in `libdocument/`)
  - `EvDocumentSecurity` (boxed type, in `libdocument/`)
  - `EvDocumentSignatures`, `EvSignature` (interface + GObject,
    in `libdocument/` for X.509 PKI verification)
  - `EvDocumentOCR`, `EvOCRResult` (interface + GObject,
    in `libdocument/` for Tesseract OCR extraction)
  - `EvLink`, `EvLinkAction`, `EvLinkDest` (boxed types,
    in `libdocument/`)
  - `EvAttachment` (GObject, in `libdocument/`)
  - `EvAnnotation`, `EvAnnotationMarkup`,
    `EvAnnotationText` (GObjects, in `libdocument/`)
  - `EvFormField`, `EvFormFieldText`, `EvFormFieldButton`,
    `EvFormFieldChoice`, `EvFormFieldSignature` (GObjects,
    in `libdocument/`)
  - `EvDocumentFonts`, `EvDocumentFont` (boxed types,
    in `libdocument/`)
  - `EvDocumentImages`, `EvDocumentImage` (boxed types,
    in `libdocument/`)
  - `EvDocumentLayers`, `EvDocumentLayer` (boxed types,
    in `libdocument/`)
  - `EvDocumentPrint` (boxed type, in `libdocument/`)
  - `EvDocumentText` (GObject, in `libdocument/`)
  - `EvDocumentTransition`, `EvDocumentTransitionEffect`
    (boxed types, in `libdocument/`)
  - `EvView` (GtkWidget, in `libview/`)
  - `EvThumbnail` (GtkWidget, in `libview/`)
  - `EvSidebar` (GtkBox, in `libview/`)
  - `EvSidebarLinks`, `EvSidebarOutline`,
    `EvSidebarAttachments`, `EvSidebarLayers`,
    `EvSidebarAnnotations` (subclasses of `EvSidebar`,
    in `libview/`)
  - `EvJob`, `EvJobRender`, `EvJobPageRender`,
    `EvJobPageData`, `EvJobLoad`, `EvJobSave`,
    `EvJobPrint`, `EvJobFind`, `EvJobLinks`,
    `EvJobAnnotations`, `EvJobAttach`, `EvJobOutline`,
    `EvJobFonts`, `EvJobLayers`, `EvJobExport`,
    `EvJobThumbnail`, `EvJobPresentation` (GObjects,
    in `libview/`)
  - `EvApplication` (GApplication, in `shell/`)
  - `EvWindow` (GtkApplicationWindow, in `shell/` featuring
    native multi-document full-width tab management)
  - `EvTabManager`, `EvTab` (GObjects, in `shell/`)
  - `EvPropertiesView` (GtkBox, in `shell/`)
  - `EvMetadata` (boxed type, in `shell/`)

For each, there's a `.c` and `.h` file.  The `.h` file
defines the GType macros, the public API, and the gtk-doc
block; the `.c` file implements the type.

## GTK3 / GTK4 Multi-Target Compatibility Layer

`libmisc/ev-gtk-compat.h` provides unified abstraction wrappers
bridging GTK3 and GTK4 APIs:
  - `ev_gtk_box_append()` & `ev_gtk_box_prepend()`
  - `ev_gtk_widget_set_cursor_name()`
  - `ev_g_action_activate()`
  - `ev_gtk_dialog_run_async()`
  - `GtkSnapshot` hardware-accelerated rendering pathways.

## The GApplication / GAction migration

Xreader is in the middle of a long-term migration from the
GtkAction / UIManager API to the modern GAction / GMenu
API.  See `docs/GACTION_MIGRATION.md` for the full plan
and the current status.

## The build system

Xreader uses Meson (>= 0.56).  The `meson.build` at the
top level is the entry point.  The most important
subdirectories have their own `meson.build` files:

  - `libdocument/meson.build`
  - `libview/meson.build`
  - `shell/meson.build`
  - `tests/meson.build`
  - `fuzz/meson.build`
  - `thumbnailer/meson.build`
  - `previewer/meson.build`

For a full description of the build, see `HACKING.md`.

## Where to make a change

  - **A new file format backend**: add a new directory
    under `backend/`, implement the `EvDocument` interface,
    register it in `libdocument/ev-document-factory.c`.
  - **A new public API in libdocument**: add the function
    to the relevant `.h` (with a gtk-doc block and
    G_GNUC_* annotations as appropriate) and the
    implementation in the matching `.c`.
  - **A new sidebar widget**: subclass `EvSidebar` in
    `libview/`, add the `EvSidebar*Class::add_tab` to
    the shell code.
  - **A new menu / toolbar item**: register a GAction
    (see `docs/GACTION_MIGRATION.md`) and add a `.menu`
    file entry in `shell/`.
  - **A new CLI option**: extend `shell/main.c` and
    update the man page (`help/C/`).
  - **A new unit test**: add a `.c` file under `tests/`
    and a corresponding `executable()` in `tests/meson.build`.
  - **A new fuzzer**: add a `.c` file under `fuzz/` and
    a corresponding `executable()` in `fuzz/meson.build`.
