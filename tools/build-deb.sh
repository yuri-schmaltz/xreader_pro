#!/usr/bin/env bash
set -e

REPO_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${REPO_DIR}/build"
PKG_ROOT="${BUILD_DIR}/deb_pkg_root"
DEB_NAME="xreader-pro_4.8.0-1_amd64.deb"

echo "==> 1. Compiling and staging files..."
rm -rf "${PKG_ROOT}"
DESTDIR="${PKG_ROOT}" ninja -C "${BUILD_DIR}" install

echo "==> 2. Generating DEBIAN metadata..."
mkdir -p "${PKG_ROOT}/DEBIAN"
chmod 755 "${PKG_ROOT}/DEBIAN"

cat <<'DEB_CTRL' > "${PKG_ROOT}/DEBIAN/control"
Package: xreader-pro
Version: 4.8.0-1
Section: graphics
Priority: optional
Architecture: amd64
Maintainer: Yuri Schmaltz <yuri.schmaltz@gmail.com>
Depends: libc6, libglib2.0-0t64 | libglib2.0-0, libgtk-3-0t64 | libgtk-3-0, libpoppler-glib8t64 | libpoppler-glib8, libspectre1, libtiff6 | libtiff5, libgxps2, libwebkit2gtk-4.1-0, libsecret-1-0, libxapp1, libarchive13t64 | libarchive13, libkpathsea6, libtesseract5 | tesseract-ocr
Description: Advanced Document Viewer Pro with Native Tabs, X.509 Signatures & OCR
 Xreader Pro is a modern, fast, and feature-rich document viewer supporting
 PDF, PostScript, DjVu, TIFF, DVI, XPS, EPUB, and Comic books.
 .
 Features:
  - Native tabbed document viewing by default with closed tab restoration (Ctrl+Shift+T)
  - Integrated X.509 digital signature verification and validation engine
  - Built-in optical character recognition (OCR) powered by Tesseract
  - GTK4 modernized architecture and hardware-accelerated rendering
  - Full support for annotations, bookmarks, continuous zooming, and search
DEB_CTRL
chmod 644 "${PKG_ROOT}/DEBIAN/control"

cat <<'DEB_POSTINST' > "${PKG_ROOT}/DEBIAN/postinst"
#!/bin/sh
set -e
if [ "$1" = "configure" ]; then
    glib-compile-schemas /usr/share/glib-2.0/schemas 2>/dev/null || true
    update-desktop-database -q /usr/share/applications 2>/dev/null || true
    gtk-update-icon-cache -q -t -f /usr/share/icons/hicolor 2>/dev/null || true
fi
exit 0
DEB_POSTINST
chmod 755 "${PKG_ROOT}/DEBIAN/postinst"

cat <<'DEB_POSTRM' > "${PKG_ROOT}/DEBIAN/postrm"
#!/bin/sh
set -e
if [ "$1" = "remove" ] || [ "$1" = "purge" ]; then
    glib-compile-schemas /usr/share/glib-2.0/schemas 2>/dev/null || true
    update-desktop-database -q /usr/share/applications 2>/dev/null || true
    gtk-update-icon-cache -q -t -f /usr/share/icons/hicolor 2>/dev/null || true
fi
exit 0
DEB_POSTRM
chmod 755 "${PKG_ROOT}/DEBIAN/postrm"

# Fix general directory permissions
find "${PKG_ROOT}" -type d -exec chmod 755 {} +

echo "==> 3. Building Debian package with dpkg-deb..."
dpkg-deb --build --root-owner-group "${PKG_ROOT}" "${REPO_DIR}/${DEB_NAME}"

echo "==> Package created successfully: ${REPO_DIR}/${DEB_NAME}"
dpkg-deb -I "${REPO_DIR}/${DEB_NAME}"
