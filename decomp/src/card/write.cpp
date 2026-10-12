/* Card_Write__F9EFileTypei (0x80014558) */
#include "game/card.h"
#include "game/career.h"
#include "game/video.h"

extern char CARD_FNAME[];
extern char CARD_VNAME[];

extern SCareer GCareer;
extern char Tape01IconPalette[];
extern char Tape01IconTexture[];
extern char Tony01IconPalette[];
extern char Tony01IconTexture[];

extern SVideoInfo GVInfo1;
extern SVideoInfo GVInfo2;
extern int GSaveAction;

int CardStatus;
int CardStatusTemp;
unsigned short TimeStamp;

int SyncToGoodState(void);
void _clear_event(void);
unsigned short CalcChecksum(ulong *, ulong);
void Card_SetHeader(SCardHead *, int, char *, char *);
void SetSJISChecksum(char *, char *);
void Video_SetHeader(SVideoHead *, int);
int Card_CheckStatus(int, int);
void Pause(int);

extern "C" {
	void FUN_8001571c(void);
	void Front_SetSJISName__FPc(char *);
	int erase(char *);
	int open(char *, int);
	int write(int, void *, int);
	int close(int);
}

#ifdef PC_PORT
#include "pc_saves.h"
extern "C" char SaveGameName[];
#endif

int Card_Write(EFileType FileType, int NumBlocks) {
	int fd;
	int i;
	int DataSize;
	int DataSize2;
	int HeaderSize;
	int HeaderSize2;
	ulong *pDataBuf;
	ulong *pDataBuf2;
	ulong *pHeaderBuf;
	ulong *pHeaderBuf2;
	char *pClut;
	char *pIcon;
	char *FName;

	if (SyncToGoodState() == 1)
		return 1;

	switch (FileType) {
		case 0:
			FUN_8001571c();
			GCareer.TimeStamp = TimeStamp;

			FName = CARD_FNAME;
			DataSize = 2560;
			pHeaderBuf = GVInfo1.pBlockStart;
			HeaderSize = 640;
			pDataBuf = (ulong *)&GCareer;
			pHeaderBuf2 = 0;
			HeaderSize2 = 0;
			pDataBuf2 = 0;
			DataSize2 = 0;
			pClut = Tony01IconPalette;
			pIcon = Tony01IconTexture;
			break;
		case 1:
			FName = CARD_VNAME;
			pHeaderBuf = GVInfo1.pBlockStart;
			HeaderSize = 640;
			pDataBuf = pHeaderBuf + 160;
			DataSize = 23936;
			pClut = Tape01IconPalette;
			pIcon = Tape01IconTexture;
			pHeaderBuf2 = GVInfo2.pBlockStart;
			HeaderSize2 = 640;
			pDataBuf2 = pHeaderBuf2 + 160;
			DataSize2 = 23936;
			break;
		case 2:
			break;
	}

	Video_SetHeader((SVideoHead *)(pHeaderBuf + 128), 0);

	if (pHeaderBuf2)
		Video_SetHeader((SVideoHead *)(pHeaderBuf2 + 128), 1);

	Card_SetHeader((SCardHead *)pHeaderBuf, NumBlocks, pClut, pIcon);

	if (GSaveAction == 1)
		Front_SetSJISName__FPc((char *)pHeaderBuf + 32);
	else
		Front_SetSJISName__FPc((char *)pHeaderBuf + 30);

	SetSJISChecksum((char *)pHeaderBuf + 4, FName + 22);

	switch (FileType) {
		case 0:
			*(unsigned short *)((char *)pHeaderBuf + 566) = CalcChecksum(pDataBuf, DataSize);
			break;
		case 1:
			*(unsigned short *)((char *)pHeaderBuf + 566) = CalcChecksum(pDataBuf, DataSize);

			if (pHeaderBuf2)
				*(unsigned short *)((char *)pHeaderBuf2 + 566) = CalcChecksum(pDataBuf2, DataSize2);

			break;
		case 2:
			break;
	}

#ifdef PC_PORT
	/* PC: a file named after the save (pc_saves.h), holding what follows the card header in each
	   buffer. Front_SetSJISName above has tidied SaveGameName. */
	{
		PCSaveChunk Chunks[4];
		int n = 0;

		(void)i;
		(void)fd;
		Chunks[n].p = (char *)pHeaderBuf + 512;
		Chunks[n++].n = HeaderSize - 512;
		Chunks[n].p = pDataBuf;
		Chunks[n++].n = DataSize;

		if (pHeaderBuf2) {
			Chunks[n].p = (char *)pHeaderBuf2 + 512;
			Chunks[n++].n = HeaderSize2 - 512;
			Chunks[n].p = pDataBuf2;
			Chunks[n++].n = DataSize2;
		}

		return !PC_SaveWrite(FileType == THPS_SAVE_FILE ? PC_SAVE_CAREER : PC_SAVE_REPLAY,
		                     SaveGameName, n, Chunks);
	}
#endif

	for (i = 0; i < 16; i++) {
		if (CardStatus != 2 || CardStatusTemp != 2) {
			if (FileType == 1)
				erase(FName);

			fd = open(FName, (NumBlocks << 16) | 512);

			if (fd != -1)
				close(fd);

			fd = open(FName, 2);

			if (fd != -1) {
				_clear_event();

				if (write(fd, pHeaderBuf, HeaderSize) != HeaderSize)
					return 1;

				if (write(fd, pDataBuf, DataSize) != DataSize)
					return 1;

				if (pHeaderBuf2) {
					if (write(fd, pHeaderBuf2, HeaderSize2) != HeaderSize2)
						return 1;

					if (write(fd, pDataBuf2, DataSize2) != DataSize2)
						return 1;
				}

				close(fd);
				return 0;
			}
		}

		Pause(1);
		Pause(1);
		Card_CheckStatus(0, 0);
	}

	return 1;
}
