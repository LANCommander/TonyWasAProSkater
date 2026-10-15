/* ReplaceTexture__FUiPc (0x800247a8) */
#include "names.h"

typedef struct {
	char pad[24];
	unsigned char *info;
	unsigned short w, h;
} STexEntry;

extern int Pal256Y;
extern int Pal256X;

STexEntry *Spool_FindTextureEntry(unsigned);
int FileIO_Open(const char *);
void *Mem_NewX(unsigned, int, int);
void FileIO_Load(char *);
void FileIO_Sync(void);
void DownloadBMP(char *, int, int, int, int);
void Mem_Delete(char *);

void ReplaceTexture(unsigned id, char *name) {
	STexEntry *e = Spool_FindTextureEntry(id);

	if (e) {
		unsigned char *info = e->info;
		int pal = Pal256Y;
		int w = e->w;
		int h = e->h;
		int k;
		char *bmp;

		pal += info[2];
		k = Pal256X;
		bmp = (char *)Mem_NewX(FileIO_Open(name), 1, 1);
		FileIO_Load(bmp);
		FileIO_Sync();
		DownloadBMP(bmp, w, h, k, pal);
		Mem_Delete(bmp);
	}
}
