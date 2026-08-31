# Xreader Pro 🚀

[![Release](https://img.shields.io/github/v/release/yuri-schmaltz/xreader_pro?style=flat-square&color=blue)](https://github.com/yuri-schmaltz/xreader_pro/releases)
[![Build Status](https://img.shields.io/badge/build-passing-brightgreen?style=flat-square)](https://github.com/yuri-schmaltz/xreader_pro)
[![License: GPL v2](https://img.shields.io/badge/License-GPL_v2-blue.svg?style=flat-square)](COPYING)
[![GTK Version](https://img.shields.io/badge/GTK-3.0-blue?style=flat-square)](https://gtk.org)
[![GTK 4 Port](https://img.shields.io/badge/GTK_4-port_in_progress-yellow?style=flat-square)](https://gtk.org)

**Xreader Pro** is an advanced, high-performance document viewer for Linux desktops (Cinnamon, MATE, GNOME, XFCE, etc.). It provides a fast, modern reading experience for multi-page documents (PDF, PostScript, DjVu, TIFF, DVI, XPS, EPUB, and Comic Books) with optional **tabbed document browsing**, **X.509 digital signature verification**, and **built-in OCR (Optical Character Recognition)**.

> **Note on GTK 4:** the codebase supports dual-target builds
> (`-Dgtk_version=3` default, `-Dgtk_version=4` for the port in
> progress).  The GTK 4 build is not yet fully green -- see
> [ROADMAP.md](ROADMAP.md) C2 for the current porting status.

---

## 🌟 Key Features & Highlights

### 📑 Optional Multi-Document Tabbed Viewing (opt-in)
* **Tabbed Window Mode**: opt-in via `--tabbed`/`-T` CLI flag or the `tabbed-mode` GSettings key. When enabled, opens a separate top-level window (`EvTabbedWindow`) that hosts multiple documents in tabs. When disabled (the default), each document opens in its own `EvWindow` (the classic Evince behaviour).
* **Smart Tab Header Formatting**: Document icons, middle truncation, per-tab close buttons.
* **Session & State Preservation**: Each tab preserves its own page number, zoom level, and find query across tab switches.
* **Closed Tab Restoration**: Reopen accidentally closed tabs instantly with `Ctrl + Shift + T` (LIFO history stack, bounded to 10 entries).
* **Keyboard Navigation**: `Ctrl+Tab` / `Ctrl+Shift+Tab` to switch tabs, `Ctrl+W` to close the active tab.
* **24 unit tests** in `tests/test-tab-manager.c`, `tests/test-reopen-stack.c`, `tests/test-tabbed-integration.c`, `tests/test-multi-tab-documents.c`, and `tests/test-tabbed-window.c` cover the data model and stress/leak behavior.

### 🔐 Enterprise X.509 Digital Signatures
* **PKI Signature Verification**: Inspect digital certificates, signer identity, validation timestamps, and document integrity directly in PDF documents.
* **Poppler Backend Integration**: Native `EvDocumentSignatures` and `EvSignature` framework.

### 🔍 Integrated Optical Character Recognition (OCR)
* **Tesseract-Powered Text Recognition**: On-the-fly OCR extraction for scanned PDF pages and raw image documents via `EvDocumentOCR` and `EvOCRResult`.

### 🎨 GTK 3/4 Dual-Target Build (in progress)
* **Dual-Target Meson Option**: `meson setup build -Dgtk_version=3` (default) or `-Dgtk_version=4` (experimental, see [ROADMAP C2](ROADMAP.md)).
* **GTK 3 (current default)**: fully working, 15/15 active unit tests pass, 9 E2E Python tests under Xvfb.
* **GTK 4 (in progress)**: dual-target shims live in `libmisc/ev-gtk-compat.h` (`ev_gtk_box_append`, `ev_gtk_box_pack_start`, `ev_gtk_widget_show_all`, `ev_gtk_container_add`, `ev_gtk_dialog_run_async`, ...). 35 call sites in 6 dialog/area files already use the shims.
* **Asynchronous Dialog Helper**: `ev_gtk_dialog_run_async()` wraps `GtkDialog::response` signal so dialogs can be made non-blocking.

### 🛡️ Security Hardening Gauntlet
* Fixed historical vulnerabilities in DVI TFM font parsing (`tfmfile.c`, `fontmap.c`) and SyncTeX parsing.
* Hardened EPUB decompression permissions (`0700`) and sanitized file extraction against directory traversal attacks.
* Resolved DjVu page leaks and memory leaks in document attachment descriptors.

---

## ✅ Current Status (4.8.x)

| Area | Status | Detail |
|---|---|---|
| Build | ✅ green | 9/9 backends (pdf, ps, tiff, xps, epub, **djvu, dvi, comics, pixbuf**) all default-on |
| Unit tests | ✅ 15/15 pass | under ASan + UBSan + MALLOC_PERTURB_, ~0.3s total |
| E2E tests | ✅ 9/9 in CI | python-dogtail + Xvfb, run in the `e2e-python` GitHub Actions job |
| Fuzz harness | ✅ libFuzzer | `fuzz/fuzz-document-load.c`, run for 60s in CI |
| GTK 3 | ✅ working | default target |
| GTK 4 | 🟡 in progress | shims in `ev-gtk-compat.h`, ~35 call sites migrated |
| Tabbed view | ✅ working | opt-in via `--tabbed` or `tabbed-mode` GSettings |
| OCR | ✅ working | Tesseract backend, 2 unit tests |
| X.509 signatures | ✅ working | Poppler integration, 2 unit tests |

See [ROADMAP.md](ROADMAP.md) for the long-term work tracking.

---

## 📦 Installation & Updating

### Method 1: Single-File Debian Package (`.deb`) — Recommended

You can download the pre-packaged standalone `.deb` release directly from GitHub:

```bash
# 1. Download the latest .deb release package
wget https://github.com/yuri-schmaltz/xreader_pro/releases/download/v4.8.0-1/xreader-pro_4.8.0-1_amd64.deb

# 2. Install the package
sudo dpkg -i xreader-pro_4.8.0-1_amd64.deb

# 3. Resolve any missing system dependencies automatically
sudo apt-get install -f
```

#### To Update to a New Version:
Simply download the new `.deb` file and re-run:
```bash
sudo dpkg -i xreader-pro_<version>_amd64.deb
sudo apt-get install -f
```

---

### Method 2: Building from Source with Meson / Ninja

#### 1. Install Build Dependencies (Debian / Ubuntu / Linux Mint)
```bash
sudo apt update
sudo apt install -y git meson ninja-build build-essential \
    libgtk-3-dev libglib2.0-dev libpoppler-glib-dev libspectre-dev \
    libtiff-dev libgxps-dev libwebkit2gtk-4.1-dev libsecret-1-dev \
    libxapp-dev libarchive-dev libkpathsea-dev libtesseract-dev \
    tesseract-ocr tesseract-ocr-eng tesseract-ocr-por
```

#### 2. Clone the Repository
```bash
git clone https://github.com/yuri-schmaltz/xreader_pro.git
cd xreader_pro
```

#### 3. Configure & Compile
```bash
# Configure build directory
meson setup build --prefix=/usr --buildtype=release

# Compile the application
ninja -C build

# Run unit tests
meson test -C build --verbose
```

#### 4. Install
```bash
sudo ninja -C build install
sudo ldconfig
sudo glib-compile-schemas /usr/share/glib-2.0/schemas
```

---

## ⌨️ Essential Keyboard Shortcuts

| Shortcut | Action |
| :--- | :--- |
| **`Ctrl + Tab`** / **`Ctrl + PageDown`** | Switch to Next Tab |
| **`Ctrl + Shift + Tab`** / **`Ctrl + PageUp`** | Switch to Previous Tab |
| **`Ctrl + W`** | Close Active Tab |
| **`Ctrl + Shift + T`** | Reopen Last Closed Tab |
| **`Ctrl + O`** | Open Document |
| **`Ctrl + F`** / **`/`** | Find in Document |
| **`Ctrl + +`** / **`Ctrl + -`** | Zoom In / Zoom Out |
| **`Ctrl + 0`** | Fit Page |
| **`Ctrl + 9`** | Fit Width |
| **`Ctrl + R`** | Reload Document |
| **`F11`** | Toggle Fullscreen Mode |
| **`F5`** | Presentation Mode |
| **`Ctrl + Left`** / **`Ctrl + Right`** | Rotate Document 90° Left / Right |

---

## 📄 Supported Formats

| Format | Extension | Backend Engine |
| :--- | :--- | :--- |
| **PDF** (Portable Document Format) | `.pdf` | Poppler (with X.509 Signatures & OCR) |
| **PostScript** | `.ps`, `.eps` | libspectre / GhostScript |
| **DjVu** | `.djvu`, `.djv` | DjVuLibre |
| **TIFF** (Multi-page image) | `.tiff`, `.tif` | libtiff |
| **DVI** (Device Independent TeX) | `.dvi` | Native DVI renderer / kpathsea |
| **XPS** (XML Paper Specification) | `.xps`, `.oxps` | libgxps |
| **EPUB** (Electronic Publication) | `.epub` | WebKitGTK (sandboxed & hardened) |
| **Comic Books** | `.cbz`, `.cbr`, `.cb7` | libarchive / ComicBook engine |

---

## 🏛️ Architecture Overview

```
┌─────────────────────────────────────────────────────────────┐
│                    Xreader Shell (GTK)                      │
│   EvWindow (Full-Width Tabs, Toolbar, Sidebar, Search)      │
└──────────────────────────────┬──────────────────────────────┘
                               │
┌──────────────────────────────┴──────────────────────────────┐
│             libview (Rendering & Interaction)               │
│   EvView (Cairo / GSK Snapshot), EvSidebar, EvFindBar       │
└──────────────────────────────┬──────────────────────────────┘
                               │
┌──────────────────────────────┴──────────────────────────────┐
│           libdocument (Document Abstraction Model)          │
│   EvDocument, EvDocumentSignatures, EvDocumentOCR, etc.     │
└──────────────────────────────┬──────────────────────────────┘
                               │
┌──────────────────────────────┴──────────────────────────────┐
│                     Format Backends                         │
│   PDF (Poppler), DjVu, PostScript, TIFF, XPS, DVI, EPUB     │
└─────────────────────────────────────────────────────────────┘
```

---

## 🤝 Contributing & License

Contributions are welcome! Please feel free to submit issues, bug reports, and pull requests on [GitHub](https://github.com/yuri-schmaltz/xreader_pro).

* **License**: GNU General Public License v2 (GPL-2.0). See [COPYING](COPYING) for full details.
* **Maintainer**: Yuri Schmaltz
