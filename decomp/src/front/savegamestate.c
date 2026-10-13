/* Front_SaveGameState__Fv (0x8001cbf0) */
#include "names.h"

typedef struct {
	int x, y, z;
} CVector;

typedef struct {
	int checksum;
	char name[9];
	char level[53];
	char x42;
	char pad[33];
	int x64;
	char pad2[8];
	char x70[1];
} SSaveGame;

extern char *TrigFileName;
extern int RestartNode;
extern char *MechList;
extern SSaveGame LastGameState;

char *Trig_GetPosition(CVector *, int);
void Front_SavePreferences(char *);
unsigned Front_CalculateGameChecksum(SSaveGame *);

void Front_SaveGameState(void) {
	CVector v;
	char *s = TrigFileName, *d = LastGameState.name;

	while (*s)
		*d++ = *s++;

	*d = 0;
	s = Trig_GetPosition(&v, RestartNode) + 6;
	d = LastGameState.level;

	while (*s)
		*d++ = *s++;

	*d = 0;

	if (MechList) {
		LastGameState.x64 = *(short *)(MechList + 0xe6);
		LastGameState.x42 = 0;
	}

	Front_SavePreferences(LastGameState.x70);

	LastGameState.checksum = Front_CalculateGameChecksum(&LastGameState);
}
