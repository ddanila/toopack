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

### Watcom `pragma aux` — calling external assembly

Two-step declaration: extern with `"*"` to suppress name decoration, then the call wrapper.

```c
extern void lzsa2_decompress(void);
#pragma aux lzsa2_decompress "*"        \
    modify [ax bx cx dx si di bp es];

extern unsigned int unlzsa2(...);
#pragma aux unlzsa2 =                   \
    "push bp"                           \
    "call lzsa2_decompress"             \
    "pop bp"                            \
    parm [si] [es di]                   \
    value [ax]                          \
    modify [si di ax bx cx dx es];
```

### **CRITICAL: Watcom does NOT honour BP in the `modify` clause**

If `[bp]` appears in `modify`, Watcom assumes BP is clobbered — but it does *not* save/restore BP for you. The caller's stack frame is destroyed. All subsequent `[bp-X]` accesses (including the supposed return value of the function) read garbage from random stack offsets.

The LZSA decoder zeroes BP and uses it as a 32-bit accumulator. Without `push bp / pop bp` around the `call`, after the decoder returns:

- `seg` reads back as `0x0000` instead of the real allocation result.
- Reads through `__far *` based on `seg` go to segment 0 (low memory).
- The decoded output looks correct in the alloc'd segment — but you can't see it through the (now-bogus) far pointer.

The fix: wrap the `call` in `"push bp" / "pop bp"` and remove BP from `modify`. Always do this when calling assembly that uses BP for non-frame purposes.

## kvikdos

### `--azzy` relaxes MCB validation

kvikdos enforces stricter MCB invariants than real DOS. Key one for us: **two adjacent free MCBs are illegal**, even though real DOS / DOSBox / FreeDOS tolerate it. After our shrink (AH=4Ah) creates a free trailing block, kvikdos can return reason `10` from `is_mcb_bad`.

In practice: with the BP fix above, the spurious `INT 21h AH=4Ah` calls vanish and kvikdos accepts our memory pattern *without* `--azzy`. We only saw the MCB error because corrupted BP led to corrupted execution which led to phantom DOS calls.

`--azzy` is still useful as a diagnostic — if a test passes with `--azzy` and fails without, you've got more BP-corruption-style bugs hiding.

For real-DOS or QEMU testing, `--azzy` is moot.

### Memory shrink for AH=48h to work

`.COM` starts owning all conventional memory (the `Z` block spans to `0xa000`). `INT 21h AH=48h` (allocate) needs *free* memory — we have to shrink first via `AH=4Ah`. Two requirements:

1. **Move SP into the new block** before issuing AH=4Ah, otherwise stack pushes go past the now-released memory: `mov sp, (PARAGRAPHS * 16) - 2`.
2. **Shrink size must accommodate code + data + stack**. Our resident binaries are <1 KB so 512 paragraphs (8 KB) is comfortable. Bump if your test grows.

### Debug build of kvikdos

`make clean && make XCFLAGS="-DDEBUG_ALLOC=1" kvikdos` (in `~/fun/kvikdos/`) emits per-INT-21h-malloc/realloc trace lines on stderr. Indispensable for diagnosing MCB issues.

The canonical reference test is `~/fun/kvikdos/malloct.com` (built from `malloct.nasm`) — known-good shrink + alloc + free pattern.

## LZSA + Watcom integration

### Build flow

`vendor/lzsa/asm/8088/decompress_small_v2.S` is **NASM syntax** (the `LZSA2FTA.ASM` is TASM IDEAL — Watcom's `wasm` does not accept it). Linking the NASM-emitted `.obj` directly *fails*: when wlink merges Watcom's `_TEXT` (with `org 100h`) and NASM's segment (no org), the call-site fixup for `lzsa2_decompress` lands 0x100 bytes too low.

**Workaround:** round-trip through a flat binary. `tools/gen_decoder_inc.sh` runs `nasm -f bin` to produce raw decoder bytes, then emits a wasm-compatible `db` listing. `src/unlzsa2.asm` opens `_TEXT` and includes the listing — wasm's offset bookkeeping then matches Watcom's `org 100h` on the same segment.

```
decompress_small_v2.S  →  nasm -f bin  →  unlzsa2.inc (db ...)
                                                ↓
                                        src/unlzsa2.asm  →  wasm  →  unlzsa2.o
                                                                          ↓
                                                                       wlink
```

Bonus: upstream NASM source stays untouched — the .inc is regenerated on every build.

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
