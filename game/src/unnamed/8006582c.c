/* FUN_8006582c (0x8006582c) */
#include "names.h"

typedef struct E {
	int p;
	char s[16];
	int i14, i18, i1c;
	int used;
} E;

typedef struct C {
	int f0, f4, f8, cur;
	E e[8];
} C;

extern int SpoolerBlocksFileIO;

int FileIO_Open(char *);
void FileIO_Load(int);
void FileIO_Sync(void);
int Mem_NewX(int, int, int);
void PREManager_spoolPRE(int, char *);
int Spool_PSX(char *, int, int);
void printf(char *, char *);
int strcmp(char *, char *);
void Spool_Sync(void);

void FUN_8006582c(C *a) {
	char *s;
	int ok, n, t;
	char buf[16];

	if (a->f4 == 1)
		Spool_Sync();
	else
		FileIO_Sync();

	while (a->e[a->cur].used)
		a->cur++;

	a->f4 = 0;
	s = a->e[a->cur].s;

	while (*s != 95 && *s)
		s++;

	if (*s == 95) {
		s++;
		ok = 0;

		if (!strcmp(s, "fe") || !strcmp(s, "FE") || !strcmp(s, "f") || !strcmp(s, "F"))
			ok = 1;

		if (ok)
			a->f4 = 1;
	}

	s = a->e[a->cur].s;

	while (*s != 46 && *s)
		s++;

	if (*s == 46) {
		s++;
		ok = 0;

		if (!strcmp(s, "pre") || !strcmp(s, "PRE"))
			ok = 1;

		if (ok)
			a->f4 = 2;
	}

	t = a->f4;

	switch (t) {
		case 1:
			a->e[a->cur].p = 0;
			a->e[a->cur].i14 = Spool_PSX(a->e[a->cur].s, 2, 1);
			a->e[a->cur].i1c = 0;
			a->e[a->cur].i18 = t;
			break;
		case 2:
			PREManager_spoolPRE(SpoolerBlocksFileIO, a->e[a->cur].s);

			a->e[a->cur].p = 0;
			a->e[a->cur].i14 = -1;
			a->e[a->cur].i1c = 0;
			a->e[a->cur].i18 = t;
			break;
		default:
			n = FileIO_Open(a->e[a->cur].s);

			if (n <= 0) {
				printf("\nFile %s not found", a->e[a->cur].s);
				return;
			}

			a->e[a->cur].p = Mem_NewX(n, 1, 1);

			if (a->e[a->cur].p == 0) {
				printf("\nNot enough memory to load %s", a->e[a->cur].s);
				return;
			}

			FileIO_Load(a->e[a->cur].p);

			a->e[a->cur].i14 = -1;
			a->e[a->cur].i1c = n;
			a->e[a->cur].i18 = 0;
			break;
	}

	a->e[a->cur].used = 1;
	a->cur = a->cur + 1;
}
