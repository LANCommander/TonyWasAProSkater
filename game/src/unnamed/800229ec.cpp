/* GameFMV_SetScreenTexture__FUi (0x800229ec) */
/* C++ (GAMEFMV.CPP area, after the MDEC decoder). Looks up a texture by checksum and, unless
   GameFMV_Active is set or GGame == 7, clears its VRAM rectangle (the FMV frame
   size, GameFMV_Width x GameFMV_Height) to grey, records its position, and hands the 16-bit tpage
   to GameFMV_RetargetScreenFaces. The arg is passed straight through in a0. */
extern "C" {
	#include <sys/types.h>
	#include <libgte.h>
	#include <libgpu.h>
}

#include "game/gamefmv.h"
#include "game/m3d.h"
#include "pc_texstore.h"

extern int GameFMV_Active;
extern short GameFMV_StartX;
extern short GameFMV_StartY;
extern short GameFMV_EndX;
extern ushort GameFMV_Width;
extern ushort GameFMV_Height;
extern uchar GameFMV_Is24Bit;

extern int GGame;

Texture *Spool_FindTextureEntry(uint Checksum);

void GameFMV_SetScreenTexture(uint Checksum) {
	Texture *pTex;
	RECT r;

	GameFMV_StartX = -1;

	if (GameFMV_Active == 0 && GGame != 7) {
		pTex = Spool_FindTextureEntry(Checksum);

		if (pTex) {
			ushort x = pTex->x;
			ushort y = pTex->y;

			GameFMV_StartX = x;
			GameFMV_StartY = y;
			r.x = x;
			r.w = GameFMV_Width;
			r.h = GameFMV_Height;
			r.y = y;
			PC_TexFill(r.x, r.y, r.w, r.h, 0x080808);

			GameFMV_EndX =
			    GameFMV_StartX + (GameFMV_Is24Bit ? (GameFMV_Width * 3) >> 1 : GameFMV_Width);
			GameFMV_RetargetScreenFaces(getTPage(2, 0, GameFMV_StartX, GameFMV_StartY), pTex);
		}
	}
}
