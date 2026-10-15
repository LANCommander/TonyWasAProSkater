# THPS1 native Windows and Linux port (`platform/pc/`)

The native PC port of Tony Hawk's Pro Skater (PS1, `SLUS_008.60`). Windows (MSYS2 `mingw32`
toolchain) and Linux (the system's multilib GCC), with SDL2 and OpenGL on both.

The game's code is `game/`: a fork of the matching decompilation (`decomp/`, which stays the
frozen byte-matching reference), compiled natively, with the PS1's hardware and libraries replaced
by native services and its memory by native memory. Ports to other platforms start from it.

## Architecture

- **Memory.** Nothing is at a PS1 address. The game's initialised data and `.bss` are ordinary
  C++ globals (`game/data`, compiled and linked with the game; `game/data/bss.cpp` the
  zero-initialised ones), each laid out on its own: the game code names a place inside one by an
  expression on it (`GVInfo1[1]`, `decodeSlice.y`), not as a global of its own. The game's heaps
  (`Mem_NewX`, `Mem_NewTop`, its own allocator) manage one block
  it gets from the system (`PC_GameHeap`, 8 MB); the scratchpad is a native buffer
  (`game/include/game/scratchpad.h`); primitive packets and ordering tables link by native
  pointers (`include/psyq/libgpu.h`, `P_TAG`), and the display buffers are the native
  `DoubleBuffer` (`game/include/game/dbuffer.h`). The game's null-pointer reads, which read low
  RAM on the PS1, are fixed where they happen; any left are redirected to a zero page and logged
  once (`src/pc_main.c`). The executable is 32- or 64-bit (below).
- **Globals by name, functions by retail address.** Each game TU (`game/tus.tsv`) is compiled
  natively. Its globals keep their C names: `game/data` defines them, and a TU only declares them.
  Its functions are renamed to the retail addresses recorded in its `.syms.json` (`psx_<addr>`),
  as are the data's pointers to functions, so the decompilation's many names for one function
  agree. The link resolves those (`gen/psx_syms.ld`) to the function's native code
  (`pcfn_<addr>`; one with no C, none of which is called in play, to a stub that logs the call).
  Runtime code reads the game's globals by name (`include/pc_gamedata.h`).
- **Hand-written MIPS** (Neversoft's `M3DASM`/`M3DMATHS` renderer and maths, and a few game
  helpers) is C interpretations in `game/src/m3dasm/` and elsewhere (`tus.tsv` kind `asm`); the
  Psy-Q library code the game uses is the port's own C (`src/pc_libgte.c` for libgte,
  `src/pc_libgpu.c` for libgpu's helpers). Before the port forked from the decompilation, all of
  it was checked against the retail code (differential tests running the original MIPS on recorded
  game states); those tools are in the history.
- **C++ classes.** The game's classes are real C++ classes declared once, in
  `game/include/game/*.h` (complete retail layouts, virtuals in retail vtable order). The build
  keeps the egcs ABI they were written for: it inlines those headers, drops in-class bodies of
  virtual functions so calls dispatch through the retail vtables (`game/data/*_vtables.cpp`,
  native data), and wraps each native destructor as the egcs `(this, in_chrg)` destructor, which
  runs the C++ destructor chain, then frees through the class's `operator delete` when asked. It
  compiles with `-funsigned-char` (plain `char` is unsigned on the PS1). `tools/layouts.py` and
  `tools/tu_diff.py` check header layouts and that a refactor doesn't change what a TU links to.
- **Platform services, not a PS1.** Where the game used the PS1's libraries, it calls native
  services directly:
  - the C library is the host's (`sprintf`, `strcpy`...; `build.py` `NATIVE_OK`); the game's
    debug `printf` goes to the trace log (`PC_GameDebugf`, `THPS_TRACE=1`, `NATIVE_RENAME`);
  - VSync and the vblank callback are the frame clock (`pc_frame.h`: `PC_FrameWait`,
    `PC_FrameCount`, `PC_FramePoll`, `PC_SetFrameHandler`);
  - controllers are `pc_input.h` (actions, presence, rumble), with no pad receive buffers;
  - the memory card is gone: saves are files (see Saves), with no BIOS card events;
  - songs and movies are streams of their files (`pc_stream.h`), not CD reads: the game keeps a
    stream's id where it kept the file's position on the disc;
  - sound is `pc_sound.h`: banks decoded to PCM at load (the game keeps a bank's base where it kept
    the sample data's SPU address), 24 channels with the PS1's ADSR envelopes, a native reverb,
    and the music input; no sound RAM or libspu;
  - the static constructors run from `PC_RunStaticCtors` (libsn's `__main`);
  - the GTE's 3D maths is `pc_geom.h`: plain C fixed-point functions with explicit inputs and
    outputs (projection, matrix times vector, cross product, depth cue ...), bit-identical to the
    hardware (44-bit sums, 16-bit saturation, the UNR divide, the same FLAG word), and a
    `GeomState` for what the game keeps between calls (the current rotation, translation, light
    and colour matrices, screen). libgte is `src/pc_libgte.c` on it (the game's calls go there by
    address: `build.py` `NATIVE_AT`), and the game's GTE code calls it. M3DMATHS' "last vector" and the colour code the face renderer carries between
    faces, which retail kept in GTE registers, are explicit (`game/src/m3dasm/lastvec.h`,
    `processpolys.h`);
  - video pictures (the MDEC's work) are `pc_vdec.h`: the game's run-level data decoded a whole
    frame at a time, into a movie's picture or a texture;
  - drawing is `pc_draw.h`: the game's primitive packets in its ordering tables (its display
    list) walked and drawn through OpenGL; the screens and the display are `pc_screen.h` (fills,
    the CPU's own pixels in and out, what the window shows); the textures and CLUTs are
    `pc_texstore.h`, at the places the game's own allocators give them. There is no VRAM; libgpu's
    pure helpers (`SetDefDrawEnv`, `GetTPage` ...) are `src/pc_libgpu.c` (`NATIVE_AT`).

  There's no BIOS, CD, SPU, GTE, GPU or MDEC in the port, and no retail code.

## Calling convention

Everything is `cdecl` on i386: C++ methods too (`g++ -mabi=sysv` turns off the MinGW `thiscall`
default), so `this` is the first stack argument, exactly as in the decompiled C callers.

## Layout

| path | what |
|---|---|
| `../../game/` | the game: `src/` (its code), `include/game/` (its classes and structures), `data/` (its initialised data), `tus.tsv` |
| `include/psyq/` | clean-room Psy-Q headers (`libgte.h`, `libgpu.h`, `sys/types.h`) |
| `include/pc_*.h` | interfaces between the port's subsystems |
| `src/` | runtime: boot, diagnostics, frame clock, input, video, drawing, screens, textures, saves, streams, sound, geometry maths |
| `include/pc_platform.h`, `src/pc_platform.c` | what differs between Windows and Linux: paths, folders, files, clock |
| `tools/` | build driver and code generators (`toolchain.py`: the host's compiler and object format) |

## Building

Needs MSYS2 with the `mingw32` (i686) toolchain and SDL2
(`pacman -S mingw-w64-i686-gcc mingw-w64-i686-SDL2`), Python 3, and your own copies of:

- `extern/thps1_retail/SLUS_008.60` (the USA executable), for the build only: the build takes
  function layouts from it; and
- a disc for the first startup, which the game data is extracted from: `extern/thps1_disc/thps1.bin`,
  a raw MODE2/2352 image of the USA disc (`chdman extractcd -i "Tony Hawk's Pro Skater (USA).chd"
  -o thps1.cue -ob thps1.bin`), or the Dreamcast disc (below). `thps.exe` looks for it next to
  itself (`thps1.bin`) or in `extern/` (development layout), or takes `THPS_DISC`.

```
python tools/psyq_sigmatch.py          # once: names the Psy-Q library functions (build/psyq_names.tsv)
python platform/pc/tools/build.py      # -> build/pc/thps.exe
build/pc/thps.exe
```

On Windows, `build-pc.bat` and `build-linux.bat` in the repository root wrap this:
`build-pc.bat [debug|release] [x86|x64] [build.py options]`, and the same for `build-linux.bat`,
which builds in Docker (`platform/pc/docker/Dockerfile`, image `thps-linux-build`; Docker Desktop
must be running) with Ghidra's Linux demangler from `GHIDRA_INSTALL_DIR`.

`build.py --release` makes the distributable build in `build/pc/release/`: `thps.exe` linked as
a GUI program (the log goes to `thps.log` in the player's folder when there is no console), its
debug information split into `thps.debug`; and `thps-windows.zip` with the exe,
`dist/README.txt`, the repository's LICENSE and the third-party notices.

The build also runs Ghidra's GNU v2 demangler (`decomp/tools/demangle.py`; it finds it in the
Ghidra install `GHIDRA_INSTALL_DIR` names, default `D:\Tools\ghidra`, or takes `GNU_DEMANGLER`).

### 64-bit

`THPS_ARCH=x64` builds a 64-bit program into `build/pc64` (Linux: `build/pc64-linux`), with
`--release` too. On Windows it needs MSYS2's `ucrt64` toolchain and SDL2
(`pacman -S mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-SDL2`). `tools/ptr64_check.py` (with
both builds made, and the `mingw32` toolchain for its 32-bit objects) compares every view of a
global, parameter and return value between the 32- and 64-bit layouts and lists where they
disagree.

```
THPS_ARCH=x64 python platform/pc/tools/build.py   # -> build/pc64/thps.exe
```

`THPS_ASAN=1` (Linux, with its own `THPS_BUILD_DIR`) builds the game code, its data and the runtime
with AddressSanitizer, which reports a read or write past a global's end (the decompiled code's
views of the data are its own declarations, so the compiler can't check them) and use after free.
Run it with `ASAN_OPTIONS=halt_on_error=0:handle_segv=0:allow_user_segv_handler=1:detect_leaks=0`:
the port's own fault handlers stay in charge.

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
and OpenGL libraries (`libsdl2-2.0-0:i386 libgl1:i386 libgl1-mesa-dri:i386`). The 64-bit build
(`THPS_ARCH=x64`, -> `build/pc64-linux/thps`) needs the native `libsdl2-dev` as well as the
32-bit compiler (for the data check), and runs on the native SDL2 and OpenGL libraries.
`build.py --release` makes `build/pc-linux/release/thps-linux.tar.gz`: the stripped `thps`,
`dist/README-linux.txt`, the LICENSE and the notices (SDL2 and the C library are the system's,
linked dynamically, so build releases on an old distribution for an old enough glibc).

How the Linux build differs:

- The compiler emits ELF instead of PE/COFF. Game TUs are compiled with `-fleading-underscore`,
  so their symbols look like MinGW's and the renaming pipeline is the same on both systems;
  `build.py` takes the `_` off at the end. `-freg-struct-return` keeps the Windows ABI's 8-byte
  struct returns in registers (the decomp declares some functions both ways; see Calling
  convention). Static constructors (`.init_array`, Windows `.ctors`) are dropped: the game runs
  them from its own table.
- Low-memory accesses, the MIPS division results and crash reports come from signal handlers
  (`SIGSEGV`, `SIGFPE`, `SIGTRAP` for the single step) instead of a vectored exception handler, on
  the same decoders (`src/pc_main.c`). The hang watchdog asks the main thread for its backtrace with `SIGUSR2`.
  `THPS_WATCH` (debug-register watchpoints) is Windows-only: use a gdb watchpoint.
- Gamepads are SDL game controllers only (no XInput), mapped onto the same Xbox-style inputs.
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

At runtime files are found by name anywhere under `Data/`. The game's CD.WAD loader
(FileIO_Open/Load/Continue/Sync) reads them through `pc_fileio.h` (one plain file read per load, finished
when FileIO_Load returns), and songs and movies stream
from their files (`src/pc_stream.c`). The disc image is read only by the extraction
(`src/pc_disc.c`). Missing files are extracted again on the next start
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
see `include/pc_dctex.h`. With these files each skater's front-end model is the full one (the
Dreamcast has none). Every file is
extracted unchanged. The images it redrew at a higher resolution are shown at the size the PS1
layout gives them, with all their pixels (`PC_ImageLayoutSize`): the 640x480 front-end screens
load as they are instead of line-doubled, and the HORSE loading screen (640x240 on a 512-wide
screen) and the pause logo (320x88 for the PS1's 256x44) are drawn scaled into the display
surfaces (`PC_ScreenBlitHiRes`, `SlicedImage::drawScaled`).

Its songs and movies are Sofdec films (`.sfd`: MPEG-1 video, CRI ADX audio; `src/pc_sfd.c`
demultiplexes them and decodes the ADX). They stream like the STR files (`ATVILOGO.STR` is
`activision.sfd`, `LOGO.STR` `never.sfd`): the 44.1 kHz stereo audio plays through the music input; the playlist lists the `.sfd` songs, and a
song starts over at the end of its audio as the PS1's does at its end frame. The full-screen
movies play (`src/pc_movie.c`: pl_mpeg decodes the 320x240 video into the movie's picture,
timed to the audio; any button skips, as on the PS1); the songs' 96x64 thumbnail video is not
shown. Its sound banks (`.kat`: the same sounds as 8/16-bit PCM at 22 kHz instead of 4-bit ADPCM
at about 11 kHz) become the VABs the game loads (`src/pc_dcsound.c`): each sound is a PCM sample
played in place of a stand-in in the bank's sample data, tuned to the PS1 banks' pitch. The Dreamcast
logos (Crave, Treyarch, Sofdec) are not played. Needs the OpenGL renderer.

## Checking changes

The port's changes are checked by running it deterministically (`THPS_DETERMINISTIC=1`, scripted
input `THPS_INPUT`, screenshots `THPS_SHOT`, `THPS_HEADLESS`) through the levels in both 3D
renderers, the front end, the movies and a career save, and comparing the frames and the save
file with an earlier build's: a change to the port's internals should leave them identical.

## Saves

There's no memory card. Each career and each replay is its own file, named after the name the
player gives it, in `%USERPROFILE%\Saved Games\Neversoft\Tony Hawk's Pro Skater\Saves` (Linux:
`~/.local/share/Neversoft/Tony Hawk's Pro Skater/Saves`):
`Careers\<name>.career` and `Replays\<name>.replay` (`src/pc_saves.c`). The game's save and load
screens are its own; its card layer (`game/src/card`) reads and writes these files
and is always ready, so there's no card check, formatting, block count or "do not remove" text.
A file is the game's save data without the PS1 card header (icon, Shift-JIS title), behind a
16-byte header. The load screens list up to 15 saves of each type, newest first. The career saved
most recently is loaded at startup, as on the PS1, without the "loading from memory card" pause.
PS1 saves in `.mcs` form (MemcardRex, emulators) put in the `Saves` folder are converted the next
time the game lists saves (at startup, or on a save or load screen) and renamed `.mcs.imported`.

## Graphics

The game builds its frames as on the PS1: primitive packets (`POLY_FT4`, `SPRT`, `TILE`,
`LINE_*`, `DR_*`) linked into ordering tables, which are its display list. `src/pc_draw.c` walks
them (`PC_DrawOT`, where the game called `DrawOTag`) and draws through
OpenGL 3.3 (`src/pc_gpu_gl.c`): polygons, sprites, lines and fills go to render targets, and a
shader does the PS1's CLUT texture lookup, texture windows, colour modulation and the four
semi-transparency modes. There is no VRAM. The textures and CLUTs are in the texture store
(`src/pc_texstore.c`), a 16-bit copy at the coordinates the game's own allocators (Pack, Pal) give
them, which the game's primitives refer to; the screens' pixels are only in the render targets.
The game's own reads and writes of a screen (the front end saves the background under a sprite
and puts it back, the credits and loading screens put up pictures) are `PC_ScreenRead` and
`PC_ScreenBlit` (`src/pc_screen.c`); reads scale a surface down to the PS1's resolution. There's
no dithering: the picture is never reduced to 15 bits for display.
The screens are drawn at the resolution they are shown at: each area the game uses as a whole
screen (its display area, or a screen-sized drawing area: the two 512x240 gameplay buffers, the
640x480 front end) gets a *display surface*, a render target the size of the window's 4:3
picture area, so in a 1024x768 window both are drawn at 1024x768 (in 1920x1080, at 1440x1080
with black bars, unless widescreen widens gameplay, below). Anything drawn elsewhere stays at the
PS1's resolution, in a 1x target for the whole 1024x512 space; a pixel belongs to the surface
covering it, else to that target. `THPS_GL_SCALE=n` draws the surfaces at n times the PS1
resolution instead (1: exactly the PS1's pixels, for comparisons).

**Widescreen** (Hor+, `[Video] Widescreen = On`, the default): on a window wider than 4:3, the
gameplay screens (512x240) get margins left and right, `pc_wide_pad` PS1 pixels each (85 at 16:9,
at most 400), and the view shows that much more of the world at the sides at the same zoom. The
margins exist only in the display surface: in drawing coordinates they lie at x < 0 and past the
buffer, where the game keeps textures, so only primitives drawn with a drawing area that covers
the whole screen reach them (`pc_draw.c` widens it, `PC_GlWideClip`, and flags the primitive);
every other drawing and the CPU's blits see the surface's own area alone. The game's
side culling widens to match: the frustum's side planes (`M3d_RenderSetup`) and the
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
that keeps retail's visibility, colours (the same depth-cued colours) and ordering-table position,
then draws with a projection, a depth buffer and OpenGL's near-plane clipping instead of
subdivision. The faces are drawn while `pc_draw.c` walks the ordering table, so everything still
built as packets (particles, shadows, 3D text, the HUD) keeps its place in the PS1's painter order;
of those, the ones the game projected are depth-tested against the world. Those packets use the
precise-vertex lookup: every projection (`pc_geom.h`) is also recorded unrounded, with its depth,
keyed by the screen coordinates the game puts in its packets (`src/pc_precise.c`), and the walker
looks a polygon's vertices up again and draws it at sub-pixel positions with perspective-correct
textures (THPS1's polygon subdivision re-projects and nudges vertices, which the lookup allows for,
see `PC_PrecisePolygon`). Faces the native renderer doesn't handle yet (water clipping and the
fade-to-colour mode of some levels) still go through the retail code and that lookup.
`THPS_RENDER3D=psx` draws all 3D the PS1 way. With that and the lookup turned off
(`THPS_RENDER3D=psx THPS_GL_PRECISE=0 THPS_GL_PERSPECTIVE=0`), OpenGL at 1x samples the same
points the PS1 rasteriser does.

The build compiles every TU in `game/tus.tsv`, generates the glue in `build/pc/gen/` and links.
`build/pc/build_report.txt` lists compile errors and unresolved symbols.

## Controls

Keys and gamepad buttons are bound to game actions (`src/pc_input.c`). Each action takes two keys
and two gamepad buttons, kept in `%USERPROFILE%\Saved Games\Neversoft\Tony Hawk's Pro Skater\Settings.ini`
(Linux: `~/.local/share/Neversoft/Tony Hawk's Pro Skater/Settings.ini`)
(written with the defaults on the first startup). The keyboard plays player 1; each player has
their own gamepad bindings (`[Skate Gamepad]`/`[Menu Gamepad]` for player 1, `... Gamepad 2` for
player 2).

Gamepads are read through XInput (Windows) and SDL's game controllers, into the same Xbox-style
inputs: SDL takes the pads XInput doesn't read (DualShock, DualSense, Switch, generic USB pads) and
skips the ones it does, so no pad counts twice. Other pads' buttons are named after the Xbox one in
the same place (A = Cross). The players get the first two connected, XInput's before SDL's.

**Options > Player 1/2 Controls** is the rebinding screen (it replaces the PS1's face-button
remap; `game/src/front/controlleroptions.cpp`). The top row switches between the
keyboard and gamepad bindings (player 2: gamepad only), then come Autokick and Vibration, every
Skate and Menu action with its two bindings, Reset to Default (the device shown) and Back. Left
and right pick the binding, Accept waits for the new key or button (5 seconds), Delete or X on
the gamepad clears it. Taking an input another action of the same set already has swaps the two,
so nothing is left unbound by accident. Changes are saved when the screen is left. The screen
draws its own list over `options.bmp`; it puts that area of the background back each frame from
`pc_hires_before_draw`, a hook in the hi-res frame end that runs after the wait for the next field
(restored any earlier, the background would be shown without its text).

The game reads the actions themselves. Its per-player button block (`SControl`,
`game/include/game/pad.h`) names its buttons after what they do while skating (`Ollie`,
`Grind`...), and the menus read `MenuAccept`, `MenuBack` and `MenuUp`..`MenuRight`. On the PS1
those menu names are aliases of the pad's own buttons (X, Triangle, the D-pad), so the matching
build is unchanged; on PC they are separate slots (the ones the PS1 never filled), and
`Pad_Update` (`game/src/unnamed/80070ff4.c`) fills every slot from its action's bindings. So
the same key can do one thing while skating and another in the menus. On PC, the front end's
"press Start" prompts take Menu Accept, and Pause also resumes from the pause menu.

| action | keyboard | gamepad (Xbox names) | PS1 button |
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

On PC the main menu (`game/src/unnamed/8005e754.c`) ends with **QUIT**, which exits the game
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

**Options > Graphics** (`PC_GraphicsOptions` in `game/src/front/controlleroptions.cpp`, next to
the controls screen; the options menu `game/src/unnamed/80060274.c` gets the extra row on PC)
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
| `THPS_GL_PRECISE=0` | OpenGL: draw 3D polygons at the PS1's whole-pixel positions (default: sub-pixel, from the projections) |
| `THPS_GL_PERSPECTIVE=0` | OpenGL: affine texturing like the PS1 (default: perspective-correct) |
| `THPS_RENDER3D=psx` | OpenGL: draw the world and models through the PS1 path (default `native`, see Graphics) |
| `THPS_R3DSTATS=1` | once a second: native 3D faces, culled faces, draws, and calls left to the retail code |
| `THPS_R3D_VERIFY=1` | check the native renderer's float vertices and back-face tests against retail's integers |
| `THPS_R3D_NEARDROP=1` | drop faces crossing the near plane that the PS1 dropped (default: OpenGL clips them) |
| `THPS_R3D_PROBE=f,x,y` | log the 3D faces covering screen point x,y on frame f |
| `THPS_R3D_RETAILFADE=1` | leave fade-to-colour mode (Downhill Jam, Streets, the front end) to the retail code (A/B; not for Dreamcast models) |
| `THPS_FILTER=1` | bilinear scaling to the window instead of nearest-neighbour |
| `THPS_GLSTATS=1` | once a second: draw calls, screen readbacks, texture uploads and blits, and how many polygons had precise vertices |
| `THPS_FRAMES=n` | exit after n frames |
| `THPS_SHOT=f1,f2` / `THPS_SHOTDIR` | dump the display (at the PS1's resolution, and `frameNNNNN_gl.bmp` at the resolution it was drawn at) and the 1024x512 space (`vramNNNNN.bmp`: the screens, and the texture store where it holds textures) as BMPs at those frames |
| `THPS_INPUT=frame:mask,...` | scripted pad input for player 1 (PS1 button mask, pressed = 1, held from that frame; X is both Ollie and Menu Accept, Triangle Grind and Menu Back, Start is Pause) |
| `THPS_KEYS=frame:Key+Key,...` | scripted keyboard (Settings.ini key names, held from that frame; `frame:` releases), read through the bindings |
| `THPS_NOGAMEPAD=1` | ignore connected gamepads, so scripted runs navigate the menus the same way with or without one |
| `THPS_TRACE=1` | log streams (songs, movies), the game's debug prints, and player 1's actions as they change |
| `THPS_FAST=1` | don't throttle to 60 Hz |
| `THPS_MOVIES=0` | skip every movie |
| `THPS_STRICT=1` | abort on a call to an unimplemented function |
| `THPS_DETERMINISTIC=1` | virtual clock: identical runs every time (use with `THPS_INPUT` for bisecting) |
| `THPS_POKE=name[+offset]:value,...` | store those words every frame into globals of `game/data` (`Cheat_LevelSelect:1` = level select cheat: every level unlocked; `GCareer+0x82c:4` = the last level played) |
| `THPS_WATCH=addr` | hardware write watchpoint on a 4-byte word: logs value, native EIP and callers |
| `THPS_GPUTRACE=frame` / `THPS_GPUSTATS=1` | log fills and drawing-environment changes / polygon counts per frame |
| `THPS_TRACE_CULL`, `THPS_TRACE_RENDER` | frustum-culling and renderer traces |
| `THPS_AUDIO=0` / `THPS_AUDIO_DUMP=file` | no sound device / also write the mixed output as raw s16le stereo 44.1 kHz |
| `THPS_TRACE_SOUND=1` | log sound channel key on/off, bank loads and reverb modes (with `THPS_TRACE=1`) |

Crash logs and the 5-second hang watchdog print native backtraces;
`python platform/pc/tools/symbolize.py < log` turns them into function/file:line.
`python platform/pc/tools/arity_check.py` lists decompiled functions declared with fewer arguments
than they really take (harmless on MIPS, where the value stays in its register; broken natively).

## Status

Boots through the front end (menus, skater/board/level select with 3D models) into a level, and
the game plays with full 3D rendering. Sound effects and music play: `src/pc_sound.c` decodes the sound
banks to PCM, plays them on 24 channels with the PS1's ADSR envelopes and gains, adds a reverb in
the PS1 modes' spirit (Freeverb-style rooms and halls, feedback echoes: not the SPU's algorithm)
and the music, and mixes it to an SDL audio device. The songs are XA audio inside the soundtrack STR files; `src/pc_stream.c` streams them
from the extracted files at the rate the drive read them (150 sectors a second) and `src/pc_xa.c`
decodes them. `src/pc_music.c` picks the next song from
`Data/Music/Playlist.m3u` instead of the game's fixed rotation. With the Dreamcast's data the
music, movies and sound effects come from its films and sound banks (see above). The PS1's
movies play through the game's own movie player (`GameFMV_PlayMovie`): the game decodes each
frame's bitstream itself (`DecDCTvlc3`), `src/pc_vdec.c` decodes the pictures (the MDEC's inverse
DCT and colour conversion, a whole frame at once) and the player shows them as a picture of
their own (`PC_MovieDecode`/`PC_MovieShow`, `pc_screen.h`); the player's busy-waits wait a frame
for the stream instead (`PC_MovieWait`). The songs' 96x64 video goes into the texture of the
levels' TV screens the same way (`PC_VdecToTexture`; the credits screen shows it too). Not done yet: the PS1's exact
reverb.

Port glue worth knowing about (see the tools for details):
- C++ follows the egcs ABI where the retail code depends on it: vtable layout, `delete` through
  the virtual destructor with in_chrg 3, constructors returning `this`.
- Hardware behaviour the game relies on is kept: MIPS division never traps (the fault handler
  gives the MIPS result), busy-waits let frames happen (`PC_FramePoll`).

The port's game code is `game/`: change it directly (it has no PS1 branches). `decomp/` is the
matching decompilation, kept byte-exact and not built into the port.

## License

The port layer (`platform/pc/`) is MIT, like the rest of the project (see the repository's LICENSE).
`include/pl_mpeg/pl_mpeg.h` is Dominic Szablewski's PL_MPEG (MIT, see its header). The
decompiled game code and data, and all game assets, remain © Activision / Neversoft. No assets
or original executables are committed; the build reads your own `SLUS_008.60` (for the function
layout), and the game your own disc.
