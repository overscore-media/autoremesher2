#!/usr/bin/env bash
# Install desktop entry + hicolor icon under ~/.local/share so Wayland/KDE/GNOME can show the real icon when you run a locally built ./autoremesher (not only AppImage).
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BINARY="${1:-${ROOT}/autoremesher}"
PNG="${ROOT}/packaging/appimage/autoremesher.png"
DESKTOP_ID="org.autoremesher.AutoRemesher"
if [[ ! -x "${BINARY}" ]]; then
  echo "Usage: $0 [/path/to/autoremesher]"
  echo "Default binary ${BINARY} not found or not executable."
  exit 1
fi
if [[ ! -f "${PNG}" ]]; then
  echo "Missing ${PNG}"
  exit 1
fi
ABS_BIN="$(readlink -f "${BINARY}")"
mkdir -p "${HOME}/.local/share/applications" "${HOME}/.local/share/icons/hicolor/256x256/apps"
cp -a "${PNG}" "${HOME}/.local/share/icons/hicolor/256x256/apps/${DESKTOP_ID}.png"
cat > "${HOME}/.local/share/applications/${DESKTOP_ID}.desktop" <<EOF
[Desktop Entry]
Type=Application
Name=AutoRemesher
Icon=${DESKTOP_ID}
Exec=${ABS_BIN}
Categories=Graphics;
Comment=Automatic quad remeshing tool
StartupWMClass=autoremesher
EOF
echo "Installed ~/.local/share/applications/${DESKTOP_ID}.desktop"
echo "Installed icon ~/.local/share/icons/hicolor/256x256/apps/${DESKTOP_ID}.png"
echo "Run: gtk-update-icon-cache -f -t ~/.local/share/icons/hicolor 2>/dev/null || true"
