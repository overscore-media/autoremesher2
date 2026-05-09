#!/usr/bin/env bash
set -euo pipefail
cd "$(cd "$(dirname "$0")/.." && pwd)"

SRC=build
DST=dist

rm -rf "$DST"
mkdir -p "$DST"
cp "$SRC/autoremesher.exe" "$DST/"

# Copy system/MinGW DLLs by scanning dependencies recursively
changed=1
round=0

while [[ "$changed" -eq 1 && "$round" -lt 40 ]]; do
  changed=0
  round=$((round + 1))

  while IFS= read -r -d '' f; do
    [[ -f "$f" ]] || continue
    while read -r dll; do
      dll="${dll//$'\r'/}"
      [[ -z "$dll" ]] && continue
      [[ "${dll,,}" == *.dll ]] || continue
      [[ -f "/mingw64/bin/$dll" ]] || continue
      if [[ ! -f "$DST/$dll" ]]; then
        cp "/mingw64/bin/$dll" "$DST/$dll"
        changed=1
      fi
    done < <(objdump -p "$f" 2>/dev/null | awk '/DLL Name:/{print $NF}' || true)
  done < <(find "$DST" -type f \( -name '*.exe' -o -name '*.dll' \) -print0)
done

# Copy Qt plugins
QTP=/mingw64/share/qt5/plugins

for dir in platforms styles iconengines imageformats; do
  mkdir -p "$DST/$dir"
  for f in "$QTP/$dir"/*.dll; do
    [[ -f "$f" ]] && cp "$f" "$DST/$dir/"
  done
done

echo "=== dist/ ==="
ls -la "$DST" | grep -v "^\."
echo "=== $(find "$DST" -name '*.dll' | wc -l) DLLs total ==="
