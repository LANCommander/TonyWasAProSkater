/* Settings.ini: reading and writing the file; the entries belong to the subsystems (see
 * pc/settings.h). */
#include <SDL2/SDL.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "pc/platform.h"
#include "pc/runtime.h"
#include "pc/settings.h"
#include "pc_runtime.h"
#include "pc/input.h"
#include "pc/video.h"

namespace pc {

Settings g_settings;

namespace {

char *Trim(char *s) {
    char *e;
    while (isspace((unsigned char)*s))
        s++;
    e = s + strlen(s);
    while (e > s && isspace((unsigned char)e[-1]))
        *--e = 0;
    return s;
}

} // namespace

const char *Settings::Path() {
    if (!m_path[0])
        snprintf(m_path, sizeof m_path, "%s" PC_SEP "Settings.ini", g_runtime.UserDir());
    return m_path;
}

bool Settings::Save() {
    char tmp[MAX_PATH];
    FILE *f;
    snprintf(tmp, sizeof tmp, "%s.tmp", Path());
    f = fopen(tmp, "w");
    if (!f) {
        PC_Log("cannot write %s", tmp);
        return false;
    }
    fputs("; Tony Hawk's Pro Skater (PC) settings. Options > Graphics edits the display,\n"
          "; Options > Player 1/2 Controls the controls.\n",
          f);
    g_video.WriteSettings(f);
    g_input.WriteSettings(f);
    if (fclose(f) != 0 || !os::RenameOver(tmp, Path())) {
        PC_Log("cannot write %s", Path());
        os::RemoveFile(tmp);
        return false;
    }
    return true;
}

/* Entries missing from the file keep their defaults. The file is only written when it doesn't
 * exist yet (and when a settings screen changes something). */
void Settings::Load() {
    char line[512], section[128] = "";
    bool warned = false;
    FILE *f;
    if (m_loaded)
        return;
    m_loaded = true;
    g_input.SettingsDefaults();
    g_video.SettingsDefaults();
    f = fopen(Path(), "r");
    if (!f) {
        if (Save())
            PC_Log("settings: wrote the defaults to %s", Path());
        return;
    }
    while (fgets(line, sizeof line, f)) {
        char *s = Trim(line), *eq;
        if (!*s || *s == ';' || *s == '#')
            continue;
        if (*s == '[') {
            char *e = strchr(s, ']');
            if (e)
                *e = 0;
            snprintf(section, sizeof section, "%s", Trim(s + 1));
            warned = false;
            continue;
        }
        eq = strchr(s, '=');
        if (!section[0] || !eq)
            continue;
        *eq = 0;
        if (!g_video.SettingsLine(section, Trim(s), eq + 1) &&
            !g_input.SettingsLine(section, Trim(s), eq + 1) && !warned) {
            PC_Log("Settings.ini: unknown section [%s]", section);
            warned = true;
        }
    }
    fclose(f);
    PC_Log("settings: %s", Path());
}

} // namespace pc
