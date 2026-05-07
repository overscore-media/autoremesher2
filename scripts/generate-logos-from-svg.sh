#!/usr/bin/env bash
# Regenerate raster logos from autoremesher.svg (canonical source).
# Outputs: packaging/appimage/autoremesher.png (embedded via resources.qrc) and autoremesher.png.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
SVG="${ROOT}/autoremesher.svg"
OUT_APP="${ROOT}/packaging/appimage/autoremesher.png"
OUT_ROOT="${ROOT}/autoremesher.png"
SIZE="${LOGO_SIZE:-256}"

if [[ ! -f "${SVG}" ]]; then
  echo "Missing ${SVG}"
  exit 1
fi

mkdir -p "$(dirname "${OUT_APP}")"

if command -v inkscape >/dev/null 2>&1; then
  inkscape --batch-process \
    --export-type=png \
    --export-filename="${OUT_APP}" \
    -w "${SIZE}" -h "${SIZE}" \
    "${SVG}"
else
  magick -density 384 "${SVG}" -resize "${SIZE}x${SIZE}" -background none "PNG32:${OUT_APP}"
fi

cp -f "${OUT_APP}" "${OUT_ROOT}"
echo "Wrote ${OUT_APP} (${SIZE}×${SIZE})"
echo "Wrote ${OUT_ROOT} (copy)"
