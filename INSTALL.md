# Installation & Build Guide for Xreader Pro

## 📦 Method 1: Pre-Built Debian Package (.deb) — Recommended

The easiest way to install or update Xreader Pro on Ubuntu, Debian, Linux Mint, and derivatives:

```bash
# 1. Download the latest single .deb package
wget -O /tmp/xreader-pro_4.8.0-1_amd64.deb https://github.com/yuri-schmaltz/xreader_pro/releases/download/v4.8.0-1/xreader-pro_4.8.0-1_amd64.deb

# 2. Install using dpkg
sudo dpkg -i /tmp/xreader-pro_4.8.0-1_amd64.deb

# 3. Resolve any runtime dependencies automatically
sudo apt-get install -f
```

---

## 🛠️ Method 2: Building from Source with Meson & Ninja

### 1. Install Build Dependencies

On Debian, Ubuntu, or Linux Mint:
```bash
sudo apt update
sudo apt install -y git meson ninja-build dpkg-dev \
    libgtk-3-dev libgtk-4-dev \
    libpoppler-glib-dev libspectre-dev libtiff-dev libgxps-dev \
    libwebkit2gtk-4.1-dev libsecret-1-dev libxapp-dev libarchive-dev \
    libkpathsea-dev libtesseract-dev tesseract-ocr \
    gobject-introspection libgirepository1.0-dev
```

### 2. Clone the Repository

```bash
git clone https://github.com/yuri-schmaltz/xreader_pro.git
cd xreader_pro
```

### 3. Configure the Build

```bash
# Standard GTK3 build:
meson setup build --prefix=/usr -Ddeprecated_warnings=false -Dgtk_version=3

# Or modern GTK4 build:
# meson setup build --prefix=/usr -Ddeprecated_warnings=false -Dgtk_version=4
```

### 4. Compile & Test

```bash
# Compile
ninja -C build

# Run automated tests
meson test -C build --verbose
```

### 5. Install

```bash
sudo ninja -C build install
sudo ldconfig
sudo glib-compile-schemas /usr/share/glib-2.0/schemas
```

---

## 📦 Method 3: Generating a Standalone `.deb` Package

You can generate a standalone Debian installer package directly from source:

```bash
./tools/build-deb.sh
```
This produces `xreader-pro_4.8.0-1_amd64.deb` in the project root directory.
