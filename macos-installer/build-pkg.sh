#!/bin/bash
set -euo pipefail

# Build a native macOS product installer for Yahoo! KeyKey 2 using pkgbuild/productbuild.
# This rewrites the installer packaging in a modern/native way while preserving the
# original install behavior (payload -> /Library/Input Methods, scripts, license, restart).
#
# IMPORTANT: Running natively on Apple-Silicon-only macOS also requires arm64 (or universal)
# app/framework/helper binaries inside the payload. This script packages whatever payload
# you provide; it does not by itself recompile the legacy KeyKey binaries.

ROOT="$(cd "$(dirname "$0")" && pwd)"
REPO_ROOT="$(cd "$ROOT/.." && pwd)"
PAYLOAD_ROOT="${1:-$ROOT/payload}"
OUT_DIR="${2:-$ROOT/out}"
PKG_ID="com.yahoo.keykey2.installerpackage"
VERSION="${KEYKEY2_VERSION:-1.1.2}"
COMPONENT_PKG="KeyKey2App.pkg"
PRODUCT_PKG="KeyKey2.pkg"

mkdir -p "$OUT_DIR"

if [ ! -d "$PAYLOAD_ROOT/Library/Input Methods/Yahoo! KeyKey.app" ]; then
  echo "error: expected payload at: $PAYLOAD_ROOT/Library/Input Methods/Yahoo! KeyKey.app" >&2
  echo "hint: extract orig/YahooKeyKey.pkg or provide a rebuilt arm64 payload root" >&2
  exit 1
fi

# Component package
pkgbuild \
  --root "$PAYLOAD_ROOT" \
  --identifier "$PKG_ID" \
  --version "$VERSION" \
  --install-location "/" \
  --scripts "$ROOT/scripts" \
  "$OUT_DIR/$COMPONENT_PKG"

# Product package with original-like behavior
productbuild \
  --distribution "$ROOT/distribution.xml" \
  --resources "$ROOT/resources" \
  --package-path "$OUT_DIR" \
  "$OUT_DIR/$PRODUCT_PKG"

echo "Built: $OUT_DIR/$PRODUCT_PKG"
