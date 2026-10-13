/* strKickCD__Fv (0x80023f44) */
#ifdef PC_PORT
#include "pc_stream.h"
#endif
int GameFMV_StartSector;

unsigned char *FUN_800912bc(int, unsigned char *);
int CdControl(int, unsigned char *, unsigned char *);
int CdRead2(int);

void strKickCD(void) {
	unsigned char loc[4];
	unsigned char param[4];

#ifdef PC_PORT
	/* PC: GameFMV_StartSector is the file's stream (pc_stream.h) */
	PC_StreamPlay(GameFMV_StartSector);
	return;
#endif
	param[0] = 0x80;
	FUN_800912bc(GameFMV_StartSector, loc);

	do {
		while (!CdControl(2, loc, 0))
			;

		while (!CdControl(14, param, 0))
			;
	} while (!CdRead2(480));
}
