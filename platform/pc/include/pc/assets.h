/* The port's Data directory (pc_assets.cpp; the folders and the C interface are described in
 * pc_assets.h): first-startup extraction of the game's files from the PS1 or Dreamcast disc, and
 * finding them by name at runtime. */
#pragma once

#include "pc/platform.h"

namespace pc {

class Assets {
public:
    /* Extract whatever is missing from Data/. Needs the retail executable loaded (its movie table
     * lists the files) and, if anything is missing, the disc image. */
    void Init();

    /* true if Data/ holds the Dreamcast version's files (extracted from its disc). */
    bool Dreamcast() const { return m_dc; }

    /* An image the Dreamcast redrew larger (name, its size w x h): true and the size the PS1 game
     * lays it out at, in VRAM pixels (pc_assets.h). false for any other image or the PS1's data. */
    bool ImageLayoutSize(const char *name, int w, int h, int *lw, int *lh) const;

    /* `rel` ("Music\\Playlist.m3u") under the Data directory (THPS_DATA_DIR, else Data next to
     * thps.exe). */
    void Path(char *out, int n, const char *rel);

    /* The file under Data/ with this name, in any folder ("\\SUICIDE.STR;1", "skware.psx"): true
     * and its path, or false. A mod's file of that name wins (pc/mods.h). */
    bool Find(const char *discName, char *path, int n) const;
    /* The same for a movie or song ("\\HAWK.STR;1"): the STR, else the Dreamcast's film of it
     * ("hawk.sfd"; ATVILOGO is activision.sfd, LOGO never.sfd, BURNQUIS burnquist.sfd). */
    bool FindFilm(const char *discName, char *path, int n) const;

    /* An STR file the movie table lists (pc_assets.cpp). */
    struct StrFile;

private:
    struct IndexEntry {
        char *name, *path;
        int layer;   /* 0: Data itself, else 1 + the mod (pc/mods.h) whose Data it is in */
    };

    const char *Root();
    void MakeDirs(const char *rel);
    void StrPath(const StrFile *e, char *path, int n);
    void ExtractStrFiles(const StrFile *e, int n);
    void WritePlaylist(const StrFile *e, int n);
    bool WadComplete();
    void ExtractWadFiles();
    bool ManifestIsDreamcast();
    void ExtractDreamcast(const StrFile *str, int nstr);
    void DreamcastFixups();
    static int CmpName(const void *a, const void *b);
    static int CmpIndex(const void *a, const void *b);
    void IndexDir(const char *dir, int layer);
    void BuildIndex();

    char m_root[MAX_PATH] = {};
    bool m_dc = false;   /* Data/ holds the Dreamcast version's files */
    /* file name -> path, sorted by name (malloc'd) */
    IndexEntry *m_index = nullptr;
    int m_nindex = 0, m_capindex = 0;
};

extern Assets g_assets;

} // namespace pc
