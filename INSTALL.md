# Installing & Building Xreader Pro

## Option 1: Standalone Debian Package (`.deb`) — Recommended

The easiest way to install or update Xreader Pro on Ubuntu, Debian, Linux Mint, or LMDE:

```bash
# 1. Download the pre-built .deb from GitHub Releases
wget https://github.com/yuri-schmaltz/xreader_pro/releases/download/v4.8.0-1/xreader-pro_4.8.0-1_amd64.deb

# 2. Install using dpkg
sudo dpkg -i xreader-pro_4.8.0-1_amd64.deb

# 3. Resolve any missing shared runtime libraries
sudo apt-get install -f
```

---

## Option 2: Generating a Fresh `.deb` Package Locally

You can generate your own standalone `.deb` package containing all backends, schemas, and icons using the bundled build script:

```bash
# 1. Clone repository and install build tools
git clone https://github.com/yuri-schmaltz/xreader_pro.git
cd xreader_pro

# 2. Run the automated Debian package builder
./tools/build-deb.sh

# 3. Install the generated package
sudo dpkg -i xreader-pro_4.8.0-1_amd64.deb
sudo apt-get install -f
```

---

## Option 3: Building & Installing from Source (Meson + Ninja)

### 1. Install Build Dependencies

#### On Debian / Ubuntu / Linux Mint:
```bash
sudo apt update
sudo apt install -y git meson ninja-build build-essential \
    libgtk-3-dev libglib2.0-dev libpoppler-glib-dev libspectre-dev \
    libtiff-dev libgxps-dev libwebkit2gtk-4.1-dev libsecret-1-dev \
    libxapp-dev libarchive-dev libkpathsea-dev libtesseract-dev \
    tesseract-ocr tesseract-ocr-eng tesseract-ocr-por
```

#### For Modern GTK4 Build (Optional):
```bash
sudo apt install -y libgtk-4-dev
```

### 2. Configure with Meson
```bash
# Standard GTK3 Release Build:
meson setup build --prefix=/usr --buildtype=release

# Alternatively, Modern GTK4 Build:
meson setup build-gtk4 --prefix=/usr --buildtype=release -Dgtk_version=4
```

### 3. Compile and Run Test Suite
```bash
# Build
ninja -C build

# Run automated tests
meson test -C build --verbose
```

### 4. Install System-Wide
```bash
sudo ninja -C build install
sudo ldconfig
sudo glib-compile-schemas /usr/share/glib-2.0/schemas
sudo update-desktop-database
```

### 5. Running Xreader Pro
```bash
xreader [FILE1.pdf] [FILE2.pdf]
```

