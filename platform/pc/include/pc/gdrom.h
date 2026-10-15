/* Reading files off a Dreamcast GD-ROM dump (.gdi and its track files), for the first-startup
 * extraction of the Dreamcast version's data (src/pc_gdrom.cpp, used by pc/assets.h). */
#pragma once

#include <stdint.h>
#include <stdio.h>

namespace pc {

class GdRom {
public:
    /* Every file, depth first: name is the path from the root ("1ST_READ.BIN", "DIR/FILE.EXT"),
     * without the ";1". */
    using FileFn = void (*)(const char *name, uint32_t lba, uint32_t size, void *ctx);

    /* No destructor: the tracks stay open until Close (a global instance needs no exit work). */
    constexpr GdRom() = default;
    GdRom(const GdRom &) = delete;
    GdRom &operator=(const GdRom &) = delete;

    /* Open a .gdi: true if its tracks are there and its high-density area holds an ISO 9660 file
     * system. */
    bool Open(const char *gdi);
    void Close();

    void List(FileFn fn, void *ctx);

    /* Read size bytes of a file's data starting at its lba: true on success. */
    bool Read(uint32_t lba, uint32_t size, void *dst);

private:
    static constexpr int kMaxTracks = 8;
    static constexpr uint32_t kHdArea = 45000;

    struct Track {
        uint32_t lba, count, ssize;
        int data;
        FILE *f;
    };

    bool ReadSector(uint32_t lba, uint8_t *dst);
    void Walk(uint32_t lba, uint32_t size, const char *prefix, FileFn fn, void *ctx, int depth);

    Track m_tracks[kMaxTracks] = {};
    int m_ntracks = 0;
    uint32_t m_rootLba = 0, m_rootSize = 0;
};

} // namespace pc
