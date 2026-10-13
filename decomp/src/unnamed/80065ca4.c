/* FUN_80065ca4 (0x80065ca4) */
#include "names.h"

extern int Loading;

void FileIO_Continue(int *p);
void Spool_Continue(void);
void FUN_8006582c(int *p);

void FUN_80065ca4(int *p) {
	FileIO_Continue(p);

	if (p[1] == 1)
		Spool_Continue();

	if (p[3] < p[2] && Loading == 0)
		FUN_8006582c(p);
}
