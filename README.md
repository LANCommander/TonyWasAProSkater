# Tony Hawk's Pro Skater for Windows and Linux

A native Windows and Linux port of **Tony Hawk's Pro Skater** (PlayStation, USA, `SLUS_008.60`),
built from a matching decompilation of the game's code. The game runs as a 32-bit program with an
OpenGL renderer, keyboard and gamepad controls, widescreen, high resolutions and saves as plain
files. It can also use the game assets from the Dreamcast version.

This repository contains **no game code or data from the original game**: to build it or play it you need your own copy.

## Status

- **Fully playable**: Pretend you're Superman through all classic 9 levels in career mode.
- **Accurate gameplay**: Forked from an effort for a byte-exact [decompilation](https://github.com/LANCommander/THPS) of the original PS1 (US) release.
- **Open source**: This port contains no copyrighted code or assets and is MIT licensed.
- **Cross-platform**: For the first time, play THPS natively on Windows or Linux.

Known gaps: the PS1's exact reverb (the port's own is an approximation), a 64-bit build. See [platform/pc/README.md](platform/pc/README.md#status).

## Playing

Requirements:

- 64-bit Windows 10 or 11, or x86 Linux with the 32-bit SDL 2 and OpenGL libraries, and a
  graphics card with support for OpenGL 3.3.
- A raw disc image (MODE2/2352 `.bin`) of your own **Tony Hawk's Pro Skater (USA)** PlayStation
  disc.

Put `thps.exe` (Linux: `thps`) in a folder, put the disc image next to it as `thps1.bin` and start
it. On the first startup it copies what it needs from the disc into a `Data` folder (about
630 MB); after that the disc image isn't needed. The Dreamcast disc (`.gdi` and its tracks) works
instead.

Settings, saves and the log are in `%USERPROFILE%\Saved Games\Neversoft\Tony Hawk's Pro Skater`
(Linux: settings and saves in `~/.local/share/Neversoft/Tony Hawk's Pro Skater`, the log on the
terminal). The player's guides that ship with the game are
[platform/pc/dist/README.txt](platform/pc/dist/README.txt) and
[platform/pc/dist/README-linux.txt](platform/pc/dist/README-linux.txt);
controls, display options and the debugging switches are documented in
[platform/pc/README.md](platform/pc/README.md).

## Building

On Windows or x86 Linux. You need:

| | |
|---|---|
| Windows: [MSYS2](https://www.msys2.org) | the 32-bit MinGW toolchain and SDL2: `pacman -S mingw-w64-i686-gcc mingw-w64-i686-SDL2` (expected in `C:\msys64\mingw32`, or set `THPS_MINGW` to its `bin`) |
| Linux: GCC with 32-bit support, SDL2 (32-bit) | Debian/Ubuntu: `dpkg --add-architecture i386`, then `apt install gcc-multilib g++-multilib pkg-config libsdl2-dev:i386`; other distributions in [platform/pc/README.md](platform/pc/README.md#linux) |
| Python 3 | the build scripts (standard library only) |
| [Ghidra](https://ghidra-sre.org) | its GNU v2 demangler (`GPL/DemanglerGnu`), which the build runs: set `GHIDRA_INSTALL_DIR` (default `D:\Tools\ghidra`) |
| `extern/thps1_retail/SLUS_008.60` | the executable from your disc (`dumpsxiso` from [mkpsxiso](https://github.com/Lameguy64/mkpsxiso) extracts it); the build takes the function layout from it and checks the decompiled data against it. The game doesn't need it |
| `build/psyq_names.tsv` | the addresses of the Sony library functions: `python tools/psyq_sigmatch.py` makes it once, from the Psy-Q signatures of [ghidra_psx_ldr](https://github.com/lab313ru/ghidra_psx_ldr) (set `SIGDIR` in the script to its `data/psyq` folder) |

```
python platform/pc/tools/build.py            # development build: build/pc/thps.exe (Linux: build/pc-linux/thps)
python platform/pc/tools/build.py --release  # release build:     build/pc/release/ (Linux: build/pc-linux/release/)
```

Both embed the game's data (its initialised globals: strings, tables, class tables), built from
the decompiled data in `decomp/data` and checked byte for byte against the retail executable
(`platform/pc/tools/data_image.py`; the build fails if they differ). A **development build** has
a console with the log and keeps its debug information. It finds the disc image for the first
startup in `extern/thps1_disc/thps1.bin`.

A **release build** is what gets distributed. `build/pc/release/` holds:

| file | |
|---|---|
| `thps-windows.zip` | the download: `thps.exe`, `README.txt` (the player's guide), `LICENSE.txt`, `THIRD-PARTY-NOTICES.txt` |
| `thps.exe` | no console window: the log goes to `thps.log` in the Saved Games folder |
| `thps.debug` | its debug information, not distributed: `THPS_SYMBOLS=build/pc/release/thps.debug python platform/pc/tools/symbolize.py < thps.log` turns a crash log's addresses into functions and lines |

On Linux, `build/pc-linux/release/` has `thps-linux.tar.gz` (`thps`, `README.txt` from
`README-linux.txt`, `LICENSE.txt`, `THIRD-PARTY-NOTICES.txt`), `thps` and `thps.debug`.

### Dependencies

`thps.exe` needs nothing but Windows and an OpenGL 3.3 driver. Everything else is linked in
statically:

| library | license | used for |
|---|---|---|
| [SDL 2](https://libsdl.org) | zlib | window, input, audio, OpenGL context |
| [PL_MPEG](https://github.com/phoboslab/pl_mpeg) (`platform/pc/include/pl_mpeg`) | MIT | the Dreamcast version's MPEG-1 movies |
| [glad](https://github.com/Dav1dde/glad) (`platform/pc/src/glad_gl.c`) | CC0 / WTFPL, Khronos API data Apache-2.0 | OpenGL loader |
| MinGW-w64 runtime, libgcc | ZPL-2.1 and others; GPL with the GCC Runtime Library Exception | C runtime |

It imports only system DLLs (`kernel32`, `user32`, `gdi32`, `msvcrt`, `winmm`, `ole32`, ...);
XInput and OpenGL are loaded at runtime. The release zip's `THIRD-PARTY-NOTICES.txt` carries
these libraries' notices, the SDL and MinGW-w64 ones taken from the toolchain that was linked.

The Linux `thps` links the system's SDL 2 and C library dynamically (PL_MPEG and glad are built
in); OpenGL comes through SDL. Build releases on an old distribution so the glibc they need is old
enough.

## Repository layout

| path | what |
|---|---|
| `decomp/` | the matching decompilation: `src/` (one file per function or class, with its symbols), `data/` (the initialised globals, one file per module), `include/game/` (class and struct layouts), `functions.tsv` (every function in the retail game code), `matched.tsv` and `reimpl.tsv` (what is byte-exact, what is rewritten for the port), the matching toolchain in `tools/`. See [decomp/README.md](decomp/README.md) |
| `platform/pc/` | the Windows and Linux port: runtime, reimplemented Sony libraries (GPU, GTE, MDEC), sound, renderers, build scripts, tests. See [platform/pc/README.md](platform/pc/README.md) |
| `tools/` | analysis: the Psy-Q `.SYM` parser, the demo-to-retail function matcher, disc and file format tools |
| `docs/` | write-ups: how the work was scoped, the Dreamcast data |
| `scripts/`, `ghidra_scripts/` | Ghidra project setup and symbol import |

The decompilation started from an early THPS2 demo that shipped with full debug symbols (function
names, types and source files of the same engine); [docs/coverage-report.md](docs/coverage-report.md)
describes how those were carried over to THPS1.

## Checking the code

```
cd decomp && node tools/check_all.mjs    # recompile every matched function, compare with the retail bytes
python platform/pc/tools/data_image.py build/pc/image.bin --check extern/thps1_retail/SLUS_008.60
                                         # build the data image from decomp/data, compare with the retail data
```

`check_all` needs Node, `npm install` in `decomp/`, and the Psy-Q 4.5 SDK's compiler
(`CC1PSX.EXE`, `CC1PLPSX.EXE`, `CPPPSX.EXE` and `include/`, by default in `D:\Tools\psyq4.5`;
`CC1`, `CC1PL`, `CPP` and `PSYQ_INC` point elsewhere), which isn't distributed here. The C that
replaced the hand-written assembly and the Sony libraries is checked against the original MIPS by
replaying recorded game states (`platform/pc/tools/build_test_asm.py`; see
[platform/pc/README.md](platform/pc/README.md#checking-c-against-the-retail-code)).

The C in `decomp/` is formatted with `python decomp/tools/format.py src include` (from
`decomp/`: clang-format plus a blank-line pass, which never changes the generated code).

## Legal

Tony Hawk's Pro Skater is © 1999 Activision, Inc. and Neversoft Entertainment. This project is
not affiliated with or endorsed by Activision or Neversoft. It contains the decompiled code and
data, but none of the game's assets (levels, models, textures, sounds, music, movies) and no
original executable: the game reads its assets from your own copy. Don't ask for, and don't
share, disc images or the game's files.

The project's own code is under the license in [LICENSE](LICENSE). Third-party code keeps its own
license (see Dependencies above).
