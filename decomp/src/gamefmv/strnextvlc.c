/* strNextVlc__Fi (0x80023edc) */
#include "names.h"
#ifdef PC_PORT
#include "pc_stream.h"
#endif

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

#ifdef PC_PORT
	PC_StreamFreeFrame((void *)s);
#else
	FUN_8009144c(s);
#endif
	return f;
}
