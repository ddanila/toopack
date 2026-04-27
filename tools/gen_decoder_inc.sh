#!/bin/sh
# Pre-assemble the LZSA2 8088 decoder (NASM syntax, upstream) into a flat
# binary, then emit a wasm-compatible .inc file of `db` statements. The
# wrapper at src/unlzsa2.asm includes this and exports lzsa2_decompress.
#
# Why bother: we tried linking NASM's .obj directly, but wlink doesn't
# apply Watcom's `org 100h` shift to NASM-emitted symbols, so the call
# fixup is 0x100 bytes too low. Round-tripping through `-f bin` and
# rebuilding via wasm puts the bytes inside Watcom's _TEXT under the
# correct org. See NOTES.md.

set -eu

SRC=${1:-}
OUT=${2:-}

if [ -z "$SRC" ] || [ -z "$OUT" ]; then
    echo "usage: $0 <decompress_small_v2.S> <output.inc>" >&2
    exit 1
fi

TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT

nasm -f bin -o "$TMP/decoder.bin" "$SRC"

# Emit one db per 12 bytes, MASM/wasm hex syntax (NNh with leading 0 if needed).
python3 - "$TMP/decoder.bin" <<'PY' > "$OUT"
import sys
data = open(sys.argv[1], 'rb').read()
print(f'; Auto-generated from decompress_small_v2.S — do not edit.')
print(f'; {len(data)} bytes of LZSA2 decoder, NASM-assembled, wasm-included.')
print()
WIDTH = 12
for i in range(0, len(data), WIDTH):
    row = data[i:i+WIDTH]
    print('        db ' + ', '.join(f'0{b:02X}h' for b in row))
PY
