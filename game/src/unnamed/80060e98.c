/* Front_TitleScreen__Fv (0x80060e98) */
#include "game/classsizes.h"
#include "names.h"
#include "game/padbuttons.h"

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

struct C30 {
	char c[30];
};

struct C28 {
	char c[28];
};

struct X {
	char *pad[0x38 / 4];
	char *s;
};

extern struct C30 DAT_800a1fd0;
extern struct C28 DAT_800a1ff0;
extern struct X GameText;
extern char Pad[];
extern int DemoTimer;
extern int DAT_800d1cb4;
extern int DAT_800d1cb8;
extern int DAT_800d1d80;

extern struct O *DAT_800d1d88;

extern short DAT_800d1d8c;
extern short DAT_800d1d8e;
extern short DAT_800d1d90;
extern short DAT_800d1d92;

void Pack_DefragmentFully(void);
void Mess_LoadFont__FPciii(char *, int, int, int);
void Mess_SetFontScale(unsigned);
int FileIO_Open(char *);
void FileIO_Load(void *);
void FileIO_Sync(void);
char *Mem_NewX(int, int, int);
void DownloadBMP16x2(char *);
void Pad_ClearTriggers(char *);
char *__nw__6CClassUi(unsigned);
char *__6Spriteiiiiiii(char *, int, int, int, int, int, int, int);
void DrawSync(int);
int FUN_800154f4(int);
void FrontLoader_SetFiles(int);
void Front_UpdateMusicLoop(void);
void HiRes_ClearOTag(void);
void ResetOT(void);
void FrontLoader_Continue(void);
void Mess_SetRGB(int, int, int, int);
void Mess_SetTextJustify(int);
void Sprite_draw(struct O *, int, int, int, int, int, int);
void Mess_SetFontShadowOffset(int);
void Mess_DrawText(int, int, char *, int, int, int, int);
void Pad_Update(void);
void Front_CheckSoftReset(void);
void Mess_Update(void);
void Mess_Display(void);
void HiRes_DrawOTag(void);
void SFX_PlayX(int, int, int);

void Front_TitleScreen(void) {
	char *v;
	int w;
	int s0;
	char *p;

	Pack_DefragmentFully();
	Mess_LoadFont__FPciii("Start_1.fnt", -1, -1, -1);
	Mess_SetFontScale(256);

	v = Mem_NewX(FileIO_Open("title_h.bmp"), 1, 1);

	if (v) {
		FileIO_Load(v);
		FileIO_Sync();
	}

	DownloadBMP16x2(v);
	Pad_ClearTriggers(Pad);
	Pad_ClearTriggers(Pad + PAD_SIZE);

	DAT_800d1d8c = 0;
	DAT_800d1d8e = 293;
	DAT_800d1d90 = 640;
	DAT_800d1d92 = 128;
	p = __nw__6CClassUi(SIZEOF_SPRITE);

	if (p)
		p = __6Spriteiiiiiii(p, DAT_800d1d8c, DAT_800d1d8e, DAT_800d1d90, DAT_800d1d92, 1, 32, 128);

	DAT_800d1d88 = (struct O *)p;
	DrawSync(0);

	{
		struct C30 a = DAT_800a1fd0;
		struct C28 b = DAT_800a1ff0;

		s0 = 32;
		w = DAT_800d1cb4;

		if (w) {
			DAT_800d1cb4 = 0;
			DAT_800d1d80 = FUN_800154f4(w);
		}

		FrontLoader_SetFiles(0);

		DemoTimer = 1600;

		while (1) {
			Front_UpdateMusicLoop();

			if (!DemoTimer)
				break;

			DemoTimer--;
			s0--;
			HiRes_ClearOTag();
			ResetOT();
			FrontLoader_Continue();
			Mess_SetRGB(128, 128, 128, 0);
			Mess_SetTextJustify(0);

			if (s0 < 32)
				Sprite_draw(DAT_800d1d88, DAT_800d1d8c, DAT_800d1d8e, 0, -1, -1, -1);
			else if (s0 < 128) {
				Mess_SetFontShadowOffset(4);
				Mess_DrawText(320, 357, GameText.s, 1, 0, 256, 0);
			}

			if (!s0)
				s0 = 128;

			Pad_Update();
			Front_CheckSoftReset();

			if (DAT_800d1cb8)
				break;

			Mess_Update();
			Mess_Display();
			HiRes_DrawOTag();

			if (s0 > 128 || !(Pad[PAD_MENU_START + PAD_TRIGGERED] ||
			                  Pad[PAD_SIZE + PAD_MENU_START + PAD_TRIGGERED]))
				continue;

			Pad[PAD_MENU_START + PAD_TRIGGERED] = 0;
			Pad[PAD_SIZE + PAD_MENU_START + PAD_TRIGGERED] = 0;
			SFX_PlayX(145, 8192, 0);
			break;
		}

		if (DAT_800d1d88)
			DAT_800d1d88->vt->fn((char *)DAT_800d1d88 + DAT_800d1d88->vt->off, 3);
	}
}
