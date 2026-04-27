# Build & runtime findings

Hard-won lessons. Read this before fighting the toolchain again.

## Design rationale

### Why LZSA2

Byte-aligned LZ77 with optimal parsing. The 8088 hand-tuned asm decoder is small (~167 B) and fast, and the format gives ~50–60 % compression ratios on real x86 code. Upstream: [emmanuel-marty/lzsa](https://github.com/emmanuel-marty/lzsa). We use the *raw block* variant (`lzsa -f 2 -r`) — no stream framing, max 64 KB per block, which is plenty for cold-path overlays.

### Why a BCJ E8/E9 filter

`CALL rel16` and `JMP rel16` operands are *position-dependent*: identical code at different file offsets emits different bytes, so LZ77 match-finding breaks down on x86 code. Rewriting the operands to absolute targets pre-pack typically buys 10–25 % extra ratio. Inverse pass on decode is a few bytes of asm. Filter source is xz's [`src/liblzma/simple/x86.c`](https://github.com/tukaani-project/xz) (public domain) — but that's the rel32 version, so we wrote a 30-line rel16 equivalent for 8086.

### Why tiny model + DOS-allocated overlay segments

Keeps the resident image inside 64 KB. Cold blobs unpack into separately allocated paragraphs reached via `__far` pointer. Alternative approaches (linker overlays, .EXE multi-segment) are more complex without buying anything for short-lived overlay use.

### When overlays actually save bytes

For tiny cold modules (tens of bytes), the LZSA2 raw-block overhead is more than the savings. The two demos (`04_hello`, `05_cmdloop`) cost more bytes than they save — they exist to validate the plumbing. Real break-even:

- Resident decoder + dispatcher overhead: **~480 B fixed** (LZSA2 asm 167 B + BCJ inverse ~30 B + `ovly_load` ~50 B + DOS alloc machinery + libc-free CRT + boilerplate).
- LZSA2 typically reaches 50–60 % ratio on real x86 code with the BCJ filter applied.

→ Cold modules need to be roughly **1 KB+ raw** before overlays save net resident bytes. For a single 2 KB cold path you'd save roughly 1 KB of resident segment at the cost of an off-segment DOS allocation. Multiple cold paths sharing the resident decoder amortize the overhead further.

## Cold-module ABI

Cold blobs are flat binaries (typically `nasm -f bin`) with these conventions:

- **Entry at offset 0** of the unpacked blob.
- **Caller far-calls in** (`CS = unpacked segment`). The cold module is responsible for setting `DS = CS` if it needs to access its own data.
- **Returns via `RETF`.**
- **Args via `__cdecl`** when needed: stack on entry has `[ret_IP][ret_CS][arg0]...`. See `tests/05_cmdloop/cold.nasm`.
- **Multi-entry cold modules:** put a 3-byte `jmp short do_N` table at offset 0 (entry N at offset 3·N). Or use a single entry with a dispatch arg, like `05_cmdloop`.
- **No mutable state assumptions across allocations.** Cold's segment is freed (or leaked) when the caller is done. Within one allocation, `static` data does persist across calls — cold's segment lives until `dos_free`.
- **No far constants in cold.** Cold is a flat binary with zero relocations: anything pointing at the resident must come in via an arg or callback.

## Cold ↔ resident calling conventions

The first three sections cover what works today; the API-callback section is the gap that `tests/06_apicall` fills.

### Resident → cold

Standard from `04_hello`/`05_cmdloop`. Caller does a far call into offset 0 with `__cdecl` args:

```c
typedef void (__far __cdecl *cold_fn)(unsigned int cmd);
cold = (cold_fn) ovly_load(packed, raw_size);
cold(0xCAFE);          /* args on stack, far call, RETF returns */
```

Cold's prologue sets `DS = CS` if it needs its own data, saves any clobbered regs, runs, restores, returns.

### Cold → INT 21h / BIOS

Always works. Interrupts are global, segment-agnostic. Just remember `DS:DX` for `AH=09h` (string print) needs DS pointing where the string lives.

### Cold → resident helpers (the API struct)

See `src/api.{h,asm}` and `tests/06_apicall/`. The pattern:

```c
/* src/api.h */
typedef struct {
    void (__far __cdecl *puts)(const char __far *s);
    void (__far __cdecl *puthex16)(unsigned int v);
} toopack_api_t;
```

Resident initialises a `toopack_api_t` at runtime (see "the relocation trap" below) and passes `&api` as cold's first arg. Cold treats it as a `__far *` and calls helpers through it:

```nasm
les bx, [bp+6]                   ; ES:BX = api
push ds
mov ax, msg
push ax                          ; far ptr arg: [seg][off]
call far [es:bx + 0]             ; api->puts(msg)
add sp, 4                        ; cdecl: caller cleans up
```

Each helper is `__far __cdecl` (RETF, leading-underscore name decoration), saves and restores DS, and is implemented in `src/api.asm` because Watcom's C inline-asm support has rough edges around DS handling. The `_rapi_puts` helper does `lds dx, [bp+6]` to load the caller's far string, prints, restores DS — a typical pattern.

### The relocation trap

Tiny `.COM` has **no relocation table**. Watcom can't bake the resident's segment into a static `__far` function pointer at compile time, because that segment isn't known until DOS loads the program. If you write the obvious

```c
static const toopack_api_t api = { rapi_puts, rapi_puthex16 };
```

`wlink` emits `W1019 segment relocation at …`, the segment field gets stored as zero, and cold's far calls land in low memory. The CPU then `hlt`s on the first interrupt vector it tries to execute as code.

The fix is to assemble the far pointers at runtime. Get CS via inline asm, treat the helpers as opaque address-of references (declared as `extern char` so Watcom doesn't try to compute their full far address), and write the words by hand:

```c
extern unsigned int get_cs(void);
#pragma aux get_cs = "mov ax, cs" value [ax];

extern char rapi_puts_addr;
#pragma aux rapi_puts_addr "_rapi_puts";        /* alias, same symbol */

unsigned int cs = get_cs();
unsigned int *fp = (unsigned int *)&api;
fp[0] = (unsigned int)&rapi_puts_addr;   fp[1] = cs;   /* api.puts */
```

Ugly, but it works and contains the ugliness inside the API-struct setup. Cold sees a normal struct of clean far pointers.

### `__loadds` for helpers that touch resident data

`rapi_puts` doesn't read resident data — it only reads through the caller's far pointer (`lds dx, [bp+6]`), so it's fine. `rapi_puthex16` doesn't either (uses only `INT 21h AH=02h`, which only reads DL).

If a helper *does* need its own static data, it must save DS, set DS = its-own-segment (in tiny .COM, that's `push cs / pop ds`), use the data, then restore DS. Watcom's `__loadds` keyword would automate the prologue/epilogue, but in our hand-rolled `api.asm` it's clearer to do it inline.

### Cost of the indirection

A cold→resident round-trip is roughly:
- `les bx, [api]` + `call far [es:bx+N]`: ~30–50 cycles on 8088.
- Helper prologue (push bp, mov bp/sp, push ds, lds): ~25 cycles.

So ~75 cycles per call, vs ~5 cycles for a direct near call. Fine for prompts, status messages, infrequent dispatch; don't put it in a tight loop.

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

### Make-time `$(if ...)` for optional shell redirections

Bash refuses to parse a redirection operator (`<`, `>`) with no operand even inside an unreached `then` branch, since parsing happens before evaluation:

```sh
# parse error: bash sees `< ` followed by `2>` and chokes on `2`
if [ -n "" ]; then cmd < ; else cmd; fi
```

When a Make recipe wants to optionally pipe stdin, do the branching at *Make time* with `$(if ...)` so the unused redirection isn't even emitted:

```make
# Usage: $(call run_test,target.com)             — no stdin
#        $(call run_test,target.com,input.txt)   — with stdin
$(KVIKDOS) $(if $(2),--tty-in=-2 $(1) < $(2),$(1)) ...
```

This is the trick used in `config.mk`'s `run_test` macro.

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
