/* FUN_80066768 (0x80066768) */
#include "names.h"

typedef struct {
	char p[0x24];
	int f24;
	char q[0x78 - 0x28];
} E;

extern E Levels[];
extern int Loading;

int DAT_800d2360, DAT_800d2374, DAT_800d2378, DAT_800d237c, DAT_800d2380, DAT_800d2390;

extern int DAT_800d3810[], DAT_800d3840[];
extern char s_File_s_not_found_800a21cc[];

void FileIO_Continue(void);
int FileIO_Load(int);
int FileIO_Open(char *);
int Mem_NewX(int, int, int);
void FUN_8008c190(char *, char *);
void FUN_8008e904(char *, int);

void FUN_80066768(void) {
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

		FUN_8008e904(buf, Levels[DAT_800d2360].f24);

		n = FileIO_Open(buf);

		if (n <= 0) {
			FUN_8008c190(s_File_s_not_found_800a21cc, buf);
			return;
		}

		{
			int r = Mem_NewX(n, 1, 1);

			DAT_800d3810[DAT_800d2360] = r;
			FileIO_Load(r);
		}
	}
}
