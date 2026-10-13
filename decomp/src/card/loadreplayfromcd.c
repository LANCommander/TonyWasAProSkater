/* Card_LoadReplayFromCD__FPUlT0Pc (0x80014c74) */
#include "names.h"

char *FileIO_Open(char *);
char *Mem_NewX(char *, int, int);
void FileIO_Load(char *);
void FileIO_Sync(void);
void Mem_Delete(char *);
void printf(char *);
void Mem_Copy(void *, void *, int);
int CalcChecksum(void *, int);

int Card_LoadReplayFromCD(char *a, char *b, char *name) {
	int c;
	char *p = Mem_NewX(FileIO_Open(name), 1, 1);

	if (p) {
		FileIO_Load(p);
		FileIO_Sync();
	}

	Mem_Copy(a, p, 0x6000);

	if (b)
		Mem_Copy(b, p + 0x6000, 0x6000);

	Mem_Delete(p);

	c = *(short *)(a + 566);

	if (c != CalcChecksum(a + 640, 23936))
		printf("\nChecksum error for 1P in Card_LoadReplayFromCD()");

	if (b) {
		c = *(short *)(b + 566);

		if (c != CalcChecksum(b + 640, 23936))
			printf("\nChecksum error for 2P in Card_LoadReplayFromCD()");
	}

	return 0;
}
