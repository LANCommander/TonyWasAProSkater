/* FileIO_SetSectorsPerFrame__Fi (0x8004c740) */
#include "names.h"

extern int SectorsPerFrame;

void FileIO_SetSectorsPerFrame(int v) {
	SectorsPerFrame = v;
}
