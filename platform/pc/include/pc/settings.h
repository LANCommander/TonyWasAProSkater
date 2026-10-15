/* Settings.ini in the player's folder (Runtime::UserDir): one file, a section per subsystem. The
 * controls (pc_input.cpp) and the display ([Video], pc_video.cpp) each read their own sections
 * and write them back; this class owns the file itself. The C interface is pc_settings.h. */
#pragma once

#include "pc/platform.h"

namespace pc {

class Settings {
public:
    /* Read Settings.ini once (later calls do nothing): every subsystem starts from its defaults,
     * then takes the entries it knows. A missing file is written with the defaults. */
    void Load();
    /* Write every subsystem's settings; false on failure. */
    bool Save();

private:
    const char *Path();

    char m_path[MAX_PATH] = "";
    bool m_loaded = false;
};

extern Settings g_settings;

} // namespace pc
