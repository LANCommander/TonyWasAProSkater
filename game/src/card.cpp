/* CARD.CPP: the memory card: card headers and checksums, writing and loading saves and replays
   (files in the port, pc_saves.h), the card status/event polling and the career auto-load
   (declarations in game/card.h). */
#include "pc_saves.h"
#include "pc_frame.h"
#include "game/card.h"
#include "game/front.h"
#include "game/career.h"
#include "game/video.h"
#include "game/psyq.h"
#include "game/fileio.h"
#include "game/mem.h"
#include "game/globals.h"
#include "game/rfront.h"
#include "game/utils.h"

/* a memory-card directory entry (LIBAPI's struct DIRENTRY) */
struct DIRENTRY {
	char name[20];
	int attr;
	int size;
	struct DIRENTRY *next;
	int head;
	char system[4];
};

extern char CARD_FNAME[];
extern char CARD_VNAME[];
extern char CARD_GTITLE[];
extern char CARD_VTITLE[];
extern int CardStatusMode;
extern int CardLoadFlag;
extern int CardStatusTemp;
extern int EVENT_TIME_OUT;
extern int DAT_800d137c;
extern int ev0, ev1, ev2, ev3, ev10, ev11, ev12, ev13;
extern unsigned short TimeStamp;
/* the card header's icon (Tony01Icon*, Tape01Icon* in retail): the port writes no card header
   (pc_saves.c), so it has none of the art */
static char NoIcon[128];

int SyncToGoodState();
void Card_SetHeader(SCardHead *, int, char *, char *);
void SetSJISChecksum(char *, char *);
int CalcChecksum(ulong *, ulong);
void _clear_event();
int _card_event();
void _clear_event_x();
int _card_event_x();
extern "C" {
	int open(char *, int);
	int read(int, void *, int);
	int write(int, void *, int);
	int close(int);
	int erase(char *);
	int PCinit(void);
	int PCcreat(char *, int);
	int PClseek(int, int, int);
	int PCwrite(int, char *, int);
	int PCclose(int);
	extern struct DIRENTRY *firstfile(char *, struct DIRENTRY *);
	extern struct DIRENTRY *nextfile(struct DIRENTRY *);
	/* LIBAPI's event and card calls, by address */
	void FUN_8008e944(int);
	void FUN_8008e9d4(void);
	void FUN_8008ea38(void);
	void FUN_8008ea48(int);
	int FUN_8008eab8(int, int, int, int);
	void FUN_8008eac8(void);
	void FUN_8008f0dc(void);
	int FUN_8008eb08(int);
	int FUN_8008eb4c(char *, int);
	int FUN_8008ead8(int, void *, int);
	int FUN_8008e934(int);
	int FUN_8008e914(int);
	int FUN_8008f0cc(int);
	void FUN_8008ea68(int);
	int FUN_8008eaa8(char *);
}

/* Card_SetSelectedFile__FPc (0x80014218) */
void Card_SetSelectedFile(char *p) {
	/* PC: p is a SLOT_DIR entry filled by PC_SaveListDir: the save's name */
	PC_SaveSelect(p);
	return;
	CARD_FNAME[22] = p[17];
	CARD_FNAME[23] = p[18];
	CARD_VNAME[22] = p[17];
	CARD_VNAME[23] = p[18];
}

/* SetSJISChecksum__FPcT0 (0x8001424c) */
void SetSJISChecksum(char *title, char *dst) {
	unsigned short *src = (unsigned short *)title;
	unsigned short h = 0;
	unsigned a, b;

	while (*src) {
		h = ((h << 2) + (h >> 10)) ^ (*src >> 8);
		src++;
	}

	h &= 0x3ff;
	b = h & 31;
	h >>= 5;

	if (h < 26)
		dst[0] = h + 65;
	else
		dst[0] = h + 22;

	if (b < 26)
		dst[1] = b + 65;
	else
		dst[1] = b + 22;
}

/* CalcChecksum__FPUlUl (0x800142c4) */
int CalcChecksum(ulong *pBuf, ulong n) {
	unsigned char *p = (unsigned char *)pBuf;
	unsigned i;
	short s = 0;

	for (i = 0; i < n; i++)
		s = s + (p[i] + 1);

	return s;
}

/* Card_SetHeader__FP9SCardHeadiPcT2 (0x80014300) */
void Card_SetHeader(SCardHead *h, int icon, char *name, char *data) {
	int i;

	h->Magic[0] = 83;
	h->Magic[1] = 67;
	h->Type = 17;
	h->BlockEntry = icon;

	for (i = 0; i < 64; i++)
		h->Title[i] = 0;

	if (GSaveAction == 1)
		strcpy(h->Title, CARD_GTITLE);
	else
		strcpy(h->Title, CARD_VTITLE);

	for (i = 0; i < 28; i++)
		h->reserve[i] = 0;

	for (i = 0; i < 32; i++)
		h->Clut[i] = name[i];

	for (i = 0; i < 128; i++) {
		h->Icon[0][i] = data[i];
		h->Icon[1][i] = data[i];
		h->Icon[2][i] = data[i];
	}
}

/* SyncToGoodState__Fv (0x80014424) */
int SyncToGoodState(void) {
	int n = 200;

	/* PC: no memory card to wait for (see Card_CheckStatus) */
	CardStatusMode = 0;
	CardLoadFlag = 1;
	CardStatusTemp = 1;
	CardStatus = 1;
	return 0;

	CardStatusMode = 0;
	CardLoadFlag = 0;

	for (;;) {
		PC_FrameWait(1);
		Card_CheckStatus(0, 0);

		if (CardLoadFlag == 1)
			break;

		if (--n < 0) {
			CardStatusTemp = 0;
			CardStatus = 0;
			return 1;
		}
	}

	CardStatusTemp = 1;
	CardStatus = 1;
	return 0;
}

/* Card_FileExists__F9EFileType (0x800144a8) */
/* Fuzzy demo twin Card_FileExists (0.69). Builds a one-block header for the save
   or replay file name, then opens the file for writing (mode 1) to create it. */
int Card_FileExists(EFileType FileType) {
	SCardHead Head;
	char *FName;
	int fd;

	if (FileType == 0)
		FName = CARD_FNAME;
	else
		FName = CARD_VNAME;

	Card_SetHeader(&Head, 1, (char *)&Head, (char *)&Head);

	if (GSaveAction == 1)
		Front_SetSJISName((char *)&Head + 32);
	else
		Front_SetSJISName((char *)&Head + 30);

	SetSJISChecksum(Head.Title, FName + 22);

	/* PC: saves are found by name, not by the title's 10-bit hash (pc_saves.h) */
	(void)fd;
	return PC_SaveExists(FileType == 0 ? PC_SAVE_CAREER : PC_SAVE_REPLAY, SaveGameName);

	if (SyncToGoodState() == 1)
		return 0;

	fd = open(FName, 1);

	if (fd == -1)
		return 0;

	close(fd);
	return 1;
}

/* Card_Write__F9EFileTypei (0x80014558) */
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
			Card_GetNextTimeStamp();

			GCareer.TimeStamp = TimeStamp;

			FName = CARD_FNAME;
			DataSize = 2560;
			pHeaderBuf = GVInfo1[0].pBlockStart;
			HeaderSize = 640;
			/* retail saves 2560 bytes from GCareer: SCareer and the start of the globals after
			   it (AnimNames: pointers, native in the port). The port saves zeros after SCareer;
			   loading reads back only SCareer. */
			{
				static ulong CareerBuf[2560 / 4];

				for (int i = 0; i < (int)(sizeof(SCareer) / 4); i++)
					CareerBuf[i] = ((ulong *)&GCareer)[i];

				pDataBuf = CareerBuf;
			}
			pHeaderBuf2 = 0;
			HeaderSize2 = 0;
			pDataBuf2 = 0;
			DataSize2 = 0;
			pClut = NoIcon;
			pIcon = NoIcon;
			break;
		case 1:
			FName = CARD_VNAME;
			pHeaderBuf = GVInfo1[0].pBlockStart;
			HeaderSize = 640;
			pDataBuf = pHeaderBuf + 160;
			DataSize = 23936;
			pClut = NoIcon;
			pIcon = NoIcon;
			pHeaderBuf2 = GVInfo1[1].pBlockStart;
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
		Front_SetSJISName((char *)pHeaderBuf + 32);
	else
		Front_SetSJISName((char *)pHeaderBuf + 30);

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

/* Card_WriteReplayToDisk__F9EFileTypePc (0x80014868) */
/* Debug twin of Card_Write: writes the replay header/data buffers (and the 2-player
   pair) to the PC host through the Psy-Q PCinit/PCcreat/PClseek/PCwrite/PCclose break calls,
   with the same switch(FileType) setup and checksums as Card_Write (only case 1 survives). */
int Card_WriteReplayToDisk(EFileType FileType, char *FName) {
	unsigned short fd;
	int DataSize;
	int DataSize2;
	int HeaderSize;
	ulong *pDataBuf;
	ulong *pDataBuf2;
	ulong *pHeaderBuf;
	ulong *pHeaderBuf2;

	switch (FileType) {
		case 1:
			pHeaderBuf = GVInfo1[0].pBlockStart;
			HeaderSize = 640;
			pDataBuf = pHeaderBuf + 160;
			DataSize = 23936;
			pHeaderBuf2 = GVInfo1[1].pBlockStart;
			pDataBuf2 = pHeaderBuf2 + 160;
			DataSize2 = 23936;
			break;
	}

	Video_SetHeader((SVideoHead *)(pHeaderBuf + 128), 0);

	if (pHeaderBuf2)
		Video_SetHeader((SVideoHead *)(pHeaderBuf2 + 128), 1);

	switch (FileType) {
		case 1:
			*(unsigned short *)((char *)pHeaderBuf + 566) = CalcChecksum(pDataBuf, DataSize);

			if (pHeaderBuf2)
				*(unsigned short *)((char *)pHeaderBuf2 + 566) = CalcChecksum(pDataBuf2, DataSize2);

			break;
	}

	/* PC: no development-kit host link (and nothing calls this); replays save through pc_saves.h */
	return 0;
	PCinit();

	fd = PCcreat(FName, 0);

	PClseek(fd, 0, 0);
	printf("\nWriting the header buffer, size %d", HeaderSize);
	PCwrite(fd, (char *)pHeaderBuf, HeaderSize);
	printf("\nWriting the data buffer, size %d", DataSize);
	PCwrite(fd, (char *)pDataBuf, DataSize);

	if (pHeaderBuf2) {
		printf("\nWriting the 2 player header buffer, size %d", 640);
		PCwrite(fd, (char *)pHeaderBuf2, 640);
		printf("\nWriting the 2 player data buffer, size %d", DataSize2);
		PCwrite(fd, (char *)pDataBuf2, DataSize2);
	}

	PCclose(fd);

	return 0;
}

/* Card_Load__F9EFileTypePUlT1 (0x800149f4) */
/* Demo twin. Reads a save/replay file from the memory card (two buffers for replays) and verifies the header checksums. The do{}while(0) makes the read-failure jumps loop exits (reorg predicts them not taken, filling their slots from the fall-through); every failure shares the sync-failure return. */
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

/* Card_LoadDemoReplay__FPCc (0x80014ba4) */
short Card_LoadDemoReplay(const char *name) {
	SVideoHead *h;
	char *b;
	int i;

	b = (char *)Mem_NewX(FileIO_Open(name), 1, 1);

	if (b) {
		FileIO_Load(b);
		FileIO_Sync();
	}

	h = (SVideoHead *)(b + 512);
	GVideoRestart.NumSkaters = h->NumSkaters;
	GVideoRestart.Skater = h->Skater;
	GVideoRestart.Skater2 = h->Skater2;
	GVideoRestart.Level = h->Level;
	GVideoRestart.GGame = h->GGame;
	GVInfo1[0].NumFrames = h->NumFrames;

	for (i = 0; i < 5; i++)
		GVInfo1[0].Highlights[i] = h->Highlights[i];

	Mem_Delete(b);
	return h->NumSkaters;
}

/* Card_LoadReplayFromCD__FPUlT0Pc (0x80014c74) */
int Card_LoadReplayFromCD(ulong *pBuf, ulong *pBuf2, char *name) {
	char *a = (char *)pBuf, *b = (char *)pBuf2;
	int c;
	char *p = (char *)Mem_NewX(FileIO_Open(name), 1, 1);

	if (p) {
		FileIO_Load(p);
		FileIO_Sync();
	}

	Mem_Copy(a, p, 0x6000);

	if (b)
		Mem_Copy(b, p + 0x6000, 0x6000);

	Mem_Delete(p);

	c = *(short *)(a + 566);

	if (c != CalcChecksum((ulong *)(a + 640), 23936))
		printf("\nChecksum error for 1P in Card_LoadReplayFromCD()");

	if (b) {
		c = *(short *)(b + 566);

		if (c != CalcChecksum((ulong *)(b + 640), 23936))
			printf("\nChecksum error for 2P in Card_LoadReplayFromCD()");
	}

	return 0;
}

/* FUN_80014d5c (0x80014d5c) */
void FUN_80014d5c(int pad) {
	DAT_800d137c = 1;
	/* PC: no memory card (saves are files, pc_saves.h), so no card driver or BIOS card events. */
	CardStatusMode = 0;
	CardLoadFlag = 0;
	return;
	FUN_8008e944(pad);
	FUN_8008e9d4();
	FUN_8008ea38();
	FUN_8008f0dc();

	ev0 = FUN_8008eab8(0xf4000001, 4, 0x2000, 0);
	ev1 = FUN_8008eab8(0xf4000001, 0x8000, 0x2000, 0);
	ev2 = FUN_8008eab8(0xf4000001, 0x100, 0x2000, 0);
	ev3 = FUN_8008eab8(0xf4000001, 0x2000, 0x2000, 0);
	ev10 = FUN_8008eab8(0xf0000011, 4, 0x2000, 0);
	ev11 = FUN_8008eab8(0xf0000011, 0x8000, 0x2000, 0);
	ev12 = FUN_8008eab8(0xf0000011, 0x100, 0x2000, 0);
	ev13 = FUN_8008eab8(0xf0000011, 0x2000, 0x2000, 0);
	FUN_8008eac8();
	FUN_8008ea48(ev0);
	FUN_8008ea48(ev1);
	FUN_8008ea48(ev2);
	FUN_8008ea48(ev3);
	FUN_8008ea48(ev10);
	FUN_8008ea48(ev11);
	FUN_8008ea48(ev12);
	FUN_8008ea48(ev13);

	CardStatusMode = 0;
	CardLoadFlag = 0;
}

/* _card_event__Fv (0x80014ee8) */
int _card_event(void) {
	int r = FUN_8008eb08(ev0) == 1;

	if (FUN_8008eb08(ev1) == 1)
		r = 2;

	if (FUN_8008eb08(ev2) == 1)
		r = 3;

	if (FUN_8008eb08(ev3) == 1)
		r = 4;

	if (EVENT_TIME_OUT++ >= 120)
		r = 2;

	return r;
}

/* _clear_event__Fv (0x80014f8c) */
void _clear_event(void) {
	FUN_8008eb08(ev0);
	FUN_8008eb08(ev1);
	FUN_8008eb08(ev2);
	FUN_8008eb08(ev3);
}

/* _card_event_x__Fv (0x80014fd4) */
int _card_event_x(void) {
	for (;;) {
		if (FUN_8008eb08(ev10) == 1)
			return 1;

		if (FUN_8008eb08(ev11) == 1)
			return 2;

		if (FUN_8008eb08(ev12) == 1)
			return 3;

		if (FUN_8008eb08(ev13) == 1)
			return 4;
	}
}

/* _clear_event_x__Fv (0x80015044) */
void _clear_event_x(void) {
	FUN_8008eb08(ev10);
	FUN_8008eb08(ev11);
	FUN_8008eb08(ev12);
	FUN_8008eb08(ev13);
}

/* Card_ReadFileHeader__FPcP9SCardHead (0x8001508c) */
int Card_ReadFileHeader(char *name, SCardHead *out) {
	char buf[32];
	int i, fd, n;

	/* PC: the header of a listed save is just its name (pc_saves.h) */
	PC_SaveCardHeader(out, PC_SaveListType(), name);
	return 0;

	sprintf(buf, "bu00:%s", name);

	for (i = 0; i < 8; i++) {
		fd = FUN_8008eb4c(buf, 1);

		if (fd == -1) {
			*(short *)out = 26214;
			Pause(1);
			Card_CheckStatus(0, 0);
		} else {
			_clear_event();

			n = FUN_8008ead8(fd, out, 512);
			FUN_8008e934(fd);

			if (n == 512)
				return 0;
		}
	}

	return 1;
}

/* Card_GetDirAndFreeBlocks__Fii (0x80015164) */
/* 68/69, parked. Retail (and the identical THPS2-demo twin) starts the first loop with
   `addu a0,s2,zero`: i copied from size's zero register. Ours is `move a0,$0`. S29 findings,
   checked against the egcs 1.1.2 source:
   - CSE1 folds `i = size` (and every chained-zero spelling) to const 0, since the costs tie.
   - reload_cse never swaps a CONST_INT for a register on MIPS: CONST_COSTS returns 0 for
     every CONST_INT (reload1.c:8748).
   - An empty asm forcing the copy hides i's start value from loop.c, which then adds a
     loop guard (+4 words).
   So the copy must come from an RTL shape that we haven't found. */
int Card_GetDirAndFreeBlocks(int cnct, int slot) {
	int i;
	int size = 0;
	char key[32];
	struct DIRENTRY *d;

	for (i = 0; i < 15; i++)
		FileType[i] = 3;

	sprintf(key, "bu%1d%1d:*", cnct, slot);

	d = (struct DIRENTRY *)SLOT_DIR[0];
	i = 0;

	if (firstfile(key, d) == d) {
		do {
			size += d->size;

			if (Utils_CompareStrings(d->name, CARD_FNAME + 5, 17))
				FileType[i] = 0;
			else if (Utils_CompareStrings(d->name, CARD_VNAME + 5, 17))
				FileType[i] = 1;

			i++;
			d = (struct DIRENTRY *)SLOT_DIR[i];
		} while (nextfile(d) == d);
	}

	size /= 8192;
	return 15 - size;
}

/* Card_CheckStatus__Fii (0x80015278) */
int Card_CheckStatus(int a, int b) {
	int s0 = a * 16 + b;
	int ev;

	/* PC: no memory card, the saves are files (pc_saves.h); always ready. */
	CardStatusMode = 0;
	CardLoadFlag = 1;
	CardStatusTemp = 1;
	CardStatus = 1;
	return CardStatus;

	switch (CardStatusMode) {
		case 0:
			FUN_8008e914(s0);

			CardStatusMode = 1;
			EVENT_TIME_OUT = 0;
			CardStatus = 0;
			break;
		case 1:
			ev = _card_event();

			if (ev == 0)
				break;

			switch (ev) {
				case 1:
					CardStatusTemp = 1;

					if (CardLoadFlag == 1)
						CardStatusMode = 4;
					else
						CardStatusMode = 2;

					break;
				case 2:
					CardStatusTemp = -3;
					CardStatusMode = 4;
					CardLoadFlag = 0;
					break;
				case 3:
					CardStatusTemp = -1;
					CardStatusMode = 4;
					CardLoadFlag = 0;
					break;
				case 4:
					GHasCareerLoaded = 0;
					CardStatusTemp = 2;
					_clear_event_x();
					FUN_8008ea68(s0);
					_card_event_x();

					CardStatusMode = 2;
					CardLoadFlag = 0;
					break;
				default:
					CardStatusTemp = -3;
					CardStatusMode = 4;
					CardLoadFlag = 0;
					break;
			}

			break;
		case 2:
			_clear_event();
			FUN_8008f0cc(s0);

			CardStatusMode = 3;
			EVENT_TIME_OUT = 0;
			break;
		case 3:
			ev = _card_event();

			if (ev == 0)
				break;

			CardStatusMode = 4;

			switch (ev) {
				case 1:
					CardLoadFlag = 1;
					break;
				case 2:
					CardStatusTemp = -3;
					CardLoadFlag = 0;
					break;
				case 3:
					CardStatusTemp = -1;
					CardLoadFlag = 0;
					break;
				case 4:
					CardStatusTemp = -2;
					CardLoadFlag = 0;
					break;
				default:
					CardStatusTemp = -3;
					CardLoadFlag = 0;
					break;
			}

			break;
		case 4:
			CardStatus = CardStatusTemp;
			CardStatusMode = 0;
			break;
	}

	return CardStatus;
}

/* Card_FormatCard__Fii (0x80015474) */
int Card_FormatCard(int a, int b) {
	char buf[8];
	int s, r;

	/* PC: the saves folder needs no formatting (pc_saves.h). */
	return 1;
	sprintf(buf, "bu%1d%1d:", a, b);
	_clear_event_x();

	s = FUN_8008eaa8(buf);
	r = _card_event_x();

	if (r == 1)
		r = 1;
	else if (r == 3)
		r = -1;
	else
		r = -3;

	if (s == 0 && r == 1)
		r = -3;

	return r;
}

/* FUN_800154f4 (0x800154f4) */
int FUN_800154f4(void) {
	SCareer buf;
	int i, r, s2, s4;
	char *p;
	SCareer *d;

	r = 0;
	Career_Init();

	if (SyncToGoodState() == 0) {
		/* PC: the careers in the Saves folder (pc_saves.h), with no "memory card" messages */
		PC_SaveListDir(PC_SAVE_CAREER, SLOT_DIR, (int32_t *)FileType);

		s4 = -1;
		s2 = 0;
		i = 0;

		do {
			if (FileType[i] == 0) {
				p = (char *)Mem_NewX(3200, 1, 1);
				Card_SetSelectedFile(SLOT_DIR[i]);

				if (Card_Load(THPS_SAVE_FILE, (ulong *)p, 0) == 0) {

					GHasCareerLoaded = 1;
					GLoadedCareerIsSaved = 0;
					r = 1;
					buf = *(SCareer *)(p + 640);
					d = &GCareer;

					if (buf.TimeStamp >= d->TimeStamp) {
						*d = buf;
						s4 = i;
						Front_ExtractASCIINameFromSJIS(p + 32, StoredCareerName);
					}
				}

				Mem_Delete(p);
			}

			i++;
		} while (i < 15);

		if (s4 >= 0) {
			Card_SetSelectedFile(SLOT_DIR[s4]);
			Options_GetPreferences();
			Options_SetPreferences();
		}

		return r;
	}

	return 0;
}

/* Card_GetNextTimeStamp__Fv (0x8001571c) */
void Card_GetNextTimeStamp(void) {
	/* TimeStamp as a word here */
	int &TimeStampWord = *(int *)&TimeStamp;
	SCareer buf;
	int i;
	char *p;
	char *name;
	int *flag;

	TimeStampWord = 0;

	if (SyncToGoodState() == 0) {
		i = 0;
		name = SLOT_DIR[0];
		flag = (int *)FileType;

		do {
			if (*flag == 0) {
				p = (char *)Mem_NewX(3200, 1, 1);
				Card_SetSelectedFile(name);

				if (Card_Load(THPS_SAVE_FILE, (ulong *)p, 0) == 0) {
					buf = *(SCareer *)(p + 640);

					if (buf.TimeStamp >= TimeStampWord)
						TimeStampWord = buf.TimeStamp + 1;
				}

				Mem_Delete(p);
			}

			name += 40;
			i++;
			flag++;
		} while (i < 15);
	}
}
