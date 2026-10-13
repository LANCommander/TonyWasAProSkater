/* RestartCompFrig__Fv (0x8001a6a8) */
#include "names.h"

extern int GLevel;
extern int RestartNode;

void Trig_SetRestart(char *);
void Trig_ExecuteRestart(void);

void RestartCompFrig(void) {
	if (GLevel == 8) {
		int s = RestartNode;

		Trig_SetRestart("RE_START_FRIG");
		Trig_ExecuteRestart();

		RestartNode = s;
	}
}
