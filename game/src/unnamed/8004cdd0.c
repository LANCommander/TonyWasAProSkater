/* FileIO_Sync__Fv (0x8004cdd0) */
#include "names.h"

extern int LoadFromPRE;
extern int Loading;

void FileIO_Continue(void);
void FUN_8005dcc4(void);

void FileIO_Sync(void) {
	/* loads finish inside FileIO_Load (pc_fileio.h) */
	Loading = 0;
	LoadFromPRE = 0;
}
