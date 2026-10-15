/* The PS1 disc image (pc_disc.cpp): a raw MODE2/2352 .bin of Tony Hawk's Pro Skater (USA), read
 * only to extract the game's files on first startup (pc/assets.h). The game itself never reads
 * it: it plays from the extracted files. */
#pragma once

#include <stdint.h>
#include <stdio.h>

namespace pc {

class DiscImage {
public:
    /* The image's path, if there is one (no error otherwise): 1 and the path. */
    static int FindImage(char *path, int n);
    /* No PS1 image is used (the data came from the Dreamcast disc): reads fail. */
    void SetNone(bool on) { m_none = on; }
    /* n raw sectors from lba. */
    bool ReadSectors(uint32_t lba, uint32_t n, void *dst);
    /* ISO 9660 lookup ("\\CD.WAD;1"): its first sector and size in bytes. */
    bool FindFile(const char *name, uint32_t *lba, uint32_t *size);

private:
    bool Open();
    bool ReadUserData(uint32_t lba, uint8_t *dst2048);
    bool FindInDir(uint32_t lba, uint32_t size, const char *name, int len, uint32_t *outLba,
                   uint32_t *outSize);

    FILE *m_file = nullptr;
    uint32_t m_sectors = 0;
    bool m_none = false;
};

extern DiscImage g_discImage;

} // namespace pc
