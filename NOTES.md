# Build & runtime findings

Hard-won lessons. Read this before fighting the toolchain again.

## Watcom

### Outputs `.o`, not `.obj` on macOS

The macOS-arm64 wcc/wasm builds emit `.o` files. `.gitignore` and `Makefile`s must use `.o`. (The Watcom 16-bit OMF format is the same — only the extension differs.)

### `cstart_t.obj` is incompatible with kvikdos

Stock Watcom tiny-COM startup calls `INT 21h AH=4Ah` to shrink the program block to a value kvikdos rejects (asks for ~10 paragraphs). Fix: drop the libc startup, write our own (see `tests/00_smoke/start.asm`). We don't need libc anyway — `dos.h` wraps the int 21h services we use via `#pragma aux`.

### Tiny `.COM` link incantation

`wlink format dos com` requires:

- `DGROUP group _TEXT, CONST, CONST2, _DATA, _BSS` in the startup `.asm`.
- `org 100h` at the top of `_TEXT`. **Without `org 100h`, all `OFFSET` fixups land 0x100 bytes too low** — strings print PSP garbage. Discovered via `mov dx, 0x004` instead of `0x114`.
- `wlink option offset=100h` is **rejected** for COM format; the org has to come from the assembler, not the linker.

### Watcom `pragma aux` to call NASM/external assembly

```c
extern void lzsa2_decompress(void);
#pragma aux lzsa2_decompress "*"        \
    modify [ax bx cx dx si di bp es];

extern unsigned int unlzsa2(...);
#pragma aux unlzsa2 =                   \
    "call lzsa2_decompress"             \
    parm [si] [es di]                   \
    value [ax]                          \
    modify [si di ax bx cx dx bp es];
```

The `"*"` aux name on the extern suppresses Watcom's name decoration. Without it, Watcom appends `_` and the link fails.

## kvikdos

### `--azzy` relaxes MCB validation

kvikdos enforces stricter MCB invariants than real DOS. Key one for us: **two adjacent free MCBs are illegal**, even though real DOS / DOSBox / FreeDOS tolerate it. After our shrink (AH=4Ah) creates a free trailing block, kvikdos can return reason `10` from `is_mcb_bad`.

Workaround: pass `--azzy` to kvikdos, which sets `g_azzy=1` and skips most MCB sanity checks.

For real-DOS or QEMU testing, this is a non-issue.

### Memory shrink for AH=48h to work

`.COM` starts owning all conventional memory (the `Z` block spans to `0xa000`). `INT 21h AH=48h` (allocate) needs *free* memory — we have to shrink first via `AH=4Ah`. Two requirements:

1. **Move SP into the new block** before issuing AH=4Ah, otherwise stack pushes go past the now-released memory: `mov sp, (PARAGRAPHS * 16) - 2`.
2. **Shrink size must accommodate code + data + stack**. Our resident binaries are <1 KB so 512 paragraphs (8 KB) is comfortable. Bump if your test grows.

### Debug build of kvikdos

`make clean && make XCFLAGS="-DDEBUG_ALLOC=1" kvikdos` (in `~/fun/kvikdos/`) emits per-INT-21h-malloc/realloc trace lines on stderr. Indispensable for diagnosing MCB issues.

The canonical reference test is `~/fun/kvikdos/malloct.com` (built from `malloct.nasm`) — known-good shrink + alloc + free pattern.

## LZSA + Watcom integration: open issue

**Status: not yet working in `tests/03_decode`.**

`vendor/lzsa/asm/8088/decompress_small_v2.S` is **NASM syntax** (the `LZSA2FTA.ASM` is TASM IDEAL — Watcom's `wasm` does not accept it). NASM with `-f obj` produces a Watcom-compatible OMF object.

The bug: when wlink merges Watcom's `_TEXT` (with `org 100h`) and NASM's segment (no org), the `OFFSET lzsa2_decompress` fixup ends up **0x100 bytes too low**:

- Call site: `e8 1c 01` from `0x199` → target `0x2B8`.
- Actual decoder location: `0x3B8` (in CS-relative display).
- Calling 0x2B8 lands in zero-padding; CPU executes `add [bx+si],al` repeatedly, eventually producing junk state, and (somehow) ends up issuing `INT 21h AH=4Ah` on the just-allocated overlay segment, corrupting the MCB chain.

The runtime symptom under kvikdos `--azzy` is the loop you see in the alloc-fail trace: alloc → realloc-our-overlay → alloc-again → `alloc fail`.

### Things we tried, all failed

- Putting NASM-emitted code in a separate segment `_LZSA_TEXT` grouped into DGROUP — wlink still treats NASM offsets as 0-based, so the 0x100 mismatch persists.
- `times 0x100 db 0` padding at the start of NASM's segment — shifts both the symbol *and* the bytes; net offset still off.
- `wlink ORDER ... OFFSET 100h SEGMENT _LZSA_TEXT` — wlink rejects the directive in `format dos com` mode.

### Routes forward (decision needed)

1. **Convert `decompress_small_v2.S` from NASM to wasm syntax.** ~145 lines, mostly mechanical (`segment .text` → `_TEXT segment`, NASM `.local` labels → unique names, etc.). Most reliable; keeps everything in one assembler. Trade: maintenance fork from upstream.
2. **Generate a wasm-compatible OBJ that contains the decoder bytes inline.** Pre-assemble with `nasm -f bin`, then auto-generate a `db` listing in a wasm source file. Build glue, but keeps upstream pristine.
3. **Switch from kvikdos to QEMU + FreeDOS** for testing — doesn't fix the build offset issue (which is wlink/NASM, not kvikdos), but rules out kvikdos quirks contaminating diagnosis.
4. **Use the Watcom-syntax `LZSA2FTA.ASM` after porting from TASM IDEAL to wasm-MASM.** Similar effort to (1) but starting from a different source.

(1) is probably the cleanest. (2) is the lowest-risk to upstream. (3) addresses kvikdos but not the core bug.

## Misc

### Useful debugging incantations

```sh
# Disassemble a tiny COM
ndisasm -o 0x100 -b 16 path/to/foo.com

# All int 0x21 sites with the preceding mov ah
ndisasm -o 0x100 -b 16 foo.com | awk '
/mov ah,/ { ah=$0 }
/int byte 0x21|int 0x21/ { print ah; print $0; print "" }'

# Find a byte sequence in the binary
xxd -p foo.com | tr -d '\n' | grep -ob 'b44a'   # divide by 2 for byte offset

# kvikdos with debug
~/fun/kvikdos/kvikdos --azzy foo.com
```
