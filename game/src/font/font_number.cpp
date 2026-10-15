/* Font_Number__FUsUsiUcUcUcUc (0x8004c20c) flags=-fstructure-noalias */
#include "game/font.h"

extern SString *BufferPos;
extern uint CodeVal;
extern char *pNumberStrings;

void WriteNumber(int);

void Font_Number(ushort X, ushort Y, int Number, uchar r, uchar g, uchar b, uchar BigText) {
	BufferPos->End = 0;
	BufferPos->X = X;
	BufferPos->Y = Y;
	BufferPos->Is3d = 0;
	BufferPos->BigText = BigText;
	BufferPos->pString = pNumberStrings;
	BufferPos->CodeBGR = CodeVal | (b << 16) | (g << 8) | r;
	WriteNumber(Number);

	BufferPos++;
	BufferPos->End = 1;
}
