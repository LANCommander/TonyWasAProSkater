/* LevelSelect_ContinueLoading__Fv (0x80066768) */
#include "names.h"

/* SLevel (game/career.h) */
typedef struct {
	char *pLevelName, *pCityName, *pVab;
	int isCompetition, isLocked;
	char *pTRGName;
	unsigned GoldScore, SilverScore, BronzeScore;
	char *f24;
	int map_loc[2];
	char *pLoadImage, *pBackImage, *pGoalName;
	int q[13];
	short ang[3];
} E;

extern E Levels[];
extern int Loading;

extern int DAT_800d2360, DAT_800d2374, DAT_800d2378, DAT_800d237c, DAT_800d2380, DAT_800d2390;

extern char *DAT_800d3810[];
extern int DAT_800d3840[];

void FileIO_Continue(void);
int FileIO_Load(void *);
int FileIO_Open(char *);
char *Mem_NewX(int, int, int);
void printf(char *, char *);
void strcpy(char *, char *);

void LevelSelect_ContinueLoading(void) {
	char buf[16];
	int n, t, one;

	FileIO_Continue();

	if (Loading == 0 && DAT_800d2380 == 0 && DAT_800d2390 < 4) {
		t = DAT_800d2374;

		if (t >= 0) {
			DAT_800d2390++;
			DAT_800d3840[DAT_800d2360] = one = 1;
		} else {
			DAT_800d2374 = DAT_800d2374 + 1;
			goto L50;
		}

		if (t < DAT_800d237c / 2)
			DAT_800d2374 = DAT_800d237c - t - 1;
		else {
			if (DAT_800d237c / 2 >= t) {
				DAT_800d2380 = one;
				return;
			}

			DAT_800d2374 = DAT_800d237c - t;
		}
	L50:
		DAT_800d2360 = DAT_800d2374 + DAT_800d2378;

		if (DAT_800d2360 >= DAT_800d237c)
			DAT_800d2360 -= DAT_800d237c;

		strcpy(buf, Levels[DAT_800d2360].f24);

		n = FileIO_Open(buf);

		if (n <= 0) {
			printf("\nFile %s not found", buf);
			return;
		}

		{
			char *r = Mem_NewX(n, 1, 1);

			DAT_800d3810[DAT_800d2360] = r;
			FileIO_Load(r);
		}
	}
}
