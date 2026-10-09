/* Game data the port reads from its own Data directory instead of the disc image. On the first
 * startup (and whenever something is missing) the files are extracted from the user's disc:
 *
 *   Data/Movies/   the full-screen movies (*.STR)
 *   Data/Music/    the soundtrack (*.STR: XA audio + the small video the front end shows) and
 *                  Playlist.m3u, the order the songs play in (editable)
 *
 * The extracted STR files keep every raw 2352-byte sector, so the XA audio survives and the files
 * play in tools that understand PlayStation STR (ffmpeg, jPSXdec). */
#ifndef PC_ASSETS_H
#define PC_ASSETS_H

#ifdef __cplusplus
extern "C" {
#endif

/* Extract whatever is missing from Data/. Needs the retail executable loaded (its movie table
 * lists the files) and, if anything is missing, the disc image. */
void PC_AssetsInit(void);

/* `rel` ("Music\\Playlist.m3u") under the Data directory (THPS_DATA_DIR, else Data next to
 * thps.exe). */
void PC_AssetsPath(char *out, int n, const char *rel);

/* The extracted copy of a disc file ("\\SUICIDE.STR;1"): 1 and its path, or 0. */
int PC_AssetsFind(const char *discName, char *path, int n);

#ifdef __cplusplus
}
#endif

#endif
