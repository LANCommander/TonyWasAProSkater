/* Front_MemCardDisplay__Fv (0x80011820) */
#include "names.h"
/* Memory-card front-end screen (RFRONT.CPP), as C. The save-slot list loop follows the demo twin:
   x is XMid - 150 + i * 30 (no separate column variable), and the slot letter goes into a copy of a
   3-byte template in buf. */
typedef struct {
	char pad0[0xc];
	char *f0c;
	char *f10;
	char pad1[0x18 - 0x14];
	char *f18;
	char *f1c;
	char pad2[0xd0 - 0x20];
	char *fd0;
	char pad3[4];
	char *fd8;
	char pad4[0x180 - 0xdc];
	char *f180;
	char pad5[0x1c8 - 0x184];
	char *f1c8;
	char *f1cc;
	char *f1d0;
	char *f1d4;
	char *f1d8;
	char *f1dc;
	char *f1e0;
	char *f1e4;
	char pad6[0x1f8 - 0x1e8];
	char *f1f8;
	char *f1fc;
	char *f200;
	char pad7[4];
	char *f208;
	char *f20c;
	char *f210;
	char *f214;
	char *f218;
	char *f21c;
	char pad8[0x228 - 0x220];
	char *f228;
	char pad9[0x234 - 0x22c];
	char *f234;
	char pad10[0x244 - 0x238];
	char *f244;
} S;

typedef struct {
	char pad[0x10];
	int f10;
} O;

extern S GameText;
extern char *GameText_34[];
extern char *GameText_1f4[];
extern char *GameText_23c[];
extern char *GameText_240[];
extern char s_no_space_do_you_want_to_delete_b[];
extern char s_loading_video_file[];
extern char SaveGameName[];
extern char DAT_800d112c[];
extern char DAT_800d1130[];
extern char DAT_800d1134[];

typedef struct {
	char a, b, c;
} T3;

extern char DAT_800d1138[];
extern O *pYesNoMenu;
extern int TTime;

unsigned MCState;
int MCResolution;
int CursorPos, SizeUp, GLoadAction, SaveNeedBlocks, SaveFreeBlocks;

void FUN_80011214(void);
void CMenu_Display(O *);
void Mess_SetTextJustify(int);
int Mess_DrawText(int, int, char *, int, int, int, int);
void FUN_8008b900(char *, char *, int, char *);
void Front_PrintCentredPrompt(char *);
void Front_PrintGoBackSelect(void);
void Mess_SetRGB(int, int, int, int);
void Mess_SetScale(int);
void RGBRed(void);

void Front_MemCardDisplay(void) {
	int s2, s0, s1, t;
	char buf[24];
	unsigned char c;
	O *o;
	char *a;

	s2 = MCResolution >> 1;
	Mess_SetScale(256);
	Mess_SetTextJustify(0);

	switch (MCState) {
		case 0:
		case 1:
		case 15:
		case 16:
			FUN_80011214();
#ifndef PC_PORT /* PC: no card to check (pc_saves.h) */
			Mess_DrawText(s2, 90, GameText.f200, 0, 0, 256, 0);
			RGBRed();
			Mess_DrawText(s2, 140, GameText.f1f8, 0, 0, 256, 0);
			Mess_DrawText(s2, 166, GameText.f1fc, 0, 0, 256, 0);
#endif
			break;
		case 9:
		case 17:
			RGBRed();
			Mess_DrawText(s2, 90, GameText.f1c8, 0, 0, 256, 0);
			Mess_DrawText(s2, 116, GameText.f1cc, 0, 0, 256, 0);
			Mess_DrawText(s2, 142, GameText.f1d0, 0, 0, 256, 0);
			Mess_DrawText(s2, 168, GameText.f1d4, 0, 0, 256, 0);
			break;
		case 10:
			Front_PrintGoBackSelect();
			RGBRed();
			Mess_DrawText(s2, 35, GameText.f1d8, 0, 0, 256, 0);
			Mess_DrawText(s2, 61, GameText.f1dc, 0, 0, 256, 0);
			Mess_DrawText(s2, 87, GameText.f1e0, 0, 0, 256, 0);
			Mess_DrawText(s2, 113, GameText.f1e4, 0, 0, 256, 0);

			o = pYesNoMenu;
			o->f10 = 148;
			CMenu_Display(o);
			break;
		case 11:
			FUN_80011214();

			if ((TTime / 10) & 1)
				Mess_DrawText(s2, 110, GameText_1f4[0], 0, 0, 256, 0);

#ifndef PC_PORT
			RGBRed();
			Mess_DrawText(s2, 140, GameText.f1f8, 0, 0, 256, 0);
			Mess_DrawText(s2, 166, GameText.f1fc, 0, 0, 256, 0);
#endif
			break;
		case 13:
			FUN_80011214();
			Mess_DrawText(s2, 110, GameText_240[0], 0, 0, 256, 0);
			break;
		case 12:
			RGBRed();
			Mess_DrawText(s2, 110, GameText_23c[0], 0, 0, 256, 0);
			break;
		case 8:
			RGBRed();
			Mess_SetScale(192);
			Mess_DrawText(s2, 67, GameText.f208, 0, 0, 256, 0);
			Mess_DrawText(s2, 85, GameText.f20c, 0, 0, 256, 0);
			Mess_DrawText(s2, 103, GameText.f210, 0, 0, 256, 0);
			FUN_8008b900(buf, GameText.f214, SaveNeedBlocks,
			             SaveNeedBlocks == 1 ? DAT_800d112c : DAT_800d1130);
			Mess_DrawText(s2, 121, buf, 0, 0, 256, 0);
			FUN_8008b900(buf, GameText.f218, SaveNeedBlocks - SaveFreeBlocks,
			             SaveNeedBlocks - SaveFreeBlocks == 1 ? DAT_800d112c : DAT_800d1130);
			Mess_DrawText(s2, 139, buf, 0, 0, 256, 0);
			Mess_DrawText(s2, 157, GameText.f21c, 0, 0, 256, 0);
			Front_PrintCentredPrompt(GameText.f180);
			break;
		case 3:
			FUN_80011214();
			Mess_DrawText(s2, 50, GameText.f18, 0, 0, 256, 0);
			Mess_DrawText(s2, 76, GameText.f1c, 0, 0, 256, 0);
			Mess_DrawText(s2, 102, GameText.f228, 0, 0, 256, 0);
			Front_PrintGoBackSelect();

			o = pYesNoMenu;
			o->f10 = 145;
			CMenu_Display(o);
			break;
		case 4:
			FUN_80011214();
			Mess_DrawText(s2, 110, GameText.f234, 0, 0, 256, 0);
#ifndef PC_PORT
			RGBRed();
			Mess_DrawText(s2, 140, GameText.f1f8, 0, 0, 256, 0);
			Mess_DrawText(s2, 166, GameText.f1fc, 0, 0, 256, 0);
#endif
			break;
		case 7:
			FUN_80011214();
			Mess_DrawText(s2, 110, GameText.fd0, 0, 0, 256, 0);
			Front_PrintCentredPrompt(GameText.f180);
			break;
		case 6:
			RGBRed();
			Mess_DrawText(s2, 110, GameText.fd8, 0, 0, 256, 0);
			Front_PrintCentredPrompt(GameText.f180);
			break;
		case 2:
			s0 = 0;
			RGBRed();
			Mess_SetTextJustify(0);
			Mess_SetScale(256);
			Mess_DrawText(s2, 40, GameText_34[0], 0, 0, 256, 0);

			do {
				if (SizeUp > 0)
					SizeUp--;

				Mess_SetScale(256);

				if (s0 == CursorPos)
					Mess_SetRGB(182, 132, 36, 0);
				else
					Mess_SetRGB(122, 122, 122, 0);

				Mess_DrawText(s2 - 150 + s0 * 30, 100, DAT_800d1134, 0, 0, 256, 0);

				if (s0 == CursorPos)
					Mess_SetScale(SizeUp * 6 + 256);

				if (SaveGameName[s0]) {
					*(T3 *)buf = *(T3 *)DAT_800d1138;
					buf[0] = SaveGameName[s0];
					Mess_DrawText(s2 - 150 + s0 * 30, 92, buf, 0, 0, 256, 0);
				}

				s0++;
			} while (s0 < 10);

			Front_PrintGoBackSelect();
			break;
		case 14:
			Mess_DrawText(s2, 100, s_no_space_do_you_want_to_delete_b, 0, 0, 256, 0);
			break;
		case 19:
			RGBRed();

			if (GLoadAction == 2)
				Mess_DrawText(s2, 110, GameText.f10, 0, 0, 256, 0);
			else
				Mess_DrawText(s2, 110, GameText.f0c, 0, 0, 256, 0);

			if (MCResolution == 512)
				Front_PrintCentredPrompt(GameText.f180);

			break;
		case 20:
			if (MCResolution == 512)
				Front_PrintGoBackSelect();

			break;
		case 21:
			FUN_80011214();
			Mess_DrawText(s2, 110, s_loading_video_file, 0, 0, 256, 0);
#ifndef PC_PORT
			RGBRed();
			Mess_DrawText(s2, 140, GameText.f1f8, 0, 0, 256, 0);
			Mess_DrawText(s2, 166, GameText.f1fc, 0, 0, 256, 0);
#endif
			break;
		case 22:
			RGBRed();
#ifdef PC_PORT
			Mess_DrawText(s2, 110, "load failed", 0, 0, 256, 0);
#else
			Mess_DrawText(s2, 110, GameText.f244, 0, 0, 256, 0);
#endif

			if (MCResolution == 512)
				Front_PrintCentredPrompt(GameText.f180);

			break;
		case 23:
			break;
	}
}
