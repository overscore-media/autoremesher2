# Building AutoRemesher from source (for Linux)

AutoRemesher is a **Qt 5** application (`core`, `widgets`, `opengl`) built with **qmake**. Core libraries include **Geogram 1.9.9**, **Exploragram**, **Eigen**, **libigl**, **OpenVDB**, **CGAL** (GMP/MPFR), and **OpenEXR**. Sources live under `thirdparty/` as **git submodules**; Geogram is patched for this tree (see below).

This document targets **Linux** (the configuration exercised in `autoremesher.pro`).

---

## 1. Prerequisites

Install development packages for:

| Area | Notes |
|------|--------|
| **Qt 5** | `Qt5Core`, `Qt5Widgets`, `Qt5OpenGL`, `Qt5Gui` — plus OpenGL/Mesa dev headers where your distro splits them out. |
| **OpenVDB** | Headers + shared library; pulls in **OpenEXR**, **Imath**, **TBB**, **zlib** depending on how it was built. |
| **CGAL** | Uses **GMP** and **MPFR**. |
| **Compiler** | C++17-capable **GCC** or **Clang**, **make**, **cmake** (for bundled oneTBB), standard build tools. |

Example package names (adjust for your distribution):

- **Arch / Manjaro:** `base-devel`, `cmake`, `qt5-base`, `openvdb`, `openexr`, `cgal`, `gmp`, `mpfr`
- **Debian / Ubuntu:** `build-essential`, `cmake`, `qtbase5-dev`, `libqt5opengl5-dev`, `libopenvdb-dev`, `libopenexr-dev`, `libcgal-dev`, `libgmp-dev`, `libmpfr-dev`

The `.pro` file links OpenEXR libraries with a **versioned suffix** (e.g. `-lOpenEXR-3_4` as produced on many current distros). If linking fails with “cannot find `-lOpenEXR-…`”, inspect `/usr/lib` (or `pkg-config --libs OpenEXR`) and adjust the `LIBS += ... OpenEXR ...` lines in `autoremesher.pro` to match your system.

---

## 2. Clone and initialize submodules

```bash
git clone <repository-url> autoremesher
cd autoremesher
./scripts/bootstrap-submodules.sh
```

`bootstrap-submodules.sh`:

- Runs `git submodule update --init --recursive`.
- Checks out Geogram at tag **v1.9.9** and initializes its nested submodules.
- Runs **`scripts/apply-geogram-patches.sh`** (OpenNL progress hook, `version.h` for non-CMake builds).

After this, **`thirdparty/geogram`** may show local modifications; that is expected.

Bundled native deps used by the qmake project:

- **zlib** and **oneTBB** — git submodules under `thirdparty/zlib` and `thirdparty/onetbb`, built into `thirdparty/.artifacts/` by `scripts/build-bundled-deps.sh`.
- **Geogram** uses vendored **stb** when **`GEOGRAM_USE_BUILTIN_DEPS`** is defined in `autoremesher.pro`.

---

## 3. Bundled zlib + oneTBB (recommended on Linux)

Linking your app against a **different** TBB than the one `libopenvdb.so` was built with often causes runtime crashes. Building zlib + oneTBB from the pinned submodules and letting OpenVDB keep using **system** OpenVDB (but not system `-ltbb`/`-lz`) avoids that mismatch.

Once submodules exist:

```bash
./scripts/build-bundled-deps.sh
```

This produces **`thirdparty/.artifacts/.stamp`**. When that stamp exists, `autoremesher.pro` adds bundled **onetbb** and **zlib** and links OpenVDB + OpenEXR + CGAL from the system paths configured in the `.pro` file.

Without the stamp, if **pkg-config** provides **`openvdb`**, the project uses `PKGCONFIG += openvdb`; otherwise it falls back to manual `LIBS` including system `-ltbb` and `-lz`.

Environment variable **`JOBS`** overrides parallelism for the bundled builds (default: CPU count).

---

## 4. Configure and compile

Release build (matches `CONFIG += release` in the `.pro` file):

```bash
qmake autoremesher.pro CONFIG+=release
make -j"$(nproc)"
```

The executable is **`./autoremesher`** in the project root (see `target.path` in `autoremesher.pro`).

**GCC:** `autoremesher.pro` passes **`-Ulinux -Uunix`** on Linux so QtAwesome’s `fa::linux` symbol does not collide with the `linux` preprocessor macro.

---

## 5. Run

GUI:

```bash
./autoremesher
```

Batch / CLI (same binary):

```bash
./autoremesher --cli -o out.obj input.obj
```

Use `./autoremesher --help` for options after `--cli`.

---

## 6. Icons and desktop integration (optional)

Raster icons are generated from **`autoremesher.svg`**:

```bash
./scripts/generate-logos-from-svg.sh
```

Default output: **`packaging/appimage/autoremesher.png`** (also referenced from **`resources.qrc`**) and **`autoremesher.png`** at the repo root.

On **Wayland**, the panel icon usually comes from the **desktop entry** + Freedesktop icon theme, not only from `QWindowIcon`. To register a user-local entry and icon:

```bash
./scripts/install-user-desktop.sh
```

---

## 7. AppImage (optional)

After a **release** build of `autoremesher`:

```bash
./packaging/appimage/package-appimage.sh
```

Requires **`linuxdeployqt`** or **`linuxdeploy`** (+ Qt plugin) on `PATH`. See comments at the top of `packaging/appimage/package-appimage.sh`. The AppDir uses **`packaging/appimage/org.autoremesher.AutoRemesher.desktop`** and the PNG icon under `packaging/appimage/`.

---

## 8. Version strings

Human-readable and numeric versions default in **`autoremesher.pro`** (`HUMAN_VERSION`, `VERSION`). Override when invoking qmake if needed:

```bash
qmake autoremesher.pro CONFIG+=release HUMAN_VERSION=2.0.1 VERSION=2.0.1.0
```

**`GEOGRAM_VERSION`** is fixed for this tree (**1.9.9**) in the `.pro` file and must stay aligned with the Geogram submodule tag.

---

## 9. Troubleshooting

| Symptom | Things to check |
|--------|------------------|
| Missing submodule directories | Run `./scripts/bootstrap-submodules.sh`. |
| OpenEXR / Ilm link errors | Match `-lOpenEXR-*`, `-lIlmThread-*`, etc., to filenames under `/usr/lib` or pkg-config output. |
| OpenVDB / TBB crashes at runtime | Prefer `./scripts/build-bundled-deps.sh` so the app does not mix distro TBB with VDB’s expected TBB. |
| Qt not found | Install Qt 5 dev packages; if qmake is not default, use full path, e.g. `/usr/lib/qt5/bin/qmake`. |

---

## 10. Cleaning

```bash
make distclean
# or remove build artifacts manually: rm -rf build/ Makefile .qmake.stash
```

Regenerate the Makefile after changing `autoremesher.pro`:

```bash
qmake autoremesher.pro CONFIG+=release
```
