/* Spool_LoadTexturesToVRAM__Fii (0x8006f854) flags=-fstructure-noalias */
/* C++ (SPOOL.CPP). Fuzzy demo twin (0.84); locals from its MAIN.SYM. Walks a region's PSX file
   past the models, texture lookup, 36-byte and 516-byte tables to the texture records. For each
   texture not yet resident it finds VRAM space (Pack_Pack), uploads the image (8-bit or 4-bit),
   then fills the Texture entry: palette (ref-counted), tpage, UVs, texture-window command and the
   resident flag. The flag update goes through a multiply-set local, so sched1 doesn't treat it as a
   birthing insn and keeps the store last (demo line 1135, after Window). */
extern "C" {
	#include <sys/types.h>
	#include <libgte.h>
	#include <libgpu.h>
}

#include "game/m3d.h"

extern SPSXRegion PSXRegion[];

void Pack_DefragmentFully(void);
uint *Spool_SkipPackets(uint *);
tag_S_Pal *Pal_FindPaletteEntry(uint);

extern "C" {
	int Pack_Pack__FiiiPiT3iiUi(int, int, int, int *, int *, int, int, uint);
	void FUN_8008c714(RECT *, uint *);
	ushort FUN_8008f72c(int, int, int, int);
}

#ifdef PC_PORT
#include "pc_dctex.h"
#endif

void Spool_LoadTexturesToVRAM(int Region, int defrag) {
	uint *pPSX;
	uint *pStuff;
	int Temp;
	Texture **TextureLookup;
	uint *pTextureInfo;
	uint NumTextures;
	int i;

	if (defrag)
		Pack_DefragmentFully();

	pPSX = PSXRegion[Region].pPSX;
	pStuff = Spool_SkipPackets(pPSX);
	pStuff += ((uint *)PSXRegion[Region].ppModels)[-1];
	Temp = *pStuff++;
	TextureLookup = (Texture **)pStuff;
	pStuff += Temp;
	Temp = *pStuff++;
	pStuff += Temp * 9;
	Temp = *pStuff++;
	pStuff += Temp * 129;
	pTextureInfo = pStuff;
	NumTextures = *pTextureInfo++;

	for (i = 0; i < NumTextures; i++) {
		uint *pTexture = (uint *)((char *)pPSX + *pTextureInfo++);
		uint Flags = pTexture[0];
		uint NumColours = pTexture[1];
		uint PaletteChecksum = pTexture[2];
		Texture *pTextureEntry = TextureLookup[pTexture[3]];

		if (!pTextureEntry)
			continue;

		if (*(uint *)&pTextureEntry->Usage & 0xf0000)
			continue;

		if (pTextureEntry->Checksum == 0xcce0187c)
			Flags |= 1;

		if (pTextureEntry->Checksum == 0x68b7b136)
			Flags |= 1;

#ifdef PC_PORT
		/* a Dreamcast texture (version 6 file, platform/pc/include/pc_dctex.h): its image is on
		   the PC side, drawn by OpenGL. The entry gets the handle as its CLUT and a colour depth
		   3 page; its coordinates span the whole image (256 = one repeat) */
		if (NumColours == 65536) {
			ushort h = PC_HdTexFind(pTextureEntry->Checksum);

			PC_HdTexRef(h, 1);
			pTextureEntry->pPalette = 0;
			pTextureEntry->clut = h;
			pTextureEntry->tpage = PC_HD_TPAGE;
			pTextureEntry->x = 0;
			pTextureEntry->y = 0;
			pTextureEntry->u0 = 0;
			pTextureEntry->v0 = 0;
			pTextureEntry->u1 = 255;
			pTextureEntry->v1 = 0;
			pTextureEntry->u2 = 0;
			pTextureEntry->v2 = 255;
			pTextureEntry->u3 = 255;
			pTextureEntry->v3 = 255;
			pTextureEntry->TexWin = 0;

			{
				uint f = *(uint *)&pTextureEntry->Usage;

				f &= 0xfff0ffff;
				f |= 0x10000;
				*(uint *)&pTextureEntry->Usage = f;
			}
			continue;
		}
#endif

		{
			tag_S_Pal *pPal;
			ushort Width = *(ushort *)&pTexture[4];
			ushort Height = *((ushort *)&pTexture[4] + 1);
			uint *pTextureData = pTexture + 5;
			int x;
			int y;

			if (NumColours == 256) {
				int AlignedWidth = (Width + 1) & ~1;
				RECT rec;

				Pack_Pack__FiiiPiT3iiUi(0, AlignedWidth, Height, &x, &y, 8, (Flags & 1) ? 2 : 0,
				             pTextureEntry->Checksum);
				ushort sx = x;
				ushort sy = y;

				pTextureEntry->x = sx;
				pTextureEntry->y = sy;
				rec.x = sx;
				rec.y = sy;
				rec.w = AlignedWidth / 2;
				rec.h = Height;
				FUN_8008c714(&rec, pTextureData);
			}

			if (NumColours == 16) {
				int AlignedWidth = (Width + 3) & ~3;
				RECT rec;

				Pack_Pack__FiiiPiT3iiUi(0, AlignedWidth, Height, &x, &y, 4, (Flags & 1) ? 2 : 0,
				             pTextureEntry->Checksum);
				ushort sx = x;
				ushort sy = y;

				pTextureEntry->x = sx;
				pTextureEntry->y = sy;
				rec.x = sx;
				rec.y = sy;
				rec.w = AlignedWidth / 4;
				rec.h = Height;
				FUN_8008c714(&rec, pTextureData);
			}

			pPal = Pal_FindPaletteEntry(PaletteChecksum);
			pPal->Usage++;
			pTextureEntry->pPalette = pPal;
			pTextureEntry->clut = pPal->Clut;
			pTextureEntry->tpage = FUN_8008f72c(NumColours == 256, 0, x, y);

			{
				int t = x & 63;
				uchar u;
				uchar v;

				if (NumColours == 256)
					u = t << 1;
				else
					u = t << 2;

				pTextureEntry->u0 = u;
				v = y;
				pTextureEntry->v0 = v;
				pTextureEntry->u1 = u + Width - 1;
				pTextureEntry->v1 = v;
				pTextureEntry->u2 = u;
				pTextureEntry->v2 = v + Height - 1;
				pTextureEntry->u3 = u + Width - 1;
				pTextureEntry->v3 = v + Height - 1;
				pTextureEntry->TexWin = ((v & 0xf8) << 12) | 0xe2000000 | ((u & 0xf8) << 7) |
				                        ((-Height & 0xf8) << 2) | ((-Width & 0xf8) >> 3);

				{
					uint f = *(uint *)&pTextureEntry->Usage;

					f &= 0xfff0ffff;
					f |= 0x10000;
					*(uint *)&pTextureEntry->Usage = f;
				}
			}
		}
	}
}
