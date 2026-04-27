# toopack

Tiny on-demand LZSA2 + x86 BCJ overlay decompressor for 8086 / DOS.

Pack rarely-used code into a blob inside your `.COM`, decompress it into a DOS-allocated segment on first call, far-call into it. Lets you keep cold paths out of the precious 64 KB tiny-model segment.

## Quick start

```sh
git clone --recursive https://github.com/ddanila/toopack
cd toopack
make test
```

That builds `vendor/kvikdos/kvikdos`, builds every test in `tests/*/`, runs them under kvikdos, and prints a one-line pass/fail report per test.

Requires `nasm` on the host. The OpenWatcom 16-bit cross-compiler and the kvikdos source are vendored in `vendor/`.

## What's in the box

```
src/         decoder library (asm + C)
tools/       host-side packer pipeline + decoder code generator
tests/       integration tests under kvikdos (also serve as usage examples)
vendor/      OpenWatcom V2, kvikdos, lzsa, xz
```

## Test results

| test | `.com` | what it covers |
|---|---:|---|
| `00_smoke` | 69 B | tiny .COM, custom CRT, `INT 21h AH=09h` |
| `01_alloc` | 103 B | `INT 21h AH=48h` allocate |
| `02_decode_only` | 628 B | LZSA2 decoder + BCJ inverse |
| `03_decode` | 644 B | full roundtrip, byte-compare |
| `04_hello` | 552 B | resident `hello ` + cold `world\r\n` via far-call |
| `05_cmdloop` | 666 B | stdin dispatcher, packed cold takes a `__cdecl` arg |

## Further reading

`NOTES.md` covers design rationale (why LZSA2 + BCJ, when overlays actually pay off), the cold-module ABI, and Watcom / kvikdos / wlink gotchas accumulated during the build. Read it before fighting the toolchain.

## License

zlib. See `LICENSE`. Vendored sources retain their upstream licenses.
