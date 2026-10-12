/* ProcessNewPSX__Fi (0x8006f290) flags=-fstructure-noalias */
/* C++ (SPOOL.CPP). Demo twin (0.97; retail drops the wibble flagging and the Mem_Shrink); locals
   and blocks from its MAIN.SYM. Fixes up a freshly loaded PSX file: relocates the model table,
   resolves the texture checksum table to texture entries (adding new ones), points textured
   faces at them, loads any new palettes, uploads the textures, then walks the trailing packets
   (rails, zones, anims, ...) and records where each one lives. GeometryAndTableSize keeps
   (NumChecksums*4 + 4) as one term: written plainly, fold moves the +4 onto the left sum; the
   assignment inside the expression stops it. */
#include "game/m3d.h"

extern SPSXRegion PSXRegion[];

uint *pCurrentTexWibPacket;

extern int Pack_NoBottom;
extern int Pack_NoRight;

uint *Spool_SkipPackets(uint *);
Texture *Spool_FindTextureEntry(uint);
void Spool_RemoveUnusedTextures(void);
void Pal_RemoveUnusedPalettes(void);
int Pal_FindPaletteEntry(uint);
void M3dInit_ParsePSX(int);
int Spool_GetEnvIndex(int);
void M3dZone_SetZone(int, uint *);
void PreProcessAnimPacket(uint *, uint *);

extern "C" {
	Texture *NewTextureEntry__FUi(uint);
	void Pal_LoadPalette__FUiPUiUci(uint, uint *, int);
	void Spool_LoadTexturesToVRAM__Fii(int, int);
	void FUN_8004d6a0(uint *, int);
}

void ProcessNewPSX(int region) {
	uint *pPSX;
	int NumItems;
	SModel **ppModels;
	uint NumModels;
	uint *ChecksumTable;
	uint NumChecksums;
	uint GeometryAndTableSize;
	uint *pPaletteInfo;
	uint Num16CPalettes;
	uint Num256CPalettes;
	uint *pPacket;
	int AnimPacketFound;
	int PacketType;

	pPSX = PSXRegion[region].pPSX;
	ppModels = (SModel **)(pPSX[2] * 36 + (int)pPSX) + 4;
	NumModels = ((uint *)ppModels)[-1];
	pCurrentTexWibPacket = 0;

	{
		int i;

		for (i = 0; i < NumModels; i++)
			ppModels[i] = (SModel *)((uint)ppModels[i] + (uint)pPSX);
	}

	PSXRegion[region].ppModels = ppModels;
	PSXRegion[region].pModelChecksums = Spool_SkipPackets(pPSX);
	ChecksumTable = PSXRegion[region].pModelChecksums + NumModels + 1;
	NumChecksums = ChecksumTable[-1];

	{
		int i;

		for (i = 0; i < NumChecksums; i++) {
			Texture *pTextureEntry = Spool_FindTextureEntry(ChecksumTable[i]);

			if (!pTextureEntry)
				pTextureEntry = NewTextureEntry__FUi(ChecksumTable[i]);

			pTextureEntry->Usage++;
			ChecksumTable[i] = (uint)pTextureEntry;
		}
	}

	{
		int m;

		for (m = 0; m < NumModels; m++) {
			SModel *pModel = ppModels[m];
			uint NumVertices = pModel->NumVertices;
			uint NumNormals = pModel->NumNormals;
			uint NumFaces = pModel->NumFaces;
			uint *pFace = (uint *)((char *)pModel + (NumVertices * 8 + 28) + NumNormals * 8);
			int j;

			for (j = 0; j < NumFaces; j++) {
				if (pFace[0] & 1)
					pFace[4] = ChecksumTable[pFace[4]];

				pFace = (uint *)((char *)pFace + (pFace[0] >> 18) * 4);
			}
		}
	}

	Spool_RemoveUnusedTextures();
	Pal_RemoveUnusedPalettes();

	{
		uint t;

		GeometryAndTableSize = ((char *)Spool_SkipPackets(pPSX) - (char *)pPSX) + NumModels * 4 +
		                       (t = NumChecksums * 4 + 4);
	}

	pPaletteInfo = (uint *)((char *)pPSX + GeometryAndTableSize);
	Num16CPalettes = *pPaletteInfo++;

	{
		int i;

		for (i = 0; i < Num16CPalettes; i++) {
			if (!Pal_FindPaletteEntry(*pPaletteInfo))
				Pal_LoadPalette__FUiPUiUci(*pPaletteInfo, pPaletteInfo + 1, 1);

			pPaletteInfo += 9;
		}
	}

	Num256CPalettes = *pPaletteInfo++;

	{
		int i;

		for (i = 0; i < Num256CPalettes; i++) {
			if (!Pal_FindPaletteEntry(*pPaletteInfo))
				Pal_LoadPalette__FUiPUiUci(*pPaletteInfo, pPaletteInfo + 1, 2);

			pPaletteInfo += 129;
		}
	}

	Pack_NoBottom = 0;
	Pack_NoRight = 0;

	if (PSXRegion[region].Filename[0] == 'b' && PSXRegion[region].Filename[1] == 'i' &&
	    PSXRegion[region].Filename[2] == 't' && PSXRegion[region].Filename[3] == 's' &&
	    !PSXRegion[region].Filename[4]) {
		Pack_NoBottom = 1;
		Pack_NoRight = 1;
	}

	PSXRegion[region].TexturesInRAM = 1;
	Spool_LoadTexturesToVRAM__Fii(region, 0);

	Pack_NoBottom = 0;
	Pack_NoRight = 0;

	if (!PSXRegion[region].KeepTexturesInRAM) {
		FUN_8004d6a0(pPSX, GeometryAndTableSize);

		PSXRegion[region].TexturesInRAM = 0;
	}

	AnimPacketFound = 0;
	M3dInit_ParsePSX(region);

	PSXRegion[region].IsSuper = 0;
	PSXRegion[region].pTexWibData = 0;
	PSXRegion[region].pColourPulseData = 0;
	pPacket = (uint *)((char *)pPSX + pPSX[1]);
	PacketType = *pPacket;

	while (PacketType != -1) {
		pPacket++;
		NumItems = *pPacket++;

		switch (PacketType) {
			case 10:
				M3dZone_SetZone(Spool_GetEnvIndex(region), pPacket);
				break;
			case 0x45:
				PreProcessAnimPacket(pPSX, pPacket);

				AnimPacketFound = 1;
				break;
			case 0x2a:
			case 0x2c:
				PSXRegion[region].pAnimFile = pPacket;
				PSXRegion[region].IsSuper = 1;
				break;
			case 0x73424752:
				PSXRegion[region].pColourTable = pPacket;
				break;
			case 6:
				PSXRegion[region].pTexWibData = pPacket;
				pCurrentTexWibPacket = pPacket - 2;
				break;
			case 7:
				PSXRegion[region].pColourPulseData = pPacket;
				break;
			case 0x52454948:
				PSXRegion[region].pHierarchy = (ushort *)pPacket;
				break;
		}

		pPacket = (uint *)((char *)pPacket + NumItems);
		PacketType = *pPacket;
	}

	pPacket++;

	if (AnimPacketFound)
		FUN_8004d6a0(pPSX, (char *)pPacket - (char *)pPSX);

	PSXRegion[region].Usable = 1;
}
