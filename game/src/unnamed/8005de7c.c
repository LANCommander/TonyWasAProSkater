/* Front_MainMenu__Fi (0x8005de7c) */
#include "game/classsizes.h"
#include "names.h"

struct VT {
	short off0, i0; /* entry 0 (PsxVtEntry): zeros */
	void *fn0;
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

extern unsigned char GCareer[];
#define CharacterProgress ((R *)(GCareer + 8)) /* GCareer.CharacterProgress */

extern int DAT_800d1d24;

extern struct O *DAT_800d2358;
extern struct O *DAT_800d235c;

extern void Pack_DefragmentFully(void);
extern void FrontLoader_LoadSet(int);
extern void FrontLoader_Defragment(void);
extern char *FrontLoader_TakeFile(char *);
extern void DownloadBMP16x2(char *);
extern void Mess_LoadFont__FPcT0iii(char *, char *, int, int, int);
extern void Mess_SetFontScale(unsigned);
extern struct O *__nw__6CClassUi(unsigned);
extern struct O *__6SpritePciii(struct O *, char *, int, int, int);
extern void FrontLoader_SetFiles(int);
extern int FUN_8005e754(int);

int Front_MainMenu(int a) {
	int i, j, r;
	struct O *o, *p, *q;

	Pack_DefragmentFully();
	FrontLoader_LoadSet(0);
	FrontLoader_Defragment();

	if (DAT_800d1d24) {
		for (i = 0; i < 11; i++) {
			CharacterProgress[i].a = 0;
			CharacterProgress[i].d = 0;
			CharacterProgress[i].c = 0;
			CharacterProgress[i].b = 0;

			for (j = 0; j < 10; j++) {
				CharacterProgress[i].e[j] = 255;
				CharacterProgress[i].a += 5;
				CharacterProgress[i].d += 1;
				CharacterProgress[i].c += 1;
				CharacterProgress[i].b += 1;
			}
		}
	}

	DownloadBMP16x2(FrontLoader_TakeFile("main_h.bmp"));
	Mess_LoadFont__FPcT0iii("mainf.fnt", FrontLoader_TakeFile("mainf.fnt"), -1, -1, -1);
	Mess_SetFontScale(200);

	o = __nw__6CClassUi(SIZEOF_SPRITE);

	if (o)
		o = __6SpritePciii(o, "levbut2.bmp", 1, 32, 128);

	DAT_800d2358 = o;
	p = __nw__6CClassUi(SIZEOF_SPRITE);

	if (p)
		p = __6SpritePciii(p, "levbut1.bmp", 1, 32, 128);

	DAT_800d235c = p;
	FrontLoader_SetFiles(1);

	r = FUN_8005e754(a);
	q = DAT_800d2358;

	if (q)
		q->vt->fn((char *)q + q->vt->off, 3);

	q = DAT_800d235c;

	if (q)
		q->vt->fn((char *)q + q->vt->off, 3);

	return r;
}
