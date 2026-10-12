/* FUN_800154f4 (0x800154f4) */
#include "names.h"
struct B {
	int w[628];
};

extern int FileType[];

typedef struct {
	char n[40];
} E;

extern E SLOT_DIR[];
extern struct B GCareer;
extern char StoredCareerName[];
extern int GHasCareerLoaded, GLoadedCareerIsSaved;

void Career_Init(void);
int SyncToGoodState(void);
void FUN_80061270(int);
void Card_GetDirAndFreeBlocks(int, int);
int Mem_NewX(int, int, int);
void Card_SetSelectedFile(E *);
int Card_Load(int, int, int);
int FUN_8008eebc(int);
void Front_ExtractASCIINameFromSJIS(char *, char *);
void Mem_Delete(int);
void FUN_800103a4(void);
void Options_SetPreferences(void);

#ifdef PC_PORT
#include "pc_saves.h"
#endif

int FUN_800154f4(void) {
	struct B buf;
	int i, r, s2, s4, p;
	E *name;
	struct B *d;

	r = 0;
	Career_Init();

	if (SyncToGoodState() == 0) {
#ifdef PC_PORT
		/* PC: the careers in the Saves folder (pc_saves.h), with no "memory card" messages */
		PC_SaveListDir(PC_SAVE_CAREER, SLOT_DIR, (int32_t *)FileType);
#else
		FUN_80061270(0);
		Card_GetDirAndFreeBlocks(0, 0);
#endif

		s4 = -1;
		s2 = 0;
		i = 0;

		do {
			if (FileType[i] == 0) {
				p = Mem_NewX(3200, 1, 1);
				Card_SetSelectedFile(&SLOT_DIR[i]);

				if (Card_Load(0, p, 0) == 0) {
#ifndef PC_PORT
					if (s2 == 0)
						s2 = FUN_8008eebc(-1);

					FUN_80061270(1);
#endif

					GHasCareerLoaded = 1;
					GLoadedCareerIsSaved = 0;
					r = 1;
					buf = *(struct B *)(p + 640);
					d = &GCareer;

					if (((unsigned short *)&buf)[1047] >= ((unsigned short *)d)[1047]) {
						*d = buf;
						s4 = i;
						Front_ExtractASCIINameFromSJIS((char *)(p + 32), StoredCareerName);
					}
				}

				Mem_Delete(p);
			}

			i++;
		} while (i < 15);

		if (s4 >= 0) {
			Card_SetSelectedFile(&SLOT_DIR[s4]);
			FUN_800103a4();
			Options_SetPreferences();
		}

		while (s2 != 0 && FUN_8008eebc(-1) < s2 + 60)
			;

		return r;
	}

	return 0;
}
