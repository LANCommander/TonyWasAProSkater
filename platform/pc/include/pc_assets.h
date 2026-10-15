/* Game data the port reads from its own Data directory instead of the disc image. On the first
 * startup (and whenever something is missing) the files are extracted from the user's disc:
 *
 *   Data/Movies/   the full-screen movies (*.STR)
 *   Data/Music/    the soundtrack (*.STR: XA audio + the small video the front end shows) and
 *                  Playlist.m3u, the order the songs play in (editable)
 *   Data/Levels/   level geometry (sk<level>*.psx)    Data/Scripts/   level scripts (*.trg)
 *   Data/Models/   skaters, cars, items (*.psx)       Data/Textures/  images (*.bmp)
 *   Data/Sounds/   sound banks (*.vab)                Data/Fonts/     fonts (*.fnt)
 *   Data/Packs/    PRE archives of small files        Data/Demos/     attract-mode replays (*.rec)
 *   Data/Text/, Data/Misc/                            Data/Manifest.txt  what came from CD.WAD
 *
 * The extracted STR files keep every raw 2352-byte sector, so the XA audio survives and the files
 * play in tools that understand PlayStation STR (ffmpeg, jPSXdec). The CD.WAD files are the
 * plain file contents.
 *
 * Or the files come from the Dreamcast version's disc (a .gdi), into the same folders: models
 * and levels as version 6 .psx (pc_dctex.h), sound banks as .kat (pc_dcsound.h), films as .sfd
 * (MPEG-1 + ADX: pc_sfd.h; Playlist.m3u then lists them), images unchanged (some at higher
 * resolutions). */
#ifndef PC_ASSETS_H
#define PC_ASSETS_H

#ifdef __cplusplus
extern "C" {
#endif

/* An image the Dreamcast redrew larger (name, its size w x h): 1 and the size the PS1 game lays
 * it out at, in VRAM pixels (the front-end screens, line-doubled on the PS1: 640x480). Loaders
 * show it at that size with all its pixels: the screens load without line-doubling, others are
 * drawn scaled (PC_ScreenBlitHiRes, Sprite). 0 for any other image or the PS1's data. */
int PC_ImageLayoutSize(const char *name, int w, int h, int *lw, int *lh);
/* The same for a loaded .bmp (FileIO marks it in memory; pc_fileio.cpp). */
int PC_BmpLayoutSize(const void *bmp, int *lw, int *lh);

#ifdef __cplusplus
}
#endif

#endif
