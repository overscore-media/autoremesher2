#!/usr/bin/env bash
# Apply AutoRemesher-specific additions to the Geogram submodule (non-CMake qmake builds):
# - generated geogram/version.h (normally produced by CMake / MakeMake.sh)
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
GEO="$ROOT/thirdparty/geogram"
VERSION_H="$GEO/src/lib/geogram/version.h"

if [[ ! -d "$GEO" ]]; then
  echo "apply-geogram-patches: missing $GEO (initialize submodules first)." >&2
  exit 1
fi

if [[ ! -d "$GEO/src/lib/geogram" ]]; then
  echo "apply-geogram-patches: geogram sources not checked out — run: git submodule update --init --recursive in thirdparty/geogram" >&2
  exit 1
fi

if [[ ! -f "$VERSION_H" ]]; then
  mkdir -p "$(dirname "$VERSION_H")"
  cat >"$VERSION_H" << 'EOF'
#ifndef GEOGRAM_BASIC_VERSION
#define GEOGRAM_BASIC_VERSION

#define VORPALINE_VERSION_MAJOR "1"
#define VORPALINE_VERSION_MINOR "9"
#define VORPALINE_VERSION_PATCH "9"
#define VORPALINE_VERSION "1.9.9"
#define VORPALINE_BUILD_NUMBER ""
#define VORPALINE_BUILD_DATE ""
#define VORPALINE_SVN_REVISION ""

#endif
EOF
  echo "apply-geogram-patches: wrote geogram/version.h (CMake-generated file stub)."
fi