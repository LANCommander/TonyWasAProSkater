/* DecompressStream__FPUcPsii (0x8001393c) */
/* C++ (DECOMP.CPP). Locals, types and scopes from the demo's MAIN.SYM. */
#include "game/types.h"

uchar *DecompressStream(uchar *pSource, short *pDest, int StepSize, int StreamLength) {
	uchar Type;
	int Interval;
	int Segs;
	int RemainderLength;
	int Bits;
	int Val;
	int Off;

	Type = *pSource++;
	Interval = (Type >> 4) + 1;
	Type &= 15;

	if (Interval >= 2) {
		Segs = (StreamLength - 1) / Interval;
		RemainderLength = (StreamLength - 1) - Segs * Interval;
	} else {
		Segs = StreamLength - 1;
		RemainderLength = 0;
	}

	switch (Type) {
		case 0: {
			short v = *pSource++;

			v += *pSource++ << 8;
			*pDest = v;
			pDest += StepSize;

			for (int i = 0; i < Segs; i++) {
				short NewVal = *pSource++;

				NewVal += *pSource++ << 8;
				short Inc = (NewVal - v) / Interval;

				for (int j = 0; j < Interval - 1; j++) {
					v += Inc;
					*pDest = v;
					pDest += StepSize;
				}

				v = NewVal;
				*pDest = v;
				pDest += StepSize;
			}

			if (RemainderLength) {
				short NewVal = *pSource++;

				NewVal += *pSource++ << 8;
				short Inc = (NewVal - v) / RemainderLength;

				for (int j = 0; j < RemainderLength - 1; j++) {
					v += Inc;
					*pDest = v;
					pDest += StepSize;
				}

				*pDest = NewVal;
			}

			break;
		}

		case 1:
		case 2:
		case 3:
		case 4:
		case 5:
		case 6:
		case 7:
		case 8:
		case 9:
		case 10:
		case 11:
		case 12:
		case 13: {
			Val = *pSource++;
			Val += *pSource++ << 8;
			Off = 0;
			Bits = Type + 1;
			*pDest = Val;
			pDest += StepSize;

			for (int i = 0; i < Segs; i++) {
				int k;
				short NewVal;
				int d;
				short Inc;

				k = pSource[0];
				k = (k << 8) | pSource[1];
				k = (k << 8) | pSource[2];
				k <<= Off + 8;
				k >>= 32 - Bits;
				NewVal = Val + k;
				d = Off + Bits;
				pSource += d >> 3;
				Off = d & 7;
				Inc = k / Interval;

				for (int j = 0; j < Interval - 1; j++) {
					Val += Inc;
					*pDest = Val;
					pDest += StepSize;
				}

				Val = NewVal;
				*pDest = Val;
				pDest += StepSize;
			}

			if (RemainderLength) {
				int k;
				short NewVal;
				int d;
				short Inc;

				k = pSource[0];
				k = (k << 8) | pSource[1];
				k = (k << 8) | pSource[2];
				k <<= Off + 8;
				k >>= 32 - Bits;
				NewVal = Val + k;
				d = Off + Bits;
				pSource += d >> 3;
				Off = d & 7;
				Inc = k / RemainderLength;

				for (int j = 0; j < RemainderLength - 1; j++) {
					Val += Inc;
					*pDest = Val;
					pDest += StepSize;
				}

				*pDest = NewVal;
			}

			if (Off)
				pSource++;

			break;
		}

		case 14:
			Val = *pSource++;
			Val += *pSource++ << 8;

			for (int i = 0; i < StreamLength; i++) {
				*pDest = Val;
				pDest += StepSize;
			}

			break;

		case 15:
			for (int i = 0; i < StreamLength; i++) {
				*pDest = 0;
				pDest += StepSize;
			}

			break;
	}

	return pSource;
}
