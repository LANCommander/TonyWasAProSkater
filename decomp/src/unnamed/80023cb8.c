/* strInit__Fv (0x80023cb8) */
#include "names.h"

int pRingBuffer;
int GameFMV_RingBufferSectors;
unsigned char GameFMV_Is24Bit;

extern int Redbook_XACheckSectorOnVSync;
extern int DAT_800d169c;
extern int Redbook_XAModeSet;

void FUN_80023d3c(void);
void FUN_8009028c(int a);
void FUN_8009035c(void (*f)(void));
void FUN_800911cc(int a, int b);
void FUN_800907cc(int a, int b, int c, int d, int e);
void strKickCD(void);
void FUN_8008f3a0(int a, int b);

void strInit(void) {
	FUN_8009028c(0);
	FUN_8009035c(FUN_80023d3c);
	FUN_800911cc(pRingBuffer, GameFMV_RingBufferSectors);
	FUN_800907cc(GameFMV_Is24Bit, 0, -1, 0, 0);

	Redbook_XACheckSectorOnVSync = 0;
	DAT_800d169c = 1;
	Redbook_XAModeSet = 0;
	strKickCD();
	FUN_8008f3a0(12, 0);
}
