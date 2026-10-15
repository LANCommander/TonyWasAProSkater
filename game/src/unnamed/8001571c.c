/* Card_GetNextTimeStamp__Fv (0x8001571c) */
#include "names.h"

struct B {
	int w[628];
};

extern int FileType[];
extern char SLOT_DIR[];

extern int TimeStamp;

int SyncToGoodState(void);
void *Mem_NewX(unsigned, int, int);
void Card_SetSelectedFile(char *);
int Card_Load(int, unsigned int *, unsigned int *);
void Mem_Delete(void *);

void Card_GetNextTimeStamp(void) {
	struct B buf;
	int i;
	char *p;
	char *name;
	int *flag;

	TimeStamp = 0;

	if (SyncToGoodState() == 0) {
		i = 0;
		name = SLOT_DIR;
		flag = FileType;

		do {
			if (*flag == 0) {
				p = Mem_NewX(3200, 1, 1);
				Card_SetSelectedFile(name);

				if (Card_Load(0, (unsigned int *)p, 0) == 0) {
					buf = *(struct B *)(p + 640);

					if (((unsigned short *)&buf)[1047] >= TimeStamp)
						TimeStamp = ((unsigned short *)&buf)[1047] + 1;
				}

				Mem_Delete(p);
			}

			name += 40;
			i++;
			flag++;
		} while (i < 15);
	}
}
