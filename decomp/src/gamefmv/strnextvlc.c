/* strNextVlc__Fi (0x80023edc) */
#include "names.h"

int currentVlcBuffer;
int vlcBuffer[2];

int strNext(void);
void DecDCTvlc3(int, int);
void FUN_8009144c(int);

int strNextVlc(int f) {
	int s = strNext();

	if (!s)
		return 0;

	if (f)
		DecDCTvlc3(s, vlcBuffer[currentVlcBuffer]);

	FUN_8009144c(s);
	return f;
}
