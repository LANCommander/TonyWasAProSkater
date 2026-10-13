/* FileIO_Sync__Fv (0x8004cdd0) */
#include "names.h"

int LoadFromPRE;
int Loading;

void FileIO_Continue(void);
void FUN_8005dcc4(void);

void FileIO_Sync(void) {
	if (LoadFromPRE) {
		Loading = 0;
		LoadFromPRE = 0;
		return;
	}

	while (Loading) {
		FileIO_Continue();
		FUN_8005dcc4();
	}

	FileIO_Continue();
}
