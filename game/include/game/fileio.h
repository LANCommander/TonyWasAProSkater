/* CD file streaming (FILEIO.CPP): open/load/continue/sync of the background file reader. */
#ifndef GAME_FILEIO_H
#define GAME_FILEIO_H

#include "game/types.h"

/* functions */
void FileIO_SetSectorsPerFrame(int v);
void FileIO_Init();
int FileIO_Open(const char *filename);
void FileIO_Load(void *p);
void FileIO_Continue();
void FileIO_Sync();
#endif
