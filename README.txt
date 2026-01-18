HEIC to WebP Converter
======================

A fast C++ command-line tool to convert HEIC/HEIF images to WebP format.

Uses:
- libheif for HEIC decoding
- libwebp for WebP encoding


PREREQUISITES
-------------

macOS:
  brew install libheif libwebp

  Or use the Makefile:
  make deps

Ubuntu/Debian:
  apt-get install libheif-dev libwebp-dev


BUILD
-----

  make


USAGE
-----

Convert a single file:
  ./heic2webp photo.heic

Convert all HEIC files in a directory:
  ./heic2webp photos/

Convert recursively:
  ./heic2webp photos/ -r -v

Specify output directory:
  ./heic2webp photos/ -o converted/

Adjust quality (1-100, default 85):
  ./heic2webp photo.heic -q 90

Verbose output:
  ./heic2webp photos/ -r -v


OPTIONS
-------

  -o, --output <dir>   Output directory (default: same as input)
  -q, --quality <n>    WebP quality 1-100 (default: 85)
  -r, --recursive      Process directories recursively
  -v, --verbose        Show detailed progress
  -h, --help           Show help message


INSTALL
-------

To install system-wide:
  sudo make install


EXAMPLES
--------

# Convert all photos in current folder with high quality
./heic2webp . -q 95 -v

# Batch convert with output to a specific folder
./heic2webp ~/Pictures/iPhone -r -o ~/Pictures/WebP

# Quick conversion with default settings
./heic2webp photo.heic
