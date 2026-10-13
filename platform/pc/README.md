# THPS1 native Windows and Linux port (`platform/pc/`)

The native PC port of Tony Hawk's Pro Skater (PS1, `SLUS_008.60`), built on the matching
decompilation in `decomp/`. Windows (MSYS2 `mingw32` toolchain) and Linux (the system's
multilib GCC), with SDL2 and OpenGL on both.

## Architecture: PS1 memory at its original addresses

The decompiled C is written for the PS1: 32-bit pointers, hand-padded structs, globals resolved
by absolute address, and a data image full of MIPS addresses (vtables, pointer tables, linked
lists). Instead of rewriting all of that, the port keeps the PS1 address space:

- The executable is a **32-bit** program (large-address-aware on Windows). At startup it maps PS1
  main RAM at `0x80000000` (2 MB) and the scratchpad at `0x1f800000` (1 KB). The game's
  initialised data (`decomp/data`) is native data in the exe, in its retail layout:
  `tools/data_image.py` writes it as `gen/data.s` (native pointers, string literals) and the code's
  references by retail address (`psx_<addr>`, `psx_syms.ld`) resolve into it; runtime code that
  reads a game global by address goes through `PC_Data` (`PSX_DATA_ADDR`). Only
  `decomp/data/fixed` (vtables, `DoubleBuffer`) is copied to its address in PS1 RAM at startup.
  The build checks the data against `SLUS_008.60` (`decomp/README.md`, Data), but the port
  doesn't need `SLUS_008.60` to run. Every pointer in the data is a valid native pointer, and
  struct layouts are unchanged (int, long and pointers are all 4 bytes).
- **Globals.** Each decompiled TU is compiled natively, then its symbols are rewritten to the
  PS1 addresses recorded in its `.syms.json` (`psx_<addr>`). The link resolves those: an
  initialised global is its place in the native data (`decomp/data`), anything else (`.bss`,
  the fixed data) its address in PS1 RAM. Data never lives in the TUs' own sections.
- **Functions.** Every function keeps its PS1 address as its identity. At startup a 5-byte
  `jmp` to the native code is written over the start of each function's (now unused) MIPS
  code. So `&Func`, vtable entries and callback tables in the retail data all work as native
  function pointers.
- **Hand-written MIPS** (Neversoft's `M3DASM`/`M3DMATHS` renderer and maths, 0x800858ac-
  0x8008b900, and a few game helpers) is C interpretations in `decomp/src/m3dasm/` and elsewhere,
  listed in `decomp/reimpl.tsv` (built like decompiled C); the Psy-Q library code the game uses is
  the port's own C (the HLE files below; `src/hle_libgte.c` for the GTE maths). Each is checked
  against the retail code by a test in `tests/` that runs the original MIPS in a small reference
  interpreter (`tests/mips_ref.c`); see "Checking C against the retail code" below.
  `reimpl.tsv` also holds byte-exact C for the few compiler-generated C++ functions a native
  build can't take from their matching source. No retail code runs in the port: functions with
  no C are stubs that log a call (none is called in play). For debugging, `build.py --mips`
  also translates the retail MIPS to C (`tools/mips2c.py`: a register file, memory at the same
  addresses, GTE instructions on the software GTE), which `THPS_MIPS` and `THPS_SNAP` use.
- **C++ classes.** To stay byte-exact, the matching decomp wrote many class members as C (raw
  `this + offset` access, explicit vptr stores and base-class calls). The port's copies are real
  C++ classes with the same logic (`reimpl.tsv` rows marked `cxx`; `decomp/tools/port_cxx.py`).
  A native destructor is wrapped at its retail address as the egcs `(this, in_chrg)` destructor:
  it runs the C++ destructor chain, then frees through the class's `operator delete` when asked.
  Classes are declared once, in `decomp/include/game/*.h` (complete retail layouts, virtuals in
  retail vtable order; also compiled by the matching egcs, hence `#pragma interface`). The build
  inlines those headers before its egcs-ABI pass, drops in-class bodies of virtual functions so
  calls dispatch through the retail vtable, and compiles with `-funsigned-char` (plain `char` is
  unsigned on the PS1). `tools/header_check.py`, `tools/layouts.py` and `tools/tu_diff.py` check
  header layouts and that a refactor doesn't change what a TU links to.
- **Hardware libraries** (GPU, CD, SPU, pad, BIOS) are high-level reimplementations
  ("HLE") of the Psy-Q entry points, in `src/`. The memory card isn't one: saves are files
  (see Saves).

## Calling convention

Everything is `cdecl` on i386: C++ methods too (`g++ -mabi=sysv` turns off the MinGW `thiscall`
default), so `this` is the first stack argument, exactly as in the decompiled C callers.

## Layout

| path | what |
|---|---|
| `include/psyq/` | clean-room Psy-Q headers (`libgte.h`, `libgpu.h`, `inline_c.h`, `sys/types.h`) |
| `include/pc_*.h` | interfaces between the port's subsystems |
| `src/` | runtime: boot, memory map, frame clock, input, video, saves, and the remaining HLE libraries (GPU, GTE, CD, SPU, MDEC) |
| `include/pc_platform.h`, `src/pc_platform.c` | what differs between Windows and Linux: paths, folders, files, clock |
| `tools/` | build driver and code generators (`toolchain.py`: the host's compiler and object format) |

## Building

Needs MSYS2 with the `mingw32` (i686) toolchain and SDL2
(`pacman -S mingw-w64-i686-gcc mingw-w64-i686-SDL2`), Python 3, and your own copies of:

- `extern/thps1_retail/SLUS_008.60` (the USA executable), for the build only: the build takes
  function layouts from it and checks the decompiled data against it; and
- a disc for the first startup, which the game data is extracted from: `extern/thps1_disc/thps1.bin`,
  a raw MODE2/2352 image of the USA disc (`chdman extractcd -i "Tony Hawk's Pro Skater (USA).chd"
  -o thps1.cue -ob thps1.bin`), or the Dreamcast disc (below). `thps.exe` looks for it next to
  itself (`thps1.bin`) or in `extern/` (development layout), or takes `THPS_DISC`.

```
python tools/psyq_sigmatch.py          # once: names the Psy-Q library functions (build/psyq_names.tsv)
python platform/pc/tools/build.py      # -> build/pc/thps.exe (--mips: plus translated retail code, for debugging)
build/pc/thps.exe
```

`build.py --release` makes the distributable build in `build/pc/release/`: `thps.exe` linked as
a GUI program (the log goes to `thps.log` in the player's folder when there is no console), its
debug information split into `thps.debug`; and `thps-windows.zip` with the exe,
`dist/README.txt`, the repository's LICENSE and the third-party notices.

A `--mips` build runs translated retail code, which also uses what the decompiled data leaves
out (jump tables, library data), so it embeds the retail image instead (`tools/embed_image.py`:
`SLUS_008.60` with its code cleared).

The build also runs Ghidra's GNU v2 demangler (`decomp/tools/demangle.py`; it finds it in the
Ghidra install `GHIDRA_INSTALL_DIR` names, default `D:\Tools\ghidra`, or takes `GNU_DEMANGLER`).

### Linux

The same build runs on x86 Linux with a GCC that can build 32-bit programs, binutils, Python 3,
pkg-config and the 32-bit SDL2 development files; on Debian or Ubuntu:

```
sudo dpkg --add-architecture i386 && sudo apt update
sudo apt install gcc-multilib g++-multilib pkg-config python3 libsdl2-dev:i386
python3 platform/pc/tools/build.py     # -> build/pc-linux/thps
build/pc-linux/thps
```

(Fedora: `gcc gcc-c++ glibc-devel.i686 libstdc++-devel.i686 SDL2-devel.i686`; Arch: `multilib`
with `gcc lib32-gcc-libs lib32-sdl2`.) `build.py` finds SDL2 through pkg-config's 32-bit
directories, or takes `THPS_SDL_CFLAGS`/`THPS_SDL_LIBS`; `THPS_TOOLBIN` points at another
toolchain, `THPS_BUILD_DIR` moves the output (`tools/toolchain.py`). Running needs the 32-bit SDL2
and OpenGL libraries (`libsdl2-2.0-0:i386 libgl1:i386 libgl1-mesa-dri:i386`).
`build.py --release` makes `build/pc-linux/release/thps-linux.tar.gz`: the stripped `thps`,
`dist/README-linux.txt`, the LICENSE and the notices (SDL2 and the C library are the system's,
linked dynamically, so build releases on an old distribution for an old enough glibc).

How the Linux build differs:

- The compiler emits ELF instead of PE/COFF. Game TUs are compiled with `-fleading-underscore`,
  so their symbols look like MinGW's and the renaming pipeline is the same on both systems;
  `build.py` takes the `_` off at the end. `-freg-struct-return` keeps the Windows ABI's 8-byte
  struct returns in registers (the decomp declares some functions both ways; see Calling
  convention). Static constructors (`.init_array`, Windows `.ctors`) are dropped: the game runs
  them from its own table. `data_image.py` reads ELF objects as well as COFF.
- PS1 RAM is one `memfd` mapped at `0x80000000`, `0xa0000000` and at its KUSEG addresses from
  `0x10000` (Linux keeps the first 64 KB unmapped, `vm.mmap_min_addr`). Low-memory accesses, the
  MIPS division results and crash reports come from signal handlers (`SIGSEGV`, `SIGFPE`,
  `SIGTRAP` for the single step) instead of a vectored exception handler, on the same decoders
  (`src/pc_main.c`). The hang watchdog asks the main thread for its backtrace with `SIGUSR2`.
  `THPS_WATCH` (debug-register watchpoints) is Windows-only: use a gdb watchpoint.
- Gamepads are SDL game controllers (XInput on Windows), mapped onto the same Xbox-style inputs.
- The player's folder is `$XDG_DATA_HOME/Neversoft/Tony Hawk's Pro Skater` (default
  `~/.local/share/...`); the log stays on stderr. Paths in `Data/Manifest.txt` and the code keep
  backslashes; `PC_AssetsPath` turns them into slashes, so a `Data` folder moves between the
  systems unchanged.

On the first startup the port extracts the game data from the disc image into `Data/` next to
`thps.exe` (`src/pc_assets.c`) and from then on reads only those files: after that the disc image
is not needed (it is only opened again if something is missing).

| path | what |
|---|---|
| `Data/Levels/` | level geometry (`sk<level>*.psx`) |
| `Data/Scripts/` | level trigger scripts (`*.trg`) |
| `Data/Models/` | skaters, cars, medals, items (`*.psx`) |
| `Data/Textures/` | images (`*.bmp`) |
| `Data/Sounds/` | sound banks (`*.vab`) |
| `Data/Fonts/`, `Data/Packs/`, `Data/Demos/`, `Data/Text/`, `Data/Misc/` | fonts, PRE archives, attract-mode replays, credits text, the rest |
| `Data/Movies/*.STR` | the full-screen movies (MDEC video, XA audio) |
| `Data/Music/*.STR` | the soundtrack (XA audio plus the 96x64 video the front end shows) |
| `Data/Music/Playlist.m3u` | the song order, written once from the game's own table, then yours to edit |
| `Data/Manifest.txt` | the files that came from `CD.WAD`; written last, so extraction resumes if interrupted |

The game's own indexes say what to extract: `CD.HED` lists the files packed in `CD.WAD`
(extracted with their names, by type; the level codes come from the `sk<level>_t.trg` scripts),
and the retail executable's movie table lists the STR files and which are songs. STR files keep
their raw 2352-byte sectors (XA audio intact; ffmpeg and jPSXdec play them).

At runtime files are found by name anywhere under `Data/`. `src/pc_fileio.c` replaces the game's
CD.WAD loader (FileIO_Open/Load/Continue/Sync) with plain file reads; STR files are mounted at
virtual CD positions for the streaming code. Missing files are extracted again on the next start
(with the disc image present); the playlist is never overwritten (delete it to get the default
back). Playlist entries are relative to `Data/Music` or absolute; an STR the game doesn't know
plays with its first song's settings.

### The Dreamcast version's data

The first extraction can come from the **Dreamcast** disc instead (Treyarch's port; a `.gdi`
dump with its track files next to `thps.exe`, or `THPS_DISC=<file>.gdi`; a PS1 image is used
first if both are there). Its files are loose on the disc (`src/pc_gdrom.c` reads the GD-ROM's
ISO 9660 area) and go to the same folders; `Manifest.txt`'s first line records the source. The
models and levels are version 6 `.psx` files: the same format with 16-bit PowerVR textures up to
256x256 (filtered, tiling, with alpha) and remodelled skaters. They are loaded without their
texture images, which stay on the PC (`src/pc_dctex.c`) and are drawn by the native 3D renderer;
see `include/pc_dctex.h`. With these files the game gets 8 MB of RAM (they are bigger than the
PS1's) and each skater's front-end model is the full one (the Dreamcast has none). Every file is
extracted unchanged. The images it redrew at a higher resolution are shown at the size the PS1
layout gives them, with all their pixels (`PC_ImageLayoutSize`): the 640x480 front-end screens
load as they are instead of line-doubled, and the HORSE loading screen (640x240 on a 512-wide
screen) and the pause logo (320x88 for the PS1's 256x44) are drawn scaled into the display
surfaces (`PC_GpuLoadImageHiRes`, `SlicedImage::drawScaled`).

Its songs and movies are Sofdec films (`.sfd`: MPEG-1 video, CRI ADX audio; `src/pc_sfd.c`
demultiplexes them and decodes the ADX). They mount at CD positions like the STR files
(`ATVILOGO.STR` is `activision.sfd`, `LOGO.STR` `never.sfd`), so the game's own streaming code
plays the 44.1 kHz stereo audio through the CD input; the playlist lists the `.sfd` songs, and a
song starts over at the end of its audio as the PS1's does at its end frame. The full-screen
movies play (`src/pc_movie.c`: pl_mpeg decodes the 320x240 video into the 24-bit movie screen,
timed to the audio; any button skips, as on the PS1); the songs' 96x64 thumbnail video is not
shown. Its sound banks (`.kat`: the same sounds as 8/16-bit PCM at 22 kHz instead of 4-bit ADPCM
at about 11 kHz) become the VABs the game loads (`src/pc_dcsound.c`): each sound is a PCM sample
the SPU plays in place of a stand-in in sound RAM, tuned to the PS1 banks' pitch. The Dreamcast
logos (Crave, Treyarch, Sofdec) are not played. Needs the OpenGL renderer.

## Checking C against the retail code

The C that replaced hand-written assembly and library code is checked against the original MIPS
on real game states. A `build.py --mips` build records snapshots at function entries
(`THPS_SNAP=80086f40:8:500,...`: RAM, scratchpad, GTE registers, arguments and the native stack
the arguments point into; `src/pc_snap.c`). `tools/build_test_asm.py --run` builds
`build/pc/test_asm.exe` from every `reimpl.tsv` row marked `test_asm` and `src/hle_libgte.c`, and
replays each snapshot (default: all under `build/pc/snaps`) through the retail MIPS in
`tests/mips_ref.c` (COP2 on the software GTE) and through the C: both must leave the same RAM,
scratchpad, native stack, GTE registers and return value. Paths the game rarely takes are covered
by snapshots edited to take them. Exceptions (scratch words only the retail code's register saves
use, pointers the port keeps in KSEG0) are listed in the sources as `test_asm:` comments.
`test_asm` builds and runs on Windows only for now.

## Saves

There's no memory card. Each career and each replay is its own file, named after the name the
player gives it, in `%USERPROFILE%\Saved Games\Neversoft\Tony Hawk's Pro Skater\Saves` (Linux:
`~/.local/share/Neversoft/Tony Hawk's Pro Skater/Saves`):
`Careers\<name>.career` and `Replays\<name>.replay` (`src/pc_saves.c`). The game's save and load
screens are its own; its card layer (`decomp/src/card`, PC branches) reads and writes these files
and is always ready, so there's no card check, formatting, block count or "do not remove" text.
A file is the game's save data without the PS1 card header (icon, Shift-JIS title), behind a
16-byte header. The load screens list up to 15 saves of each type, newest first. The career saved
most recently is loaded at startup, as on the PS1, without the "loading from memory card" pause.
PS1 saves in `.mcs` form (MemcardRex, emulators) put in the `Saves` folder are converted the next
time the game lists saves (at startup, or on a save or load screen) and renamed `.mcs.imported`.

## Graphics

`src/pc_gpu.c` is the PS1 GPU: it takes the GP0/GP1 words the game sends (ordering tables,
`LoadImage`, `PutDrawEnv`...) and keeps VRAM. It draws through OpenGL 3.3 (`src/pc_gpu_gl.c`):
polygons, sprites, lines and fills go to render targets, and a shader does the PS1's CLUT texture lookup,
texture windows, colour modulation and the four semi-transparency modes. Textures come from a
16-bit copy of VRAM. VRAM is tracked in 16x16 cells, so the two sides stay in sync when the game
reads back what it drew (the HUD saves the background under a sprite with `StoreImage`) or draws
into an area the CPU wrote. There's no dithering: the picture is never reduced to 15 bits for display.
The screen buffers are drawn at the resolution they are shown at: each VRAM area the game uses as
a whole screen (a `PutDispEnv` display area, or a screen-sized `PutDrawEnv` drawing area: the two
512x240 gameplay buffers, the 640x480 front end) gets a *display surface*, a render target the size
of the window's 4:3 picture area, so in a 1024x768 window both are drawn at 1024x768 (in
1920x1080, at 1440x1080 with black bars, unless widescreen widens gameplay, below). Everything else in VRAM (textures, CLUTs, off-screen
drawing) stays at the PS1's resolution. A VRAM pixel belongs to the surface covering it, else to
the 1x target; drawing goes to every target its area touches, and readbacks scale the surface down.
`THPS_GL_SCALE=n` draws the surfaces at n times the PS1 resolution instead (1: exactly the PS1's
pixels, for comparisons).

**Widescreen** (Hor+, `[Video] Widescreen = On`, the default): on a window wider than 4:3, the
gameplay screens (512x240) get margins left and right, `pc_wide_pad` PS1 pixels each (85 at 16:9,
at most 400), and the view shows that much more of the world at the sides at the same zoom. The
margins exist only in the display surface: in drawing coordinates they lie at x < 0 and past the
buffer, where VRAM holds textures, so only primitives drawn with a drawing area that covers the
whole screen reach them (`pc_gpu.c` widens it, `PC_GlWideClip`, and flags the primitive); every
other drawing, the uploads and the cell bookkeeping see the surface's VRAM area alone. The game's
side culling widens to match: the frustum's side planes (`M3d_RenderSetup`, PC branch) and the
vertex transforms' outcodes, which `pc_render3d.c` works out again against the widened viewport
(the fast transforms keep theirs inside the screen words, so the face walk tests those itself).
The HUD and menus keep their 4:3 positions in the middle; full-width flat rectangles and polygons
(the screen clear, the pause menu's dimming, fades) are stretched over the margins. Needs the
native 3D renderer; split screen, the front end and movies stay 4:3.
The PS1 rounds every projected vertex to a whole pixel, has no perspective-correct texturing and
no depth buffer, which is where its wobbling geometry, seams and warping textures come from. With
OpenGL, the world and models (everything THPS1's hand-written M3DASM renderer draws: level,
skaters, cars, sky) are drawn natively by `src/pc_render3d.c`: the retail vertex transforms still
run, but their view-space vertices are kept unrounded, and ProcessPolys is replaced by a face walk
that keeps retail's visibility, colours (the same GTE colour commands) and ordering-table position,
then draws with a projection, a depth buffer and OpenGL's near-plane clipping instead of
subdivision. The faces are drawn while the GPU walks the ordering table, so everything still sent
as GPU packets (particles, shadows, 3D text, the HUD) keeps its place in the PS1's painter order;
of those, the ones the GTE projected are depth-tested against the world. Those packets use the
precise-vertex lookup: the GTE (`src/pc_gte.c`) also records each projection unrounded, with its
depth, keyed by the screen coordinates the game puts in its GPU packets, and the GPU looks a
polygon's vertices up again and draws it at sub-pixel positions with perspective-correct textures
(THPS1's polygon subdivision re-projects and nudges vertices, which the lookup allows for, see
`PC_GtePrecisePolygon`). Faces the native renderer doesn't handle yet (water clipping and the
fade-to-colour mode of some levels) still go through the retail code and that lookup.
`THPS_RENDER3D=psx` draws all 3D the PS1 way. With that and the lookup turned off
(`THPS_RENDER3D=psx THPS_GL_PRECISE=0 THPS_GL_PERSPECTIVE=0`), OpenGL at 1x samples the same
points the PS1 rasteriser does.

The build compiles every TU in `decomp/matched.tsv` (and the near-miss sources in
`decomp/nearmiss.tsv`), translates the remaining MIPS with `tools/mips2c.py`, generates the glue in
`build/pc/gen/` and links. `build/pc/build_report.txt` lists compile errors and unresolved symbols.

## Controls

Keys and gamepad buttons are bound to game actions (`src/pc_input.c`). Each action takes two keys
and two gamepad buttons, kept in `%USERPROFILE%\Saved Games\Neversoft\Tony Hawk's Pro Skater\Settings.ini`
(Linux: `~/.local/share/Neversoft/Tony Hawk's Pro Skater/Settings.ini`)
(written with the defaults on the first startup). The keyboard plays player 1; each player has
their own gamepad bindings (`[Skate Gamepad]`/`[Menu Gamepad]` for player 1, `... Gamepad 2` for
player 2).

**Options > Player 1/2 Controls** is the rebinding screen (it replaces the PS1's face-button
remap; `decomp/src/front/controlleroptions.cpp`, PC branch). The top row switches between the
keyboard and gamepad bindings (player 2: gamepad only), then come Autokick and Vibration, every
Skate and Menu action with its two bindings, Reset to Default (the device shown) and Back. Left
and right pick the binding, Accept waits for the new key or button (5 seconds), Delete or X on
the gamepad clears it. Taking an input another action of the same set already has swaps the two,
so nothing is left unbound by accident. Changes are saved when the screen is left. The screen
draws its own list over `options.bmp`; it puts that area of the background back each frame from
`pc_hires_before_draw`, a hook in the hi-res frame end that runs after the wait for the next field
(restored any earlier, the background would be shown without its text).

The game reads the actions themselves. Its per-player button block (`SControl`,
`decomp/include/game/pad.h`) names its buttons after what they do while skating (`Ollie`,
`Grind`...), and the menus read `MenuAccept`, `MenuBack` and `MenuUp`..`MenuRight`. On the PS1
those menu names are aliases of the pad's own buttons (X, Triangle, the D-pad), so the matching
build is unchanged; on PC they are separate slots (the ones the PS1 never filled), and
`Pad_Update` (`decomp/src/unnamed/80070ff4.c`) fills every slot from its action's bindings. So
the same key can do one thing while skating and another in the menus. On PC, the front end's
"press Start" prompts take Menu Accept, and Pause also resumes from the pause menu.

| action | keyboard | gamepad (XInput) | PS1 button |
|---|---|---|---|
| Up / Down / Left / Right | W A S D, arrows | D-pad, left stick | D-pad |
| Ollie | Space, Keypad 2 | A | X |
| Flip | J, Keypad 4 | X | Square |
| Grab | L, Keypad 6 | B | Circle |
| Grind | I, Keypad 8 | Y | Triangle |
| SpinLeft / SpinRight (held) | Q / E, Keypad 7 / 9 | LB / RB | L1 / R1 |
| TapSpinLeft / TapSpinRight (180 per tap) | Z / C, Keypad 1 / 3 | LT / RT | L2 / R2 |
| Camera | Tab, Keypad 5 | Back | Select |
| Pause | Escape, P | Start | Start |
| Menu Up / Down / Left / Right | arrows, W A S D | D-pad, left stick | D-pad |
| Menu Accept | Return, Space | A | X (and Start) |
| Menu Back | Escape, Backspace | B | Triangle |

The pause-menu cheat codes are entered with the Skate controls (hold SpinLeft). The front end's
button legend (PS1 symbols for Select, Accept and Back) is part of its background images.

On PC the main menu (`decomp/src/unnamed/8005e754.c`) ends with **QUIT**, which exits the game
the same way closing the window does (`PC_RequestQuit`, acted on at the next frame). Its row id
7 is also the hidden cheat menu's, which is only reached through a menu path, so the menu only
quits when 7 was picked by hand.

## Display

The window is windowed, borderless (fullscreen at the desktop's resolution) or fullscreen (the
display switches to the chosen mode, at 60 Hz when the monitor has it), kept in the `[Video]`
section of `Settings.ini` (`Mode`, `WindowSize`, `FullscreenResolution`, `Widescreen`;
`src/pc_video.c`). The game
is drawn at the resolution it is shown at (see Graphics). The window is DPI-aware, so sizes are
real pixels. Alt+Enter switches between the window and fullscreen; a window resized by hand keeps
its size.

**Options > Graphics** (`PC_GraphicsOptions` in `decomp/src/front/controlleroptions.cpp`, next to
the controls screen; the options menu `decomp/src/unnamed/80060274.c` gets the extra row on PC)
sets the display mode, the resolution and widescreen: left and right step through the display's
modes (windowed: those that fit on the desktop). Apply switches the window and asks to keep the change;
Accept keeps it and saves Settings.ini, Back or 15 seconds without an answer go back.

## Running and debugging

| variable | effect |
|---|---|
| `THPS_DISC` | the disc image for the first-startup extraction (else found next to the exe or in `extern/`); a `.gdi` is the Dreamcast disc |
| `THPS_DATA_DIR` | where the extracted data lives (default `Data` next to `thps.exe`) |
| `THPS_SAVE_DIR` | the saves folder (default `%USERPROFILE%\Saved Games\Neversoft\Tony Hawk's Pro Skater\Saves`; Linux `~/.local/share/Neversoft/Tony Hawk's Pro Skater/Saves`) |
| `THPS_SCALE=n` | headless: window size n x 320x240 (default 3) |
| `THPS_WINDOW=WxH` | a window of that size (drawing area), whatever Settings.ini says; headless too |
| `THPS_WIDESCREEN=0\|1` | widescreen off or on, whatever Settings.ini says |
| `THPS_DRAWDIST=pct\|full`, `THPS_OBJDIST=pct`, `THPS_FOG=0\|1\|2` | draw distance (50-400, or the entire level), object distance (50-400) and fog (normal, light, off), whatever Settings.ini says |
| `THPS_FAR_RANGE=n` | entire level: items reaching beyond n (default 30000) take the double-precision path; a small n compares it with the PS1 transforms on near items |
| `THPS_HEADLESS=1` | no visible window (for automated runs: OpenGL draws into a hidden one) |
| `THPS_GL_SCALE=n` | OpenGL: draw the screens at n times the PS1 resolution (default: the window's resolution) |
| `THPS_GL_PRECISE=0` | OpenGL: draw 3D polygons at the PS1's whole-pixel positions (default: sub-pixel, from the GTE) |
| `THPS_GL_PERSPECTIVE=0` | OpenGL: affine texturing like the PS1 (default: perspective-correct) |
| `THPS_RENDER3D=psx` | OpenGL: draw the world and models through the PS1 path (default `native`, see Graphics) |
| `THPS_R3DSTATS=1` | once a second: native 3D faces, culled faces, draws, and calls left to the retail code |
| `THPS_R3D_VERIFY=1` | check the native renderer's float vertices and back-face tests against retail's integers |
| `THPS_R3D_NEARDROP=1` | drop faces crossing the near plane that the PS1 dropped (default: OpenGL clips them) |
| `THPS_R3D_PROBE=f,x,y` | log the 3D faces covering screen point x,y on frame f |
| `THPS_R3D_RETAILFADE=1` | leave fade-to-colour mode (Downhill Jam, Streets, the front end) to the retail code (A/B; not for Dreamcast models) |
| `THPS_FILTER=1` | bilinear scaling to the window instead of nearest-neighbour |
| `THPS_GLSTATS=1` | once a second: draw calls, VRAM readbacks and uploads, and how many polygons had precise vertices |
| `THPS_FRAMES=n` | exit after n frames |
| `THPS_SHOT=f1,f2` / `THPS_SHOTDIR` | dump the display and VRAM as BMPs at those frames (with OpenGL, also `frameNNNNN_gl.bmp` at the resolution it was drawn at) |
| `THPS_INPUT=frame:mask,...` | scripted pad input for player 1 (PS1 button mask, pressed = 1, held from that frame; X is both Ollie and Menu Accept, Triangle Grind and Menu Back, Start is Pause) |
| `THPS_KEYS=frame:Key+Key,...` | scripted keyboard (Settings.ini key names, held from that frame; `frame:` releases), read through the bindings |
| `THPS_NOGAMEPAD=1` | ignore connected gamepads, so scripted runs navigate the menus the same way with or without one |
| `THPS_TRACE=1` | log CD activity, the game's debug prints, and player 1's actions as they change |
| `THPS_FAST=1` | don't throttle to 60 Hz |
| `THPS_MOVIES=0` | skip every movie |
| `THPS_STRICT=1` | abort on a call to an unimplemented function |
| `THPS_DETERMINISTIC=1` | virtual clock: identical runs every time (use with `THPS_INPUT` for bisecting) |
| `THPS_MIPS=all,!lo-hi,addr` | `build.py --mips` builds: run those functions (game code and the rewritten assembly) as translated retail MIPS instead of C (A/B debugging) |
| `THPS_SNAP=addr[:n[:every]],...` / `THPS_SNAPDIR` | `--mips` builds: record the machine state at those functions' entries for the differential tests (`src/pc_snap.c`, below) |
| `THPS_POKE=addr:value,...` | store those words every frame (`0x800d1708:1` = level select cheat: every level unlocked) |
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
the drive does (from the extracted files, which it mounts at CD positions past the end of the
image) and `src/pc_xa.c` decodes them. `src/pc_music.c` picks the next song from
`Data/Music/Playlist.m3u` instead of the game's fixed rotation. With the Dreamcast's data the
music, movies and sound effects come from its films and sound banks (see above). The PS1's
movies play through the game's own movie player (`GameFMV_PlayMovie`): `src/pc_mdec.c` is the
MDEC (the game decodes the bitstream itself, `DecDCTvlc3`; this does the inverse DCT and colour
conversion into the slices the game uploads), and the player's busy-waits wait a frame for the
CD stream instead (PC branch, `PC_MovieWait`). The same decoder shows the songs' 96x64 video on
the levels' TV screens (the credits screen shows them the same way). Not done yet: SPU reverb,
exact ADSR curves, 64-bit build. Running every game function as translated MIPS
(`THPS_MIPS=all`) still crashes in the renderer: a translator issue to chase (the translation
also loses level polygons in `THPS_RENDER3D=psx` mode, which the C renderer doesn't).

Port glue worth knowing about (see the tools for details):
- C++ follows the egcs ABI where the retail code depends on it: vtable layout, `delete` through
  the virtual destructor with in_chrg 3, constructors returning `this`.
- File statics in decompiled TUs live at their retail addresses (listed in each `.syms.json`).
- Hardware behaviour the game relies on is emulated: MIPS division never traps (the fault
  handler gives the MIPS result), low KUSEG addresses map to RAM, busy-waits let frames happen (`PC_FramePoll`).

Changes the port needs in `decomp/src` are guarded by `#ifdef PC_PORT` or are honest signatures
that still compile to the same bytes. `node decomp/tools/check_all.mjs` must stay green.

## License

The port layer (`platform/pc/`) is MIT, like the rest of the project (see the repository's LICENSE).
`include/pl_mpeg/pl_mpeg.h` is Dominic Szablewski's PL_MPEG (MIT, see its header). The
decompiled game code and data, and all game assets, remain © Activision / Neversoft. No assets
or original executables are committed; the build reads your own `SLUS_008.60` (to check against),
and the game your own disc.
