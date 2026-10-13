/* Options_GetPreferences__Fv (0x800103a4) */
#include "names.h"

/* Copies the saved preferences (career block GCareer) into the live game settings
   (GPreferences): two words, the per-player controller settings and both name strings, with the
   first name zero-padded to 11 bytes. */
typedef struct {
	int a, b, c, d;
} SPadPrefs;

typedef struct {
	int w0, w4;
	char pad8[0x110 - 8];
	int x110[2];
	int x118[2];
	SPadPrefs Pad[2];        /* 0x120 */
	char Str[0x184 - 0x140]; /* 0x140 */
	int x184, x188;
	int NameLen;   /* 0x18c */
	char Name[16]; /* 0x190 */
} SGame;

typedef struct {
	char pad0[0x830];
	int w830, w834;
	char pad838[0x940 - 0x838];
	int x940[2];
	int x948[2];
	SPadPrefs Pad[2];        /* 0x950 */
	char Str[0x9b4 - 0x970]; /* 0x970 */
	int x9b4, x9b8;
	int NameLen;   /* 0x9bc */
	char Name[16]; /* 0x9c0 */
} SCareer;

extern SGame GPreferences;
extern SCareer GCareer;

int sprintf(char *, const char *, ...);

void Options_GetPreferences(void) {
	int i;

	GPreferences.w0 = GCareer.w830;
	GPreferences.w4 = GCareer.w834;
	GPreferences.x184 = GCareer.x9b4;
	GPreferences.x188 = GCareer.x9b8;

	for (i = 0; i < 2; i++) {
		GPreferences.x110[i] = GCareer.x940[i];
		GPreferences.x118[i] = GCareer.x948[i];
		GPreferences.Pad[i].a = GCareer.Pad[i].a;
		GPreferences.Pad[i].b = GCareer.Pad[i].b;
		GPreferences.Pad[i].c = GCareer.Pad[i].c;
		GPreferences.Pad[i].d = GCareer.Pad[i].d;
	}

	sprintf(GPreferences.Name, GCareer.Name);

	GPreferences.NameLen = GCareer.NameLen;

	{
		int j;

		for (j = GPreferences.NameLen; j < 11; j++)
			GPreferences.Name[j] = 0;
	}

	sprintf(GPreferences.Str, GCareer.Str);
}
