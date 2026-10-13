/* Flash_Screen__FUcUcUciUci (0x8001f334) */
#include "names.h"

int FlashCountdown, FlashSort, CurrentR, CurrentG, CurrentB, dR, dG, dB;
unsigned char CurrentImportance;

void Flash_Screen(unsigned char r, unsigned char g, unsigned char b, int n, unsigned char c,
                  int d) {
	if (c >= CurrentImportance && n != 0) {
		unsigned R = r << 16, G = g << 16, B = b << 16;
		unsigned nn = n;

		CurrentR = R;
		CurrentG = G;
		CurrentB = B;
		FlashCountdown = n;
		CurrentImportance = c;
		FlashSort = d;
		dR = R / nn;
		dG = G / nn;
		dB = B / nn;
	}
}
