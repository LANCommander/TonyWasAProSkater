/* Settings.ini in the player's folder (PC_UserDir): one file, a section per subsystem. The
 * controls (pc_input.c) and the display ([Video], pc_video.c) each read their own sections and
 * write them back; this file owns the file itself. */
#ifndef PC_SETTINGS_H
#define PC_SETTINGS_H

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Read Settings.ini once (later calls do nothing): every subsystem starts from its defaults,
 * then takes the entries it knows. A missing file is written with the defaults. */
void PC_SettingsLoad(void);
/* Write every subsystem's settings; 0 on failure. */
int PC_SettingsSave(void);

/* The subsystems' parts. *Line returns 1 when the section is theirs; value may be modified. */
void PC_InputSettingsDefaults(void);
int PC_InputSettingsLine(const char *section, const char *key, char *value);
void PC_InputWriteSettings(FILE *f);
void PC_VideoSettingsDefaults(void);
int PC_VideoSettingsLine(const char *section, const char *key, char *value);
void PC_VideoWriteSettings(FILE *f);

#ifdef __cplusplus
}
#endif

#endif
