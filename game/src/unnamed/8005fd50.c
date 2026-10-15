/* FUN_8005fd50 (0x8005fd50) */
#include "game/classsizes.h"
#include "names.h"

typedef struct {
	int x, y, z, pad;
} V;

typedef struct {
	char *s[3];
} L3;

struct VT {
	short off0, i0; /* entry 0 (PsxVtEntry): zeros */
	void *fn0;
	short off;
	short p2;
	void (*fn)(char *, int);
};

typedef struct {
	struct VT *vt;
} O;

struct VerticalMenu;

extern L3 DAT_800a1d7c;
extern int GPreferences[];
extern int XALEVEL;
extern short GameFMV_StartX;

extern O *DAT_800d1d5c;
extern O *DAT_800d1d60;

static inline void VI(V *p, int x, int y, int z) {
	p->x = x;
	p->y = y;
	p->z = z;
}

void Front_SetMusicLoop(int);
void FrontLoader_LoadSet(int);
void FrontLoader_Defragment(void);
char *FrontLoader_TakeFile(char *);
void DownloadBMP16x2(char *);
O *__nw__6CClassUi(int);
O *__6SpritePciii(O *, char *, int, int, int);
void GameFMV_SetStartTrack(int);
void GameFMV_AddSoundSource(V *);
void GameFMV_StartFMV(int);
void GameFMV_StopFMV(void);
void Front_SoundOptionsHandler(struct VerticalMenu *);
int Front_RunMenu(char **, int *, int, int, int *, void (*)(struct VerticalMenu *), int *, int *, int);

int FUN_8005fd50(int a) {
	V z;
	char *t[4];
	V k, c, d;
	O *o;
	O *o2;
	int v;

	Front_SetMusicLoop(-1);
	FrontLoader_LoadSet(12);
	FrontLoader_Defragment();
	DownloadBMP16x2(FrontLoader_TakeFile("opt_snd.bmp"));

	o = __nw__6CClassUi(SIZEOF_SPRITE);

	if (o)
		o = __6SpritePciii(o, "opt_dotr.bmp", 1, 256, 256);

	DAT_800d1d5c = o;
	o2 = __nw__6CClassUi(SIZEOF_SPRITE);

	if (o2)
		o2 = __6SpritePciii(o2, "opt_dotg.bmp", 1, 256, 256);

	DAT_800d1d60 = o2;
	XALEVEL = 0;
	VI(&z, 0, 0, 0);
	GameFMV_SetStartTrack(2);
	GameFMV_AddSoundSource(&z);

	GameFMV_StartX = -1;
	GameFMV_StartFMV(0);

	*(L3 *)t = DAT_800a1d7c;
	k.x = 0;
	k.y = 0;
	k.z = 0;
	c.x = 350;
	c.y = 350;
	c.z = 350;
	d.x = 250;
	d.y = 330;
	d.z = 370;
	v = Front_RunMenu(t, &k.x, 3, 1312, &a, Front_SoundOptionsHandler, &c.x, &d.x, 0);

	if (v == -1 || v == 1315) {
		v = -1;
		GameFMV_StopFMV();
		FrontLoader_LoadSet(11);
		FrontLoader_Defragment();
		DownloadBMP16x2(FrontLoader_TakeFile("options.bmp"));

		XALEVEL = GPreferences[1] * 1365;
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
