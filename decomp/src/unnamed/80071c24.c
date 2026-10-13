/* Init_Cleanup__Fi (0x80071c24) */
#include "names.h"

typedef struct {
	int x, y, z;
} V;

extern short DAT_800a679e[];
extern char LastGameState_d[];
extern int TTime, CurrentWaterDamage, SuspendDistance;
extern int CurrentWaterLevel, CurrentWaterLevel2;

void FUN_8004b8e0(void);
void FUN_8004b8e8(void);
void FUN_8004ba34(void);
void GameFMV_StopFMV(void);
void Init_KillAll(void);
void FUN_800448b8(void);
void FUN_8004e1e0(void);
void FUN_8004bb5c(void);
void FUN_8004b180(void);
void SFX_ReverbOff(void);
void Spool_ClearAllPSXs(void);
void Trig_RemoveDualBuffer(void);
void Trig_DeleteTrigFile(void);
void Pack_DefragmentFully(void);
void Spool_ClearEnvironmentRegions(void);
void Redbook_InitialRand(int);
void Trig_ClearTrigMenu(void);
void FUN_800827ac(void);
void Flash_Reset(void);
void FUN_8004ee24(void);
void FUN_8004f5d0(void);
void SetGravity(V *);
void Pad_ClearAll(void);

static inline void VI(V *p, int x, int y, int z) {
	p->x = x;
	p->y = y;
	p->z = z;
}

void Init_Cleanup(int a) {
	V v;

	FUN_8004b8e0();
	FUN_8004b8e8();
	FUN_8004ba34();

	DAT_800a679e[0] = 256;
	GameFMV_StopFMV();
	Init_KillAll();
	FUN_800448b8();
	FUN_8004e1e0();

	if (a == 0) {
		FUN_8004bb5c();
		FUN_8004b180();
		SFX_ReverbOff();
		Spool_ClearAllPSXs();
		Trig_RemoveDualBuffer();
		Trig_DeleteTrigFile();
		Pack_DefragmentFully();

		LastGameState_d[0] = 0;
	} else {
		Spool_ClearEnvironmentRegions();
	}

	Redbook_InitialRand(0x12a6cc58);
	Trig_ClearTrigMenu();
	FUN_800827ac();
	Flash_Reset();
	FUN_8004ee24();
	FUN_8004f5d0();
	VI(&v, 0, 0x8000, 0);
	SetGravity(&v);

	TTime = 0;
	Pad_ClearAll();

	SuspendDistance = 13000;
	CurrentWaterDamage = 0;
	CurrentWaterLevel = 0x7fffffff;
	CurrentWaterLevel2 = 0x7fffffff;
}
