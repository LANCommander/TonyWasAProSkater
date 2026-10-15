/* Font_Reset__Fv (0x8004c180) */
#include "names.h"

extern int StringBuffer[];
extern char NumberStrings[];

void *BufferPos;
char *pNumberStrings;

void Font_Reset(void) {
	BufferPos = StringBuffer;
	StringBuffer[0] = 1;
	pNumberStrings = NumberStrings;
}
