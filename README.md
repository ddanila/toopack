# toopack

Tiny on-demand LZSA2 + x86 BCJ overlay decompressor for 8086 / DOS.

Pack rarely-used code into a blob inside your `.COM`, decompress it into a
DOS-allocated segment on first call, far-call into it. Lets you keep cold
paths out of the precious 64 KB tiny-model segment.

## What this is

- **Resident decoder:** LZSA2 8088 assembly decoder + ~30 B of E8/E9 fixup.
- **Dispatcher:** allocates a DOS memory block, decodes, BCJ-unfilters, returns a `far` pointer.
- **Host pipeline:** runs the BCJ E8/E9 forward filter on a raw cold-module binary, packs with LZSA2, emits a C header with the packed bytes.

## Why these choices

- **LZSA2** — byte-aligned LZ77 with optimal parsing. Hand-tuned 8088 asm decoder is small and fast. ([emmanuel-marty/lzsa](https://github.com/emmanuel-marty/lzsa))
- **BCJ E8/E9 filter** — `CALL rel16` / `JMP rel16` operands are position-dependent, which destroys LZ match-finding on x86 code. Rewriting them to absolute targets pre-pack typically buys 10–25 % extra ratio. Inverse pass is a few bytes. (Filter source: [tukaani-project/xz](https://github.com/tukaani-project/xz) `src/liblzma/simple/x86.c`, public domain.)
- **Tiny model + DOS-allocated overlay segments** — keeps the resident image inside 64 KB; cold blobs unpack into separately allocated paragraphs accessed via `far` pointer.

## Layout

```
src/         decoder library (asm + C)
tools/       host-side packer pipeline
tests/       test apps (also serve as usage examples)
vendor/      submodules: lzsa, xz
```

## Building

Requires OpenWatcom (vendored under `~/fun/beta_kappa/vendor/openwatcom-v2`) and a host C compiler. Tests run under `kvikdos`.

(Concrete `make` targets land in Phase 2.)

## License

zlib. See `LICENSE`. Vendored sources retain their upstream licenses.
