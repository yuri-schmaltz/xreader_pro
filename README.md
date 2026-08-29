# Xreader Pro 🚀

[![Release](https://img.shields.io/github/v/release/yuri-schmaltz/xreader_pro?color=blue&logo=github)](https://github.com/yuri-schmaltz/xreader_pro/releases/latest)
[![Build Status](https://img.shields.io/badge/build-passing-brightgreen.svg)](https://github.com/yuri-schmaltz/xreader_pro)
[![License: GPL v2](https://img.shields.io/badge/License-GPL%20v2-blue.svg)](COPYING)
[![Platform](https://img.shields.io/badge/platform-Linux%20%7C%20Ubuntu%20%7C%20Debian%20%7C%20Mint-orange.svg)](https://github.com/yuri-schmaltz/xreader_pro)

**Xreader Pro** is an advanced, high-performance, multi-format document viewer for Linux. Built upon the solid foundations of Xreader, this Pro edition incorporates native multi-document tabs, X.509 digital signature verification, integrated Tesseract OCR, GTK4 modernization, and comprehensive security hardening.

---

## 🌟 Key Features

### 📑 Native Multi-Document Tab Bar
- **Tabbed Browsing by Default**: Open multiple PDF, PostScript, DjVu, EPUB, and TIFF files inside a single clean window.
- **Full-Width Header Integration**: Sleek tab bar positioned below the main toolbar with file-type icons, readable middle-ellipsized names, and dedicated close buttons.
- **State Preservation**: Each tab independently tracks its active page number, zoom scale, search state, and sidebar layout.
- **LIFO Closed Tab Recovery**: Quickly restore accidentally closed tabs using `Ctrl + Shift + T`.

### 🔐 Enterprise Digital Signatures (X.509)
- **Cryptographic Verification**: Inspect PDF digital signatures with full X.509 certificate validation.
- **Integrity Validation**: Check signer identity, signing timestamp, certificate chain, and document modification alerts.

### 🔍 Integrated Optical Character Recognition (OCR)
- **Tesseract Powered**: Extract and search text from scanned PDFs and raster images without external tools.
- **On-the-Fly Recognition**: Instant text layer generation directly within the viewer.

### 🎨 GTK4 Modernization & Multi-Target Architecture
- **Dual-Target Build Support**: Compile seamlessly for GTK3 or GTK4 (`-Dgtk_version=3` or `-Dgtk_version=4`).
- **Hardware-Accelerated Rendering**: Support for modern `GtkSnapshot` / GSK scene graph composition alongside Cairo pipelines.
- **Asynchronous Dialogs**: Elimination of blocking modal calls (`gtk_dialog_run`) in favor of asynchronous signal responses.

### 🛡️ Security & Hardening Gauntlet
- **Buffer Overflow Protections**: Fixed TeX TFM and SyncTeX parser memory safety vulnerabilities.
- **Safe EPUB Decompression**: Directory traversal prevention and strict directory permission enforcement (`0700`).
- **Memory Leak Resolution**: Clean teardown of DjVu page renderers, file descriptors, and spawn handlers.

---

## 📦 Quick Installation (Ubuntu / Debian / Linux Mint)

### Install via Single `.deb` Package (Recommended)

Download and install the latest pre-compiled release package in one step:

```bash
# 1. Download the latest .deb package
wget -O /tmp/xreader-pro_4.8.1-1_amd64.deb https://github.com/yuri-schmaltz/xreader_pro/releases/download/v4.8.1-1/xreader-pro_4.8.1-1_amd64.deb

# 2. Install the package
sudo dpkg -i /tmp/xreader-pro_4.8.1-1_amd64.deb

# 3. Resolve any missing system runtime dependencies
sudo apt-get install -f
```

---

## 🔄 Updating to the Latest Version

To update an existing installation:

```bash
# Download and reinstall the latest release package
wget -O /tmp/xreader-pro_4.8.1-1_amd64.deb https://github.com/yuri-schmaltz/xreader_pro/releases/latest/download/xreader-pro_4.8.1-1_amd64.deb
sudo dpkg -i /tmp/xreader-pro_4.8.1-1_amd64.deb
sudo apt-get install -f
```

---

## ⌨️ Keyboard Shortcuts Reference

| Action | Shortcut |
|---|---|
| **Next Tab** | `Ctrl + Tab` or `Ctrl + Page Down` |
| **Previous Tab** | `Ctrl + Shift + Tab` or `Ctrl + Page Up` |
| **Close Active Tab** | `Ctrl + W` |
| **Reopen Last Closed Tab** | `Ctrl + Shift + T` |
| **Open Document** | `Ctrl + O` |
| **Find in Document** | `Ctrl + F` |
| **Zoom In / Out** | `Ctrl + +` / `Ctrl + -` |
| **Fit Page / Fit Width** | `Ctrl + E` / `Ctrl + W` (view mode) |
| **Rotate Left / Right** | `Ctrl + Left` / `Ctrl + Right` |
| **Presentation Mode** | `F5` |
| **Fullscreen Mode** | `F11` |

---

## 🛠️ Building from Source

### 1. Install Build Dependencies (Debian / Ubuntu)

```bash
sudo apt update
sudo apt install -y git meson ninja-build libgtk-3-dev libgtk-4-dev \
    libpoppler-glib-dev libspectre-dev libtiff-dev libgxps-dev \
    libwebkit2gtk-4.1-dev libsecret-1-dev libxapp-dev libarchive-dev \
    libkpathsea-dev libtesseract-dev tesseract-ocr
```

### 2. Clone and Compile

```bash
git clone https://github.com/yuri-schmaltz/xreader_pro.git
cd xreader_pro

# Configure build directory with prefix /usr
meson setup build --prefix=/usr -Ddeprecated_warnings=false -Dgtk_version=3

# Compile
ninja -C build

# Run automated test suite
meson test -C build --verbose

# Install (optional)
sudo ninja -C build install
```

### 3. Generate Single `.deb` Package

```bash
./tools/build-deb.sh
```
This generates `xreader-pro_4.8.1-1_amd64.deb` in the repository root.

---

## 📂 Supported Formats

- **PDF** (Poppler GLib with X.509 Signatures & OCR)
- **PostScript / EPS** (`libspectre`)
- **TIFF** Multi-page images
- **DVI** TeX Device Independent format
- **DjVu** (`DjVuLibre`)
- **XPS / OpenXPS** (`libgxps`)
- **EPUB** E-Books (`WebKitGTK`)
- **Comic Books** (`cbz`, `cbr`, `cb7`, `cbt` via `libarchive`)

---

## 📄 License

Xreader Pro is licensed under the **GNU General Public License v2.0 or later** (see [COPYING](COPYING)).
