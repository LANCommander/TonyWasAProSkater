/* Saves on PC (src/pc_saves.c). There's no memory card: every career and every replay is its own
 * file in the player's folder, under the name the player gave it:
 *     Saves\Careers\<name>.career      Saves\Replays\<name>.replay
 * The game's card layer (game/src/card) reads and writes them through this; the
 * front end's save and load screens stay the game's own.
 *
 * A file is a 16-byte header ("THPS", format version, type, payload size) and the payload: the
 * part of the game's save buffer past the PS1 card header (icon, Shift-JIS title), i.e. from
 * byte 512 of each buffer Card_Write writes (career: 2688 bytes; replay: 24064 per player, two
 * players back to back). The name is the file's name, so it isn't stored.
 *
 * PS1 saves (.mcs, the single-save format of MemcardRex and the emulators) dropped into the Saves
 * folder are converted on the next listing and renamed to .mcs.imported. */
#ifndef PC_SAVES_H
#define PC_SAVES_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* EFileType's THPS_SAVE_FILE / THPS_VIDEO_FILE */
#define PC_SAVE_CAREER 0
#define PC_SAVE_REPLAY 1

/* Names: up to 10 characters (the name entry's A-Z and space); longer arguments are cut there. */
#define PC_SAVE_NAME_LEN 10

/* A piece of the payload, for writing it from (or reading it into) several buffers. */
typedef struct {
    void *p;
    uint32_t n;
} PCSaveChunk;

/* The game's save directory table (SLOT_DIR, 15 libapi DIRENTRYs of 40 bytes; FileType, one
 * EFileType per entry): fills it with the saves of one type, newest first, each entry's name
 * being the save's. Unused entries get OTHER_FILE (3). Returns the number listed. */
int PC_SaveListDir(int type, void *slotDir, int32_t *fileTypes);

/* The save a SLOT_DIR entry names (Card_SetSelectedFile): the one Card_Load reads. */
void PC_SaveSelect(const char *name);
const char *PC_SaveSelected(void);

/* A card header (SCardHead, 512 bytes) for a listed save, as Card_ReadFileHeader gives the front
 * end: zeroes, with the name as the Shift-JIS title Card_Write would have written for that type
 * (at byte 32 for a career, 30 for a replay). */
void PC_SaveCardHeader(void *head, int type, const char *name);

/* The type of the last PC_SaveListDir. */
int PC_SaveListType(void);

int PC_SaveExists(int type, const char *name);
/* 1 on success. The file is replaced whole (through a temporary file). */
int PC_SaveWrite(int type, const char *name, int n, const PCSaveChunk *chunks);
/* Fills the chunks from the start of the payload: 1 on success, 0 if the file is missing,
 * isn't a save of that type, or is shorter than the chunks. */
int PC_SaveRead(int type, const char *name, int n, const PCSaveChunk *chunks);

#ifdef __cplusplus
}
#endif

#endif
