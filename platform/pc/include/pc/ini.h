/* Reading INI files (Settings.ini, the mods' manifests; pc_ini.cpp): `[Section]` lines, then
 * `key = value` lines; blank lines and lines starting with ';' or '#' are skipped, and keys,
 * values and section names are trimmed. */
#pragma once

#include <stdio.h>

namespace pc {

class IniReader {
public:
    IniReader() = default;
    ~IniReader();
    IniReader(const IniReader &) = delete;
    IniReader &operator=(const IniReader &) = delete;

    /* false if the file can't be read */
    bool Open(const char *path);
    /* The next entry: true and its section ("" before the first one), key and value (valid, and
     * the value writable, until the next call); false at the end of the file. A line with no
     * '=' is skipped. */
    bool Next(const char **section, const char **key, char **value);
    /* true once, after Next moved into a new section (for "unknown section" warnings) */
    bool NewSection();

private:
    FILE *m_f = nullptr;
    char m_line[1024] = {};
    char m_section[128] = {};
    bool m_newSection = false;
};

/* Ini values. */
namespace ini {
/* true for 1/yes/true/on (any case) */
bool Bool(const char *value);
/* A copy of value into out (n bytes; "\n" in the value becomes a line break). */
void Copy(char *out, int n, const char *value);
/* `value` is a comma-separated list (any case, spaces ignored) that contains `word` */
bool HasWord(const char *value, const char *word);
} // namespace ini

} // namespace pc
