/* The game's files, read from Data/ instead of CD.WAD (pc_fileio.c).
 *
 * FILEIO.CPP keeps its own state (the PRE archive lookup, FileSize, NumSectors, Loading ...) and
 * calls these where it searched the CD directory and read sectors: a load is finished by the time
 * PC_FileRead returns. */
#ifndef PC_FILEIO_H
#define PC_FILEIO_H

#ifdef __cplusplus
extern "C" {
#endif

/* Finds the file the game asks for (fatal if it isn't in Data/) and returns its size in bytes. */
int PC_FileOpen(const char *name);
/* Reads the file PC_FileOpen found to dest, zeroing up to size (the game allocates whole
 * sectors). */
void PC_FileRead(void *dest, int size);

#ifdef __cplusplus
}
#endif

#endif
