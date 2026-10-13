/* FUN_80014ba4 (0x80014ba4) */
#include "names.h"

typedef struct {
	int a, b;
} T;

typedef struct {
	char pad[44];
	short s[5];
} H;

extern int GVideoRestart[];

typedef struct {
	char pad[28];
	int f;
	char pad2[48];
	T t[5];
} S;

extern S GVInfo1;

char *FileIO_Open(const char *);
char *Mem_NewX(char *a, int b, int c);
void FileIO_Load(char *a);
void FileIO_Sync(void);
void Mem_Delete(char *a);

short FUN_80014ba4(const char *name) {
	H *h;
	char *b;
	int i;

	b = Mem_NewX(FileIO_Open(name), 1, 1);

	if (b) {
		FileIO_Load(b);
		FileIO_Sync();
	}

	h = (H *)(b + 512);
	GVideoRestart[0] = h->s[0];
	GVideoRestart[1] = h->s[1];
	GVideoRestart[2] = h->s[2];
	GVideoRestart[3] = h->s[3];
	GVideoRestart[4] = h->s[4];
	GVInfo1.f = *(int *)(b + 512);

	for (i = 0; i < 5; i++)
		GVInfo1.t[i] = ((T *)(b + 516))[i];

	Mem_Delete(b);
	return h->s[0];
}
