/* Load8BitBMP__FPcPPcPiT2PUs (0x80061654) */
#include "names.h"

int FileIO_Open(char *);
char *Mem_NewX(int, int, int);
void FileIO_Load(char *);
void FileIO_Sync(void);
void printf(char *, char *);
void Load8BitBMP2(char *, char **, int *, int *, unsigned short *);

int Load8BitBMP(char *name, char **a, int *b, int *c, unsigned short *d) {
	int r = FileIO_Open(name);
	char *p;

	if (r <= 0) {
		printf("\nFile %s not found", name);
		return 0;
	}

	p = Mem_NewX(r, 1, 1);

	if (p) {
		FileIO_Load(p);
		FileIO_Sync();
		Load8BitBMP2(p, a, b, c, d);
		return 1;
	}

	printf("\nNot enough memory to load %s", name);
	return 0;
}
