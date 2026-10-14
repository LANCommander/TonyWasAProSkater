/* Trig_DeleteTrigFile__Fv (0x80044ea8) */
#include "names.h"

void *TrigFile;
int NumCheatRestarts, NumHorseRestarts;

void Mem_Delete(void *p);

void Trig_DeleteTrigFile(void) {
	if (TrigFile) {
		Mem_Delete(TrigFile);

		TrigFile = 0;
	}

	NumCheatRestarts = 0;
	NumHorseRestarts = 0;
}
