/* Font_Reset__Fv (0x8004c180) */
#include "names.h"

extern int StringBuffer[];
extern char NumberStrings[];

int BufferPos, pNumberStrings;

void Font_Reset(void) {
	BufferPos = (int)StringBuffer;
	StringBuffer[0] = 1;
	pNumberStrings = (int)NumberStrings;
}
