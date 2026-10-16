# Mods

The PC port loads mods: folders of files that replace the game's files or add to them, including
new levels and new skaters. Code: `platform/pc/src/pc_mods.cpp` (finding mods, the overlay),
`pc_modcontent.cpp` (levels, skaters, their career progress), `pc_ini.cpp` (the INI reader).

## Installing a mod

A mod is a folder with a `mod.ini` in it. The game looks for mods in, in this order:

1. `Data\Mods\<mod>\`, next to `thps.exe`;
2. `Mods\<mod>\` in the player's folder, next to `Settings.ini`
   (`%USERPROFILE%\Saved Games\Neversoft\Tony Hawk's Pro Skater\Mods`, Linux
   `~/.local/share/Neversoft/Tony Hawk's Pro Skater/Mods`);
3. `THPS_MODS`: a list of folders, separated by `;` on Windows and `:` on Linux. Each is a mod
   (it has a `mod.ini`) or a folder of mods. `THPS_MODS=none` turns all mods off.

Within a folder, mods load in alphabetical order of their folder names. A folder found twice is
loaded once. A folder without a `mod.ini` is skipped, with a note in the log.

The log (`thps.log` in the player's folder, or the terminal) lists every mod as it loads, with its
name, version, author, date, website and description, what it replaces and adds, and anything
wrong with it.

## mod.ini

```ini
[Mod]
Name = Disco Mod
Author = Someone
Version = 1.0
Date = 2026-10-15
Description = Shiny new menus.\nAnd a second line.
Website = https://example.com
; Enabled = 0 leaves the mod out without removing it
```

Every entry is optional; `Name` defaults to the folder's name. `\n` in a value is a line break.

## Replacing and adding files

A mod's `Data` folder overlays the game's `Data` folder:

```
Data/
  Mods/
    DiscoMod/
      mod.ini
      Data/
        Textures/
          accept.bmp
```

The game finds its files by name, whatever folder they are in. So `DiscoMod\Data\Textures\accept.bmp`
is loaded instead of the game's `accept.bmp`. A file the game doesn't have is added (a new
level's script, models and pictures). When two mods have a file with the same name, the one
loaded later wins. Run with `THPS_TRACE=1` to log every replaced file.

The files have the formats the game's own have: `.psx` models and levels (the PlayStation's, or
the Dreamcast version's version 6), `.trg` level scripts (see `tools/trg_dump.py`), `.bmp`
images (most of them 8-bit, 256 colours, at the size of the file they replace), `.vab` sound
banks. Files inside the game's PRE archives (`Data\Packs`) are found in the archive first:
replace the whole archive to change one of them.

## Levels

`<mod>\Levels\<name>.ini` adds a level. The level's own files go in the mod's `Data` folder.
The game spools level geometry from its trigger script (`SPOOLIN`, `SPOOLENV`), so a level is a
script, its `.psx` files, and a picture for the level select.

```ini
[Level]
Name = Hangar
City = Mountain View
Base = Warehouse
Script = SkHang_T
Modes = FreeSkate, SingleSession, Multiplayer
Thumbnail = levhang.bmp
MapPosition = 470, 300
```

Only `Name` and `Script` are needed. Everything else defaults to the `Base` level's setting.

| entry | what | default |
|---|---|---|
| `Name`, `City` | the level select's two lines. The menu font has letters, digits and spaces only. Up to 31 characters. | |
| `Base` | a game level whose settings are the defaults: its name (`Warehouse`, `School`, `Mall`, `Skate Park`, `Downtown`, `Downhill Jam`, `Burnside`, `Streets`, `Roswell`), its script (`SkWare_T`) or its number (0-8) | Warehouse |
| `Script` | the trigger script, `<Script>.trg`. Up to 27 characters. | |
| `Modes` | the game modes the level is offered in, comma-separated: `Career`, `SingleSession`, `FreeSkate`, `Multiplayer` (all the two-player games) | all four |
| `Thumbnail` | the level select's picture: an 8-bit BMP, 448x161 (as `levsel0.bmp`) | the base's |
| `MapPosition` | where the level select's arrow points on the map, on the 640x480 screen | the base's |
| `SoundBank` | the level's sound bank, `<SoundBank>.vab` (`ware`, `comp`, ...) | the base's |
| `Competition` | `1`: a competition (three heats, judges, medals) instead of tape goals | the base's |
| `MedalScores` | a competition's gold, silver and bronze scores | the base's |
| `LoadScreen`, `LoadBanner` | a competition's loading screen pictures (8-bit BMP, 512x240 and 432x40) | the base's |
| `ScoreGoals` | the high score and pro score tape goals | the base's |
| `GoalName` | what the collect-five tape goal collects (`boxes`) | the base's |
| `TapesToUnlock`, `MedalsToUnlock` | what unlocks it in career mode | the base's |
| `Crowd` | `1`: the crowd's ambience and reactions (cheers, "oww"s) in career mode | the base's |
| `PolyBuffer` | extra polygon buffer, one player and two players | the base's |
| `TrickTips` | the two trick tip masks | the base's |
| `TV` | `1`: the level has the in-level video screen | the base's |
| `IntroPosition`, `IntroAngle` | a competition's opening camera, x, y, z | the base's |

A level that is missing its script or a picture it names is left out, with the reason in the log.
The game's own levels are in every mode. Mods can add up to 22 levels in all.

In career mode, tape goals on a mod's level are kept per skater like the game's own. The career's
tape count (which unlocks the game's levels, boards and Officer Dick) counts the game's levels
only. A mod level's `TapesToUnlock` counts the game's and the mods' levels' tapes together.
Endings and the Roswell unlock count only the game's own competitions.

## Skaters

`<mod>\Skaters\<name>.ini` adds a skater. The mod's `Data` folder has its models.

```ini
[Skater]
Name = Disco Stu
Base = Hawk
Model = disco
Model2P = disco2p
FrontModel = disco_fe
Stats = 6, 7, 5, 6
Age = 33
Country = USA
HomeTown = Springfield
YearsPro = 4
Stance = Regular
Height = 5'9
BoardSet = Muska

[Board1]
Image = disco01
Name = Disco Deck
Tapes = 0
Stats = 1, 1, 1, 1
```

Only `Name` and `Model` are needed.

| entry | what | default |
|---|---|---|
| `Name` | the full name. The first word is shown alone too and can be up to 15 characters. Up to 31 characters. | |
| `Base` | a game skater whose tricks (special tricks, trick set), model part numbers (board, wheels, head) and board/wheel/truck texture checksums are used: a surname (`Hawk`, `Burnquist`, `Rowley`, `Lasek`, `Muska`, `Campbell`, `Reynolds`, `Glifberg`, `Thomas`, `Steamer`, `Dick`), a full name, the model's name (`hawk`) or a number (0-10) | Hawk |
| `Model` | the in-game model, `<Model>.psx`. Up to 8 characters. | |
| `Model2P` | the model in two-player split screen | `Model` |
| `FrontModel` | the model the front end shows | `Model` |
| `Stats` | the four stat bars: ollie, speed, air, balance (1-10) | the base's |
| `Age`, `Country`, `HomeTown`, `YearsPro`, `Stance`, `Height` | the bio. A `Stance` starting with R is regular, anything else goofy. | the base's |
| `BoardSet` | a game skater whose eight boards are used (named as for `Base`) | Hawk's |

`[Board1]` to `[Board8]` replace that board of the set:

| entry | what |
|---|---|
| `Image` | the board's pictures: `b1<Image>.bmp` (one-player board select), `b2<Image>.bmp` (two-player board select) and `b_<Image>.bmp` (the deck texture in game), all three in the mod's `Data`. Up to 9 characters. They have the sizes and formats of the game's (`b1hawk02.bmp`, `b2hawk02.bmp`, `b_hawk02.bmp`). |
| `Name` | the board's name | `Image` |
| `Tapes` | career tapes needed to unlock it | the set's |
| `Stats` | the bonus bars of the best unlocked board (ollie, speed, air, balance) | the set's |

So a skater with no boards of its own and no `BoardSet` gets Tony Hawk's boards. The deck
texture goes where the `Base` skater's model has its board texture. A skater's own model has to
use the same texture checksums, or name a `Base` whose do.

Each skater needs model files of its own. The game keeps loaded models by file name, so a mod's
skater can't share a model file with another skater: copy it under another name. Mods' skaters
come after the game's in the skater selects (after Officer Dick once he is unlocked). In a
competition, a mod's skater takes the place of a random one of the ten competitors, as Officer
Dick does. Mods can add up to 21 skaters in all.

## Saves

A career save (`.career`) has room for the game's 11 skaters and 9 levels only. A career's
progress with mods' skaters and levels goes in `<name>.mods.ini` beside it: tapes, medals and
high scores, the last skater and level. They are kept by name (`<mod folder>/<manifest name>`,
e.g. `TestPack/hangar`), not by number, so adding, removing or reordering mods doesn't mix them
up. The entries of mods that aren't loaded are kept as they are when the career is saved again.
A career loaded without the mods it last used starts on Tony Hawk and Warehouse.
