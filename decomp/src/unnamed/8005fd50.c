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

void FUN_8005c734(int);
void FUN_8005c97c(int);
void FUN_8005ca64(void);
int FUN_8005ca3c(char *);
void FUN_80061714(int);
O *__nw__6CClassUi(int);
O *__6SpritePciii(O *, char *, int, int, int);
void FUN_80023168(int);
void FUN_80022aec(V *);
void GameFMV_StartFMV(int);
void GameFMV_StopFMV(void);
int FUN_8005f9ec(void);
int FUN_8005e0e0(V *, V *, int, int, int *, int (*)(void), V *, V *, int);

int FUN_8005fd50(int a) {
	V z, t, k, c, d;
	O *o;
	O *o2;
	int v;

	FUN_8005c734(-1);
	FUN_8005c97c(12);
	FUN_8005ca64();
	FUN_80061714(FUN_8005ca3c("opt_snd.bmp"));

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
	FUN_80023168(2);
	FUN_80022aec(&z);

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
	v = FUN_8005e0e0(&t, &k, 3, 1312, &a, FUN_8005f9ec, &c, &d, 0);

	if (v == -1 || v == 1315) {
		v = -1;
		GameFMV_StopFMV();
		FUN_8005c97c(11);
		FUN_8005ca64();
		FUN_80061714(FUN_8005ca3c("options.bmp"));

		XALEVEL = GPreferences_4[0] * 1365;
	}

	if (DAT_800d1d60)
		DAT_800d1d60->vt->fn((char *)DAT_800d1d60 + DAT_800d1d60->vt->off, 3);

	DAT_800d1d60 = 0;

	if (DAT_800d1d5c)
		DAT_800d1d5c->vt->fn((char *)DAT_800d1d5c + DAT_800d1d5c->vt->off, 3);

	DAT_800d1d5c = 0;
	FUN_8005c734(157);
	return v;
}
