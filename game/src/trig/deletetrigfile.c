/* Trig_DeleteTrigFile__Fv (0x80044ea8) */
#include "names.h"

void *TrigFile;
short **OffsetList;
int NumCheatRestarts, NumHorseRestarts;

void Mem_Delete(void *p);

void Trig_DeleteTrigFile(void) {
	if (TrigFile) {
		Mem_Delete(TrigFile);
		Mem_Delete(OffsetList); /* (the port's node array, Trig_InitialParseTRGFile) */

		TrigFile = 0;
		OffsetList = 0;
	}

	NumCheatRestarts = 0;
	NumHorseRestarts = 0;
}
