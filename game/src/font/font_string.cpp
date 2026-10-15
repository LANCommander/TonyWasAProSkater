/* Font_String__FUsUsPcUcUcUcUc (0x8004c1a8) flags=-fstructure-noalias */
#include "game/font.h"

extern SString *BufferPos;
extern uint CodeVal;

void Font_String(ushort X, ushort Y, char *pString, uchar r, uchar g, uchar b, uchar BigText) {
	BufferPos->End = 0;
	BufferPos->X = X;
	BufferPos->Y = Y;
	BufferPos->Is3d = 0;
	BufferPos->BigText = BigText;
	BufferPos->pString = pString;
	BufferPos->CodeBGR = CodeVal | (b << 16) | (g << 8) | r;
	BufferPos++;
	BufferPos->End = 1;
}
