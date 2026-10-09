# THPS1 native Windows port (`platform/pc/`)

The native PC port of Tony Hawk's Pro Skater (PS1, `SLUS_008.60`), built on the matching
decompilation in `decomp/`. Windows only for now (MSYS2 `mingw32` toolchain, SDL2).

## Architecture: PS1 memory at its original addresses

The decompiled C is written for the PS1: 32-bit pointers, hand-padded structs, globals resolved
by absolute address, and a data image full of MIPS addresses (vtables, pointer tables, linked
lists). Instead of rewriting all of that, the port keeps the PS1 address space:

- The executable is a **32-bit, large-address-aware** Windows program. At startup it maps PS1
  main RAM at `0x80000000` (2 MB) and the scratchpad at `0x1f800000` (1 KB), then copies the
  retail `SLUS_008.60` image (code + data) to `0x80010000`. Every pointer in the retail data is
  therefore a valid native pointer, and struct layouts are unchanged (int, long and pointers are
  all 4 bytes).
- **Globals.** Each decompiled TU is compiled natively, then its symbols are rewritten to the
  absolute PS1 addresses recorded in its `.syms.json`. Data lives in the retail image, not in
  the Windows executable.
- **Functions.** Every function keeps its PS1 address as its identity. At startup a 5-byte
  `jmp` to the native code is written over the start of each function's (now unused) MIPS
  code. So `&Func`, vtable entries and callback tables in the retail data all work as native
  function pointers.
- **Hand-written MIPS** (Neversoft's `M3DASM`/`M3DMATHS` modules and the pure parts of the Psy-Q
  libraries) is translated to C by `tools/mips2c.py`. The output runs on a register file, with
  memory at the same addresses and GTE instructions going to the software GTE.
- **Hardware libraries** (GPU, CD, SPU, pad, memory card, BIOS) are high-level reimplementations
  ("HLE") of the Psy-Q entry points, in `src/`.

## Calling convention

Everything is `cdecl` on i386: C++ methods too (`g++ -mabi=sysv` turns off the MinGW `thiscall`
default), so `this` is the first stack argument, exactly as in the decompiled C callers.

## Layout

| path | what |
|---|---|
| `include/psyq/` | clean-room Psy-Q headers (`libgte.h`, `libgpu.h`, `inline_c.h`, `sys/types.h`) |
| `include/pc_*.h` | interfaces between the port's subsystems |
| `src/` | runtime: boot, memory map, HLE libraries, GTE, GPU, input, video |
| `tools/` | build driver and code generators |

## Building

Needs MSYS2 with the `mingw32` (i686) toolchain and SDL2
(`pacman -S mingw-w64-i686-gcc mingw-w64-i686-SDL2`), Python 3, and your own copies of:

- `extern/thps1_retail/SLUS_008.60` (the USA executable), and
- `extern/thps1_disc/thps1.bin`, a raw MODE2/2352 image of the USA disc
  (`chdman extractcd -i "Tony Hawk's Pro Skater (USA).chd" -o thps1.cue -ob thps1.bin`).

```
python tools/psyq_sigmatch.py          # once: names the Psy-Q library functions (build/psyq_names.tsv)
python platform/pc/tools/build.py      # -> build/pc/thps.exe
build/pc/thps.exe
```

The build compiles every TU in `decomp/matched.tsv` (and the near-miss sources in
`decomp/nearmiss.tsv`), translates the remaining MIPS with `tools/mips2c.py`, generates the glue in
`build/pc/gen/` and links. `build/pc/build_report.txt` lists compile errors and unresolved symbols.

## Running and debugging

Keyboard: arrows = D-pad, Z/Space = Cross, X = Circle, A = Square, S = Triangle, Enter = Start,
Backspace = Select, Q/W/E/R = L1/R1/L2/R2. Game controllers work through SDL.

| variable | effect |
|---|---|
| `THPS_EXE`, `THPS_DISC` | paths to `SLUS_008.60` and the disc image (else found next to the exe or in `extern/`) |
| `THPS_SCALE=n` | window scale (default 3) |
| `THPS_HEADLESS=1` | no window (for automated runs) |
| `THPS_FRAMES=n` | exit after n frames |
| `THPS_SHOT=f1,f2` / `THPS_SHOTDIR` | dump the display and VRAM as BMPs at those frames |
| `THPS_INPUT=frame:mask,...` | scripted pad input (PS1 button mask, pressed = 1, held from that frame) |
| `THPS_TRACE=1` | log CD, BIOS and debug-print activity |
| `THPS_FAST=1` | don't throttle to 60 Hz |
| `THPS_MOVIES=1` | run the original FMV code (movies are skipped by default) |
| `THPS_STRICT=1` | abort on a call to an unimplemented function |
| `THPS_DETERMINISTIC=1` | virtual clock: identical runs every time (use with `THPS_INPUT` for bisecting) |
| `THPS_MIPS=all,!lo-hi,addr` | run those game functions as translated retail MIPS instead of decompiled C (A/B debugging) |
| `THPS_WATCH=addr` | hardware write watchpoint on a 4-byte word: logs value, native EIP and callers |
| `THPS_COUNT=addr,...` | log per-frame call counts of those functions |
| `THPS_GPUTRACE=frame` / `THPS_GPUSTATS=1` | log VRAM transfers and drawing-environment changes / polygon counts per frame |
| `THPS_TRACE_PSX`, `THPS_TRACE_CULL`, `THPS_TRACE_RENDER` | spooler, frustum-culling and renderer traces |
| `THPS_AUDIO=0` / `THPS_AUDIO_DUMP=file` | no sound device / also write the mixed output as raw s16le stereo 44.1 kHz |
| `THPS_TRACE_SPU=1` | log SPU key on/off per voice |

Crash logs and the 5-second hang watchdog print native backtraces;
`python platform/pc/tools/symbolize.py < log` turns them into function/file:line.
`python platform/pc/tools/arity_check.py` lists decompiled functions declared with fewer arguments
than they really take (harmless on MIPS, where the value stays in its register; broken natively).

## Status

Boots through the front end (menus, skater/board/level select with 3D models) into a level, and
the game plays with full 3D rendering. Sound effects and music play: `src/pc_spu.c` models the
SPU (sound RAM, 24 ADPCM voices, ADSR, master volume, CD audio input) and mixes it to an SDL audio
device. The songs are XA audio inside the soundtrack STR files; `src/hle_libcd.c` streams them like
the drive does and `src/pc_xa.c` decodes them. Not done yet: SPU reverb, exact ADSR curves, FMV
video (MDEC; full-screen movies are skipped), memory card (always "no card"), 64-bit build.
Running every game function
as translated MIPS (`THPS_MIPS=all`) still crashes in the renderer: a translator issue to chase.

Port glue worth knowing about (see the tools for details):
- C++ follows the egcs ABI where the retail code depends on it: vtable layout, `delete` through
  the virtual destructor with in_chrg 3, constructors returning `this`.
- File statics in decompiled TUs live at their retail addresses (listed in each `.syms.json`).
- Hardware behaviour the game relies on is emulated: MIPS division never traps (the fault
  handler gives the MIPS result), low KUSEG addresses map to RAM, busy-waits let vblanks happen.

Changes the port needs in `decomp/src` are guarded by `#ifdef PC_PORT` or are honest signatures
that still compile to the same bytes. `node decomp/tools/check_all.mjs` must stay green.

## License

The port layer (`platform/pc/`) is GPL-2.0: parts of it are adapted from the Vandal Hearts PC
port (`HalmyLyseas/VandalHearts-PcPort`, GPL-2.0), credited in each file that uses them. The
decompiled game code and all game data remain © Activision / Neversoft. Nothing copyrighted is
committed; the build reads your own `SLUS_008.60` and disc files.
