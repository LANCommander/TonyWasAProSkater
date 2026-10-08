/* FUN_80053dbc (0x80053dbc) flags=-fstructure-noalias */
/* C++ (front end, 1-player board select). Called once by FUN_80054510 (boardselect1p.cpp) before
   its loop: fills in the picture file names of the three columns (decks the skater hasn't got the
   tapes for show "b1lock01.bmp"; trucks and wheels come from fixed tables), resets the streaming
   state that FUN_80054134 (boardstream1p.cpp) works through, and loads the "locked" picture. */
typedef unsigned char uchar;
typedef unsigned short ushort;
typedef unsigned int uint;

/* SBoard as in boardselect2p.cpp, with the picture file name that precedes it: the array really
   starts at 0x54, so `Name` is at 0x58 and `Req` at 0x5c as there. */
struct SBoard {
	char *File;   /* 0: picture */
	char *Name;   /* 4 */
	uchar Req;    /* 8: tapes needed */
	int Stats[4]; /* 0xc */
	int pad;
};

struct SStats {
	char pad0[0x54];
	SBoard Boards[8]; /* 0x54 */
};

struct SSkaterCareer {
	uchar pad0[4];
	uchar NumTapes; /* 4 (0x8 in SCareer) */
	uchar pad5[9];
};

struct SCareer {
	int Version;
	SSkaterCareer Skater[148];
	char pad[0x824 - 4 - 148 * 14];
	char LastSkater[2]; /* 0x824 */
	char LastDeck[2];   /* 0x826 */
	char LastWheel[2];  /* 0x828 */
	char LastTrucks[2]; /* 0x82a */
};

class CItem {
  public:
	virtual ~CItem();
	void *operator new(uint) throw();
};

class SlicedImage : public CItem {
  public:
	int m[12];
	SlicedImage(char *, int, int, int, int, int, ushort);
	void pack();
};

extern SCareer GCareer;
extern SStats Stats[];
extern int DAT_800d3394[3];          /* load offset from the start entry (-1: not started) */
extern int DAT_800d33a4[3];          /* start entry */
extern int DAT_800d33b4[3];          /* column fully loaded */
extern int DAT_800d33c4[3];          /* pictures per column */
extern char DAT_800d33d4[3][15][16]; /* picture file names */
extern SlicedImage *DAT_800d36a4[3][15];

SlicedImage *DAT_800d22dc; /* "locked" picture */
int DAT_800d22e0;
char *DAT_800d22e4;
uchar DAT_800d22e8;
int DAT_800d22ec;
int DAT_800d22f0; /* skater */

extern "C" {
	char *strcpy(char *, const char *);
	void FileIO_Sync__Fv(void);
	int FileIO_Open__FPCc(char *);
	char *Mem_NewX__FUiii(int, int, int);
	void FileIO_Load__FPv(char *);
}

int Load8BitBMP2(char *, char **, int *, int *, ushort *);

extern "C" void FUN_80053dbc(void) {
	DAT_800d3394[0] = -1;
	DAT_800d33c4[0] = 8;
	DAT_800d33b4[0] = 0;
	DAT_800d3394[1] = 0;
	DAT_800d33c4[1] = 3;
	DAT_800d33b4[1] = 0;
	DAT_800d3394[2] = 0;
	DAT_800d33c4[2] = 15;
	DAT_800d33b4[2] = 0;
	DAT_800d22e8 = 1;
	DAT_800d33a4[0] = GCareer.LastDeck[0];
	DAT_800d33a4[1] = GCareer.LastTrucks[0];
	DAT_800d33a4[2] = GCareer.LastWheel[0];

	for (int i = 0; i < DAT_800d33c4[0]; i++) {
		if (GCareer.Skater[DAT_800d22f0].NumTapes >= Stats[DAT_800d22f0].Boards[i].Req)
			strcpy(DAT_800d33d4[0][i], Stats[DAT_800d22f0].Boards[i].File);
		else
			strcpy(DAT_800d33d4[0][i], "b1lock01.bmp");

		DAT_800d36a4[0][i] = 0;
	}

	char *trucks[3] = { "T1_loose.bmp", "T1_mdium.bmp", "T1_tight.bmp" };

	for (int j = 0; j < DAT_800d33c4[1]; j++) {
		strcpy(DAT_800d33d4[1][j], trucks[j]);

		DAT_800d36a4[1][j] = 0;
	}

	char *wheels[15] = {
		"W1blue01.bmp", "W1blue02.bmp", "W1blue03.bmp", "W1blue04.bmp", "W1brow01.bmp",
		"W1dark01.bmp", "W1gree01.bmp", "W1gree02.bmp", "W1gree03.bmp", "W1oran01.bmp",
		"W1purp01.bmp", "W1red_01.bmp", "W1red_02.bmp", "W1whit01.bmp", "W1yell01.bmp",
	};

	for (int j = 0; j < DAT_800d33c4[2]; j++) {
		strcpy(DAT_800d33d4[2][j], wheels[j]);

		DAT_800d36a4[2][j] = 0;
	}

	DAT_800d22ec = 0;
	DAT_800d22e4 = 0;
	DAT_800d22e0 = 0;
	DAT_800d22e4 = Mem_NewX__FUiii(FileIO_Open__FPCc("b1lock01.bmp"), 1, 1);

	FileIO_Load__FPv(DAT_800d22e4);
	FileIO_Sync__Fv();

	ushort clut;
	char *data;
	int w, h;

	Load8BitBMP2(DAT_800d22e4, &data, &w, &h, &clut);

	DAT_800d22e4 = 0;
	DAT_800d22dc = new SlicedImage(data, w, h, 128, 128, 8, clut);

	DAT_800d22dc->pack();
}
