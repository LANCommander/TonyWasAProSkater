Tony Hawk's Pro Skater for Linux
================================

A native Linux port of Tony Hawk's Pro Skater (PlayStation, USA), built from a decompilation
of the game. This download contains none of the game's assets (levels, models, textures, sounds,
music, movies): you need your own copy of the game, and the port reads them from it.

Requirements
------------

- x86 Linux (64-bit is fine) with 32-bit library support. thps is a 32-bit program: the game
  keeps the PlayStation's memory addresses.
- The 32-bit SDL 2 library and a 32-bit OpenGL 3.3 driver. For example:
    Debian, Ubuntu:   sudo dpkg --add-architecture i386 && sudo apt update
                      sudo apt install libsdl2-2.0-0:i386 libgl1:i386 libgl1-mesa-dri:i386
    Fedora:           sudo dnf install SDL2.i686 mesa-dri-drivers.i686 mesa-libGL.i686
    Arch:             enable [multilib], then sudo pacman -S lib32-sdl2 lib32-mesa
  (with the NVIDIA driver, its 32-bit libraries instead of Mesa's).
- Your own disc of Tony Hawk's Pro Skater (USA) for the PlayStation, as a raw disc image
  (MODE2/2352 .bin). Images in other formats can be converted, for example a .chd with
  chdman:  chdman extractcd -i "Tony Hawk's Pro Skater (USA).chd" -o thps1.cue -ob thps1.bin

Setting up
----------

1. Put thps in a folder of its own.
2. Put the disc image next to it, named thps1.bin.
3. Start thps (./thps from a terminal shows the log).

On the first startup the game copies what it needs off the disc image into a Data folder next
to thps (about 630 MB; about 950 MB from the Dreamcast disc). After that the disc image is no
longer needed and can be removed; it is only opened again if something in Data is missing. A
Data folder from the Windows version works as it is.

The Dreamcast version's data can be used instead (higher-resolution textures, remodelled
skaters, better-quality music): put the .gdi file and its track files next to thps instead.
If both discs are there, the PlayStation one is used.

THPS_DISC=<path to .bin or .gdi> picks a disc image anywhere else.

Controls
--------

Keyboard and gamepads (any controller SDL knows). Rebind them in Options > Player 1/2 Controls.
Gamepad buttons are named as on an Xbox controller.
A pad the game doesn't recognise can be added with a gamecontrollerdb.txt next to the
game or Settings.ini (https://github.com/mdqinc/SDL_GameControllerDB).

  Move                       W A S D / arrows          D-pad, left stick
  Ollie                      Space                     A
  Flip                       J                         X
  Grab                       L                         B
  Grind                      I                         Y
  Spin left / right (held)   Q / E                     LB / RB
  180 spin per tap           Z / C                     LT / RT
  Camera                     Tab                       Back
  Pause                      Escape                    Start
  Menu accept / back         Return / Escape           A / B

Options > Graphics sets the display mode (windowed, borderless, fullscreen), the resolution and
widescreen. Alt+Enter switches between windowed and fullscreen.

Files
-----

Settings and saves are in ~/.local/share (or $XDG_DATA_HOME):

  ~/.local/share/Neversoft/Tony Hawk's Pro Skater/
    Settings.ini         controls, display, draw distance
    Saves/Careers/       one .career file per saved career
    Saves/Replays/       one .replay file per saved replay

The log goes to the terminal (include it with bug reports: ./thps 2> thps.log).

PlayStation saves in .mcs form (from MemcardRex or emulators) put in the Saves folder are
converted the next time the game lists saves.

Data/Music/Playlist.m3u is the order the soundtrack plays in; edit it to change or reorder the
songs.

Legal
-----

Tony Hawk's Pro Skater is (c) 1999 Activision, Inc. and Neversoft Entertainment. This project
is not affiliated with or endorsed by Activision or Neversoft. You need to own the game to
play it. See LICENSE.txt and THIRD-PARTY-NOTICES.txt.
