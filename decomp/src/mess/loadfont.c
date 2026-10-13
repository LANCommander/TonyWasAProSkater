/* Mess_LoadFont__FPciii (0x8001eed4) */
#include "names.h"

extern int FileIO_Open(char *);
extern void *Mem_NewX(int, int, int);
extern void FileIO_Load(void *);
extern void FileIO_Sync(void);
extern void Mess_LoadFont__FPcT0iii(char *, void *, int, int, int);

void Mess_LoadFont(char *name, int a, int b, int c) {
	void *buf = Mem_NewX(FileIO_Open(name), 1, 1);

	FileIO_Load(buf);
	FileIO_Sync();
	Mess_LoadFont__FPcT0iii(name, buf, a, b, c);
}
