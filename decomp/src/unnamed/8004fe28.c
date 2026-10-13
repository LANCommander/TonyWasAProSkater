/* Credits_Roll__Fii (0x8004fe28) */
#include "names.h"
#include "game/padbuttons.h"

extern char Pad[];

int DAT_800d1b98[1];

void Mess_SetTextJustify(int);
int Mess_TextHeightAboveBaseline(char *);
void Mess_DrawText(int, int, char *, int, int, int, int);
void PollHost(void);
void Db_Init(void);
void FUN_80049474(void);
void FUN_800494dc(void);
void SFX_PlayX(int, int, int);
int FileIO_Open(char *);
void FileIO_Load(char *);
void FileIO_Sync(void);
char *Mem_NewX(int, int, int);
void Mem_Delete(char *);
void FUN_8005027c(int, int, int);
void Front_UpdateMusicLoop(void);
void DrawColoredBox(int, int, int, int, int, int, int);
void SetClippingBox(int, int, int, int);
void ResetOT(void);
void HiRes_BlankScreen(void);
void Pad_Update(void);
void VSync(int);
void Front_ClearScreen(void);
void Mess_LoadFont__FPciii(char *, int, int, int);
void Mess_SetRGB(int, int, int, int);
void Mess_SetScale(int);
int Mess_TextHeight(char *);
void Mess_UnloadFont(void);
void Pack_DefragmentFully(void);
void Pad_ClearTriggers(char *);

void Credits_Roll(int a0, int a1) {
	char *s0;
	char *a2;

	char (*out)[30];
	unsigned char *p;
	int c;
	int y, s7, i, s6, s3, s4, s2, th, bot, top, off, tog, s0i;

	if (a1)
		HiRes_BlankScreen();
	else
		Front_ClearScreen();

	Pack_DefragmentFully();

	if (a0 == 0)
		s0i = FileIO_Open("music.txt");

	if (a0 == 1)
		s0i = FileIO_Open("credits.txt");

	s0 = Mem_NewX(s0i, 1, 1);
	FileIO_Load(s0);

	s7 = 0;
	FileIO_Sync();

	out = (char (*)[30])Mem_NewX(12000, 1, 1);
	p = (unsigned char *)s0;

	while (1) {
		c = *p;

		if ((unsigned char)c == 35)
			break;

		a2 = out[s7];
		i = 0;

		if ((unsigned char)c != 13) {
			for (;;) {
				if (c != 35) {
					a2[i] = c;
					i++;
					p++;

					if (i < 29) {
						c = *p;

						if (c != 13)
							continue;
					}
				}

				break;
			}
		}

		a2[i] = 0;

		if (*p == 13)
			p += 2;

		s7++;

		if (s7 < 400)
			continue;

		break;
	}

	Mem_Delete(s0);
	Mess_LoadFont__FPciii("credits.fnt", -1, -1, -1);
	Mess_SetTextJustify(0);
	Mess_SetScale(255);
	Mess_SetRGB(150, 100, 25, 0);

	s6 = 0;
	s3 = Mess_TextHeightAboveBaseline((char *)out) + 256;
	th = Mess_TextHeight((char *)out) + 8;
	bot = -th;
	top = s3;
	off = a0 * 4;
	s4 = *(int *)((char *)DAT_800d1b98 + off);
	Pad_ClearTriggers(Pad);
	Pad_ClearTriggers(Pad + 364);
	Db_Init();

	tog = 0;
	FUN_8005027c(a0, 0, a1);

	while (1) {
		s2 = s6;
		Front_UpdateMusicLoop();
		FUN_80049474();
		ResetOT();
		SetClippingBox(0, tog << 8, 512, 240);
		DrawColoredBox(0, 0, 512, 240, 0, 0, 0);

		tog = 1 - tog;

		if (!(s6 < s7))
			break;

		y = s3;

		while (y < top) {
			Mess_DrawText(256, y, out[s2], 1, 0, 256, 0);

			s2++;
			y += th;

			if (!(s2 < s7))
				break;
		}

		s4--;

		if (s4 == 0) {
			s3--;
			s4 = *(int *)((char *)DAT_800d1b98 + off);
		}

		if (s3 < bot) {
			s6++;
			s3 += th;
		}

		VSync(0);
		FUN_8005027c(a0, 1, a1);
		VSync(0);
		FUN_800494dc();
		Pad_Update();
		PollHost();

		if (Pad[PAD_MENU_ACCEPT + PAD_TRIGGERED] ||
		    Pad[PAD_SIZE + PAD_MENU_ACCEPT + PAD_TRIGGERED] ||
		    Pad[PAD_MENU_START + PAD_TRIGGERED] || Pad[PAD_SIZE + PAD_MENU_START + PAD_TRIGGERED] ||
		    Pad[PAD_CAMERA + PAD_TRIGGERED] || Pad[PAD_SIZE + PAD_CAMERA + PAD_TRIGGERED]) {
			Pad_ClearTriggers(Pad);
			Pad_ClearTriggers(Pad + 364);
			SFX_PlayX(145, 8192, 0);
			break;
		}
	}

	FUN_8005027c(a0, 2, a1);
	Mem_Delete((char *)out);
	Mess_UnloadFont();
}
