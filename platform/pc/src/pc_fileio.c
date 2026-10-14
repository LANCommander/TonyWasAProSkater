/* The game's files (pc_fileio.h), read from Data/ instead of CD.WAD.
 *
 * On the PS1, FileIO_Init reads the CD.HED directory, FileIO_Open looks a name up in it, and
 * FileIO_Load starts an asynchronous CdRead of the file's sectors that FileIO_Continue/FileIO_Sync
 * drive to completion. In the port FILEIO.CPP calls PC_FileOpen for the lookup
 * and PC_FileRead for the read: a file is the extracted copy under Data/ (pc_assets.c), loaded in
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

#include "pc_assets.h"
#include "pc_dcsound.h"
#include "pc_dctex.h"
#include "pc_fileio.h"
#include "pc_platform.h"
#include "pc_runtime.h"

static char s_path[260];
static char s_name[64];    /* as the game asked for it */
static int s_size;
static uint8_t *s_image;   /* the converted file (version 6 .psx), else NULL: read the file */

/* A loaded .bmp the Dreamcast redrew larger (pc_assets.h): the size the PS1 game lays it out at
 * goes into its header, as "PCLS" in the file header's reserved words and the size in the
 * pixels-per-metre fields. */
#define BMP_MARK 0x534c4350u   /* "PCLS" */

static void MarkLayoutSize(uint8_t *bmp, int size) {
    int w, h, lw, lh;
    if (size < 54 || bmp[0] != 'B' || bmp[1] != 'M')
        return;
    w = (int)*(int32_t *)(bmp + 18);
    h = (int)*(int32_t *)(bmp + 22);
    if (!PC_ImageLayoutSize(s_name, w, h, &lw, &lh))
        return;
    *(uint32_t *)(bmp + 6) = BMP_MARK;
    *(int32_t *)(bmp + 38) = lw;
    *(int32_t *)(bmp + 42) = lh;
    PC_Trace("FileIO: %s (%dx%d) is laid out at %dx%d", s_name, w, h, lw, lh);
}

int PC_BmpLayoutSize(const void *bmp, int *lw, int *lh) {
    const uint8_t *b = bmp;
    if (!b || *(const uint32_t *)(b + 6) != BMP_MARK)
        return 0;
    *lw = (int)*(const int32_t *)(b + 38);
    *lh = (int)*(const int32_t *)(b + 42);
    return 1;
}

/* A silent sound bank (VAB): 32 programs of 16 tones at volume 0, one empty sample. Header,
 * 128 program entries, 16 tone entries per program, 256 sample sizes (in 8-byte units), samples:
 * the game keeps the first three parts and loads the samples as a bank (FUN_8004b188, pc_sound.h). */
static void SilentVab(void) {
    enum { PROGS = 32, TONES = 16, HEAD = 2592 + PROGS * 512, SAMPLE = 32 };
    uint8_t *v = calloc(1, HEAD + SAMPLE);
    int p, t;
    memcpy(v, "pBAV", 4);
    v[4] = 7;                                  /* version */
    *(uint32_t *)(v + 12) = HEAD + SAMPLE;     /* file size */
    *(uint16_t *)(v + 0x12) = PROGS;
    *(uint16_t *)(v + 0x14) = PROGS * TONES;
    *(uint16_t *)(v + 0x16) = 1;               /* samples */
    v[0x18] = 127;                             /* master volume, pan */
    v[0x19] = 64;
    for (p = 0; p < PROGS; p++) {
        uint8_t *pa = v + 0x20 + p * 16;
        pa[0] = TONES;
        pa[1] = 127;
        pa[4] = 64;
        for (t = 0; t < TONES; t++) {
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
    free(s_image);
    s_image = v;
    s_size = HEAD + SAMPLE;
}

/* A version 6 .psx: read it and keep the converted image for FileIO_Load. */
static void ConvertDcFile(void) {
    size_t n = strlen(s_path);
    uint8_t *data;
    uint32_t outSize;
    FILE *f;
    free(s_image);
    s_image = NULL;
    if (n < 4 || _stricmp(s_path + n - 4, ".psx") || s_size < 12)
        return;
    f = fopen(s_path, "rb");
    if (!f)
        PC_Fatal("cannot open %s", s_path);
    data = malloc((size_t)s_size);
    if (fread(data, 1, (size_t)s_size, f) != (size_t)s_size)
        PC_Fatal("cannot read %s", s_path);
    fclose(f);
    if (PC_DcPsxIsV6(data, (uint32_t)s_size)) {
        s_image = PC_DcPsxStrip(data, (uint32_t)s_size, &outSize);
        if (!s_image)
            PC_Fatal("%s is damaged", s_path);
        PC_Trace("FileIO: %s is a Dreamcast file, %d -> %u bytes", s_path, s_size, outSize);
        s_size = (int)outSize;
    }
    free(data);
}

int PC_FileOpen(const char *name) {
    snprintf(s_name, sizeof s_name, "%s", name);
    if (!PC_AssetsFind(name, s_path, sizeof s_path)) {
        size_t n = strlen(name);
        if (!PC_AssetsDreamcast() || n < 4 || _stricmp(name + n - 4, ".vab"))
            PC_Fatal("The game asked for %s, which is not in the Data folder. Delete "
                     "Data" PC_SEP "Manifest.txt to extract the game files from the disc again.", name);
        free(s_image);
        if ((s_image = PC_DcSoundVab(name, &s_size))) {
            snprintf(s_path, sizeof s_path, "%s (from its .kat)", name);
        } else {
            snprintf(s_path, sizeof s_path, "%s (silent)", name);
            SilentVab();
        }
    } else {
        FILE *f = fopen(s_path, "rb");
        if (!f)
            PC_Fatal("cannot open %s", s_path);
        fseek(f, 0, SEEK_END);
        s_size = (int)ftell(f);
        fclose(f);
        ConvertDcFile();
    }
    PC_Trace("FileIO_Open(%s) -> %s, %d bytes", name, s_path, s_size);
    return s_size;
}

void PC_FileRead(void *dest, int size) {
    FILE *f;
    if (s_image) {
        memcpy(dest, s_image, (size_t)s_size);
        free(s_image);
        s_image = NULL;
    } else {
        f = fopen(s_path, "rb");
        if (!f || fread(dest, 1, (size_t)s_size, f) != (size_t)s_size)
            PC_Fatal("cannot read %s", s_path);
        fclose(f);
        MarkLayoutSize(dest, s_size);
    }
    if (size > s_size)
        memset((uint8_t *)dest + s_size, 0, (size_t)(size - s_size));
}
