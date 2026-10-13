/* frug__FUi (0x800782b4) */
#include "names.h"

unsigned CheatCheck;

int arccos(void);

void frug(unsigned n) {
	unsigned v = CheatCheck ^ n;

	CheatCheck = ((v << 1) ^ (v >> 31)) * 521;
	CheatCheck += arccos();
}
