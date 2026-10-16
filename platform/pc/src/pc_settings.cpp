/* Settings.ini: reading and writing the file; the entries belong to the subsystems (see
 * pc/settings.h). */
#include <SDL2/SDL.h>
#include <stdio.h>
#include <string.h>

#include "pc/ini.h"
#include "pc/platform.h"
#include "pc/runtime.h"
#include "pc/settings.h"
#include "pc_runtime.h"
#include "pc/input.h"
#include "pc/video.h"

namespace pc {

Settings g_settings;

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
    const char *section, *key;
    char *value;
    bool warned = false;
    IniReader ini;
    if (m_loaded)
        return;
    m_loaded = true;
    g_input.SettingsDefaults();
    g_video.SettingsDefaults();
    if (!ini.Open(Path())) {
        if (Save())
            PC_Log("settings: wrote the defaults to %s", Path());
        return;
    }
    while (ini.Next(&section, &key, &value)) {
        if (ini.NewSection())
            warned = false;
        if (!section[0])
            continue;
        if (!g_video.SettingsLine(section, key, value) &&
            !g_input.SettingsLine(section, key, value) && !warned) {
            PC_Log("Settings.ini: unknown section [%s]", section);
            warned = true;
        }
    }
    PC_Log("settings: %s", Path());
}

} // namespace pc
