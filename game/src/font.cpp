/* FONT.CPP: the queued debug strings (Font_String / Font_Number, drawn by Font_Display; SString in
   game/font.h). */

extern "C" {
	#include <sys/types.h>
	#include <libgte.h>
	#include <libgpu.h>
}

#include "game/font.h"
#include "game/camera.h"
#include "game/m3d.h"
#include "game/bit.h"
#include "game/globals.h"
#include "game/m3dasm.h"
#include "game/spool.h"

extern char *pNumberStrings;
extern SAnimFrame *FontAnim;
extern char NumberStrings[];
extern SString *BufferPos;
extern uint CodeVal;
void WriteNumber(int);
extern SString StringBuffer[];
extern uchar Mapping[];

/* WriteNumber__Fi (0x8004c070) */
void WriteNumber(int n) {
	int started = 0;
	int d, q;

	if (n < 0) {
		n = -n;
		*pNumberStrings++ = '-';
	}

	for (d = 1000000000; d > 0; d /= 10) {
		if (n >= d) {
			q = n / d;
			started = 1;
			*pNumberStrings++ = q + '0';
			n -= q * d;
		} else if (started || d == 1) {
			*pNumberStrings++ = '0';
		}
	}

	*pNumberStrings++ = 0;
}

/* Font_Init__Fv (0x8004c14c) */
void Font_Init(void) {
	FontAnim = (SAnimFrame *)Spool_FindAnim("Fontsmll");
	BigFontAnim = (SAnimFrame *)Spool_FindAnim("Font");
}

/* Font_Reset__Fv (0x8004c180) */
void Font_Reset(void) {
	BufferPos = StringBuffer;
	StringBuffer[0].End = 1;
	pNumberStrings = NumberStrings;
}

/* Font_String__FUsUsPcUcUcUcUc (0x8004c1a8) */
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

/* Font_Number__FUsUsiUcUcUcUc (0x8004c20c) */
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

/* Font_Display__Fv (0x8004c290) */
/* Fuzzy demo twin (tools/fuzzy_twins.py); locals, blocks and the SKIP label from its
   MAIN.SYM. Draws every queued string as POLY_FT4 letters, projecting 3D strings with the GTE. The
   four UV copies come before Width/Height so CSE keeps the u1 byte for the BigText `u1--`. */
void Font_Display() {
	SString *pStringInfo;
	char *CurrentChar;
	POLY_FT4 *apoly;
	int LetterFrame;
	int Width;
	int Height;
	int LetterX;
	int LetterY;
	CVector camera_pos;
	uint ColourInfo;

	geom_ld_mat(pc_geom.rot, &MikeCamera[Camera_CurrentMikeCamera].View);
	geom_load_vector(pc_geom.tr, 0, 0, 0);

	camera_pos.vx = MikeCamera[Camera_CurrentMikeCamera].Position.vx << 12;
	camera_pos.vy = MikeCamera[Camera_CurrentMikeCamera].Position.vy << 12;
	camera_pos.vz = MikeCamera[Camera_CurrentMikeCamera].Position.vz << 12;

	for (pStringInfo = StringBuffer; !pStringInfo->End; pStringInfo++) {
		int zed;

		CurrentChar = pStringInfo->pString;

		if (pStringInfo->Is3d) {
			CVector Temp;
			uint y0x0;
			GeomProj proj;

			Temp = (pStringInfo->Pos - camera_pos) >> 12;
			proj = geom_project_lv(&Temp);

			ColourInfo = pStringInfo->CodeBGR;
			zed = proj.v.mac[2];

			if (zed < pCurrentViewport->Hither || zed > pCurrentViewport->Yon)
				goto SKIP;

			y0x0 = proj.sxy;

			LetterX = y0x0 & 0xffff;
			LetterY = y0x0 >> 16;
		} else {
			LetterX = pStringInfo->X;
			LetterY = pStringInfo->Y;
			ColourInfo = pStringInfo->CodeBGR;
		}

		while (*CurrentChar) {
			char letter = *CurrentChar++;

			LetterFrame = Mapping[letter & 0x7f];

			if (letter == '_')
				LetterFrame = Mapping[45];

			if (LetterFrame != 255) {
				SAnimFrame *pFrame = &FontAnim[LetterFrame];
				Texture *stored = pFrame->pTexture;
				uchar *newppoly;

				Width = pFrame->Width;

				if (Width < 4)
					LetterX++;

				apoly = (POLY_FT4 *)pPoly;
				newppoly = (uchar *)apoly + sizeof(POLY_FT4);

				if ((uchar *)PolyBufferEnd < newppoly)
					return;

				pPoly = (char *)newppoly;
				setlen(apoly, 9);
				*(uint *)&apoly->r0 = ColourInfo;
				*(uint *)&apoly->u0 = *(uint *)&stored->u0;
				*(uint *)&apoly->u1 = *(uint *)&stored->u1;
				*(uint *)&apoly->u2 = *(uint *)&stored->u2;
				*(ushort *)&apoly->u3 = *(ushort *)&stored->u3;
				Width = apoly->u1 - apoly->u0;
				Height = apoly->v2 - apoly->v0;

				if (pStringInfo->BigText) {
					Width = Width * 55 >> 5;
					Height = Height * 55 >> 5;
					apoly->u1--;
					apoly->u3--;
					apoly->v2--;
					apoly->v3--;
				}

				if (LetterFrame == 30) {
					if (letter == '_') {
						apoly->x0 = LetterX;
						apoly->y0 = LetterY + 5;
						apoly->x1 = LetterX + Width;
						apoly->y1 = LetterY + 5;
						apoly->x2 = LetterX;
						apoly->y2 = LetterY + 5 + Height;
						apoly->x3 = LetterX + Width;
						apoly->y3 = LetterY + 5 + Height;
					} else {
						apoly->x0 = LetterX;
						apoly->y0 = LetterY + 2;
						apoly->x1 = LetterX + Width;
						apoly->y1 = LetterY + 2;
						apoly->x2 = LetterX;
						apoly->y2 = LetterY + 2 + Height;
						apoly->x3 = LetterX + Width;
						apoly->y3 = LetterY + 2 + Height;
					}
				} else {
					apoly->x0 = LetterX;
					apoly->y0 = LetterY;
					apoly->x1 = LetterX + Width;
					apoly->y1 = LetterY;
					apoly->x2 = LetterX;
					apoly->y2 = LetterY + Height;
					apoly->x3 = LetterX + Width;
					apoly->y3 = LetterY + Height;
				}

				if (pStringInfo->Is3d) {
					addPrim(&pDoubleBuffer->OT3D[(zed & 0x3ffc) >> 2], apoly);
				} else {
					P_TAG *ot = &pDoubleBuffer->OT2D;

					addPrim(ot, apoly);
				}

				LetterX += Width + 3;

				if (Width < 4)
					LetterX++;
			} else {
				LetterX += 5;
			}
		}

	SKIP:;
	}
}

/* FUN_8004c724 (0x8004c724) */
extern "C" void FUN_8004c724(void) {
	int i;

	for (i = 99; i != -1; i--)
		;
}
