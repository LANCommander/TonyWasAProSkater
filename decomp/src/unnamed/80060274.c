/* FUN_80060274 (0x80060274) */
#include "names.h"

typedef struct {
	int w[7];
} T;

typedef struct {
	char p[0xf0];
	int f0, f4, f8, fc;
} Q;

typedef struct {
	char p[0x184];
	int f184, f188;
} D;

extern T DAT_800a1df8;
extern Q GameText;
extern D GPreferences;

int DAT_800d1d28;

int FUN_8005ffac(int);
void FrontLoader_LoadSet(int);
void FrontLoader_Defragment(void);
int FrontLoader_TakeFile(char *);
void DownloadBMP16x2(int);
int sprintf(char *, char *, int, int);
int Front_RunMenu(T *, int *, int, int, int *, int (*)(int), int, int, int);
void Front_ControllerOptions(int);
void FUN_8005fd50(int);
void FUN_800601fc(int);

#ifdef PC_PORT
/* PC: GRAPHICS (the display mode and resolution, PC_GraphicsOptions in
   front/controlleroptions.cpp) between SCORE DISPLAY and LOAD DATA. The trick tips and score
   display toggles keep their rows (FUN_8005ffac edits rows 3 and 4); ids are 81 + row. */
void PC_GraphicsOptions(int);

static char s_graphics[] = "GRAPHICS";

int FUN_80060274(int a) {
	char A[32];
	char B[32];
	int t[8];
	int k[8];
	int v, i;

	FrontLoader_LoadSet(11);
	FrontLoader_Defragment();
	DownloadBMP16x2(FrontLoader_TakeFile("options.bmp"));

	for (;;) {
		for (i = 0; i < 5; i++)
			t[i] = DAT_800a1df8.w[i];

		t[5] = (int)s_graphics;
		t[6] = DAT_800a1df8.w[5];
		t[7] = DAT_800a1df8.w[6];

		for (i = 0; i < 8; i++)
			k[i] = 0;

		sprintf(A, "%s %s", GameText.f8, GPreferences.f184 ? GameText.f0 : GameText.f4);

		t[3] = (int)A;
		sprintf(B, "%s %s", GameText.fc, GPreferences.f188 ? GameText.f0 : GameText.f4);

		t[4] = (int)B;
		v = Front_RunMenu((T *)t, k, 8, 80, &a, FUN_8005ffac, 0, 0, 0);

		if (v == -1 || v == 88) {
			FrontLoader_LoadSet(0);
			FrontLoader_Defragment();
			DownloadBMP16x2(FrontLoader_TakeFile("main_h.bmp"));

			return -1;
		}

		switch (v) {
			case 81:
			case 82:
				DAT_800d1d28 = v != 81;
				Front_ControllerOptions(a);
				break;
			case 83:
				FUN_8005fd50(a);
				break;
			case 86:
				PC_GraphicsOptions(a);
				break;
			case 87:
				FUN_800601fc(a);
				break;
			default:
				return v;
		}

		a = 0;
	}
}
#else
int FUN_80060274(int a) {
	char A[32];
	char B[32];

	T t __attribute__((aligned(8)));
	int k[7] __attribute__((aligned(8)));
	int v, r;

	FrontLoader_LoadSet(11);
	FrontLoader_Defragment();
	DownloadBMP16x2(FrontLoader_TakeFile("options.bmp"));

	for (;;) {
		t = DAT_800a1df8;
		k[0] = 0;
		k[1] = 0;
		k[2] = 0;
		k[3] = 0;
		k[4] = 0;
		k[5] = 0;
		k[6] = 0;
		sprintf(A, "%s %s", GameText.f8, GPreferences.f184 ? GameText.f0 : GameText.f4);

		t.w[3] = (int)A;
		sprintf(B, "%s %s", GameText.fc, GPreferences.f188 ? GameText.f0 : GameText.f4);

		t.w[4] = (int)B;
		v = Front_RunMenu(&t, k, 7, 80, &a, FUN_8005ffac, 0, 0, 0);

		if (v == -1 || v == 87) {
			FrontLoader_LoadSet(0);
			FrontLoader_Defragment();
			DownloadBMP16x2(FrontLoader_TakeFile("main_h.bmp"));

			v = -1;
		} else {
			switch (v) {
				case 81:
				case 82:
					v ^= 81;
					v = v != 0;
					DAT_800d1d28 = v;
					Front_ControllerOptions(a);

					v = -1;
					break;
				case 83:
					FUN_8005fd50(a);

					v = -1;
					break;
				case 86:
					FUN_800601fc(a);

					v = -1;
					break;
			}

			if (v == -1) {
				a = 0;
				continue;
			}
		}

		r = v;
		break;
	}

	return r;
}
#endif
