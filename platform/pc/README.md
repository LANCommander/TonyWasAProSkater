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
  memory at the same addresses and GTE instructions going to the software GTE. Every game
  function now has C instead: the hand-written ones are C interpretations listed in
  `decomp/reimpl.tsv` (built like decompiled C), each checked against the retail code by a test
  in `tests/` that runs the original MIPS in a small reference interpreter (`tests/mips_ref.c`).
  `reimpl.tsv` also holds byte-exact C for the few compiler-generated C++ functions a native
  build can't take from their matching source. What is still translated is Psy-Q library code.
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
  (`chdman extractcd -i "Tony Hawk's Pro Skater (USA).chd" -o thps1.cue -ob thps1.bin`), needed
  for the first startup only.

```
python tools/psyq_sigmatch.py          # once: names the Psy-Q library functions (build/psyq_names.tsv)
python platform/pc/tools/build.py      # -> build/pc/thps.exe
build/pc/thps.exe
```

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
| `Data/Movies/*.STR` | the full-screen movies |
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
logos (Crave, Treyarch, Sofdec) are not played. The executable is still the PS1's `SLUS_008.60`.
Needs the OpenGL renderer.

## Saves

The memory card is a folder: `%USERPROFILE%\Saved Games\Neversoft\Tony Hawk's Pro Skater\Saves`
(`src/hle_libcard.c`). The game's own card code runs unchanged and sees a formatted card in slot 1;
each card file (career saves `BASLUS-00860THPSG..`, replays `..THPSV..`) is one `.mcs` file there,
the single-save format MemcardRex and the emulators import and export, so saves can be moved
between the port, an emulator and a real card. Like a real card it holds 15 blocks (a career save
takes 1, a replay 3 or 6); files that don't fit are ignored. The career saved most recently is
loaded at startup, as on the PS1.

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
points the PS1 rasteriser does, and the result matches the software renderer almost pixel for
pixel. That renderer (a Mednafen-style rasteriser) is still there for comparison and for when
OpenGL is unavailable: `THPS_RENDERER=sw`.

The build compiles every TU in `decomp/matched.tsv` (and the near-miss sources in
`decomp/nearmiss.tsv`), translates the remaining MIPS with `tools/mips2c.py`, generates the glue in
`build/pc/gen/` and links. `build/pc/build_report.txt` lists compile errors and unresolved symbols.

## Controls

Keys and gamepad buttons are bound to game actions (`src/pc_input.c`). Each action takes two keys
and two gamepad buttons, kept in `%USERPROFILE%\Saved Games\Neversoft\Tony Hawk's Pro Skater\Settings.ini`
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
the same way closing the window does (`PC_RequestQuit`, acted on at the next vblank). Its row id
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
| `THPS_EXE`, `THPS_DISC` | paths to `SLUS_008.60` and the disc image (else found next to the exe or in `extern/`); a `.gdi` is the Dreamcast disc |
| `THPS_DATA_DIR` | where the extracted data lives (default `Data` next to `thps.exe`) |
| `THPS_SAVE_DIR` | the memory card folder (default `%USERPROFILE%\Saved Games\Neversoft\Tony Hawk's Pro Skater\Saves`) |
| `THPS_SCALE=n` | headless: window size n x 320x240 (default 3) |
| `THPS_WINDOW=WxH` | a window of that size (drawing area), whatever Settings.ini says; headless too |
| `THPS_WIDESCREEN=0\|1` | widescreen off or on, whatever Settings.ini says |
| `THPS_HEADLESS=1` | no window (for automated runs; uses the software renderer unless `THPS_RENDERER=gl`) |
| `THPS_RENDERER=gl\|sw` | OpenGL (default) or the software rasteriser |
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
| `THPS_TRACE=1` | log CD, BIOS and debug-print activity, and player 1's actions as they change |
| `THPS_FAST=1` | don't throttle to 60 Hz |
| `THPS_MOVIES=0` / `1` | skip every movie / run the original FMV code for all (by default the Dreamcast's films play and STR movies are skipped) |
| `THPS_STRICT=1` | abort on a call to an unimplemented function |
| `THPS_DETERMINISTIC=1` | virtual clock: identical runs every time (use with `THPS_INPUT` for bisecting) |
| `THPS_MIPS=all,!lo-hi,addr` | run those game functions as translated retail MIPS instead of decompiled C (A/B debugging) |
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
music, movies and sound effects come from its films and sound banks (see above). Not done yet:
SPU reverb, exact ADSR curves, PS1 FMV video (MDEC; STR movies are skipped), 64-bit build.
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
port (`HalmyLyseas/VandalHearts-PcPort`, GPL-2.0), credited in each file that uses them.
`include/pl_mpeg/pl_mpeg.h` is Dominic Szablewski's PL_MPEG (MIT, see its header). The
decompiled game code and all game data remain © Activision / Neversoft. Nothing copyrighted is
committed; the build reads your own `SLUS_008.60` and disc files.
