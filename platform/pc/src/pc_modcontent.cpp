/* Skaters and levels mods add, and their career progress (pc/mod_content.h, pc_mods.h). */
#include <ctype.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <initializer_list>
#include <string>

#include "pc/assets.h"
#include "pc/game_data.h"
#include "pc/ini.h"
#include "pc/mod_content.h"
#include "pc/mods.h"
#include "pc/platform.h"
#include "pc/saves.h"
#include "pc_mods.h"
#include "pc_runtime.h"

static_assert(offsetof(GameCareer, HighScore) == 0xa4, "SCareer layout");
static_assert(offsetof(GameCareer, LastLevel) == 0x82c, "SCareer layout");
static_assert(sizeof(GameCareer) == 0x9d0, "SCareer layout");
static_assert(sizeof(GameSkaterCareer) == 14 && sizeof(GameHighScore) == 12, "SCareer layout");

namespace pc {

ModContent g_modContent;

namespace {

constexpr int kAllModes =
    GAME_LEVEL_CAREER | GAME_LEVEL_SINGLESESSION | GAME_LEVEL_FREESKATE | GAME_LEVEL_MULTIPLAYER;

/* A copy of a manifest's text for the game's tables (they live as long as the game), cut to the
 * longest the game's buffers take. */
char *Text(const char *value, int max) {
    char buf[512];
    ini::Copy(buf, sizeof buf, value);
    if ((int)strlen(buf) > max)
        buf[max] = 0;
    return _strdup(buf);
}

/* comma-separated integers (0x.. for hex): how many were read */
int Ints(const char *value, long long *out, int max) {
    int n = 0;
    const char *s = value;
    while (n < max && *s) {
        char *end;
        while (*s == ' ' || *s == '\t')
            s++;
        long long v = strtoll(s, &end, 0);
        if (end == s)
            break;
        out[n++] = v;
        s = end;
        while (*s == ' ' || *s == '\t')
            s++;
        if (*s != ',')
            break;
        s++;
    }
    return n;
}

bool Digits(const char *s) {
    if (!*s)
        return false;
    for (; *s; s++)
        if (!isdigit((unsigned char)*s))
            return false;
    return true;
}

/* the manifests in a folder (*.ini), alphabetically */
int CmpName(const void *a, const void *b) {
    return _stricmp((const char *)a, (const char *)b);
}

const char *ModeNames(int modes, char *out, int n) {
    snprintf(out, n, "%s%s%s%s", modes & GAME_LEVEL_CAREER ? " career" : "",
             modes & GAME_LEVEL_SINGLESESSION ? " single-session" : "",
             modes & GAME_LEVEL_FREESKATE ? " free-skate" : "",
             modes & GAME_LEVEL_MULTIPLAYER ? " multiplayer" : "");
    return out;
}

} // namespace

/* by name ("Warehouse"), script ("SkWare_T") or number (0-8) */
int ModContent::StockLevel(const char *name) {
    if (Digits(name))
        return atoi(name) < GAME_LEVELS ? atoi(name) : -1;
    for (int i = 0; i < GAME_LEVELS; i++)
        if (!_stricmp(name, Levels[i].pLevelName) || !_stricmp(name, Levels[i].pTRGName))
            return i;
    return -1;
}

/* by full name ("Tony Hawk"), surname ("Hawk"), model ("hawk") or number (0-10) */
int ModContent::StockSkater(const char *name) {
    if (Digits(name))
        return atoi(name) < GAME_SKATERS ? atoi(name) : -1;
    for (int i = 0; i < GAME_SKATERS; i++) {
        const char *full = Stats[i].Name, *sur = strrchr(full, ' ');
        if (!_stricmp(name, full) || (sur && !_stricmp(name, sur + 1)) ||
            !_stricmp(name, Stats[i].PSXName))
            return i;
    }
    return -1;
}

bool ModContent::HasFile(const char *id, const char *what, const char *file) const {
    char path[MAX_PATH];
    if (g_assets.Find(file, path, sizeof path))
        return true;
    PC_Log("mods: %s: its %s %s is neither in the game's Data nor a mod's", id, what, file);
    return false;
}

bool ModContent::AddLevel(const Mod &mod, const char *path, const char *id) {
    const char *section, *key;
    char *value, file[96];
    long long v[3];
    IniReader ini;
    int base = 0, modes = kAllModes;
    /* the files the manifest names (the base level's may be in a PRE archive: not checked) */
    bool thumb = false, load = false, banner = false, vab = false;
    (void)mod;
    if (m_nlevels == GAME_MOD_LEVELS) {
        PC_Log("mods: %s: more than %d levels from mods, not added", id, GAME_MOD_LEVELS);
        return false;
    }
    /* the base level first: it gives the defaults */
    if (!ini.Open(path))
        return false;
    while (ini.Next(&section, &key, &value))
        if (!_stricmp(section, "Level") && !_stricmp(key, "Base") && (base = StockLevel(value)) < 0) {
            PC_Log("mods: %s: Base = %s is not one of the game's levels; using Warehouse", id, value);
            base = 0;
        }
    GameLevel l = Levels[base];
    l.pLevelName = l.pTRGName = nullptr;
    ini.Open(path);
    while (ini.Next(&section, &key, &value)) {
        if (_stricmp(section, "Level"))
            continue;
        if (!_stricmp(key, "Name"))
            l.pLevelName = Text(value, 31);
        else if (!_stricmp(key, "City"))
            l.pCityName = Text(value, 31);
        else if (!_stricmp(key, "Script"))
            l.pTRGName = Text(value, 27);
        else if (!_stricmp(key, "Modes")) {
            modes = (ini::HasWord(value, "Career") ? GAME_LEVEL_CAREER : 0) |
                    (ini::HasWord(value, "SingleSession") ? GAME_LEVEL_SINGLESESSION : 0) |
                    (ini::HasWord(value, "FreeSkate") ? GAME_LEVEL_FREESKATE : 0) |
                    (ini::HasWord(value, "Multiplayer") ? GAME_LEVEL_MULTIPLAYER : 0);
        } else if (!_stricmp(key, "Thumbnail")) {
            l.pImageName = Text(value, 63);
            thumb = true;
        } else if (!_stricmp(key, "LoadScreen")) {
            l.pLoadImage = Text(value, 63);
            load = true;
        } else if (!_stricmp(key, "LoadBanner")) {
            l.pBackImage = Text(value, 63);
            banner = true;
        } else if (!_stricmp(key, "SoundBank")) {
            l.pVab = Text(value, 19);
            vab = true;
        } else if (!_stricmp(key, "GoalName"))
            l.pGoalName = Text(value, 23);
        else if (!_stricmp(key, "MapPosition") && Ints(value, v, 2) == 2) {
            l.map_loc[0] = (int32_t)v[0];
            l.map_loc[1] = (int32_t)v[1];
        } else if (!_stricmp(key, "Competition"))
            l.isCompetition = ini::Bool(value);
        else if (!_stricmp(key, "MedalScores") && Ints(value, v, 3) == 3) {
            l.GoldScore = (uint32_t)v[0];
            l.SilverScore = (uint32_t)v[1];
            l.BronzeScore = (uint32_t)v[2];
        } else if (!_stricmp(key, "ScoreGoals") && Ints(value, v, 2) == 2) {
            l.HiScore = (int32_t)v[0];
            l.ProScore = (int32_t)v[1];
        } else if (!_stricmp(key, "TapesToUnlock"))
            l.TapesToUnlock = atoi(value);
        else if (!_stricmp(key, "MedalsToUnlock"))
            l.MedalsToUnlock = atoi(value);
        else if (!_stricmp(key, "Crowd"))
            l.Crowd = atoi(value);
        else if (!_stricmp(key, "PolyBuffer") && Ints(value, v, 2) == 2) {
            l.ExtraPolyBuffer[0] = (int32_t)v[0];
            l.ExtraPolyBuffer[1] = (int32_t)v[1];
        } else if (!_stricmp(key, "TrickTips") && Ints(value, v, 2) == 2) {
            l.TrickTipMask[0] = (int32_t)v[0];
            l.TrickTipMask[1] = (int32_t)v[1];
        } else if (!_stricmp(key, "TV"))
            l.FMV = ini::Bool(value);
        else if (!_stricmp(key, "IntroPosition") && Ints(value, v, 3) == 3) {
            l.IntroStartPosX = (int32_t)v[0];
            l.IntroStartPosY = (int32_t)v[1];
            l.IntroStartPosZ = (int32_t)v[2];
        } else if (!_stricmp(key, "IntroAngle") && Ints(value, v, 3) == 3) {
            l.IntroStartAngX = (int16_t)v[0];
            l.IntroStartAngY = (int16_t)v[1];
            l.IntroStartAngZ = (int16_t)v[2];
        } else if (_stricmp(key, "Base"))
            PC_Log("mods: %s: unknown entry \"%s\" in [Level]", id, key);
    }
    if (!l.pLevelName || !*l.pLevelName || !l.pTRGName || !*l.pTRGName) {
        PC_Log("mods: %s: a level needs a Name and a Script; not added", id);
        return false;
    }
    if (!modes) {
        PC_Log("mods: %s: Modes names none of Career, SingleSession, FreeSkate, Multiplayer; "
               "not added", id);
        return false;
    }
    snprintf(file, sizeof file, "%s.trg", l.pTRGName);
    bool ok = HasFile(id, "script", file);
    ok = ok && (!thumb || HasFile(id, "thumbnail", l.pImageName));
    ok = ok && (!load || HasFile(id, "loading screen", l.pLoadImage));
    ok = ok && (!banner || HasFile(id, "loading banner", l.pBackImage));
    snprintf(file, sizeof file, "%s.vab", l.pVab);
    /* (the Dreamcast's files have .kat sound banks, or none) */
    ok = ok && (!vab || g_assets.Dreamcast() || HasFile(id, "sound bank", file));
    if (!ok) {
        PC_Log("mods: %s: not added", id);
        return false;
    }
    int idx = GAME_LEVELS + m_nlevels;
    Levels[idx] = l;
    LevelModes[idx] = modes;
    snprintf(m_levelId[m_nlevels++], sizeof m_levelId[0], "%s", id);
    char names[96];
    PC_Log("mods: level \"%s\" (%s, %s.trg):%s%s", l.pLevelName, id, l.pTRGName,
           ModeNames(modes, names, sizeof names), l.isCompetition ? ", a competition" : "");
    return true;
}

bool ModContent::AddSkater(const Mod &mod, const char *path, const char *id) {
    static const char *const kBio[6] = {"Age", "Country", "HomeTown", "YearsPro", "Stance", "Height"};
    const char *section, *key;
    char *value, file[96];
    long long v[5];
    IniReader ini;
    int base = 0, boards = 0;
    bool named[8] = {};
    (void)mod;
    if (m_nskaters == GAME_MOD_SKATERS) {
        PC_Log("mods: %s: more than %d skaters from mods, not added", id, GAME_MOD_SKATERS);
        return false;
    }
    /* Base and BoardSet first: they give the defaults */
    if (!ini.Open(path))
        return false;
    while (ini.Next(&section, &key, &value)) {
        if (_stricmp(section, "Skater"))
            continue;
        if (!_stricmp(key, "Base") && (base = StockSkater(value)) < 0) {
            PC_Log("mods: %s: Base = %s is not one of the game's skaters; using Tony Hawk", id, value);
            base = 0;
        } else if (!_stricmp(key, "BoardSet") && (boards = StockSkater(value)) < 0) {
            PC_Log("mods: %s: BoardSet = %s is not one of the game's skaters; using Tony Hawk's "
                   "boards", id, value);
            boards = 0;
        }
    }
    GameStats s = Stats[base];
    memcpy(s.Boards, Stats[boards].Boards, sizeof s.Boards);
    s.PSXName = s.PSXName2 = s.FrontName = s.Name = nullptr;
    ini.Open(path);
    while (ini.Next(&section, &key, &value)) {
        int b;
        if (!_stricmp(section, "Skater")) {
            int k;
            for (k = 0; k < 6 && _stricmp(key, kBio[k]); k++)
                ;
            if (k < 6)
                s.pBio[k] = Text(value, 23);
            else if (!_stricmp(key, "Name"))
                s.Name = Text(value, 31);
            else if (!_stricmp(key, "Model"))
                s.PSXName = Text(value, 8);
            else if (!_stricmp(key, "Model2P"))
                s.PSXName2 = Text(value, 8);
            else if (!_stricmp(key, "FrontModel"))
                s.FrontName = Text(value, 8);
            else if (!_stricmp(key, "Stats")) {
                int n = Ints(value, v, 4);
                for (int i = 0; i < n; i++)
                    s.Stats[i] = (int32_t)(v[i] < 1 ? 1 : v[i] > 10 ? 10 : v[i]);
            } else if (_stricmp(key, "Base") && _stricmp(key, "BoardSet"))
                PC_Log("mods: %s: unknown entry \"%s\" in [Skater]", id, key);
        } else if (!_strnicmp(section, "Board", 5) && (b = atoi(section + 5)) >= 1 && b <= 8) {
            GameBoard &d = s.Boards[b - 1];
            if (!_stricmp(key, "Image")) {
                char img[16];
                bool have = true;
                ini::Copy(img, sizeof img, value);
                if (strlen(img) > 9) {
                    PC_Log("mods: %s: [%s] Image = %s is longer than 9 characters", id, section, img);
                    continue;
                }
                for (const char *pre : {"b1", "b2", "b_"}) {
                    snprintf(file, sizeof file, "%s%s.bmp", pre, img);
                    have = have && HasFile(id, "board picture", file);
                }
                if (!have)
                    continue;
                snprintf(file, sizeof file, "b1%s.bmp", img);
                d.File = _strdup(file);
                if (!named[b - 1])
                    d.Name = Text(img, 31);
            } else if (!_stricmp(key, "Name")) {
                d.Name = Text(value, 31);
                named[b - 1] = true;
            }
            else if (!_stricmp(key, "Tapes"))
                d.Req = (uint8_t)atoi(value);
            else if (!_stricmp(key, "Stats")) {
                int n = Ints(value, v, 5);
                for (int i = 0; i < n; i++)
                    d.Stats[i] = (int32_t)v[i];
            } else
                PC_Log("mods: %s: unknown entry \"%s\" in [%s]", id, key, section);
        }
    }
    if (!s.Name || !*s.Name || !s.PSXName || !*s.PSXName) {
        PC_Log("mods: %s: a skater needs a Name and a Model; not added", id);
        return false;
    }
    if (!s.PSXName2)
        s.PSXName2 = s.PSXName;
    if (!s.FrontName)
        s.FrontName = s.PSXName;
    /* the 1P select shows the first name alone, in 16 bytes */
    const char *sp = strchr(s.Name, ' ');
    if ((sp ? sp - s.Name : (ptrdiff_t)strlen(s.Name)) > 15) {
        PC_Log("mods: %s: the first word of Name = %s is longer than 15 characters", id, s.Name);
        return false;
    }
    /* the spooler knows a loaded model by its file's name: the selects hold every skater's
     * front-end model at once, a two-player game both players' models */
    for (int i = 0; i < GNumSkaters; i++) {
        const char *clash = !_stricmp(s.PSXName, Stats[i].PSXName)     ? s.PSXName
                            : !_stricmp(s.PSXName2, Stats[i].PSXName2) ? s.PSXName2
                            : !_stricmp(s.FrontName, Stats[i].FrontName) ? s.FrontName
                                                                         : nullptr;
        if (clash) {
            PC_Log("mods: %s: %s.psx is %s's model too (two skaters need two files: copy it "
                   "under another name); not added", id, clash, Stats[i].Name);
            return false;
        }
    }
    bool ok = true;
    for (const char *m : {s.PSXName, s.PSXName2, s.FrontName}) {
        snprintf(file, sizeof file, "%s.psx", m);
        ok = ok && HasFile(id, "model", file);
    }
    if (!ok) {
        PC_Log("mods: %s: not added", id);
        return false;
    }
    int idx = GAME_SKATERS + m_nskaters;
    Stats[idx] = s;
    GNumSkaters = idx + 1;
    snprintf(m_skaterId[m_nskaters++], sizeof m_skaterId[0], "%s", id);
    PC_Log("mods: skater \"%s\" (%s, %s.psx): plays as %s, %s's boards", s.Name, id, s.PSXName,
           Stats[base].Name, Stats[boards].Name);
    return true;
}

/* <mod>/Levels/*.ini or <mod>/Skaters/*.ini, alphabetically */
void ModContent::ReadDir(const Mod &mod, const char *sub, bool levels) {
    char dir[MAX_PATH], path[MAX_PATH], id[96];
    char (*names)[260] = nullptr;
    int n = 0, cap = 0;
    if (!Mods::FindEntry(mod.dir, sub, true, dir, sizeof dir))
        return;
    {
        os::DirEntry e;
        os::DirList list(dir);
        while (list && list.Next(&e)) {
            size_t len = strlen(e.name);
            if (e.isDir || len < 5 || _stricmp(e.name + len - 4, ".ini"))
                continue;
            if (n == cap) {
                cap = cap ? cap * 2 : 16;
                names = (char (*)[260])realloc(names, sizeof *names * cap);
            }
            snprintf(names[n++], sizeof *names, "%s", e.name);
        }
    }
    qsort(names, n, sizeof *names, CmpName);
    for (int i = 0; i < n; i++) {
        snprintf(path, sizeof path, "%s" PC_SEP "%s", dir, names[i]);
        snprintf(id, sizeof id, "%s/%.*s", mod.folder, (int)strlen(names[i]) - 4, names[i]);
        if (levels)
            AddLevel(mod, path, id);
        else
            AddSkater(mod, path, id);
    }
    free(names);
}

void ModContent::Load() {
    GameLevel end = Levels[GAME_LEVELS]; /* the empty end entry */
    for (int i = 0; i < g_mods.Count(); i++) {
        ReadDir(g_mods[i], "Levels", true);
        ReadDir(g_mods[i], "Skaters", false);
    }
    Levels[GAME_LEVELS + m_nlevels] = end;
    if (Any())
        PC_Log("mods: %d levels and %d skaters added", m_nlevels, m_nskaters);
}

/* ---- the career ------------------------------------------------------------------------------ */

void ModContent::SkaterKey(int skater, char *out, int n) const {
    if (skater >= GAME_SKATERS && skater < GAME_SKATERS + m_nskaters)
        snprintf(out, n, "%s", m_skaterId[skater - GAME_SKATERS]);
    else
        snprintf(out, n, "%d", skater < GAME_SKATERS ? skater : 0);
}

void ModContent::LevelKey(int level, char *out, int n) const {
    if (level >= GAME_LEVELS && level < GAME_LEVELS + m_nlevels)
        snprintf(out, n, "%s", m_levelId[level - GAME_LEVELS]);
    else
        snprintf(out, n, "%d", level < GAME_LEVELS ? level : 0);
}

int ModContent::SkaterByKey(const char *key) const {
    if (Digits(key))
        return atoi(key) < GAME_SKATERS ? atoi(key) : -1;
    for (int i = 0; i < m_nskaters; i++)
        if (!_stricmp(key, m_skaterId[i]))
            return GAME_SKATERS + i;
    return -1;
}

int ModContent::LevelByKey(const char *key) const {
    if (Digits(key))
        return atoi(key) < GAME_LEVELS ? atoi(key) : -1;
    for (int i = 0; i < m_nlevels; i++)
        if (!_stricmp(key, m_levelId[i]))
            return GAME_LEVELS + i;
    return -1;
}

namespace {

/* "Skater <key>" -> "<key>" (the section names of <name>.mods.ini) */
const char *After(const char *s, const char *word) {
    size_t n = strlen(word);
    return !_strnicmp(s, word, n) && s[n] == ' ' ? s + n + 1 : nullptr;
}

/* the next comma-separated field of s (trimmed, into out) */
const char *Field(const char *s, char *out, int n) {
    while (*s == ' ' || *s == '\t')
        s++;
    const char *e = strchr(s, ',');
    int len = e ? (int)(e - s) : (int)strlen(s);
    while (len > 0 && (s[len - 1] == ' ' || s[len - 1] == '\t'))
        len--;
    snprintf(out, n, "%.*s", len, s);
    return e ? e + 1 : s + strlen(s);
}

} // namespace

void ModContent::CareerLoad(const char *saveName) {
    const char *section, *key, *k;
    char *value, path[MAX_PATH], f[3][96];
    int lastSkater[2] = {-1, -1}, lastLevel = -1;
    /* the game's levels' high-score entries naming a mod's skater: [level][rank] -> skater */
    static int8_t scoreSkater[20][8];
    IniReader ini;
    memset(scoreSkater, -1, sizeof scoreSkater);
    if (g_saves.CareerModsPath(saveName, path, sizeof path) && ini.Open(path)) {
        PC_Trace("mods: career %s: %s", saveName, path);
        while (ini.Next(&section, &key, &value)) {
            int s, l;
            if (!_stricmp(section, "Career")) {
                if (!_stricmp(key, "LastSkater")) {
                    const char *p = value;
                    for (int i = 0; i < 2; i++) {
                        p = Field(p, f[0], sizeof f[0]);
                        lastSkater[i] = SkaterByKey(f[0]);
                    }
                } else if (!_stricmp(key, "LastLevel")) {
                    lastLevel = LevelByKey(value);
                } else if (!_stricmp(key, "HighScore")) {
                    const char *p = value;
                    for (int i = 0; i < 3; i++)
                        p = Field(p, f[i], sizeof f[i]);
                    l = atoi(f[0]);
                    int r = atoi(f[1]);
                    if (l >= 0 && l < 20 && r >= 0 && r < 8)
                        scoreSkater[l][r] = (int8_t)SkaterByKey(f[2]);
                }
            } else if ((k = After(section, "Skater")) && (s = SkaterByKey(k)) >= GAME_SKATERS) {
                GameSkaterCareer &c = GCareerExt.CharacterProgress[s - GAME_SKATERS];
                long long v[GAME_LEVELS];
                int n;
                if (!_stricmp(key, "Career") && Ints(value, v, 4) == 4) {
                    c.NumTapes = (uint8_t)v[0];
                    c.NumBronze = (uint8_t)v[1];
                    c.NumSilver = (uint8_t)v[2];
                    c.NumGold = (uint8_t)v[3];
                } else if (!_stricmp(key, "Tapes")) {
                    n = Ints(value, v, GAME_LEVELS);
                    for (int i = 0; i < n; i++)
                        c.Tapes[i] = (uint8_t)v[i];
                } else if (!_stricmp(key, "MedalMessages")) {
                    GCareerExt.MedalMessagesUsed[s - GAME_SKATERS] = (int32_t)strtol(value, nullptr, 0);
                }
            } else if ((k = After(section, "Level")) && (l = LevelByKey(k)) >= GAME_LEVELS) {
                const char *sk;
                if ((sk = After(key, "Skater")) && (s = SkaterByKey(sk)) >= 0) {
                    GCareerExt.Tapes[s][l - GAME_LEVELS] = (uint8_t)strtol(value, nullptr, 0);
                } else if ((sk = After(key, "HighScore")) && atoi(sk) >= 1 && atoi(sk) <= 8) {
                    GameHighScore &h = GCareerExt.HighScore[l - GAME_LEVELS][atoi(sk) - 1];
                    const char *p = value;
                    for (int i = 0; i < 3; i++)
                        p = Field(p, f[i], sizeof f[i]);
                    memset(h.Name, 0, sizeof h.Name);
                    memcpy(h.Name, f[0], strlen(f[0]) < 3 ? strlen(f[0]) : 3);
                    s = SkaterByKey(f[1]);
                    h.Skater = (uint8_t)(s >= 0 ? s : 0);
                    h.Score = (uint32_t)strtoul(f[2], nullptr, 10);
                }
            }
        }
    }
    /* what the save names by index: a mod's skater or level at that index now, or the game's */
    for (int i = 0; i < 2; i++)
        if ((uint8_t)GCareer.LastSkater[i] >= GAME_SKATERS)
            GCareer.LastSkater[i] = (char)(lastSkater[i] >= 0 ? lastSkater[i] : 0);
    if (GCareer.LastLevel >= GAME_LEVELS)
        GCareer.LastLevel = (uint8_t)(lastLevel >= 0 ? lastLevel : 0);
    for (int l = 0; l < 20; l++)
        for (int r = 0; r < 8; r++)
            if (GCareer.HighScore[l][r].Skater >= GAME_SKATERS)
                GCareer.HighScore[l][r].Skater =
                    (uint8_t)(scoreSkater[l][r] >= 0 ? scoreSkater[l][r] : 0);
}

void ModContent::CareerSave(const char *saveName) {
    char path[MAX_PATH], tmp[MAX_PATH], line[1024], key[96];
    std::string kept;
    FILE *f;
    if (!g_saves.CareerModsPath(saveName, path, sizeof path))
        return;
    /* the sections of mods that aren't loaded stay as they are */
    if ((f = fopen(path, "r"))) {
        bool keep = false;
        while (fgets(line, sizeof line, f)) {
            if (line[0] == '[') {
                char name[256];
                const char *k;
                snprintf(name, sizeof name, "%s", line + 1);
                if (char *e = strchr(name, ']'))
                    *e = 0;
                keep = ((k = After(name, "Skater")) && !Digits(k) && SkaterByKey(k) < 0) ||
                       ((k = After(name, "Level")) && !Digits(k) && LevelByKey(k) < 0);
                if (keep && !kept.empty())
                    kept += "\n";
            }
            if (keep && line[strspn(line, " \t\r\n")])
                kept += line;
        }
        fclose(f);
    }
    if (!Any() && kept.empty()) {
        if (os::Exists(path))
            os::RemoveFile(path);
        return;
    }
    snprintf(tmp, sizeof tmp, "%s.tmp", path);
    if (!(f = fopen(tmp, "w"))) {
        PC_Log("mods: cannot write %s", tmp);
        return;
    }
    fprintf(f, "; Tony Hawk's Pro Skater: the career %s's progress with mods' skaters and levels,\n"
               "; which the .career file has no room for. Written with the career.\n\n[Career]\n",
            saveName);
    char k2[96];
    SkaterKey((uint8_t)GCareer.LastSkater[0], key, sizeof key);
    SkaterKey((uint8_t)GCareer.LastSkater[1], k2, sizeof k2);
    fprintf(f, "LastSkater = %s, %s\n", key, k2);
    LevelKey(GCareer.LastLevel, key, sizeof key);
    fprintf(f, "LastLevel = %s\n", key);
    for (int l = 0; l < 20; l++)
        for (int r = 0; r < 8; r++)
            if (GCareer.HighScore[l][r].Skater >= GAME_SKATERS) {
                SkaterKey(GCareer.HighScore[l][r].Skater, key, sizeof key);
                fprintf(f, "HighScore = %d, %d, %s\n", l, r, key);
            }
    for (int i = 0; i < m_nskaters; i++) {
        const GameSkaterCareer &c = GCareerExt.CharacterProgress[i];
        fprintf(f, "\n[Skater %s]\nCareer = %d, %d, %d, %d\nTapes =", m_skaterId[i], c.NumTapes,
                c.NumBronze, c.NumSilver, c.NumGold);
        for (int l = 0; l < GAME_LEVELS; l++)
            fprintf(f, "%s %d", l ? "," : "", c.Tapes[l]);
        fprintf(f, "\nMedalMessages = 0x%x\n", (unsigned)GCareerExt.MedalMessagesUsed[i]);
    }
    for (int i = 0; i < m_nlevels; i++) {
        fprintf(f, "\n[Level %s]\n", m_levelId[i]);
        for (int s = 0; s < GNumSkaters; s++)
            if (GCareerExt.Tapes[s][i]) {
                SkaterKey(s, key, sizeof key);
                fprintf(f, "Skater %s = %d\n", key, GCareerExt.Tapes[s][i]);
            }
        for (int r = 0; r < 8; r++) {
            const GameHighScore &h = GCareerExt.HighScore[i][r];
            SkaterKey(h.Skater, key, sizeof key);
            fprintf(f, "HighScore %d = %.3s, %s, %u\n", r + 1, h.Name, key, (unsigned)h.Score);
        }
    }
    if (!kept.empty())
        fprintf(f, "\n; mods not loaded when this was written\n%s", kept.c_str());
    if (fclose(f) != 0 || !os::RenameOver(tmp, path)) {
        PC_Log("mods: cannot write %s", path);
        os::RemoveFile(tmp);
    }
}

} // namespace pc

/* ---- the game's interface (pc_mods.h) ---- */

extern "C" {

void PC_ModCareerLoad(const char *name) { pc::g_modContent.CareerLoad(name); }
void PC_ModCareerSave(const char *name) { pc::g_modContent.CareerSave(name); }

}
