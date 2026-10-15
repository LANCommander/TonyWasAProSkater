/* The game's files (pc_fileio.h), read from Data/ instead of CD.WAD.
 *
 * On the PS1, FileIO_Init reads the CD.HED directory, FileIO_Open looks a name up in it, and
 * FileIO_Load starts an asynchronous CdRead of the file's sectors that FileIO_Continue/FileIO_Sync
 * drive to completion. In the port FILEIO.CPP calls PC_FileOpen for the lookup
 * and PC_FileRead for the read: a file is the extracted copy under Data/ (pc_assets.cpp), loaded in
 * one go, so the game's polling loops see a finished read. The PRE archive lookup, the sector
 * rounding and stopping the music stream stay in the game's code; the tail past the file is zeroed
 * instead of holding the next WAD file.
 *
 * Dreamcast model/level files (version 6 .psx) are loaded without their texture images, which
 * stay on the PC side (pc_dctex.h): the game allocates and receives the smaller image. The
 * Dreamcast has no PS1 sound banks: with its files, a .vab the game asks for is made from its
 * .kat of the same name (pc_dcsound.h), or is a silent one if there is none, and an image it
 * redrew larger carries the size the PS1 game lays it out at (PC_BmpLayoutSize). */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "pc/assets.h"
#include "pc/dc_textures.h"
#include "pc/file_io.h"
#include "pc_assets.h"
#include "pc_fileio.h"
#include "pc/platform.h"
#include "pc_runtime.h"
#include "pc/dc_sound.h"

namespace pc {

namespace {
/* A loaded .bmp the Dreamcast redrew larger (pc_assets.h): the size the PS1 game lays it out at
 * goes into its header, as "PCLS" in the file header's reserved words and the size in the
 * pixels-per-metre fields. */
constexpr uint32_t kBmpMark = 0x534c4350u;   /* "PCLS" */
}

FileIo g_fileIo;

void FileIo::MarkLayoutSize(uint8_t *bmp, int size) {
    int w, h, lw, lh;
    if (size < 54 || bmp[0] != 'B' || bmp[1] != 'M')
        return;
    w = (int)*(int32_t *)(bmp + 18);
    h = (int)*(int32_t *)(bmp + 22);
    if (!g_assets.ImageLayoutSize(m_name, w, h, &lw, &lh))
        return;
    *(uint32_t *)(bmp + 6) = kBmpMark;
    *(int32_t *)(bmp + 38) = lw;
    *(int32_t *)(bmp + 42) = lh;
    PC_Trace("FileIO: %s (%dx%d) is laid out at %dx%d", m_name, w, h, lw, lh);
}

bool FileIo::BmpLayoutSize(const void *bmp, int *lw, int *lh) {
    const uint8_t *b = (const uint8_t *)bmp;
    if (!b || *(const uint32_t *)(b + 6) != kBmpMark)
        return false;
    *lw = (int)*(const int32_t *)(b + 38);
    *lh = (int)*(const int32_t *)(b + 42);
    return true;
}

/* A silent sound bank (VAB): 32 programs of 16 tones at volume 0, one empty sample. Header,
 * 128 program entries, 16 tone entries per program, 256 sample sizes (in 8-byte units), samples:
 * the game keeps the first three parts and loads the samples as a bank (FUN_8004b188, pc_sound.h). */
void FileIo::SilentVab() {
    enum { PROGS = 32, TONES = 16, HEAD = 2592 + PROGS * 512, SAMPLE = 32 };
    uint8_t *v = (uint8_t *)calloc(1, HEAD + SAMPLE);
    memcpy(v, "pBAV", 4);
    v[4] = 7;                                  /* version */
    *(uint32_t *)(v + 12) = HEAD + SAMPLE;     /* file size */
    *(uint16_t *)(v + 0x12) = PROGS;
    *(uint16_t *)(v + 0x14) = PROGS * TONES;
    *(uint16_t *)(v + 0x16) = 1;               /* samples */
    v[0x18] = 127;                             /* master volume, pan */
    v[0x19] = 64;
    for (int p = 0; p < PROGS; p++) {
        uint8_t *pa = v + 0x20 + p * 16;
        pa[0] = TONES;
        pa[1] = 127;
        pa[4] = 64;
        for (int t = 0; t < TONES; t++) {
            uint8_t *ta = v + 0x820 + (p * TONES + t) * 32;
            ta[3] = 64;                        /* volume 0, pan, centre note, note range */
            ta[4] = 60;
            ta[7] = 127;
            *(uint16_t *)(ta + 16) = 0x80ff;   /* ADSR */
            *(uint16_t *)(ta + 18) = 0x5fc0;
            *(int16_t *)(ta + 20) = (int16_t)p;
            *(int16_t *)(ta + 22) = 1;
        }
    }
    *(uint16_t *)(v + 0x820 + PROGS * 512 + 2) = SAMPLE >> 3;
    v[HEAD + 16 + 1] = 1;                      /* the second block ends the sample */
    free(m_image);
    m_image = v;
    m_size = HEAD + SAMPLE;
}

/* A version 6 .psx: read it and keep the converted image for FileIO_Load. */
void FileIo::ConvertDcFile() {
    size_t n = strlen(m_path);
    uint32_t outSize;
    free(m_image);
    m_image = nullptr;
    if (n < 4 || _stricmp(m_path + n - 4, ".psx") || m_size < 12)
        return;
    FILE *f = fopen(m_path, "rb");
    if (!f)
        PC_Fatal("cannot open %s", m_path);
    uint8_t *data = (uint8_t *)malloc((size_t)m_size);
    if (fread(data, 1, (size_t)m_size, f) != (size_t)m_size)
        PC_Fatal("cannot read %s", m_path);
    fclose(f);
    if (DcTextures::PsxIsV6(data, (uint32_t)m_size)) {
        m_image = g_dcTextures.PsxStrip(data, (uint32_t)m_size, &outSize);
        if (!m_image)
            PC_Fatal("%s is damaged", m_path);
        PC_Trace("FileIO: %s is a Dreamcast file, %d -> %u bytes", m_path, m_size, outSize);
        m_size = (int)outSize;
    }
    free(data);
}

int FileIo::Open(const char *name) {
    snprintf(m_name, sizeof m_name, "%s", name);
    if (!g_assets.Find(name, m_path, sizeof m_path)) {
        size_t n = strlen(name);
        if (!g_assets.Dreamcast() || n < 4 || _stricmp(name + n - 4, ".vab"))
            PC_Fatal("The game asked for %s, which is not in the Data folder. Delete "
                     "Data" PC_SEP "Manifest.txt to extract the game files from the disc again.", name);
        free(m_image);
        if ((m_image = g_dcSound.Vab(name, &m_size))) {
            snprintf(m_path, sizeof m_path, "%s (from its .kat)", name);
        } else {
            snprintf(m_path, sizeof m_path, "%s (silent)", name);
            SilentVab();
        }
    } else {
        FILE *f = fopen(m_path, "rb");
        if (!f)
            PC_Fatal("cannot open %s", m_path);
        fseek(f, 0, SEEK_END);
        m_size = (int)ftell(f);
        fclose(f);
        ConvertDcFile();
    }
    PC_Trace("FileIO_Open(%s) -> %s, %d bytes", name, m_path, m_size);
    return m_size;
}

void FileIo::Read(void *dest, int size) {
    if (m_image) {
        memcpy(dest, m_image, (size_t)m_size);
        free(m_image);
        m_image = nullptr;
    } else {
        FILE *f = fopen(m_path, "rb");
        if (!f || fread(dest, 1, (size_t)m_size, f) != (size_t)m_size)
            PC_Fatal("cannot read %s", m_path);
        fclose(f);
        MarkLayoutSize((uint8_t *)dest, m_size);
    }
    if (size > m_size)
        memset((uint8_t *)dest + m_size, 0, (size_t)(size - m_size));
}

} // namespace pc

/* ---- the game's interface (pc_fileio.h) ---- */

extern "C" {

int PC_FileOpen(const char *name) { return pc::g_fileIo.Open(name); }
void PC_FileRead(void *dest, int size) { pc::g_fileIo.Read(dest, size); }
int PC_BmpLayoutSize(const void *bmp, int *lw, int *lh) { return pc::FileIo::BmpLayoutSize(bmp, lw, lh); }

}
