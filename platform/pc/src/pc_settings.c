/* Settings.ini: reading and writing the file; the entries belong to the subsystems (see
 * pc_settings.h). */
#include <windows.h>
#include <SDL2/SDL.h>
#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "pc_runtime.h"
#include "pc_settings.h"

static const char *SettingsPath(void) {
    static char path[MAX_PATH];
    if (!path[0])
        snprintf(path, sizeof path, "%s\\Settings.ini", PC_UserDir());
    return path;
}

static char *Trim(char *s) {
    char *e;
    while (isspace((unsigned char)*s))
        s++;
    e = s + strlen(s);
    while (e > s && isspace((unsigned char)e[-1]))
        *--e = 0;
    return s;
}

int PC_SettingsSave(void) {
    char tmp[MAX_PATH];
    FILE *f;
    snprintf(tmp, sizeof tmp, "%s.tmp", SettingsPath());
    f = fopen(tmp, "w");
    if (!f) {
        PC_Log("cannot write %s", tmp);
        return 0;
    }
    fputs("; Tony Hawk's Pro Skater (PC) settings. Options > Graphics edits the display,\n"
          "; Options > Player 1/2 Controls the controls.\n",
          f);
    PC_VideoWriteSettings(f);
    PC_InputWriteSettings(f);
    if (fclose(f) != 0 || !MoveFileExA(tmp, SettingsPath(), MOVEFILE_REPLACE_EXISTING)) {
        PC_Log("cannot write %s", SettingsPath());
        DeleteFileA(tmp);
        return 0;
    }
    return 1;
}

/* Entries missing from the file keep their defaults. The file is only written when it doesn't
 * exist yet (and when a settings screen changes something). */
void PC_SettingsLoad(void) {
    static int loaded;
    char line[512], section[128] = "";
    int warned = 0;
    FILE *f;
    if (loaded)
        return;
    loaded = 1;
    PC_InputSettingsDefaults();
    PC_VideoSettingsDefaults();
    f = fopen(SettingsPath(), "r");
    if (!f) {
        if (PC_SettingsSave())
            PC_Log("settings: wrote the defaults to %s", SettingsPath());
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
            warned = 0;
            continue;
        }
        eq = strchr(s, '=');
        if (!section[0] || !eq)
            continue;
        *eq = 0;
        if (!PC_VideoSettingsLine(section, Trim(s), eq + 1) &&
            !PC_InputSettingsLine(section, Trim(s), eq + 1) && !warned) {
            PC_Log("Settings.ini: unknown section [%s]", section);
            warned = 1;
        }
    }
    fclose(f);
    PC_Log("settings: %s", SettingsPath());
}
