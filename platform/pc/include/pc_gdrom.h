/* Reading files off a Dreamcast GD-ROM dump (.gdi and its track files), for the first-startup
 * extraction of the Dreamcast version's data (src/pc_gdrom.c, used by pc_assets.c). */
#ifndef PC_GDROM_H
#define PC_GDROM_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Open a .gdi: 1 if its tracks are there and its high-density area holds an ISO 9660 file
 * system. */
int PC_GdOpen(const char *gdi);
void PC_GdClose(void);

/* Every file, depth first: name is the path from the root ("1ST_READ.BIN", "DIR/FILE.EXT"),
 * without the ";1". */
typedef void (*PC_GdFileFn)(const char *name, uint32_t lba, uint32_t size, void *ctx);
void PC_GdList(PC_GdFileFn fn, void *ctx);

/* Read size bytes of a file's data starting at its lba: 1 on success. */
int PC_GdRead(uint32_t lba, uint32_t size, void *dst);

#ifdef __cplusplus
}
#endif

#endif
