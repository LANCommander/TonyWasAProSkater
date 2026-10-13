/* FUN_8005fd50 (0x8005fd50) */
#include "names.h"

typedef struct {
	int x, y, z, pad;
} V;

typedef struct {
	int x, y, z;
} V12;

struct VT {
	char p[8];
	short off;
	short p2;
	void (*fn)(char *, int);
};

typedef struct {
	struct VT *vt;
} O;

extern V DAT_800a1d7c;
extern int GPreferences_4[];
extern int XALEVEL;
extern short GameFMV_StartX;

O *DAT_800d1d5c;
O *DAT_800d1d60;

static inline void VI(V *p, int x, int y, int z) {
	p->x = x;
	p->y = y;
	p->z = z;
}

void Front_SetMusicLoop(int);
void FrontLoader_LoadSet(int);
void FrontLoader_Defragment(void);
int FrontLoader_TakeFile(char *);
void DownloadBMP16x2(int);
O *__nw__6CClassUi(int);
O *__6SpritePciii(O *, char *, int, int, int);
void GameFMV_SetStartTrack(int);
void GameFMV_AddSoundSource(V *);
void GameFMV_StartFMV(int);
void GameFMV_StopFMV(void);
int Front_SoundOptionsHandler(void);
int Front_RunMenu(V *, V *, int, int, int *, int (*)(void), V *, V *, int);

int FUN_8005fd50(int a) {
	V z, t, k, c, d;
	O *o;
	O *o2;
	int v;

	Front_SetMusicLoop(-1);
	FrontLoader_LoadSet(12);
	FrontLoader_Defragment();
	DownloadBMP16x2(FrontLoader_TakeFile("opt_snd.bmp"));

	o = __nw__6CClassUi(24);

	if (o)
		o = __6SpritePciii(o, "opt_dotr.bmp", 1, 256, 256);

	DAT_800d1d5c = o;
	o2 = __nw__6CClassUi(24);

	if (o2)
		o2 = __6SpritePciii(o2, "opt_dotg.bmp", 1, 256, 256);

	DAT_800d1d60 = o2;
	XALEVEL = 0;
	VI(&z, 0, 0, 0);
	GameFMV_SetStartTrack(2);
	GameFMV_AddSoundSource(&z);

	GameFMV_StartX = -1;
	GameFMV_StartFMV(0);

	*(V12 *)&t = *(V12 *)&DAT_800a1d7c;
	k.x = 0;
	k.y = 0;
	k.z = 0;
	c.x = 350;
	c.y = 350;
	c.z = 350;
	d.x = 250;
	d.y = 330;
	d.z = 370;
	v = Front_RunMenu(&t, &k, 3, 1312, &a, Front_SoundOptionsHandler, &c, &d, 0);

	if (v == -1 || v == 1315) {
		v = -1;
		GameFMV_StopFMV();
		FrontLoader_LoadSet(11);
		FrontLoader_Defragment();
		DownloadBMP16x2(FrontLoader_TakeFile("options.bmp"));

		XALEVEL = GPreferences_4[0] * 1365;
	}

	if (DAT_800d1d60)
		DAT_800d1d60->vt->fn((char *)DAT_800d1d60 + DAT_800d1d60->vt->off, 3);

	DAT_800d1d60 = 0;

	if (DAT_800d1d5c)
		DAT_800d1d5c->vt->fn((char *)DAT_800d1d5c + DAT_800d1d5c->vt->off, 3);

	DAT_800d1d5c = 0;
	Front_SetMusicLoop(157);
	return v;
}
