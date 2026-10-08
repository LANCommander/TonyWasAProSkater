/* FUN_8005f6bc (0x8005f6bc) */
/* C++ (front end, THPS1-only; no demo twin). The CONTROLLER options screen (opt_ctr.bmp): for the
   player in DAT_800d1d28 it copies the pad mapping out of the options into the edit buffer
   DAT_800bf260, labels the four button rows with their current function (FUN_8005ea74), builds
   the "vibration on/off" and "Autokick on/off" rows, snapshots the mapping into DAT_800bf280 and
   runs the 8-entry menu (TRIANGLE, SQUARE, CIRCLE, X, VIBRATION, AUTOKICK, RESET TO DEFAULT,
   BACK TO OPTIONS; ids 1296..) with FUN_8005eb78 as the per-frame handler. Backing out restores
   the options background and returns -1. Called from the options menu FUN_80060274. */
struct SOptions {
	int SFXVolume;
	int MusicVolume;
	int OtherOptions[66];
	int AutoKickEnabled[2];       /* 0x110 */
	int VibrationEnabled[2];      /* 0x118 */
	int ControllerFunction[2][4]; /* 0x120 */
};

struct SPadMap {
	int Function[4];
};

struct SText {
	char pad0[0x4c];
	char *Autokick; /* 0x4c */
	char pad50[0xf0 - 0x50];
	char *On;  /* 0xf0: "on <off>" */
	char *Off; /* 0xf4: "<on> off" */
	char pad8[0x100 - 0xf8];
	char *Vibration; /* 0x100 */
};

struct SLabels {
	char *Name[8];
};

extern SOptions GPreferences;
extern SPadMap DAT_800bf260[2];
extern SPadMap DAT_800bf280[2];
extern SText GameText;
extern SLabels DAT_800a1d0c;
extern char s_opt_ctr_bmp[];
extern char s_options_bmp[];
extern char s_s_s[];

int DAT_800d1d28;

extern "C" {
	void FUN_8005c97c(int);
	void FUN_8005ca64(void);
	int FUN_8005ca3c(char *);
	void FUN_80061714(int);
	int FUN_8008b900(char *, char *, ...);
	char *FUN_8005ea74(int);
	int FUN_8005eb78(void);
	int FUN_8005e0e0(char **, int *, int, int, int *, int (*)(void), int *, int *, int);
}

extern "C" int FUN_8005f6bc(int a) {
	int pl = DAT_800d1d28;
	SLabels labels;
	int disabled[8];
	int xs[8];
	int ys[8];
	char vib[32];
	char ak[32];
	int r;

	FUN_8005c97c(13);
	FUN_8005ca64();
	FUN_80061714(FUN_8005ca3c(s_opt_ctr_bmp));

	labels = DAT_800a1d0c;
	disabled[0] = 0;
	disabled[1] = 0;
	disabled[2] = 0;
	disabled[3] = 0;
	disabled[4] = 0;
	disabled[5] = 0;
	disabled[6] = 0;
	disabled[7] = 0;
	xs[0] = 487;
	ys[0] = 95;
	xs[1] = 224;
	ys[1] = 126;
	xs[2] = 493;
	ys[2] = 238;
	xs[3] = 254;
	ys[3] = 271;
	xs[4] = 350;
	ys[4] = 315;
	xs[5] = 350;
	ys[5] = 345;
	xs[6] = 350;
	ys[6] = 375;
	xs[7] = 350;
	ys[7] = 405;
	DAT_800bf260[pl].Function[0] = GPreferences.ControllerFunction[pl][0];
	DAT_800bf260[pl].Function[1] = GPreferences.ControllerFunction[pl][1];
	DAT_800bf260[pl].Function[2] = GPreferences.ControllerFunction[pl][2];
	DAT_800bf260[pl].Function[3] = GPreferences.ControllerFunction[pl][3];
	labels.Name[0] = FUN_8005ea74(DAT_800bf260[pl].Function[0]);
	labels.Name[1] = FUN_8005ea74(DAT_800bf260[pl].Function[1]);
	labels.Name[2] = FUN_8005ea74(DAT_800bf260[pl].Function[2]);
	labels.Name[3] = FUN_8005ea74(DAT_800bf260[pl].Function[3]);

	FUN_8008b900(vib, s_s_s, GameText.Vibration,
	             GPreferences.VibrationEnabled[pl] ? GameText.On : GameText.Off);

	labels.Name[4] = vib;

	FUN_8008b900(ak, s_s_s, GameText.Autokick,
	             GPreferences.AutoKickEnabled[pl] ? GameText.On : GameText.Off);

	labels.Name[5] = ak;
	DAT_800bf280[pl].Function[0] = DAT_800bf260[pl].Function[0];
	DAT_800bf280[pl].Function[2] = DAT_800bf260[pl].Function[2];
	DAT_800bf280[pl].Function[1] = DAT_800bf260[pl].Function[1];
	DAT_800bf280[pl].Function[3] = DAT_800bf260[pl].Function[3];
	r = FUN_8005e0e0(labels.Name, disabled, 8, 1296, &a, FUN_8005eb78, xs, ys, 0);

	if (r == -1 || r == 1304) {
		FUN_8005c97c(11);
		FUN_8005ca64();
		FUN_80061714(FUN_8005ca3c(s_options_bmp));

		r = -1;
	}

	return r;
}
