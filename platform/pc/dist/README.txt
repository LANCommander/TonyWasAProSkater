Tony Hawk's Pro Skater for Windows
==================================

A native Windows port of Tony Hawk's Pro Skater (PlayStation, USA), built from a decompilation
of the game. This download contains none of the game's assets (levels, models, textures, sounds,
music, movies): you need your own copy of the game, and the port reads them from it.

Requirements
------------

- 64-bit Windows 10 or 11.
- A graphics card with OpenGL 3.3.
- Your own disc of Tony Hawk's Pro Skater (USA) for the PlayStation, as a raw disc image
  (MODE2/2352 .bin). Images in other formats can be converted, for example a .chd with
  chdman:  chdman extractcd -i "Tony Hawk's Pro Skater (USA).chd" -o thps1.cue -ob thps1.bin

Nothing else needs to be installed.

Setting up
----------

1. Put thps.exe in a folder of its own.
2. Put the disc image next to it, named thps1.bin.
3. Start thps.exe.

On the first startup the game copies what it needs off the disc image into a Data folder next
to thps.exe (about 630 MB; about 950 MB from the Dreamcast disc). After that the disc image is
no longer needed and can be removed; it is only opened again if something in Data is missing.

The Dreamcast version's data can be used instead (higher-resolution textures, remodelled
skaters, better-quality music): put the .gdi file and its track files next to thps.exe instead.
If both discs are there, the PlayStation one is used.

THPS_DISC=<path to .bin or .gdi> picks a disc image anywhere else.

Controls
--------

Keyboard and gamepads (XInput, and any controller SDL knows). Rebind them in Options > Player
1/2 Controls. Gamepad buttons are named as on an Xbox controller.
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

Settings, saves and the log are in your Saved Games folder:

  %USERPROFILE%\Saved Games\Neversoft\Tony Hawk's Pro Skater\
    Settings.ini         controls, display, draw distance
    Saves\Careers\       one .career file per saved career
    Saves\Replays\       one .replay file per saved replay
    thps.log             what the last session logged (include it with bug reports)

PlayStation saves in .mcs form (from MemcardRex or emulators) put in the Saves folder are
converted the next time the game lists saves.

Data\Music\Playlist.m3u is the order the soundtrack plays in; edit it to change or reorder the
songs.

Legal
-----

Tony Hawk's Pro Skater is (c) 1999 Activision, Inc. and Neversoft Entertainment. This project
is not affiliated with or endorsed by Activision or Neversoft. You need to own the game to
play it. See LICENSE.txt and THIRD-PARTY-NOTICES.txt.
