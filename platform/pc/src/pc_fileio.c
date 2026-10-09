/* The game's file loader (FILEIO.CPP) reading from Data/ instead of CD.WAD.
 *
 * On the PS1, FileIO_Init reads the CD.HED directory, FileIO_Open looks a name up in it (or in
 * the loaded PRE archive, SpoolerBlocksFileIO), and FileIO_Load starts an asynchronous CdRead
 * of the file's sectors that FileIO_Continue/FileIO_Sync drive to completion. Here a file is the
 * extracted copy under Data/ (pc_assets.c), loaded in one go: by the time FileIO_Load returns
 * the data is in place and Loading is 0, so the game's polling loops see a finished read.
 *
 * Kept from the original: the PRE archive lookup, the sizes the game allocates by (rounded up to
 * whole sectors; the tail past the file is zeroed instead of holding the next WAD file), and
 * FileIO_Load stopping the music stream first (the game frees the stream's buffers there before
 * allocating for the load). */
#include <stdio.h>
#include <string.h>

#include "pc_assets.h"
#include "pc_hle.h"
#include "pc_runtime.h"

#define VAR(type, addr) (*(type *)PSX_PTR(addr))
#define Loading VAR(int32_t, 0x800d1afc)
#define LastLoading VAR(int32_t, 0x800d1b34)
#define WadStart VAR(int32_t, 0x800d2258)
#define ReadSyncFailures VAR(int32_t, 0x800d1b0c)
#define CurrentPos VAR(int32_t, 0x800d1af8)
#define FileOpen VAR(int32_t, 0x800d1b04)
#define SpoolerBlocksFileIO VAR(uint32_t, 0x800d1b10)
#define LoadFromPRE VAR(int32_t, 0x800d1b14)
#define ThePREFile VAR(uint32_t, 0x800d1b18)
#define ThePREFileSize VAR(int32_t, 0x800d1b1c)
#define Opening VAR(uint32_t, 0x800d1b30)
#define FileSize VAR(int32_t, 0x800d224c)
#define NumSectors VAR(int32_t, 0x800d2264)
#define Start VAR(int32_t, 0x800d2268)
#define Dest VAR(uint32_t, 0x800d2260)
#define Redbook_XACheckSectorOnVSync VAR(int32_t, 0x800d1698)
#define Redbook_XAModeSet VAR(int32_t, 0x800d16a0)
#define DAT_800d169c VAR(int32_t, 0x800d169c)

#define PREManager_getFile 0x800663d8u /* char *PREManager::getFile(char *, int *) */
#define GameFMV_StopFMV 0x800233d0u

static char s_path[260];
static int s_size;

void hleov_8004c74c(void) { /* FileIO_Init */
    Loading = 0;
    WadStart = 0;
    ReadSyncFailures = 0;
}

int hleov_8004c8a4(const char *name) { /* FileIO_Open */
    CurrentPos = 0;
    LoadFromPRE = 0;
    if (SpoolerBlocksFileIO) {
        uint32_t (*getFile)(uint32_t, const char *, int32_t *) = PSX_PTR(PREManager_getFile);
        ThePREFile = getFile(SpoolerBlocksFileIO, name, &ThePREFileSize);
        if (ThePREFile) {
            LoadFromPRE = 1;
            FileOpen = 1;
            return ThePREFileSize;
        }
    }
    if (!PC_AssetsFind(name, s_path, sizeof s_path))
        PC_Fatal("The game asked for %s, which is not in the Data folder. Delete Data\\Manifest.txt "
                 "to extract the game files from the disc again.", name);
    {
        FILE *f = fopen(s_path, "rb");
        if (!f)
            PC_Fatal("cannot open %s", s_path);
        fseek(f, 0, SEEK_END);
        s_size = (int)ftell(f);
        fclose(f);
    }
    Opening = (uint32_t)(uintptr_t)name;
    Start = 0;
    NumSectors = (s_size + 2047) / 2048;
    FileSize = NumSectors * 2048;
    FileOpen = 1;
    PC_Trace("FileIO_Open(%s) -> %s, %d bytes", name, s_path, s_size);
    return FileSize;
}

void hleov_8004cb10(uint32_t dest) { /* FileIO_Load */
    FILE *f;
    if (SpoolerBlocksFileIO && LoadFromPRE) {
        memcpy(PSX_PTR(dest), PSX_PTR(ThePREFile), (size_t)ThePREFileSize);
        FileOpen = 0;
        return;
    }
    ((void (*)(void))PSX_PTR(GameFMV_StopFMV))();
    Dest = dest;
    Redbook_XACheckSectorOnVSync = 0;
    DAT_800d169c = 1;
    Redbook_XAModeSet = 0;
    f = fopen(s_path, "rb");
    if (!f || fread(PSX_PTR(dest), 1, (size_t)s_size, f) != (size_t)s_size)
        PC_Fatal("cannot read %s", s_path);
    fclose(f);
    memset((uint8_t *)PSX_PTR(dest) + s_size, 0, (size_t)(FileSize - s_size));
    CurrentPos = FileSize;
    Loading = 0;
    FileOpen = 0;
}

void hleov_8004cc18(void) { /* FileIO_Continue: loads finish inside FileIO_Load */
    LastLoading = Loading;
    Loading = 0;
}

void hleov_8004cdd0(void) { /* FileIO_Sync */
    Loading = 0;
    LoadFromPRE = 0;
}
