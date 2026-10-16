/* Mods (pc/mods.h): finding them, reading mod.ini, and logging what loads. Their Data folders
 * are indexed with the game's own (Assets::BuildIndex); their levels and skaters are read by
 * pc_modcontent.cpp. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pc/assets.h"
#include "pc/ini.h"
#include "pc/mods.h"
#include "pc/platform.h"
#include "pc/runtime.h"
#include "pc_runtime.h"

namespace pc {

Mods g_mods;

namespace {

#ifdef _WIN32
constexpr char kListSep = ';';
#else
constexpr char kListSep = ':';
#endif

int CmpName(const void *a, const void *b) {
    return _stricmp((const char *)a, (const char *)b);
}

} // namespace

bool Mods::FindEntry(const char *dir, const char *name, bool wantDir, char *out, int n) {
    os::DirEntry e;
    os::DirList list(dir);
    if (!list)
        return false;
    while (list.Next(&e))
        if (!!e.isDir == wantDir && !_stricmp(e.name, name)) {
            snprintf(out, n, "%s" PC_SEP "%s", dir, e.name);
            return true;
        }
    return false;
}

/* A folder of mods: each subfolder, alphabetically. */
void Mods::AddFolder(const char *dir) {
    char (*names)[260] = nullptr, sub[MAX_PATH];
    int n = 0, cap = 0;
    os::DirEntry e;
    {
        os::DirList list(dir);
        if (!list)
            return;
        while (list.Next(&e)) {
            if (!e.isDir || e.name[0] == '.')
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
        snprintf(sub, sizeof sub, "%s" PC_SEP "%s", dir, names[i]);
        AddMod(sub);
    }
    free(names);
}

void Mods::AddMod(const char *dir) {
    char full[MAX_PATH], manifest[MAX_PATH];
    const char *section, *key;
    char *value;
    IniReader ini;
    if (!os::FullPath(dir, full, sizeof full))
        snprintf(full, sizeof full, "%s", dir);
    for (int i = 0; i < m_n; i++)
        if (!_stricmp(m_mods[i].dir, full))
            return;
    if (!FindEntry(full, "mod.ini", false, manifest, sizeof manifest) || !ini.Open(manifest)) {
        PC_Log("mods: %s has no mod.ini: not loaded", full);
        return;
    }
    if (m_n == kMaxMods) {
        PC_Log("mods: more than %d mods: %s not loaded", kMaxMods, full);
        return;
    }
    Mod &m = m_mods[m_n];
    memset(&m, 0, sizeof m);
    snprintf(m.dir, sizeof m.dir, "%s", full);
    const char *slash = strrchr(full, PC_SEPC);
    snprintf(m.folder, sizeof m.folder, "%s", slash ? slash + 1 : full);
    snprintf(m.name, sizeof m.name, "%s", m.folder);
    bool enabled = true;
    while (ini.Next(&section, &key, &value)) {
        if (_stricmp(section, "Mod"))
            continue;
        if (!_stricmp(key, "Name"))
            ini::Copy(m.name, sizeof m.name, value);
        else if (!_stricmp(key, "Author"))
            ini::Copy(m.author, sizeof m.author, value);
        else if (!_stricmp(key, "Version"))
            ini::Copy(m.version, sizeof m.version, value);
        else if (!_stricmp(key, "Date"))
            ini::Copy(m.date, sizeof m.date, value);
        else if (!_stricmp(key, "Description"))
            ini::Copy(m.description, sizeof m.description, value);
        else if (!_stricmp(key, "Website"))
            ini::Copy(m.website, sizeof m.website, value);
        else if (!_stricmp(key, "Enabled"))
            enabled = ini::Bool(value);
        else
            PC_Log("mods: %s: unknown entry \"%s\" in [Mod]", manifest, key);
    }
    if (!enabled) {
        PC_Log("mods: %s (%s) is disabled (Enabled = 0 in its mod.ini)", m.name, full);
        return;
    }
    if (!FindEntry(full, "Data", true, m.dataDir, sizeof m.dataDir))
        m.dataDir[0] = 0;
    m_n++;
    PC_Log("mods: loading \"%s\"%s%s from %s", m.name, m.version[0] ? " version " : "", m.version,
           full);
    if (m.author[0])
        PC_Log("mods:   author: %s", m.author);
    if (m.date[0])
        PC_Log("mods:   date: %s", m.date);
    if (m.website[0])
        PC_Log("mods:   website: %s", m.website);
    if (m.description[0]) {
        /* a line of the log per line of the description */
        for (const char *s = m.description; *s;) {
            const char *e = strchr(s, '\n');
            int len = e ? (int)(e - s) : (int)strlen(s);
            PC_Log("mods:   %s%.*s", s == m.description ? "description: " : "             ",
                   len, s);
            s += len + (e ? 1 : 0);
        }
    }
}

void Mods::Init() {
    char dir[MAX_PATH];
    const char *env = getenv("THPS_MODS");
    if (m_done)
        return;
    m_done = true;
    if (env && !_stricmp(env, "none")) {
        PC_Log("mods: THPS_MODS=none: no mods loaded");
        return;
    }
    g_assets.Path(dir, sizeof dir, "Mods");
    AddFolder(dir);
    snprintf(dir, sizeof dir, "%s" PC_SEP "Mods", g_runtime.UserDir());
    AddFolder(dir);
    /* THPS_MODS: a mod's folder (it has a mod.ini), or a folder of mods */
    for (const char *s = env; s && *s;) {
        const char *e = strchr(s, kListSep);
        int len = e ? (int)(e - s) : (int)strlen(s);
        char path[MAX_PATH], manifest[MAX_PATH];
        snprintf(path, sizeof path, "%.*s", len, s);
        os::PathFix(path);
        if (!path[0]) {
        } else if (!os::Exists(path)) {
            PC_Log("mods: THPS_MODS names %s, which doesn't exist", path);
        } else if (FindEntry(path, "mod.ini", false, manifest, sizeof manifest)) {
            AddMod(path);
        } else {
            AddFolder(path);
        }
        s += len + (e ? 1 : 0);
    }
    if (m_n)
        PC_Log("mods: %d loaded", m_n);
}

} // namespace pc
