# Dreamcast assets in the PC port

Investigated 2026-10-11 from the user's GD-ROM dump of *Tony Hawk's Pro Skater v1.000 (2000)(Crave)
(NTSC)(US)* (Treyarch's port). Tools written for it:

| tool | does |
|---|---|
| `tools/gdi_extract.py <gdi> [outdir]` | lists/extracts the ISO9660 filesystem of a `.gdi` dump (high-density area at LBA 45000, extents spanning tracks 3 and 5) |
| `tools/psxfile.py <file> [-v]` | parses a Neversoft `.PSX` (v4 = PS1, v6 = DC) the way `ProcessNewPSX`/`Spool_LoadTexturesToVRAM` read it |
| `tools/pvr.py` | decodes PowerVR payloads (ARGB1555/RGB565/ARGB4444; twiddled, VQ, rectangle; mip chains) |

The disc holds 834 loose files (~990 MB; no `CD.WAD`). Extracted copy: `extern/dc/` (git-ignored).

## What is on the disc

| PS1 | DC | difference |
|---|---|---|
| `CD.WAD` → `sk*.psx` levels, skater/car models (`.psx` **v4**) | 90 loose `.PSX` (**v6**) | same container and packets; hi-res 16-bit textures, 16-bit UVs, remodelled skaters (below) |
| `*.vab` SFX banks (SPU ADPCM) | 10 `*.KAT` (same names: `WARE`, `SKATE2`, `SHELL`, ...) | 8/16-bit PCM 22050 Hz, ~2x the sample data; entries keyed by VAB program/tone |
| 26 `.STR` (MDEC video + XA; songs mono 37.8 kHz 4-bit) | 30 `.SFD` (Sofdec: MPEG-1 video + ADX audio) | music stereo 44.1 kHz ADX; movies 320x240; extra DC logos (CRAVE, TREYARCH, SOFDEC) |
| `*.bmp` front-end images | 491 `.BMP` + 154 `.PVR` | 376 BMPs byte-identical; 30 differ, 85 DC-only (VMU, button art); `B_*.PVR` = RGB565 versions of the 8-bit bail photos |
| `*.trg`, `*.pre`, `*.fnt`, `*.rec` | same names | `.TRG` scripts identical (checked Warehouse) |

Also on the disc: `SKPARK_T.TRG`, `SKATE_T.TRG`, `TWEAK.DAT`, `GAMMAS.TXT`, `SIZES.DAT`.
`1ST_READ.BIN` is an unscrambled SH-4 Katana/Shinobi exe (Kamui `dcgfx.cpp`, `dcmovieplayer.cpp`,
CRI ADX/Sofdec libs); no game symbols.

## `.PSX` version 6 (the core of it)

Same layout as v4 (header, 36-byte objects, model table, packets, model checksums, texture
checksum table, palettes, texture records), with these differences:

- **Faces**: the header words (`SFace` 0x00-0x13: flags, vertex numbers, colour, normal,
  `Flags2` collision flags, texture index) are unchanged, and the size is still `flags >> 18`
  words. A textured face's coordinates follow as **u0..u3 then v0..v3, u16 each (words 5-8),
  normalised: 512 = one texture repeat** (levels tile up to ~30x; PS1 used texture windows
  instead), so textured faces grow by 8 bytes and the texture-window slot moves from word 7 to 9.
- **Textures**: no palettes (`n16 = n256 = 0`); every record has `num_colours = 65536` and an
  embedded PowerVR payload: `+16 u16 w,h; +20 u8 pixel_fmt, u8 data_fmt, u16 0; +24 u32 size
  (payload + 8); +28 data`. Seen: RGB565 VQ+mips (levels), twiddled+mips, rectangle (`0x09`),
  rectangle-twiddled (`0x0d`, one file). Up to 256x256. Twiddled mip chains put the full level
  at the end; VQ chains pad at the end. Decoded output checked visually (Hawk's face/shirt,
  Warehouse bricks, Transworld banner).
- **Texture checksums are all new** (0 shared with PS1 in Warehouse and Hawk): textures were
  re-authored and re-atlased, so they cannot be swapped onto PS1 geometry by checksum.
- **Levels**: Warehouse 251 models (PS1 288; 243 model checksums shared); of the shared models
  233 have identical vertices, so geometry/collision is essentially the PS1 level with new UVs
  and textures. School: 964 of 983 shared models identical.
- **Skaters**: same 19 parts, hierarchy packet byte-identical and the `0x2c` anim packet nearly
  the same size, but each part is remodelled (Hawk's torso: 20 verts/21 faces → 59/95).

So DC models only make sense **as whole files with their own textures**; mixing per file (DC
skaters in a PS1 level or vice versa) works because each `.PSX` is self-contained.

Not every `.PSX` on the disc is version 6: the `_L`/`L2` texture libraries, `SKATELIB`, `BITS`,
`TAXI`, `C_POLICE`, `SKFWAY*`, `SKATREST`, `SKATWARE`, `PROCFIRE`, `RASTA` and a few `_O` files are
PS1-style version 4 (8/4-bit textures in VRAM). The DC texture-wibble packets (type 6) keep the
amplitude/phase bytes but zero the base u/v: the offsets move the face's own coordinates.

## In the port

### 1. Models and textures: done (2026-10-12)

The player extracts from either the PS1 or the Dreamcast disc on first startup; the DC files go
into the same `Data/` folders (`platform/pc/README.md`, "The Dreamcast version's data"). How the
version 6 files are handled (`platform/pc/include/pc_dctex.h`):

- **Loading** (`pc_fileio.cpp`, `pc_dctex.cpp`): FileIO strips the PowerVR images from a version 6
  file and registers them by checksum; the game gets the rest (texture records keep their 28-byte
  headers). `Spool_LoadTexturesToVRAM` gives a 65536-colour texture an entry with colour depth 3
  in its tpage (never produced by the PS1 game) and the registry handle as its CLUT; no VRAM or
  palette. `Spool_RemoveUnusedTextures` releases the handle (the OpenGL texture goes).
- **Faces**: `M3dInit_ParsePSX` leaves a version 6 face's word 4 as its `Texture` pointer (bit 31
  set; PS1 faces have `u0v0 | clut << 16` there) and its 16-bit coordinates alone. The native
  renderer (`pc_render3d.cpp`, `dc_face`) reads them, relative to corner 0 so wrapped values don't
  tear a face; `pc_gpu_gl.cpp` binds the handle's texture (decoded on first use, mipmapped,
  `GL_REPEAT`) on unit 1, and the shader samples it with its alpha on top of the PS1's
  semi-transparency. Draws split per texture.
- **Other readers**: shatter uses the `Texture` pointer (shards are GPU packets with the depth-3
  page, which the emulated path also maps to the HD texture, 256 units = one repeat); the texture
  wibble has a version 6 branch (`8008528c.cpp`: base coordinates kept per region, PS1 texel
  offsets at 8 units each); the front end's 30000-byte model backup copies a version 6 model's
  whole block (`8006ed2c.c`/`8006edb8.c`).
- **Fade-to-colour** (front end, Downhill Jam, Streets) is now native for every face, as retail
  draws it (the face additive over a flat polygon in `DPCS(fade, 1 - fog)`); only the subtractive
  variant still goes to retail, which can't draw version 6 faces. Layer draws now take the GPU's
  drawing area at their point of the ordering table (the skater-select box clips its 3D).
- **Memory**: stripped DC files are still bigger (Downtown +300 KB, skaters +60 KB, full models
  in the front end), so the RAM map is 8 MB and with DC data the heap/stack top moves to 8 MB
  (`Init_ResetMemory` takes `pc_ram_top`).
- **Gaps of a DC-only `Data/`**: no `*_fe.psx` (each skater's `FrontName` points at its full
  model, as the DC's own table does), no `.vab` or `.STR` (the `.kat` and `.sfd` files stand in:
  sections 2 and 3; the disc image is never opened). `b_*.bmp`, `loading.bmp` and `title_h.zlb` are unused
  by the PS1 executable. Every file is extracted unchanged; the 12 images the DC
  redrew larger keep their resolution and take the PS1 layout's size on screen (front-end
  screens loaded unscaled at 640x480; the HORSE loading screen and pause logo drawn scaled). The
  front-end art shows Dreamcast buttons (A/B).

Verified: extraction (828 files), every level (Warehouse, School, Mall, Skate Park, Downtown,
Downhill Jam, Burnside, Streets, Roswell) and the front end run on DC data; the PS1 data's
`THPS_RENDER3D=psx` frames are pixel-identical to the previous build, native frames identical
except the skater-select screen (now clipped like retail). Not tested: two-player split screen,
the water surfaces and shatterable glass on DC data.

### 2. Music and movies: done (2026-10-12)

- **Container** (`src/pc_sfd.cpp`, `include/pc/sfd.h`): `.SFD` = MPEG-1 program stream; video
  `0xE0` (MPEG-1), audio `0xC0` carrying **ADX** (standard type 3, 18-byte frames, 4-bit; songs
  44.1 kHz stereo, movies 44.1 kHz stereo or 22.05 kHz mono; `ATVILOGO.SFD` is a bare
  640x288 video stream, unused). Songs are 125 s, like the PS1's. The decoded Goldfinger track
  lines up with the PS1 XA decode (50 ms envelope correlation 1.0 at zero offset, RMS within
  1.5%), so no gain change. `platform/pc/tests/test_sfd.c` decodes a film to WAV and checks
  seek/rewind.
- **Music**: a movie-table name finds the film of an STR when there's no STR
  (`PC_StreamFind` → `PC_AssetsFindFilm`: same name, except ATVILOGO → `activision`, LOGO →
  `never`, BURNQUIS → `burnquist`), and it streams like an STR (`src/pc_stream.cpp`): ~12k frames
  of decoded ADX stay queued in the music input instead of sectors being paced; pause/resume
  carry on where it was. The game's mix/MusicScale/pause/skip paths are unchanged. There is no STR video for the game's frame
  counter, so at the end of a song's audio the stream stops and `PC_MusicStreamEnded` sets
  `GameFMV_RewindSwitch` (what `strNext` does at the end frame): the pump's `strKickCD`
  restarts it, gaplessly (verified with a truncated song). With DC data `Playlist.m3u` lists the
  `.sfd` songs; playlist entries can mix `.STR` and `.sfd`.
- **Movies** (`src/pc_movie.cpp`, PC branch in `decomp/src/gamefmv/playmovie.cpp`): the override
  runs `GameFMV_PlayMovie` when the entry has a film (`PC_MovieAvailable`); the retail setup
  (buffers, CdMix from MIDILEVEL, `strInit` → the film's audio streams as above) runs, then the
  loop is replaced: pl_mpeg (`include/pl_mpeg/pl_mpeg.h`, MIT) decodes the video from our
  demuxer and each picture is loaded as 24-bit pixels at VRAM (0, 256) when the film's audio
  (`PC_CdFilmTime`: the mixer's consumed-frame count since the stream started) reaches its time,
  on a 320-wide 24-bit display; Start/Accept/Back skip; holding Start skips the boot movies as
  on the PS1. `THPS_MOVIES=0` skips all. Verified: Activision and Neversoft logos at boot.
- Not done: the songs' 96x64 thumbnail video in the front end (the PS1 data doesn't show it in
  the port either: MDEC); the DC-only logo films (CRAVE, TREYARCH, SOFDEC) are not played.

### 3. Sound effects: done (2026-10-12)

`.KAT`: `u32 count`, then 44-byte entries `{type=1, offset, size, rate, loop, bits(8/16),
id, loop_start, loop_end (bytes; 0 or the size), 0, 0}`; 8-bit samples are signed;
`id = program*1000 + tone` of the matching VAB (a few ids such as 100006 or 8002 have no VAB
slot; a few PS1 tones have no entry and stay silent). Compared with the PS1 VABs:

- **Pitch**: the DC sample count over the PS1's, times the PS1 tone's playback rate at note 60
  (from its centre note/fine tune), over the KAT rate, is 0.94-0.95 for nearly every sound across
  banks, rates (8000/11025/22050) and centre notes: the PS1 plays them one semitone below the
  KAT rate. The port plays them at `rate * 2^(-1/12)` (`PCM_TUNE` in `pc_sound.cpp`), so they sound as on
  the PS1 (outliers are the sounds the DC re-cut).
- **Loudness**: median RMS ratio DC/PS1 0.96: mixed at unity.
- **Tone attributes**: `playSFX` only reads centre/fine, ADSR and the sample index from the VAB.

`src/pc_dcsound.cpp` builds a VAB from the `.kat` when the game asks for the `.vab`: centre note
60, the PS1 banks' usual ADSR (looped `a9ff/5fcc`, one-shots `80ff/1fc0`), and per sound a
32-byte stand-in sample (`DCPC` + id in a near-silent ADPCM block, then an end block). The
samples go to the sound's PCM registry (`PC_SoundPcmAdd`, once per bank); a channel keyed on at a
stand-in plays the PCM with the channel's pitch, volume and envelope, and zeroes its envelope at
the end like an ADPCM one-shot (the voice-freeing rule). Verified: every key-on in a
front-end-to-gameplay run hits a PCM sample.

### 4. Front end (low value)

Mostly identical; optional: the 16-bit `B_*.PVR` bail photos and `W_*.PVR`/`WT*.PVR` wheels.

## Next

1. Optional: the DC's 640x480 front-end screens at full resolution, the 16-bit `B_*.PVR` photos.
2. Optional: the songs' 96x64 thumbnail video (decode the SFD video into the VRAM spot the front
   end draws from), the DC logo films at boot.
3. Untested on DC data: two-player split screen, water surfaces, shatterable glass.
