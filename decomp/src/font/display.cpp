/* Font_Display__Fv (0x8004c290) */
/* C++ (FONT.CPP). Fuzzy demo twin (tools/fuzzy_twins.py); locals, blocks and the SKIP label from its
   MAIN.SYM. Draws every queued string as POLY_FT4 letters, projecting 3D strings with the GTE. The
   four UV copies come before Width/Height so CSE keeps the u1 byte for the BigText `u1--`. */
extern "C" {
	#include <sys/types.h>
	#include <libgte.h>
	#include <libgpu.h>
	#include <inline_c.h>
}

#include "game/font.h"
#include "game/camera.h"
#include "game/m3d.h"

extern SCamera MikeCamera[];
extern int Camera_CurrentMikeCamera;
extern SString StringBuffer[];
extern uchar Mapping[];

SAnimFrame *FontAnim;

extern char *pPoly;
extern char *PolyBufferEnd;
extern SViewport *pCurrentViewport;

extern SDoubleBuffer *pDoubleBuffer;

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

	gte_SetRotMatrix(&MikeCamera[Camera_CurrentMikeCamera].View);
	__asm__ volatile("ctc2 $0, $5; ctc2 $0, $6; ctc2 $0, $7");

	camera_pos.vx = MikeCamera[Camera_CurrentMikeCamera].Position.vx << 12;
	camera_pos.vy = MikeCamera[Camera_CurrentMikeCamera].Position.vy << 12;
	camera_pos.vz = MikeCamera[Camera_CurrentMikeCamera].Position.vz << 12;

	for (pStringInfo = StringBuffer; !pStringInfo->End; pStringInfo++) {
		long zed;

		CurrentChar = pStringInfo->pString;

		if (pStringInfo->Is3d) {
			CVector Temp;
			uint y0x0;

			Temp = (pStringInfo->Pos - camera_pos) >> 12;
			gte_ldlv0(&Temp);
			gte_rtps();

			ColourInfo = pStringInfo->CodeBGR;
			gte_stlvnl2(&zed);

			if (zed < pCurrentViewport->Hither || zed > pCurrentViewport->Yon)
				goto SKIP;

			gte_stsxy2(&y0x0);

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
				apoly->tag = 0x09000000;
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
					u_long *ot = (u_long *)((zed & 0x3ffc) + (int)pDoubleBuffer);

					addPrim(&ot[35], apoly);
				} else {
					u_long *ot = &pDoubleBuffer->OT2D;

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
