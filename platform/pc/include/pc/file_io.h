/* The game's files, read from Data/ instead of CD.WAD (pc_fileio.cpp; the C interface is
 * pc_fileio.h).
 *
 * FILEIO.CPP keeps its own state (the PRE archive lookup, FileSize, NumSectors, Loading ...) and
 * calls these where it searched the CD directory and read sectors: a load is finished by the time
 * Read returns. */
#pragma once

#include <stdint.h>

namespace pc {

class FileIo {
public:
    /* Finds the file the game asks for (fatal if it isn't in Data/) and returns its size in bytes. */
    int Open(const char *name);
    /* Reads the file Open found to dest, zeroing up to size (the game allocates whole sectors). */
    void Read(void *dest, int size);

    /* The size the PS1 game lays out a loaded .bmp at, if Read marked it (pc_assets.h). */
    static bool BmpLayoutSize(const void *bmp, int *lw, int *lh);

private:
    void MarkLayoutSize(uint8_t *bmp, int size);
    void SilentVab();
    void ConvertDcFile();

    char m_path[260] = {};
    char m_name[64] = {};    /* as the game asked for it */
    int m_size = 0;
    uint8_t *m_image = nullptr;   /* the converted file (malloc'd), else null: read the file */
};

extern FileIo g_fileIo;

} // namespace pc
