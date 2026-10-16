/* Reading INI files (pc/ini.h). */
#include <ctype.h>
#include <stdio.h>
#include <string.h>

#include "pc/ini.h"
#include "pc/platform.h"

namespace pc {

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

IniReader::~IniReader() {
    if (m_f)
        fclose(m_f);
}

bool IniReader::Open(const char *path) {
    if (m_f)
        fclose(m_f);
    m_section[0] = 0;
    m_newSection = false;
    m_f = fopen(path, "r");
    if (!m_f)
        return false;
    /* a UTF-8 byte order mark (Notepad writes one) */
    int c[3] = {getc(m_f), getc(m_f), getc(m_f)};
    if (c[0] != 0xef || c[1] != 0xbb || c[2] != 0xbf)
        rewind(m_f);
    return true;
}

bool IniReader::Next(const char **section, const char **key, char **value) {
    if (!m_f)
        return false;
    while (fgets(m_line, sizeof m_line, m_f)) {
        char *s = Trim(m_line), *eq;
        if (!*s || *s == ';' || *s == '#')
            continue;
        if (*s == '[') {
            char *e = strchr(s, ']');
            if (e)
                *e = 0;
            snprintf(m_section, sizeof m_section, "%s", Trim(s + 1));
            m_newSection = true;
            continue;
        }
        if (!(eq = strchr(s, '=')))
            continue;
        *eq = 0;
        *section = m_section;
        *key = Trim(s);
        *value = Trim(eq + 1);
        return true;
    }
    return false;
}

bool IniReader::NewSection() {
    bool b = m_newSection;
    m_newSection = false;
    return b;
}

namespace ini {

bool Bool(const char *value) {
    return !_stricmp(value, "1") || !_stricmp(value, "yes") || !_stricmp(value, "true") ||
           !_stricmp(value, "on");
}

void Copy(char *out, int n, const char *value) {
    int k = 0;
    for (; *value && k < n - 1; value++) {
        if (value[0] == '\\' && value[1] == 'n') {
            out[k++] = '\n';
            value++;
        } else {
            out[k++] = *value;
        }
    }
    out[k] = 0;
}

bool HasWord(const char *value, const char *word) {
    size_t n = strlen(word);
    for (const char *s = value; *s;) {
        while (*s == ',' || isspace((unsigned char)*s))
            s++;
        const char *e = s;
        while (*e && *e != ',')
            e++;
        const char *t = e;
        while (t > s && isspace((unsigned char)t[-1]))
            t--;
        if ((size_t)(t - s) == n && !_strnicmp(s, word, n))
            return true;
        s = e;
    }
    return false;
}

} // namespace ini

} // namespace pc
