/* Spool_Sync__Fv (0x8007079c) */
#include "names.h"

extern char SpoolQ[][17];
extern int Loading;

extern int SpoolPos;

void Spool_Continue(void);
void FileIO_Continue(void);

void Spool_Sync(void) {
	while (Loading || SpoolQ[SpoolPos][13]) {
		Spool_Continue();
		FileIO_Continue();
	}
}
