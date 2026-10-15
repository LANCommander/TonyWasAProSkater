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
void *Mem_NewX(unsigned, int, int);
void Card_SetSelectedFile(E *);
int Card_Load(int, unsigned int *, unsigned int *);
int VSync(int);
void Front_ExtractASCIINameFromSJIS(char *, char *);
void Mem_Delete(void *);
void Options_GetPreferences(void);
void Options_SetPreferences(void);

#include "pc_saves.h"
#include "pc_frame.h"

int FUN_800154f4(void) {
	struct B buf;
	int i, r, s2, s4;
	char *p;
	E *name;
	struct B *d;

	r = 0;
	Career_Init();

	if (SyncToGoodState() == 0) {
		/* PC: the careers in the Saves folder (pc_saves.h), with no "memory card" messages */
		PC_SaveListDir(PC_SAVE_CAREER, SLOT_DIR, (int32_t *)FileType);

		s4 = -1;
		s2 = 0;
		i = 0;

		do {
			if (FileType[i] == 0) {
				p = Mem_NewX(3200, 1, 1);
				Card_SetSelectedFile(&SLOT_DIR[i]);

				if (Card_Load(0, (unsigned int *)p, 0) == 0) {

					GHasCareerLoaded = 1;
					GLoadedCareerIsSaved = 0;
					r = 1;
					buf = *(struct B *)(p + 640);
					d = &GCareer;

					if (((unsigned short *)&buf)[1047] >= ((unsigned short *)d)[1047]) {
						*d = buf;
						s4 = i;
						Front_ExtractASCIINameFromSJIS(p + 32, StoredCareerName);
					}
				}

				Mem_Delete(p);
			}

			i++;
		} while (i < 15);

		if (s4 >= 0) {
			Card_SetSelectedFile(&SLOT_DIR[s4]);
			Options_GetPreferences();
			Options_SetPreferences();
		}

		return r;
	}

	return 0;
}
