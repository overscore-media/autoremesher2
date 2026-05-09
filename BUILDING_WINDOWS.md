# Building AutoRemesher on Windows

## Prerequisites

- **MSYS2** with **MINGW64** environment (install via [scoop](https://scoop.sh): `scoop install msys2 mingw`)
- Open a **MINGW64** shell (run `mingw64.exe` from the MSYS2 installation, or use `MSYSTEM=MINGW64 bash`)

## Install dependencies

```bash
pacman -S --noconfirm \
  mingw-w64-x86_64-gcc \
  mingw-w64-x86_64-make \
  mingw-w64-x86_64-cmake \
  mingw-w64-x86_64-qt5 \
  mingw-w64-x86_64-openvdb \
  mingw-w64-x86_64-openexr \
  mingw-w64-x86_64-imath \
  mingw-w64-x86_64-gmp \
  mingw-w64-x86_64-mpfr \
  mingw-w64-x86_64-cgal \
  mingw-w64-x86_64-tbb \
  mingw-w64-x86_64-zlib
```

## Initialize submodules

```bash
cd /path/to/autoremesher2
git submodule update --init --recursive
```

## Apply geogram patch

```bash
./scripts/apply-geogram-patches.sh
```

## Build

```bash
qmake-qt5 autoremesher.pro CONFIG+=release
mingw32-make -j$(nproc)
```

The binary is written to `build/autoremesher.exe`.

## Run

Add the MSYS2 mingw64 bin directory to your PATH first:

```bash
export PATH=/mingw64/bin:$PATH
./build/autoremesher.exe
```

## Standalone bundle

To collect all required DLLs and Qt plugins into `dist/` for redistribution:

```bash
./scripts/bundle-windows-dlls.sh
```

This creates `dist/autoremesher.exe` with all dependencies — runnable without any extra setup.
