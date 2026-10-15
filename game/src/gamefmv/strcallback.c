/* strCallback__Fv (0x80023d3c) */
#include "names.h"

typedef struct {
	short x, y, w, h;
} RECT;

extern unsigned int *decodeSliceBuffer[];
extern int StCdIntrFlag;

extern int currentDecodeBuffer, GameFMV_IsMovie;
extern unsigned char GameFMV_Is24Bit, GameFMV_SliceBuffers;
extern short GameFMV_StartX;
extern RECT decodeSlice;
extern unsigned short GameFMV_EndX;

void FUN_8008f79c(void);
void DecDCTout(unsigned int *, int);
void LoadImage(RECT *, unsigned int *);

void strCallback(void) {
	RECT r;
	unsigned int *p;

	return;

	r = decodeSlice;
	p = decodeSliceBuffer[currentDecodeBuffer];
	currentDecodeBuffer++;

	if (currentDecodeBuffer >= GameFMV_SliceBuffers)
		currentDecodeBuffer = 0;

	decodeSlice.x += ((unsigned short)decodeSlice.w);

	if (decodeSlice.x < GameFMV_EndX) {
		DecDCTout(decodeSliceBuffer[currentDecodeBuffer], (short)((unsigned short)decodeSlice.w) * decodeSlice.h / 2);
	} else {
		decodeSlice.x = GameFMV_StartX;

		if (GameFMV_IsMovie) {
			if (decodeSlice.y < 256)
				decodeSlice.y += 256;
			else
				decodeSlice.y -= 256;
		}
	}

	if (GameFMV_StartX != -1)
		LoadImage(&r, p);
}
