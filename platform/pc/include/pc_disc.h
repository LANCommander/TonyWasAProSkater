/* The PS1 disc image (pc_disc.c): a raw MODE2/2352 .bin of Tony Hawk's Pro Skater (USA), read
 * only to extract the game's files on first startup (pc_assets.c). The game itself never reads
 * it: it plays from the extracted files. */
#ifndef PC_DISC_H
#define PC_DISC_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* The image's path, if there is one (no error otherwise): 1 and the path. */
int PC_DiscFindImage(char *path, int n);
/* No PS1 image is used (the data came from the Dreamcast disc): reads fail. */
void PC_DiscSetNone(int on);
/* n raw sectors from lba. */
int PC_DiscReadSectors(uint32_t lba, uint32_t n, void *dst);
/* ISO 9660 lookup ("\\CD.WAD;1"): its first sector and size in bytes. */
int PC_DiscFindFile(const char *name, uint32_t *lba, uint32_t *size);

#ifdef __cplusplus
}
#endif

#endif
