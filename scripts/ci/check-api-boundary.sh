#!/bin/bash
# check-api-boundary.sh — Enforce API boundaries in the monorepo.
#
# Two rules:
#   1. rtorrent may only include public libtorrent headers (torrent/...).
#   2. libtorrent must never include rtorrent headers.
#
# Public headers are those listed in libtorrent/src/torrent/Makefile.am
# with a _HEADERS suffix (installed to $prefix/include/torrent/).
#
# Usage: ./scripts/ci/check-api-boundary.sh [rtorrent-src-dir] [libtorrent-dir]

set -euo pipefail

RTORRENT_DIR="${1:-src}"
LIBTORRENT_DIR="${2:-libtorrent}"

MAKEFILE="${LIBTORRENT_DIR}/src/torrent/Makefile.am"
VIOLATIONS=0

# ── rule 1: rtorrent must only include public libtorrent headers ──

echo "=== Rule 1: rtorrent only includes public libtorrent headers ==="

PUBLIC_HEADERS=$(awk '
  /_include_HEADERS[[:space:]]*=[[:space:]]*\\?$/ {
    sub(/[[:space:]]*\\?$/, ""); sub(/^.*=[[:space:]]*/, "");
    if ($0 != "") print $0
    in_headers=1; next
  }
  in_headers {
    if (/\\$/) { sub(/[[:space:]]*\\$/, ""); gsub(/^[[:space:]]+/, ""); if ($0 != "") print $0 }
    else { gsub(/^[[:space:]]+/, ""); if ($0 != "") print $0; in_headers=0 }
  }
' "$MAKEFILE" | sort -u)

if [ -z "$PUBLIC_HEADERS" ]; then
  echo "ERROR: could not parse public headers from $MAKEFILE"
  exit 1
fi

while IFS= read -r include_line; do
  header_path=$(echo "$include_line" | sed -n 's/.*#include <\(torrent\/[^>]*\)>.*/\1/p')
  [ -z "$header_path" ] && continue

  relative="${header_path#torrent/}"
  if ! echo "$PUBLIC_HEADERS" | grep -qxF "$relative"; then
    echo "  VIOLATION: $include_line"
    echo "    -> $header_path is NOT a public libtorrent header"
    VIOLATIONS=$((VIOLATIONS + 1))
  fi
done < <(grep -rh '#include <torrent/' "${RTORRENT_DIR}/" 2>/dev/null || true)

echo "  OK ($(grep -rh '#include <torrent/' "${RTORRENT_DIR}/" 2>/dev/null | wc -l) includes checked)"

# ── rule 2: libtorrent must not include rtorrent ──

echo ""
echo "=== Rule 2: libtorrent does not include rtorrent ==="

RT_VIOLATIONS=$(grep -rIl '#include.*rtorrent\|#include.*"core/\|#include.*"rpc/\|#include.*"display/\|#include.*"input/\|#include.*"ui/"' \
  "${LIBTORRENT_DIR}/src/" 2>/dev/null || true)

if [ -n "$RT_VIOLATIONS" ]; then
  echo "  VIOLATION: libtorrent files include rtorrent headers:"
  echo "$RT_VIOLATIONS"
  VIOLATIONS=$((VIOLATIONS + $(echo "$RT_VIOLATIONS" | wc -l)))
else
  echo "  OK (no rtorrent includes in libtorrent)"
fi

# ── rule 3: no angled includes to internal libtorrent dirs from rtorrent ──

echo ""
echo "=== Rule 3: rtorrent does not include internal libtorrent dirs ==="

while IFS= read -r include_line; do
  echo "  VIOLATION (internal header): $include_line"
  VIOLATIONS=$((VIOLATIONS + 1))
done < <(grep -rh '#include <dht/\|#include <download/\|#include <protocol/\|#include <tracker/\|#include <net/\|#include <data/\|#include <utils/' \
  "${RTORRENT_DIR}/" 2>/dev/null || true)

echo "  OK (no angled includes to internal libtorrent dirs)"

# ── summary ──

echo ""
if [ "$VIOLATIONS" -eq 0 ]; then
  echo "PASS: All API boundary checks passed."
  exit 0
else
  echo "FAIL: found $VIOLATIONS API boundary violation(s)."
  exit 1
fi
