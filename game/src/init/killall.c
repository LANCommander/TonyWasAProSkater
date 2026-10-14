/* Init_KillAll__Fv (0x80071b7c) */
#include "names.h"

extern void *MechList, *CameraList, *BaddyList, *EnvironmentalObjectList, *BackgroundList,
    *PowerUpList, *SuspendedList;

void DeleteList(void *);
void TrickOb_DeleteAll(void);
void Rail_DeleteAll(void);
void Bit_DeleteAll(void);
void Mess_DeleteAll(void);

void Init_KillAll(void) {
	DeleteList(MechList);
	DeleteList(CameraList);
	DeleteList(BaddyList);
	DeleteList(EnvironmentalObjectList);
	DeleteList(BackgroundList);
	DeleteList(PowerUpList);
	DeleteList(SuspendedList);
	TrickOb_DeleteAll();
	Rail_DeleteAll();
	Bit_DeleteAll();
	Mess_DeleteAll();
}
