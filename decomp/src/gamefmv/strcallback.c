/* strCallback__Fv (0x80023d3c) */
#include "names.h"

typedef struct {
	short x, y, w, h;
} RECT;

extern int decodeSliceBuffer[];
extern int StCdIntrFlag;

int currentDecodeBuffer, GameFMV_IsMovie;
unsigned char GameFMV_Is24Bit, GameFMV_SliceBuffers;
short GameFMV_StartX, decodeSlice_2, decodeSlice_6;
RECT decodeSlice;
unsigned short GameFMV_EndX, decodeSlice_4;

void FUN_8008f79c(void);
void FUN_8009033c(int, int);
void FUN_8008c714(RECT *, int);

void strCallback(void) {
	RECT r;
	int p;

	if (GameFMV_Is24Bit && StCdIntrFlag) {
		FUN_8008f79c();

		StCdIntrFlag = 0;
	}

	r = decodeSlice;
	p = decodeSliceBuffer[currentDecodeBuffer];
	currentDecodeBuffer++;

	if (currentDecodeBuffer >= GameFMV_SliceBuffers)
		currentDecodeBuffer = 0;

	decodeSlice.x += decodeSlice_4;

	if (decodeSlice.x < GameFMV_EndX) {
		FUN_8009033c(decodeSliceBuffer[currentDecodeBuffer],
		             (short)decodeSlice_4 * decodeSlice_6 / 2);
	} else {
		decodeSlice.x = GameFMV_StartX;

		if (GameFMV_IsMovie) {
			if (decodeSlice_2 < 256)
				decodeSlice_2 += 256;
			else
				decodeSlice_2 -= 256;
		}
	}

	if (GameFMV_StartX != -1)
		FUN_8008c714(&r, p);
}
