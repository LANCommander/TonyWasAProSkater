/* Mods (pc_mods.cpp): folders of files that overlay or add to the game's data.
 *
 * A mod is a folder with a mod.ini manifest, found in (loaded in this order):
 *   1. Data/Mods/<mod>/                      every subfolder, alphabetically
 *   2. Mods/<mod>/ beside Settings.ini       (the player's folder, Runtime::UserDir)
 *   3. THPS_MODS                             a list (';'-separated on Windows, ':' elsewhere)
 *                                            of mod folders, or folders of mods, in order
 * THPS_MODS=none loads no mods at all. A folder found twice is loaded once.
 *
 * mod.ini:
 *   [Mod]
 *   Name = Disco Mod
 *   Author = ...          Version = 1.0          Date = 2026-10-15
 *   Description = ...     ("\n" is a line break)  Website = ...
 *   Enabled = 0           (optional: leave the mod out)
 *
 * <mod>/Data/ overlays the game's Data folder: the game finds its files by name, whatever the
 * folder (pc_assets.h), so <mod>/Data/Textures/accept.bmp replaces accept.bmp, and a file the
 * game doesn't have is added. A later mod's file wins over an earlier one's.
 *
 * <mod>/Levels/*.ini and <mod>/Skaters/*.ini add levels and skaters (pc/mod_content.h). */
#pragma once

#include "pc/platform.h"

namespace pc {

struct Mod {
    char dir[MAX_PATH];       /* the mod's folder */
    char folder[64];          /* its folder's name */
    char name[96], author[96], version[32], date[32], website[160];
    char description[512];
    char dataDir[MAX_PATH];   /* <mod>/Data, or "" */
    int files, replaced;      /* files in dataDir, and how many of them replace another file */
};

class Mods {
public:
    /* Find the mods and read their manifests, logging each (once; Assets::Init calls it before
     * indexing Data). */
    void Init();

    int Count() const { return m_n; }
    Mod &operator[](int i) { return m_mods[i]; }
    const Mod &operator[](int i) const { return m_mods[i]; }

    /* `dir`'s entry named `name` in any case (Linux paths are case-sensitive, mod authors'
     * spelling isn't): true and its path. */
    static bool FindEntry(const char *dir, const char *name, bool wantDir, char *out, int n);

private:
    void AddFolder(const char *dir);
    void AddMod(const char *dir);

    static constexpr int kMaxMods = 64;
    Mod m_mods[kMaxMods] = {};
    int m_n = 0;
    bool m_done = false;
};

extern Mods g_mods;

} // namespace pc
