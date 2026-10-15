/* strNextVlc__Fi (0x80023edc) */
#include "names.h"
#include "pc_stream.h"

extern int currentVlcBuffer;
extern unsigned int *vlcBuffer[2];

unsigned int *strNext(void);
int DecDCTvlc3(unsigned int *, unsigned int *);
void FUN_8009144c(int);

int strNextVlc(int f) {
	unsigned int *s = strNext();

	if (!s)
		return 0;

	if (f)
		DecDCTvlc3(s, vlcBuffer[currentVlcBuffer]);

	PC_StreamFreeFrame(s);
	return f;
}
