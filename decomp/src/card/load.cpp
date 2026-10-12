/* Card_Load__F9EFileTypePUlT1 (0x800149f4) */
/* C++ (CARD.CPP). Demo twin. Reads a save/replay file from the memory card (two buffers for replays) and verifies the header checksums. The do{}while(0) makes the read-failure jumps loop exits (reorg predicts them not taken, filling their slots from the fall-through); every failure shares the sync-failure return. */
#include "game/card.h"

extern char CARD_FNAME[];
extern char CARD_VNAME[];

int SyncToGoodState(void);
void _clear_event(void);
int CalcChecksum(ulong *, ulong);

extern "C" {
	int open(char *, int);
	int read(int, void *, int);
	int close(int);
}

#ifdef PC_PORT
#include "pc_saves.h"

/* PC: reads the save picked from the list (Card_SetSelectedFile) from its file (pc_saves.h). The
   buffers come back as the PS1's would: a card header whose Shift-JIS title is the save's name,
   then what was saved, with the same checksum checks. */
int Card_Load(EFileType FileType, ulong *pBuf, ulong *pBuf2) {
	int Type;
	int ReadSize;
	PCSaveChunk Chunks[2];

	switch (FileType) {
		case THPS_SAVE_FILE:
			Type = PC_SAVE_CAREER;
			ReadSize = 3200;
			break;
		case THPS_VIDEO_FILE:
			Type = PC_SAVE_REPLAY;
			ReadSize = 24576;
			break;
		case THPS_VIDEO_HEADER:
			Type = PC_SAVE_REPLAY;
			ReadSize = 640;
			break;
		default:
			return 1;
	}

	Chunks[0].p = (char *)pBuf + 512;
	Chunks[0].n = ReadSize - 512;
	Chunks[1].p = (char *)pBuf2 + 512;
	Chunks[1].n = ReadSize - 512;

	if (!PC_SaveRead(Type, PC_SaveSelected(), pBuf2 ? 2 : 1, Chunks))
		return 1;

	PC_SaveCardHeader(pBuf, Type, PC_SaveSelected());

	if (pBuf2)
		PC_SaveCardHeader(pBuf2, Type, PC_SaveSelected());

	if (FileType == THPS_SAVE_FILE &&
	    *(short *)((char *)pBuf + 566) != (short)CalcChecksum(pBuf + 160, 2560))
		return 1;

	if (FileType == THPS_VIDEO_FILE) {
		if (*(short *)((char *)pBuf + 566) != (short)CalcChecksum(pBuf + 160, 23936))
			return 1;

		if (pBuf2 && *(short *)((char *)pBuf2 + 566) != (short)CalcChecksum(pBuf2 + 160, 23936))
			return 1;
	}

	return 0;
}
#else
int Card_Load(EFileType FileType, ulong *pBuf, ulong *pBuf2) {
	int fd;
	char *FName;
	int ReadSize;

	switch (FileType) {
		case 0:
			FName = CARD_FNAME;
			ReadSize = 3200;
			break;
		case 1:
			FName = CARD_VNAME;
			ReadSize = 24576;
			break;
		case 2:
			FName = CARD_VNAME;
			ReadSize = 640;
			break;
		case 3:
			FName = 0;
			ReadSize = 0;
			break;
	}

	if (SyncToGoodState() == 1) {
	Error:
		return 1;
	}

	do {
		fd = open(FName, 1);

		if (fd == -1)
			goto Error;

		_clear_event();

		if (read(fd, pBuf, ReadSize) != ReadSize)
			goto Error;

		{
			int Chk1 = 0;
			int Chk2 = 0;

			switch (FileType) {
				case 0:
					Chk1 = *(short *)((char *)pBuf + 566);
					Chk2 = CalcChecksum(pBuf + 160, 2560);
					break;
				case 1:
					Chk1 = *(short *)((char *)pBuf + 566);
					Chk2 = CalcChecksum(pBuf + 160, 23936);
					break;
				case 2:
				case 3:
					break;
			}

			if (Chk1 != Chk2)
				goto Error;
		}

		if (pBuf2) {
			if (read(fd, pBuf2, ReadSize) != ReadSize)
				goto Error;

			{
				int Chk1 = 0;
				int Chk2 = 0;

				if (FileType == 1) {
					Chk1 = *(short *)((char *)pBuf2 + 566);
					Chk2 = CalcChecksum(pBuf2 + 160, 23936);
				}

				if (Chk1 != Chk2)
					goto Error;
			}
		}

		close(fd);
		return 0;
	} while (0);
}
#endif
