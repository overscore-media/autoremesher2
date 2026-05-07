#!/usr/bin/env bash
# Clone and pin all third-party git submodules, initialize Geogram nested submodules, and apply AutoRemesher patches to Geogram.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

git submodule sync --recursive
git submodule update --init --recursive

echo "Checking Geogram tag v1.9.9..."
git -C "$ROOT/thirdparty/geogram" fetch --tags origin 2>/dev/null || true
git -C "$ROOT/thirdparty/geogram" checkout v1.9.9
git -C "$ROOT/thirdparty/geogram" submodule update --init --recursive

"$ROOT/scripts/apply-geogram-patches.sh"

echo "bootstrap-submodules: done. Geogram submodule may show local modifications (OpenNL patch); keep them for builds."
