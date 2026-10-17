/* INIT.CPP: game initialisation and cleanup: deleting the object lists, resetting memory,
   restarting a level. Declarations in game/init.h. */
#include "game/init.h"
#include "game/ob.h"
#include "pc_frame.h"
#include "pc_screen.h"
#include "pc_texstore.h"
#include "pc_draw.h"
#include "game/mem.h"
#include "game/backgrnd.h"
#include "game/baddy.h"
#include "game/bit.h"
#include "game/campt.h"
#include "game/card.h"
#include "game/db.h"
#include "game/exp.h"
#include "game/fileio.h"
#include "game/flash.h"
#include "game/font.h"
#include "game/front.h"
#include "game/gamefmv.h"
#include "game/globals.h"
#include "game/m3d.h"
#include "game/m3dasm.h"
#include "game/m3dutils.h"
#include "game/m3dzone.h"
#include "game/main.h"
#include "game/mech.h"
#include "game/mess.h"
#include "game/pack.h"
#include "game/pad.h"
#include "game/pal.h"
#include "game/physics.h"
#include "game/rail.h"
#include "game/sfx.h"
#include "game/shell.h"
#include "game/shellutils.h"
#include "game/spool.h"
#include "game/trickob.h"
#include "game/trig.h"
#include "game/utils.h"
#include "game/video.h"
#include "pc_runtime.h"

void DeleteList(void *);
typedef struct {
	int x, y, z;
} V;
void Init_KillAll(void);
typedef struct {
	short x, y, w, h;
} R;
extern char Vertices[];
extern char *RotatedVertices;
extern R DAT_800d1f8c;
void FUN_80047870(void);
int FUN_80096b84(void);
void FUN_800971a4(void);
void FUN_8008c1a0(int a);
void FUN_8008c314(int a);
void ClearImage(R *r, int a, int b, int c);
void FUN_80096c14(void (*a)(void));
void FUN_8008c3d0(int a);
extern void *pCompressedBMR;
extern int TitleTimeout;
void FUN_80071d60(void);

/* DeleteList__FP5CBody (0x80071b00) */
/* C++: deletes every body on a list (virtual deleting destructor) except the ones with
   CBODY_KEEP (which nothing sets) */
void DeleteList(CBody *p) {
	CBody *n;

	if (!p)
		return;

	n = (CBody *)p->mNextItem;

	for (;;) {
		if (!(p->mCBodyFlags & CBODY_KEEP))
			delete p;

		p = n;

		if (n)
			n = (CBody *)n->mNextItem;
		else
			break;
	}
}

/* Init_KillAll__Fv (0x80071b7c) */
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

static inline void VI(V *p, int x, int y, int z) {
	p->x = x;
	p->y = y;
	p->z = z;
}

/* Init_Cleanup__Fi (0x80071c24) */
void Init_Cleanup(int a) {
	V v;

	FUN_8004b8e0();
	FUN_8004b8e8();
	FUN_8004ba34();

	ApocalypseViewport.Zoom = 256;
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

		LastGameState.State.RestartName[0] = 0;
	} else {
		Spool_ClearEnvironmentRegions();
	}

	Utils_InitialRand(0x12a6cc58);
	Trig_ClearTrigMenu();
	FUN_800827ac();
	Flash_Reset();
	FUN_8004ee24();
	FUN_8004f5d0();
	VI(&v, 0, 0x8000, 0);
	SetGravity((CVector *)&v);

	TTime = 0;
	Pad_ClearAll();

	SuspendDistance = 13000;
	CurrentWaterDamage = 0;
	CurrentWaterLevel = 0x7fffffff;
	CurrentWaterLevel2 = 0x7fffffff;
}

/* FUN_80071d60 (0x80071d60) */
void FUN_80071d60(void) {
	R r;

	ProjectedVertices = (SProjectedVertex *)Vertices;
	RotatedVertices = Vertices + 3200;
	FUN_80014d5c(1);
	Pad_InitAtStart();
	FUN_800971a4();
	PC_DrawReset();
	PC_ScreenReset();
	FUN_800491a0();

	r = DAT_800d1f8c;
	/* all of the PS1's VRAM: the screens and the textures */
	PC_ScreenFill(r.x, r.y, r.w, r.h, 0);
	PC_TexFill(r.x, r.y, r.w, r.h, 0);
	PC_SetFrameHandler(FUN_80047870); /* MyVSync: once a frame (pc_frame.h) */
	PC_ScreenEnable(1);
	FUN_800785cc(0x9f6ad);
	SFX_InitAtStart();
	FileIO_Init();
	FUN_80022b4c();
}

/* Init_ResetMemory__Fi (0x80071e40) */
/* The game's two heaps over the memory the port gives it (PC_GameHeap): the small one (a bytes)
   from the bottom, the big one after it up to 8 KB short of the end, as retail had them between
   .bss and its stack. */
void Init_ResetMemory(int a) {
	uint32_t size;
	char *v = (char *)PC_GameHeap(&size);

	DAT_800be09c[1].End = v + size - 0x2000;
	DAT_800be09c[0].Start = v;
	v += a;
	DAT_800be09c[0].End = v;
	DAT_800be09c[1].Start = v;
	FUN_8004cf7c();
}

/* Init_Restart__F13EPackInitTypei (0x80071e88) */
void Init_Restart(int type, int b) {
	pBMR = 0;
	pCompressedBMR = 0;
	Init_Cleanup(0);
	Init_ResetMemory(b);
	Db_Reset();
	Spool_Init();

	TitleTimeout = 20;
	Flash_Reset();

	if (type == 1)
		Pack_Init640x480();
	else
		Pack_Init();

	SFX_InitAtStart();
	Pal_Init();
	Font_Reset();
	Pad_LockAnalog();
}

/* Init_ForGame__Fv (0x80071f3c) */
void Init_ForGame(void) {
	Db_Init();
	Front_Init();
	Video_InitOnce();
	FUN_8004e1d4();
	FUN_8006952c();
	FUN_80082054();
}

/* FUN_80071f84 (0x80071f84) */
void FUN_80071f84(void) {
	FUN_80071d60();
}
