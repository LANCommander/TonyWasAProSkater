/* FUN_800144a8 (0x800144a8) */
/* C++ (CARD.CPP). Fuzzy demo twin Card_FileExists (0.69). Builds a one-block header for the save
   or replay file name, then opens the file for writing (mode 1) to create it. */
#include "game/card.h"

extern char CARD_FNAME[];
extern char CARD_VNAME[];
extern int GSaveAction;

int SyncToGoodState(void);
void Card_SetHeader(SCardHead *, int, char *, char *);
void SetSJISChecksum(char *, char *);

extern "C" {
	void Front_SetSJISName__FPc(char *);
	int open(char *, int);
	int close(int);
}

int FUN_800144a8(int FileType) {
	SCardHead Head;
	char *FName;
	int fd;

	if (FileType == 0)
		FName = CARD_FNAME;
	else
		FName = CARD_VNAME;

	Card_SetHeader(&Head, 1, (char *)&Head, (char *)&Head);

	if (GSaveAction == 1)
		Front_SetSJISName__FPc((char *)&Head + 32);
	else
		Front_SetSJISName__FPc((char *)&Head + 30);

	SetSJISChecksum(Head.Title, FName + 22);

	if (SyncToGoodState() == 1)
		return 0;

	fd = open(FName, 1);

	if (fd == -1)
		return 0;

	close(fd);
	return 1;
}
