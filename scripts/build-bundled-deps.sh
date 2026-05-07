#!/usr/bin/env bash
# Build zlib (static) and oneTBB (shared) from git submodules into thirdparty/.artifacts/ so qmake can link them instead of system -ltbb / -lz.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
ART="$ROOT/thirdparty/.artifacts"
ZSRC="$ROOT/thirdparty/zlib"
TSRC="$ROOT/thirdparty/onetbb"

for d in "$ZSRC" "$TSRC"; do
  if [[ ! -d "$d" ]]; then
    echo "Missing $d — run: git submodule update --init --recursive" >&2
    exit 1
  fi
done

mkdir -p "$ART"
rm -f "$ART/.stamp"

JOBS="${JOBS:-$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)}"

echo "=== zlib (static) → $ART/zlib ==="
cd "$ZSRC"
if [[ -f Makefile ]]; then
  make distclean 2>/dev/null || true
fi
./configure --prefix="$ART/zlib" --static
make -j"$JOBS"
make install

echo "=== oneTBB → $ART/onetbb ==="
cmake -S "$TSRC" -B "$ART/onetbb-build" \
  -DCMAKE_INSTALL_PREFIX="$ART/onetbb" \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_INSTALL_LIBDIR=lib \
  -DTBB_TEST=OFF \
  -DTBB_STRICT=OFF \
  -DCMAKE_POSITION_INDEPENDENT_CODE=ON \
  -DBUILD_SHARED_LIBS=ON \
  -DTBB_ENABLE_IPO=OFF

cmake --build "$ART/onetbb-build" --parallel "$JOBS"
cmake --install "$ART/onetbb-build"

: >"$ART/.stamp"
echo "Bundled deps ready: $ART/.stamp"
