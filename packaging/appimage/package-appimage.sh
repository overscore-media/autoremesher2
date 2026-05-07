#!/usr/bin/env bash
# Build an AppDir and optionally produce an AppImage using linuxdeployqt / linuxdeploy.
# Prerequisites: release build of autoremesher, Qt 5, and on PATH one of:
#   linuxdeployqt (from https://github.com/probonopd/linuxdeployqt), or
#   linuxdeploy + linuxdeploy-plugin-qt (https://github.com/linuxdeploy).
#
# Bundle OpenVDB / OpenEXR / TBB / zlib consistently with how you built the binary
# (see BUILDING.md and scripts/build-bundled-deps.sh).
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
BINARY="${1:-${ROOT}/autoremesher}"
OUT_ROOT="${APPIMG_OUT:-${ROOT}/packaging/appimage/dist}"
APPDIR="${OUT_ROOT}/AppDir"
DESKTOP="${ROOT}/packaging/appimage/org.autoremesher.AutoRemesher.desktop"
METAINFO="${ROOT}/packaging/appimage/autoremesher.metainfo.xml"

if [[ ! -f "${BINARY}" || ! -x "${BINARY}" ]]; then
  echo "Usage: $0 [/path/to/autoremesher]"
  echo "Default binary ${ROOT}/autoremesher not found or not executable."
  echo "Build release first, e.g.:"
  echo "  cd \"${ROOT}\" && ./scripts/build-bundled-deps.sh"
  echo "  qmake autoremesher.pro CONFIG+=release && make -j\"\$(nproc)\""
  exit 1
fi

if [[ ! -f "${DESKTOP}" ]]; then
  echo "Missing ${DESKTOP}"
  exit 1
fi

ICON_SRC="${ROOT}/packaging/appimage/autoremesher.png"
if [[ ! -f "${ICON_SRC}" ]]; then
  echo "Missing ${ICON_SRC} (256×256 PNG matching Icon= in the .desktop file)."
  exit 1
fi

rm -rf "${APPDIR}"
mkdir -p "${APPDIR}/usr/bin"
mkdir -p "${APPDIR}/usr/share/applications"
mkdir -p "${APPDIR}/usr/share/icons/hicolor/256x256/apps"

cp -a "${BINARY}" "${APPDIR}/usr/bin/autoremesher"
cp -a "${DESKTOP}" "${APPDIR}/usr/share/applications/"
# appimagetool resolves Icon= against the AppDir root; keep a copy there too.
cp -a "${ICON_SRC}" "${APPDIR}/org.autoremesher.AutoRemesher.png"
cp -a "${ICON_SRC}" "${APPDIR}/usr/share/icons/hicolor/256x256/apps/org.autoremesher.AutoRemesher.png"
if [[ -f "${METAINFO}" ]]; then
  mkdir -p "${APPDIR}/usr/share/metainfo"
  cp -a "${METAINFO}" "${APPDIR}/usr/share/metainfo/"
fi

DESKTOP_ID="$(basename "${DESKTOP}")"
DESKTOP_ID="${DESKTOP_ID%.desktop}"
DESKTOP_IN_APPDIR="${APPDIR}/usr/share/applications/${DESKTOP_ID}.desktop"

echo "AppDir ready: ${APPDIR}"
echo "Binary: $(readlink -f "${APPDIR}/usr/bin/autoremesher")"

if command -v linuxdeployqt >/dev/null 2>&1; then
  echo "Running linuxdeployqt..."
  # linuxdeployqt/appimagetool write the bundle to the current directory.
  (cd "${OUT_ROOT}" && exec linuxdeployqt "${APPDIR}/usr/bin/autoremesher" \
    -desktopfile="${DESKTOP_IN_APPDIR}" \
    -bundle-non-qt-libs \
    -appimage \
    -verbose=2)
  exit 0
fi

if command -v linuxdeploy >/dev/null 2>&1; then
  echo "Running linuxdeploy + Qt plugin..."
  # Bundled strip in many linuxdeploy AppImages cannot handle ELF with .relr.dyn
  # (modern GCC/binutils). Skip stripping unless explicitly overridden.
  export NO_STRIP="${NO_STRIP:-1}"
  export QMAKE="${QMAKE:-qmake}"
  (cd "${OUT_ROOT}" && linuxdeploy \
    --appdir "${APPDIR}" \
    --executable "${APPDIR}/usr/bin/autoremesher" \
    --desktop-file "${DESKTOP_IN_APPDIR}" \
    --icon-file "${ICON_SRC}" \
    --plugin qt \
    --output appimage)
  exit 0
fi

echo ""
echo "Neither linuxdeployqt nor linuxdeploy was found in PATH."
echo "Install one of them, then re-run this script. AppDir is already prepared at:"
echo "  ${APPDIR}"
echo ""
echo "Manual example (adjust path to your Qt qmake):"
echo "  export QMAKE=/path/to/Qt/5.15.2/gcc_64/bin/qmake"
echo "  linuxdeployqt \"${APPDIR}/usr/bin/autoremesher\" -bundle-non-qt-libs -appimage"
exit 1
