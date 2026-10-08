/* Card_LoadReplayFromCD__FPUlT0Pc (0x80014c74) */
#include "names.h"
extern char s_Checksum_error_for_1P_in_Card_Lo[];
extern char s_Checksum_error_for_2P_in_Card_Lo[];

char *FileIO_Open(char *);
char *Mem_NewX(char *, int, int);
void FileIO_Load(char *);
void FileIO_Sync(void);
void Mem_Delete(char *);
void FUN_8008c190(char *);
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
		FUN_8008c190(s_Checksum_error_for_1P_in_Card_Lo);

	if (b) {
		c = *(short *)(b + 566);

		if (c != CalcChecksum(b + 640, 23936))
			FUN_8008c190(s_Checksum_error_for_2P_in_Card_Lo);
	}

	return 0;
}
