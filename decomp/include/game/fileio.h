/* CD file streaming (FILEIO.CPP): open/load/continue/sync of the background file reader. */
#ifndef GAME_FILEIO_H
#define GAME_FILEIO_H

#include "game/types.h"

/* functions */
void FileIO_SetSectorsPerFrame(int);   /* 0x8004c740 */
int FileIO_Open(const char *filename); /* 0x8004c8a4 */
void FileIO_Load(void *p);             /* 0x8004cb10 */
void FileIO_Continue();                /* 0x8004cc18 */
void FileIO_Sync();                    /* 0x8004cdd0 */

#endif
