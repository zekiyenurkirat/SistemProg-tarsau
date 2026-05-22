#!/usr/bin/env bash
# tarsau otomatik test betigi (Linux/Unix)

set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BIN="$ROOT/bin/tarsau"
TEST_DIR="$ROOT/tests"
OUT_DIR="$TEST_DIR/out"
EXTRACT_DIR="$OUT_DIR/extracted"

cd "$ROOT"

echo "=== tarsau testleri basliyor ==="

# 1) Basarili arsivleme ve acma
rm -rf "$OUT_DIR"
mkdir -p "$OUT_DIR" "$EXTRACT_DIR"

"$BIN" -b "$TEST_DIR/sample1.txt" "$TEST_DIR/sample2.txt" -o "$OUT_DIR/test.sau"
"$BIN" -a "$OUT_DIR/test.sau" "$EXTRACT_DIR"

cmp "$TEST_DIR/sample1.txt" "$EXTRACT_DIR/sample1.txt"
cmp "$TEST_DIR/sample2.txt" "$EXTRACT_DIR/sample2.txt"
echo "[OK] Arsivleme ve cikarma"

# 2) Varsayilan a.sau
rm -f "$ROOT/a.sau"
"$BIN" -b "$TEST_DIR/sample3.txt"
test -f "$ROOT/a.sau"
echo "[OK] Varsayilan a.sau"

# 3) Hatali: binary dosya reddi
printf '\x00\x01\x02' > "$OUT_DIR/binary.dat"
if "$BIN" -b "$OUT_DIR/binary.dat" -o "$OUT_DIR/bad.sau" 2>/dev/null; then
  echo "[FAIL] Binary dosya kabul edilmemeliydi"
  exit 1
fi
echo "[OK] Binary dosya reddedildi"

# 4) Hatali: .sau olmayan cikti
if "$BIN" -b "$TEST_DIR/sample1.txt" -o "$OUT_DIR/out.zip" 2>/dev/null; then
  echo "[FAIL] .sau uzanti kontrolu"
  exit 1
fi
echo "[OK] .sau uzanti kontrolu"

# 5) Bozuk arsiv
echo "BOZUK" > "$OUT_DIR/corrupt.sau"
if "$BIN" -a "$OUT_DIR/corrupt.sau" "$OUT_DIR/corrupt_out" 2>/dev/null; then
  echo "[FAIL] Bozuk arsiv reddedilmeliydi"
  exit 1
fi
echo "[OK] Bozuk arsiv reddedildi"

# 6) Eksik parametre
if "$BIN" 2>/dev/null; then
  echo "[FAIL] Parametresiz calisma"
  exit 1
fi
echo "[OK] Parametre kontrolu"

echo "=== Tum testler basarili ==="
