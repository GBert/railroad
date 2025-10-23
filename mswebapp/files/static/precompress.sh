#!/bin/sh
# Precompresses static assets with gzip for fast delivery on low-power devices (Omega2+).
# - Keeps original files (-k)
# - Strips timestamps (-n) for reproducible builds
# - Uses maximum compression (-9)
#
# Usage inside your Docker/OpenWrt build before packaging files into the ipk:
#   sh packaging/openwrt/precompress.sh
#
# The backend will automatically serve *.gz when the client sends Accept-Encoding: gzip.

set -eu

REPO_ROOT="$(CDPATH= cd -- "$(dirname -- "$0")"/../.. && pwd)"
STATIC_DIR="$REPO_ROOT/src/frontend/static"

compress() {
  f="$1"
  if [ -f "$f" ]; then
    printf 'Compressing %s -> %s.gz\n' "$f" "$f"
    gzip -k -n -9 "$f"
  fi
}

# Core assets
compress "$STATIC_DIR/style.css"
compress "$STATIC_DIR/script.js"

# Optionally compress other large, rarely changed assets (uncomment as needed)
# compress "$STATIC_DIR/grafics/manifest.webmanifest"
# compress "$STATIC_DIR/grafics/*.svg"

printf 'Precompression done.\n'