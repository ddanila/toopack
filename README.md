# toopack

Tiny on-demand LZSA2 + x86 BCJ overlay decompressor for 8086 / DOS.

Pack rarely-used code into a blob inside your `.COM`, decompress it into a
DOS-allocated segment on first call, far-call into it. Lets you keep cold
paths out of the precious 64 KB tiny-model segment.

## What this is

- **Resident decoder:** upstream LZSA2 8088 assembly decoder (167 B) + a small BCJ E8/E9 inverse pass.
- **Dispatcher:** `ovly_load(packed, raw_size)` allocates a DOS memory block, decodes, BCJ-unfilters, returns a `far` pointer to offset 0 of the unpacked overlay.
- **Host packer:** `tools/pack.sh raw.bin name name.h` runs the BCJ forward filter, packs with LZSA2 raw-block format, emits a C header with the packed bytes.

## Why these choices

- **LZSA2** — byte-aligned LZ77 with optimal parsing. Hand-tuned 8088 asm decoder is small and fast. ([emmanuel-marty/lzsa](https://github.com/emmanuel-marty/lzsa))
- **BCJ E8/E9 filter** — `CALL rel16` / `JMP rel16` operands are position-dependent, which destroys LZ match-finding on x86 code. Rewriting them to absolute targets pre-pack typically buys 10–25 % extra ratio. Inverse pass is a few bytes. (Filter source: [tukaani-project/xz](https://github.com/tukaani-project/xz) `src/liblzma/simple/x86.c`, public domain.)
- **Tiny model + DOS-allocated overlay segments** — keeps the resident image inside 64 KB; cold blobs unpack into separately allocated paragraphs accessed via `far` pointer.

## Layout

```
src/         decoder library (asm + C)
tools/       host-side packer pipeline + decoder code generator
tests/       test apps + integration tests under kvikdos
vendor/      submodules: lzsa, xz (the latter only for the BCJ source)
```

## Building & testing

Requires OpenWatcom (vendored under `~/fun/beta_kappa/vendor/openwatcom-v2`), NASM (host), and `kvikdos` for the test runner.

```sh
make test
```

Builds every test under `tests/*/` and runs each under kvikdos, diffing stdout against `expected.txt`. Output is a one-line-per-test pass/fail with `.com` size.

## Test sizes (current numbers)

| test | `.com` | what it covers |
|---|---:|---|
| `00_smoke` | 69 B | tiny .COM, our custom CRT, `dos_puts` via `INT 21h AH=09h` |
| `01_alloc` | 103 B | `dos_alloc` via `INT 21h AH=48h`, `--azzy`-free |
| `02_decode_only` | 628 B | LZSA2 decoder + BCJ inverse, no overlay dispatch |
| `03_decode` | 644 B | full roundtrip, byte-compare against embedded raw |
| `04_hello` | 552 B | resident "hello " + cold "world\r\n" via far-call into unpacked overlay |
| `05_cmdloop` | 666 B | stdin-driven dispatcher, packed cold receives `__cdecl` arg |

Cold-blob ratios for the demo overlays (small inputs — see "honest size analysis" below):

| test | cold raw | cold packed | ratio |
|---|---:|---:|---:|
| `04_hello` | 20 B | 24 B | 120 % |
| `05_cmdloop` | 71 B | 74 B | 104 % |

## Honest size analysis

For the demo cold modules (tens of bytes), the LZSA2 raw-block format adds more header overhead than it saves. **Overlays only pay off above some break-even point.** Roughly:

- Resident decoder + dispatcher overhead in our binaries: **~480 B fixed** (LZSA2 asm 167 B + BCJ inverse ~30 B + `ovly_load` ~50 B + DOS alloc machinery + libc-free CRT).
- LZSA2 typically reaches 50–60 % ratio on real x86 code with the BCJ filter applied.

So the break-even is **roughly 1 KB of cold code per overlay** before you actually shave bytes off the resident segment. For a single 2 KB cold path you'd save roughly 1 KB of resident segment at the cost of an 8 KB-paragraph DOS allocation that lives off-segment. Multiple cold paths sharing the resident decoder amortize the overhead further.

The two demos (`04_hello`, `05_cmdloop`) don't save bytes on their own — they exist to validate the plumbing.

## Cold-module ABI

Cold blobs are flat binaries (typically `nasm -f bin`) with these conventions:

- **Entry at offset 0** of the unpacked blob.
- **Caller far-calls in** (`CS = unpacked segment`). The cold module is responsible for setting `DS = CS` if it needs to access its own data.
- **Returns via `RETF`.**
- **Args via `__cdecl`** when needed: stack on entry has `[ret_IP][ret_CS][arg0]...`. See `tests/05_cmdloop/cold.nasm`.
- **No resident-side helpers yet** — cold uses `INT 21h` directly. A function-pointer table from the resident is the obvious next step.

## License

zlib. See `LICENSE`. Vendored sources retain their upstream licenses.
