/* Front_SaveGameState__Fv (0x8001cbf0) */
#include "game/card.h"
#include "game/bruce.h"

extern char *TrigFileName;
extern int RestartNode;
extern CBruce *MechList;
extern SSaveGame LastGameState;

ushort *Trig_GetPosition(CVector &, int);
void Front_SavePreferences(SPreferences &);
uint Front_CalculateGameChecksum(SSaveGame &);

void Front_SaveGameState(void) {
	CVector v;
	char *s = TrigFileName, *d = LastGameState.State.TrigFileName;

	while (*s)
		*d++ = *s++;

	*d = 0;
	s = (char *)Trig_GetPosition(v, RestartNode) + 6;
	d = LastGameState.State.RestartName;

	while (*s)
		*d++ = *s++;

	*d = 0;

	if (MechList) {
		LastGameState.State.Health = MechList->mHealth;
		LastGameState.State.WeaponsGot = 0;
	}

	Front_SavePreferences(LastGameState.Preferences);

	LastGameState.Checksum = Front_CalculateGameChecksum(LastGameState);
}
