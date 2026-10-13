/* FUN_8005de7c (0x8005de7c) */
#include "names.h"

struct VT {
	char pad[8];
	short off;
	short pad2;
	void (*fn)(char *, int);
};

struct O {
	struct VT *vt;
};

typedef struct {
	char a, b, c, d, e[10];
} R;

extern R GCareer_8[];

int DAT_800d1d24;

extern char s_main_h_bmp[], s_mainf_fnt_800a1be0[], s_levbut2_bmp[], s_levbut1_bmp[];

struct O *DAT_800d2358;
struct O *DAT_800d235c;

extern void Pack_DefragmentFully(void);
extern void FUN_8005c97c(int);
extern void FUN_8005ca64(void);
extern int FUN_8005ca3c(char *);
extern void FUN_80061714(int);
extern void Mess_LoadFont__FPcT0iii(char *, int, int, int, int);
extern void Mess_SetFontScale(unsigned);
extern struct O *__nw__6CClassUi(unsigned);
extern struct O *__6SpritePciii(struct O *, char *, int, int, int);
extern void FUN_8005c8dc(int);
extern int FUN_8005e754(int);

int FUN_8005de7c(int a) {
	int i, j, r;
	struct O *o, *p, *q;

	Pack_DefragmentFully();
	FUN_8005c97c(0);
	FUN_8005ca64();

	if (DAT_800d1d24) {
		for (i = 0; i < 11; i++) {
			GCareer_8[i].a = 0;
			GCareer_8[i].d = 0;
			GCareer_8[i].c = 0;
			GCareer_8[i].b = 0;

			for (j = 0; j < 10; j++) {
				GCareer_8[i].e[j] = 255;
				GCareer_8[i].a += 5;
				GCareer_8[i].d += 1;
				GCareer_8[i].c += 1;
				GCareer_8[i].b += 1;
			}
		}
	}

	FUN_80061714(FUN_8005ca3c(s_main_h_bmp));
	Mess_LoadFont__FPcT0iii(s_mainf_fnt_800a1be0, FUN_8005ca3c(s_mainf_fnt_800a1be0), -1, -1, -1);
	Mess_SetFontScale(200);

	o = __nw__6CClassUi(24);

	if (o)
		o = __6SpritePciii(o, s_levbut2_bmp, 1, 32, 128);

	DAT_800d2358 = o;
	p = __nw__6CClassUi(24);

	if (p)
		p = __6SpritePciii(p, s_levbut1_bmp, 1, 32, 128);

	DAT_800d235c = p;
	FUN_8005c8dc(1);

	r = FUN_8005e754(a);
	q = DAT_800d2358;

	if (q)
		q->vt->fn((char *)q + q->vt->off, 3);

	q = DAT_800d235c;

	if (q)
		q->vt->fn((char *)q + q->vt->off, 3);

	return r;
}
