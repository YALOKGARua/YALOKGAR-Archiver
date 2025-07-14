# YALOKGAR Archiver

**YALOKGAR Archiver** is an ultra-fast, modern, multithreaded archiver with advanced Zstandard compression and strong AES-256 encryption. Archive format: `.yal`.

<p align="center">
  <img src="https://img.shields.io/badge/Compression-Zstandard-blue" />
  <img src="https://img.shields.io/badge/Encryption-AES--256--CBC-green" />
  <img src="https://img.shields.io/badge/Multithreaded-Yes-orange" />
  <img src="https://img.shields.io/badge/Format-.yal-purple" />
</p>

---

## 🚀 Features
- Blazing fast compression and extraction for large folders
- Multithreaded file processing for maximum speed
- Modern Zstandard (zstd) compression algorithm
- Strong AES-256-CBC password-based encryption (OpenSSL)
- Real-time progress bar for packing and unpacking
- Cross-platform: Windows & Linux

---

## ⚡ Quick Start

### Build Requirements
- CMake 3.15+
- C++20 compiler (GCC, Clang, MSVC)
- Zstandard (libzstd)
- OpenSSL (libssl)
- pkg-config

```sh
# Clone the repository
 git clone https://github.com/yourname/yalokgar-archiver.git
 cd yalokgar-archiver

# Build (Linux/macOS)
 cmake -S . -B build
 cmake --build build --config Release

# Build (Windows, MSYS2)
 pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-cmake mingw-w64-x86_64-pkg-config mingw-w64-x86_64-zstd mingw-w64-x86_64-openssl
 cmake -S . -B build -G "MinGW Makefiles"
 cmake --build build --config Release
```

---

## 🛠 Usage

```sh
# Pack a folder into a .yal archive
./archiver pack <folder> <archive.yal>

# Pack with encryption
./archiver pack <folder> <archive.yal> --password <your_password>

# Unpack an archive
./archiver unpack <archive.yal> <output_folder>

# Unpack with decryption
./archiver unpack <archive.yal> <output_folder> --password <your_password>
```

### Example

```sh
./archiver pack myfolder backup.yal --password secret123
./archiver unpack backup.yal restored_folder --password secret123
```

---

## 📦 Archive Format
- Signature: `YAL1` (no encryption) or `YALC` (encrypted)
- Metadata: file structure, sizes, offsets
- Data: each file is compressed individually
- With encryption: the entire archive is encrypted (except signature and IV)

---

## 📝 License
MIT 