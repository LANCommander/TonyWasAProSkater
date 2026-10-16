/* Skaters and levels mods add (pc_modcontent.cpp; the game's interface is pc_mods.h).
 *
 * <mod>/Levels/<name>.ini adds a level and <mod>/Skaters/<name>.ini a skater; their files (models,
 * scripts, images) are in the mod's Data folder like any other (pc/mods.h). They go after the
 * game's own in its tables (Levels[], Stats[]: game/career.h), up to 22 levels and 21 skaters in
 * all. Each is known by `<mod folder>/<name>` in saves.
 *
 * Levels/<name>.ini, [Level] (only Name and Script are needed; the rest comes from Base):
 *   Name, City          the level select's two lines (letters, digits and spaces: the menu font
 *                       has nothing else; up to 31 characters)
 *   Base                a game level whose settings are the defaults (name, e.g. Warehouse;
 *                       default Warehouse)
 *   Script              the trigger script, <Script>.trg (it spools the geometry: SkWare_T)
 *   Modes               Career, SingleSession, FreeSkate, Multiplayer (comma-separated; default
 *                       all)
 *   Thumbnail           the level select picture (8-bit BMP, 448x161: levsel0.bmp)
 *   LoadScreen, LoadBanner   the competition loading screen's pictures (512x240, 432x40)
 *   SoundBank           <SoundBank>.vab (ware)
 *   MapPosition         x, y of the level select map's arrow (640x480 screen)
 *   Competition         1: a competition (three heats, medals) instead of tapes
 *   MedalScores         gold, silver, bronze (competitions)
 *   ScoreGoals          the high score and pro score tapes
 *   GoalName            what the collect-five goal collects ("boxes")
 *   TapesToUnlock, MedalsToUnlock   in career mode (tapes on mods' levels count too)
 *   Crowd, PolyBuffer (one player, two), TrickTips (two masks), TV (1: the in-level video),
 *   IntroPosition, IntroAngle (x, y, z: a competition's opening camera)
 *
 * Skaters/<name>.ini, [Skater] (only Name and Model are needed; the rest comes from Base):
 *   Name                full name (the first word, up to 15 characters, is used alone too)
 *   Base                a game skater (Hawk, Burnquist, ... by surname or model) whose tricks,
 *                       model part numbers and board/wheel/truck texture checksums are used
 *                       (default Hawk)
 *   Model, Model2P, FrontModel   <name>.psx: in game, in two-player split screen, in the front
 *                       end (up to 8 characters; Model2P and FrontModel default to Model)
 *   Stats               the four stat bars (ollie, speed, air, balance: 1-10)
 *   Age, Country, HomeTown, YearsPro, Stance (Regular or Goofy), Height   the bio
 *   BoardSet            a game skater whose eight boards are used (default Hawk's)
 *   [Board1] .. [Board8]  replace that board: Image (b1<Image>.bmp, b2<Image>.bmp,
 *                       b_<Image>.bmp: the 1P and 2P select pictures and the deck texture; up to
 *                       9 characters), Name, Tapes (to unlock), Stats (the four bonus bars)
 */
#pragma once

#include "pc/game_data.h"
#include "pc/platform.h"

namespace pc {

struct Mod;

class ModContent {
public:
    /* Read every loaded mod's levels and skaters into the game's tables (after the Data index is
     * built: their files are checked). */
    void Load();
    /* Whether mods added any: the game gets more memory (the selects hold every skater's model
     * and every level's picture). */
    bool Any() const { return m_nlevels || m_nskaters; }

    void CareerLoad(const char *saveName);
    void CareerSave(const char *saveName);

private:
    bool AddLevel(const Mod &mod, const char *path, const char *id);
    bool AddSkater(const Mod &mod, const char *path, const char *id);
    void ReadDir(const Mod &mod, const char *sub, bool levels);
    static int StockLevel(const char *name);
    static int StockSkater(const char *name);
    bool HasFile(const char *id, const char *what, const char *file) const;

    /* saves name skaters and levels by key: a game one by its index, a mod's by its id */
    void SkaterKey(int skater, char *out, int n) const;
    void LevelKey(int level, char *out, int n) const;
    int SkaterByKey(const char *key) const; /* -1 if none */
    int LevelByKey(const char *key) const;

    char m_levelId[GAME_MOD_LEVELS][96] = {};
    char m_skaterId[GAME_MOD_SKATERS][96] = {};
    int m_nlevels = 0, m_nskaters = 0;
};

extern ModContent g_modContent;

} // namespace pc
