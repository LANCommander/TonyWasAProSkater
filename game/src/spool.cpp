/* SPOOL.CPP: PSX file spooling: the load queue, region records and their textures (the texture
   checksum hash table, VRAM upload), environment item lists and animation packets
   (declarations in game/spool.h; the data structs in game/m3d.h). */
extern "C" {
	#include <sys/types.h>
	#include <libgte.h>
	#include <libgpu.h>
}

#include "game/spool.h"
#include "game/psyq.h"
#include "game/m3d.h"
#include "game/mem.h"
#include "game/ob.h"
#include "pc_dctex.h"
#include "game/pal.h"
#include "game/pack.h"
#include "pc_texstore.h"
#include "game/fileio.h"
#include "game/globals.h"
#include "game/m3dcolij.h"
#include "game/m3dinit.h"
#include "game/m3dzone.h"
#include "game/mess.h"
#include "game/shellutils.h"
#include "game/utils.h"

extern SQueue SpoolQ[20];
extern Texture *TextureChecksumHashTable[512];
extern int SpoolPos;
extern int FreePos;
extern AnimPacket *AnimPackets;
extern int DAT_800d1ed8;
extern int DAT_800d1edc;
extern uint *Memories[];
extern int HashIndex;
extern Texture *pCurrentTex;
extern uint *pCurrentTexWibPacket;
extern uint *pTexWibPacket[2];
extern int DAT_800d1f08;
uint *Spool_SkipPackets(uint *);
void GotoStartOfTextureList(void);
Texture *NextTexture(void);
void RemoveTextureEntry(Texture *);
void Spool_RemoveUnusedTextures(void);
void PreProcessAnimPacket(Texture **, uint *);
void DecrementTextureUsage(int);
void DetachOldEnviro(uchar);
void LoadNameExt(char *, char *);
void AttachNewEnviro(int);

/* Spool_Init__Fv (0x8006ec14) */
void Spool_Init() {
	int i;

	for (i = 0; i < 20; i++)
		SpoolQ[i].Occupied = 0;

	SpoolPos = 0;
	FreePos = 0;

	for (i = 0; i < 40; i++) {
		PSXRegion[i].Filename[0] = 0;
		PSXRegion[i].Protected = 0;
		PSXRegion[i].Usable = 0;
		free(PSXRegion[i].ppModels);
		free(PSXRegion[i].ppTextures);
		PSXRegion[i].ppModels = 0;
		PSXRegion[i].ppTextures = 0;
		free(PSXRegionItems[i]);
		PSXRegionItems[i] = 0;
		PSXRegion[i].pPSX = 0;
		PSXRegion[i].pAnimFile = 0;
		PSXRegion[i].pHierarchy = 0;
		PSXRegion[i].pColourPulseData = 0;
		PSXRegion[i].pTexWibData = 0;
		PSXRegion[i].pHooks = 0;
		PSXRegion[i].TexturesInRAM = 0;
		PSXRegion[i].KeepTexturesInRAM = 0;
	}

	for (i = 0; i < 512; i++)
		TextureChecksumHashTable[i] = 0;

	AnimPackets = 0;
	EnvRegions[0] = -1;
	EnvRegions[1] = -1;
	DAT_800d1ed8 = 0;
	DAT_800d1edc = 0;
	EnviroList = 0;
	OtherEnvIndex = 0;
}

/* A Dreamcast model (version 6, platform/pc/include/pc_dctex.h) is bigger than the 30000 bytes
   the front end keeps a copy of: its whole heap block */
static int CopyWords(uint *p) {
	uint n = MEM_INFO((char *)p - MEM_HEADER) >> 4;

	return (p[0] & 0xffff) == 6 && n > 30000 && n < 0x800000 ? (int)(n / 4) : 7500;
}

/* Spool_BackupPSX__Fi (0x8006ed2c) */
/* Copy of Spool_RestorePSX in the other direction; the loop keeps its entry jump, which comes from
   a goto loop (cc1 rotates while/for loops). */
void Spool_BackupPSX(int i) {
	uint *s = PSXRegion[i].pPSX;
	int n = CopyWords(s);
	uint *d = (uint *)Mem_NewX(n * 4, 1, 1);
	int j = 0;

	Memories[i] = d;
	goto test;
body:
	*d++ = *s++;
	j++;
test:
	if (j < n)
		goto body;
}

/* Spool_RestorePSX__Fi (0x8006edb8) */
void Spool_RestorePSX(int i) {
	uint *d = PSXRegion[i].pPSX;
	uint *s = Memories[i];
	int n = CopyWords(s);

	if (CopyWords(d) < n)
		n = CopyWords(d);

	do {
		int j = 0;

		goto test;
	body:
		*d++ = *s++;
	test:
		if (j < n) {
			j++;
			goto body;
		}
	} while (0);

	Mem_Delete(Memories[i]);

	Memories[i] = 0;
}

/* Spool_GetModel__FUii (0x8006ee38) */
/* (retail read the count from before the file's model table, which ppModels pointed at; the
   port's ppModels is an array of its own, the count is NumModels) */
int Spool_GetModel(uint id, int region) {
	uint n = PSXRegion[region].NumModels;
	uint *m = PSXRegion[region].pModelChecksums;
	uint i;

	for (i = 0; i < n; i++) {
		if (*m == id)
			return i;

		m++;
	}
}

/* Spool_FindEnviroItem__FUi (0x8006ee88) */
Item *Spool_FindEnviroItem(uint id) {
	Item *e = EnviroList;
	int cur = -1;
	uint *tbl;

	while (e) {
		if (e->Region != cur) {
			cur = e->Region;
			tbl = PSXRegion[cur].pModelChecksums;
		}

		if (tbl[e->Model] == id)
			return e;

		e = e->NextItem;
	}

	return 0;
}

/* FUN_8006ef00 (0x8006ef00) */
/* C++: an item's model checksum (its region's model checksum table). */
extern "C" int FUN_8006ef00(CItem *p) {
	return PSXRegion[p->mRegion].pModelChecksums[p->mModel];
}

/* NewTextureEntry__FUi (0x8006ef30) */
/* Demo twin; locals from its MAIN.SYM. Allocates a texture entry for a checksum
   and links it at the head of its hash bucket (low 9 bits of the checksum). */
Texture *NewTextureEntry(uint checksum) {
	Texture *pNew = (Texture *)Mem_NewX(sizeof(Texture), 0, 1);
	uint Lower = checksum & 511;

	pNew->pNext = TextureChecksumHashTable[Lower];
	pNew->pPrevious = 0;
	TextureChecksumHashTable[Lower] = pNew;

	if (pNew->pNext)
		pNew->pNext->pPrevious = pNew;

	pNew->Checksum = checksum;
	pNew->InVRAM = 0;
	pNew->Usage = 0;
	return pNew;
}

/* RemoveTextureEntry__FP7Texture (0x8006efb8) */
void RemoveTextureEntry(Texture *p) {
	Texture **s;

	if (p->pNext)
		p->pNext->pPrevious = p->pPrevious;

	if (p->pPrevious)
		p->pPrevious->pNext = p->pNext;

	s = &TextureChecksumHashTable[p->Checksum & 511];

	if (p == *s)
		*s = p->pNext;

	Mem_Delete(p);
}

/* Spool_FindTextureEntry__FUi (0x8006f048) */
Texture *Spool_FindTextureEntry(uint id) {
	Texture *t = TextureChecksumHashTable[id & 511];

	while (t && t->Checksum != id)
		t = t->pNext;

	return t;
}

/* GotoStartOfTextureList__Fv (0x8006f094) */
void GotoStartOfTextureList(void) {
	HashIndex = 0;
	pCurrentTex = TextureChecksumHashTable[0];
}

/* NextTexture__Fv (0x8006f0ac) */
/* C++. Walks the texture checksum hash table, one texture per call. */
Texture *NextTexture(void) {
	Texture *t;

	if (HashIndex >= 512)
		return 0;

	t = pCurrentTex;

	if (t) {
	again:
		pCurrentTex = t->pNext;
		return t;
	}

	do {
		/* (retail read the entry past the table's end before this test; the value was never used:
		   GotoStartOfTextureList sets both again) */
		if (++HashIndex >= 512) {
			pCurrentTex = 0;
			return 0;
		}

		pCurrentTex = t = TextureChecksumHashTable[HashIndex];
	} while (!t);

	goto again;
}

/* Spool_SkipPackets__FPUi (0x8006f130) */
unsigned *Spool_SkipPackets(unsigned *p) {
	unsigned *q = (unsigned *)((char *)p + p[1]);

	while (*q != -1) {
		unsigned s;

		q++;
		s = *q++;
		q = (unsigned *)((char *)q + s);
	}

	return q + 1;
}

/* DecrementTextureUsage__Fi (0x8006f178) */
/* Demo twin (the demo version also skips regions of type 2); locals from its
   MAIN.SYM. Walks the region's texture table (the port keeps it as entries, ProcessNewPSX) and
   drops each texture's usage count. */
void DecrementTextureUsage(int region) {
	Texture **ppTextures = PSXRegion[region].ppTextures;
	uint NumChecksums = PSXRegion[region].NumTextures;

	{
		int i;

		for (i = 0; i < NumChecksums; i++)
			ppTextures[i]->Usage--;
	}
}

/* Spool_RemoveUnusedTextures__Fv (0x8006f1f8) */
void Spool_RemoveUnusedTextures(void) {
	Texture *t;

	GotoStartOfTextureList();

	while ((t = NextTexture())) {
		/* a Dreamcast texture (pc_dctex.h): no palette or VRAM area; clut = handle, tpage
		   PC_HD_TPAGE */
		if ((*(uint *)&t->Usage & 0xf0000) && !t->Usage && PC_HD_IS_TPAGE(t->tpage)) {
			PC_HdTexRef(t->clut, -1);
			RemoveTextureEntry(t);
			continue;
		}
		if ((*(uint *)&t->Usage & 0xf0000) && !t->Usage) {
			t->pPalette->Usage--;
			Pack_Unpack(t->x, t->y);
			RemoveTextureEntry(t);
		}
	}
}

/* ProcessNewPSX__Fi (0x8006f290) */
/* Demo twin (0.97; retail drops the wibble flagging and the Mem_Shrink); locals
   and blocks from its MAIN.SYM. Fixes up a freshly loaded PSX file: relocates the model table,
   resolves the texture checksum table to texture entries (adding new ones), points textured
   faces at them, loads any new palettes, uploads the textures, then walks the trailing packets
   (rails, zones, anims, ...) and records where each one lives. GeometryAndTableSize keeps
   (NumChecksums*4 + 4) as one term: written plainly, fold moves the +4 onto the left sum; the
   assignment inside the expression stops it. */
void ProcessNewPSX(int region) {
	uint *pPSX;
	int NumItems;
	uint *pModelTable;
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
	/* the file's model table: offsets from its start, the count before them. Retail turned the
	   offsets into pointers in place; the port keeps an array of its own (and frees the last) */
	pModelTable = (uint *)((char *)pPSX + pPSX[2] * 36) + 4;
	NumModels = pModelTable[-1];
	pCurrentTexWibPacket = 0;
	free(PSXRegion[region].ppModels);
	ppModels = (SModel **)malloc((NumModels + 1) * sizeof *ppModels);

	{
		int i;

		for (i = 0; i < NumModels; i++)
			ppModels[i] = (SModel *)((char *)pPSX + pModelTable[i]);
	}

	PSXRegion[region].ppModels = ppModels;
	PSXRegion[region].NumModels = NumModels;
	PSXRegion[region].pModelChecksums = Spool_SkipPackets(pPSX);
	ChecksumTable = PSXRegion[region].pModelChecksums + NumModels + 1;
	NumChecksums = ChecksumTable[-1];

	{
		/* the texture table: retail turned the checksums into Texture pointers in place, and
		   pointed textured faces at them; the port's faces keep the index (M3dInit_ParsePSX) */
		Texture **ppTextures;
		int i;

		free(PSXRegion[region].ppTextures);
		ppTextures = (Texture **)malloc((NumChecksums + 1) * sizeof *ppTextures);

		for (i = 0; i < NumChecksums; i++) {
			Texture *pTextureEntry = Spool_FindTextureEntry(ChecksumTable[i]);

			if (!pTextureEntry)
				pTextureEntry = NewTextureEntry(ChecksumTable[i]);

			pTextureEntry->Usage++;
			ppTextures[i] = pTextureEntry;
		}

		PSXRegion[region].ppTextures = ppTextures;
		PSXRegion[region].NumTextures = NumChecksums;
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
				Pal_LoadPalette(*pPaletteInfo, pPaletteInfo + 1, 1);

			pPaletteInfo += 9;
		}
	}

	Num256CPalettes = *pPaletteInfo++;

	{
		int i;

		for (i = 0; i < Num256CPalettes; i++) {
			if (!Pal_FindPaletteEntry(*pPaletteInfo))
				Pal_LoadPalette(*pPaletteInfo, pPaletteInfo + 1, 2);

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
	Spool_LoadTexturesToVRAM(region, 0);

	Pack_NoBottom = 0;
	Pack_NoRight = 0;

	if (!PSXRegion[region].KeepTexturesInRAM) {
		Mem_Shrink(pPSX, GeometryAndTableSize);

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
				PreProcessAnimPacket(PSXRegion[region].ppTextures, pPacket);

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
		Mem_Shrink(pPSX, (char *)pPacket - (char *)pPSX);

	PSXRegion[region].Usable = 1;
}

/* AttachNewEnviro__Fi (0x8006f7a4) */
void AttachNewEnviro(int region) {
	/* the region's items (M3dInit_ParsePSX) go before the list's */
	if (!EnviroList) {
		EnviroList = PSXRegionItems[region];
	} else {
		Item *head = PSXRegionItems[region];
		Item *p = head;

		while (p->NextItem)
			p = p->NextItem;

		p->NextItem = EnviroList;
		EnviroList = head;
	}

	pTexWibPacket[Spool_GetEnvIndex(region)] = pCurrentTexWibPacket;
}

/* Spool_LoadTexturesToVRAM__Fii (0x8006f854) */
/* Fuzzy demo twin (0.84); locals from its MAIN.SYM. Walks a region's PSX file
   past the models, texture lookup, 36-byte and 516-byte tables to the texture records. For each
   texture not yet resident it finds VRAM space (Pack_Pack), uploads the image (8-bit or 4-bit),
   then fills the Texture entry: palette (ref-counted), tpage, UVs, texture-window command and the
   resident flag. The flag update goes through a multiply-set local, so sched1 doesn't treat it as a
   birthing insn and keeps the store last (demo line 1135, after Window). */
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
	pStuff += PSXRegion[Region].NumModels;
	Temp = *pStuff++;
	TextureLookup = PSXRegion[Region].ppTextures; /* (the file's texture table, as entries) */
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

				Pack_Pack(0, AlignedWidth, Height, &x, &y, 8, (Flags & 1) ? 2 : 0,
				          pTextureEntry->Checksum);
				ushort sx = x;
				ushort sy = y;

				pTextureEntry->x = sx;
				pTextureEntry->y = sy;
				rec.x = sx;
				rec.y = sy;
				rec.w = AlignedWidth / 2;
				rec.h = Height;
				PC_TexUpload(rec.x, rec.y, rec.w, rec.h, pTextureData);
			}

			if (NumColours == 16) {
				int AlignedWidth = (Width + 3) & ~3;
				RECT rec;

				Pack_Pack(0, AlignedWidth, Height, &x, &y, 4, (Flags & 1) ? 2 : 0,
				          pTextureEntry->Checksum);
				ushort sx = x;
				ushort sy = y;

				pTextureEntry->x = sx;
				pTextureEntry->y = sy;
				rec.x = sx;
				rec.y = sy;
				rec.w = AlignedWidth / 4;
				rec.h = Height;
				PC_TexUpload(rec.x, rec.y, rec.w, rec.h, pTextureData);
			}

			pPal = Pal_FindPaletteEntry(PaletteChecksum);
			pPal->Usage++;
			pTextureEntry->pPalette = pPal;
			pTextureEntry->clut = pPal->Clut;
			pTextureEntry->tpage = GetTPage(NumColours == 256, 0, x, y);

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

/* DetachOldEnviro__FUc (0x8006fbe4) */
/* Takes environment idx's items out of EnviroList, the chain of both
   environments' item lists (AttachNewEnviro puts the newer one first): the list then starts at
   the other environment, or the other's list ends where this one began. */
void DetachOldEnviro(uchar idx) {
	Item *other = 0;
	Item *cur;
	int o;

	pTexWibPacket[idx] = 0;
	cur = PSXRegionItems[EnvRegions[idx]];
	o = EnvRegions[idx ^ 1];

	if (o != -1)
		other = PSXRegionItems[o];

	if (!other) {
		EnviroList = 0;
		return;
	}

	if (cur == EnviroList) {
		EnviroList = other;
		return;
	}

	{
		Item *p = other;

		while (p->NextItem != cur)
			p = p->NextItem;

		p->NextItem = 0;
	}
}

/* Spool_FindRegion__FPc (0x8006fcb0) */
int Spool_FindRegion(char *name) {
	int i;

	for (i = 0; i < 40; i++)
		if (Utils_CompareStrings(name, PSXRegion[i].Filename, -1))
			return i;

	return -1;
}

/* Spool_ClearPSX__FPc (0x8006fd18) */
void Spool_ClearPSX(char *name) {
	int r = Spool_FindRegion(name);

	if (PSXRegion[r].Usable)
		ClearRegion(Spool_FindRegion(name), 1, 0);
}

/* Spool_ClearAllPSXs__Fv (0x8006fd74) */
void Spool_ClearAllPSXs() {
	int i;

	Spool_Sync();
	Spool_ClearEnvironmentRegions();

	for (i = 0; i < 40; i++)
		if (!PSXRegion[i].Protected)
			ClearRegion(i, 1, 0);

	Spool_RemoveUnusedTextures();
}

/* ClearRegion__Fiii (0x8006fde4) */
/* Demo twin; locals and blocks from its MAIN.SYM. Releases a loaded region: drops
   its texture usage counts and, unless it stays in memory, clears the region record, detaches its
   environment and frees the PSX file. Optionally purges unused textures. */
void ClearRegion(int Region, int cleanuptextures, int KeepInMemory) {
	if (Region == -1)
		return;

	if (!PSXRegion[Region].Filename[0])
		return;

	DecrementTextureUsage(Region);

	if (!KeepInMemory) {
		PSXRegion[Region].pAnimFile = 0;
		PSXRegion[Region].pColourPulseData = 0;
		PSXRegion[Region].pTexWibData = 0;
		PSXRegion[Region].Protected = 0;
		PSXRegion[Region].Usable = 0;
		PSXRegion[Region].pHooks = 0;
		free(PSXRegion[Region].ppModels);
		free(PSXRegion[Region].ppTextures);
		PSXRegion[Region].ppModels = 0;
		PSXRegion[Region].ppTextures = 0;
		PSXRegion[Region].Filename[0] = 0;
		PSXRegion[Region].TexturesInRAM = 0;
		PSXRegion[Region].KeepTexturesInRAM = 0;

		{
			int EnvIndex = Spool_GetEnvIndex(Region);

			if (EnvIndex != -1) {
				M3dZone_FreePSX(EnvIndex);
				DetachOldEnviro(EnvIndex);

				EnvRegions[EnvIndex] = -1;
			}
		}

		Mem_Delete(PSXRegion[Region].pPSX);
		free(PSXRegionItems[Region]);
		PSXRegionItems[Region] = 0;

		PSXRegion[Region].pPSX = 0;
	}

	if (cleanuptextures)
		Spool_RemoveUnusedTextures();
}

/* Spool_ClearEnvironmentRegions__Fv (0x8006fefc) */
void Spool_ClearEnvironmentRegions(void) {
	ClearRegion(EnvRegions[0], 0, 0);
	ClearRegion(EnvRegions[1], 0, 0);

	EnvRegions[0] = -1;
	EnvRegions[1] = -1;
	Spool_RemoveUnusedTextures();

	OtherEnvIndex = 0;
}

/* PreProcessAnimPacket__FPUiT0 (0x8006ff48) */
/* C++. Registers a PSX file's anim packet (AnimPackets; Spool_FindAnim looks anims up by name) and
   marks the textures its frames use. A frame in the file is 8 bytes: offsets and size, then an
   index in the file's texture table. Retail put the Texture pointers into the frames; the port
   makes the packet's frames SAnimFrames of its own (AnimPacket::pFrames): each anim's after a
   slot whose last word is its frame count, as in the file (CBit::SetAnim, M3d_RenderSuperItem
   read it before the first frame). */
void PreProcessAnimPacket(Texture **ppTextures, uint *data) {
	AnimPacket *node = (AnimPacket *)Mem_NewX(sizeof(AnimPacket), 0, 1);
	uint n, i, total = 0;
	uint *p;
	SAnimFrame *pFrame;

	node->pNext = AnimPackets;
	node->pPacket = data;
	AnimPackets = node;
	n = *data++;

	/* the frames, all anims' */
	for (i = 0, p = data; i < n; i++) {
		uint m = p[2];

		total += m;
		p += 3 + 2 * m;
	}

	pFrame = node->pFrames = (SAnimFrame *)Mem_NewX((total + n + 1) * sizeof(SAnimFrame), 0, 1);

	for (i = 0; i < n; i++) {
		uint m, j;

		data += 2;
		m = *data++;
		pFrame++;
		((uint *)pFrame)[-1] = m;

		for (j = 0; j < m; j++, pFrame++) {
			Texture *t = ppTextures[data[1]];

			*(uint *)pFrame = data[0]; /* OffX, OffY, Width, Height */
			pFrame->pTexture = t;
			t->tpage |= 0x20;
			t->u1++;
			t->v2++;
			t->u3++;
			t->v3++;
			data += 2;
		}
	}
}

/* Spool_FindAnim__FPc (0x80070064) */
/* Demo twin (identical opcodes); locals and blocks from its MAIN.SYM. Searches the loaded
   anim packets for an anim by name (case-insensitive, up to 8 characters). The masks sit in the
   assignments (line table) and cross-jumping merges the two pairs; testing `!lettera` in the break
   (CSE turns it into letterb's register) lets jump threading send that exit straight to found = 1. */
SAnimFrame *Spool_FindAnim(char *pName) {
	SAnimFrame *pAnim = 0;
	AnimPacket *pPacketInfo;

	for (pPacketInfo = AnimPackets; pPacketInfo; pPacketInfo = pPacketInfo->pNext) {
		uint NumAnims = *pPacketInfo->pPacket;
		char *pAnimName = (char *)(pPacketInfo->pPacket + 1);
		int found = 0;
		uint FirstFrame = 1; /* (the port's: the anim's first frame in pFrames, after its count) */

		{
			int i;

			for (i = 0; i < NumAnims; i++) {
				char lettera;
				char letterb;
				char *pA = pName;
				char *pB = pAnimName;
				int count = 0;
				uint NumFrames;

				lettera = *pA & 0xdf;
				letterb = *pB & 0xdf;

				while (1) {
					if (lettera != letterb || !lettera || count >= 8)
						break;

					pA++;
					pB++;
					lettera = *pA & 0xdf;
					letterb = *pB & 0xdf;
					count++;
				}

				if ((!lettera && !letterb) || count == 8) {
					found = 1;
					break;
				}

				NumFrames = *(uint *)(pAnimName + 8);
				pAnimName += NumFrames * 8 + 12;
				FirstFrame += NumFrames + 1;
			}
		}

		if (found) {
			pAnim = pPacketInfo->pFrames + FirstFrame;
			break;
		}
	}

	return pAnim;
}

/* LoadNameExt__FPcT0 (0x80070140) */
void LoadNameExt(char *a, char *b) {
	char *d = SpoolQ[FreePos].Name;
	char *s = a;
	char *t = b;

	while (*s)
		*d++ = *s++;

	while (*t)
		*d++ = *t++;

	*d = 0;
}

/* Spool_PSX__FPciii (0x800701b0) */
/* Fuzzy demo twin (0.68); locals from its MAIN.SYM. Returns the region already
   holding Name, or one from the recent-load cache (an entry matches when Name is a prefix ending
   at the '.' of the cached file name). Otherwise claims a free region, records the name and
   queues the load. Retail drops the demo's KeepTextures argument. */
int Spool_PSX(char *Name, int IsEnviro, int WhichHeap) {
	int freeregion;

	{
		int i;

		for (i = 0; i < 40; i++)
			if (PSXRegion[i].Filename[0] && Utils_CompareStrings(Name, PSXRegion[i].Filename, -1))
				return i;
	}

	{
		int i;

		for (i = 0; i < 20; i++) {
			if (SpoolQ[i].Occupied) {
				char *pQName = SpoolQ[i].Name;
				char *pPSXName = Name;
				int Identical = 1;

				while (*pPSXName) {
					if (*pPSXName != *pQName) {
						Identical = 0;
						break;
					}

					pPSXName++;
					pQName++;
				}

				if (Identical && *pQName != '.')
					Identical = 0;

				if (Identical)
					return SpoolQ[i].Region;
			}
		}
	}

	freeregion = -1;

	{
		int i;

		for (i = 0; i < 40; i++) {
			if (!PSXRegion[i].Filename[0]) {
				freeregion = i;
				break;
			}
		}
	}

	if (IsEnviro == 0 || IsEnviro == 2) {
		char *pNew = Name;
		char *pDest = PSXRegion[freeregion].Filename;
		int c;

		for (c = 0; *pNew && c < 9; c++)
			*pDest++ = *pNew++;

		*pDest = 0;
	}

	LoadNameExt(Name, ".psx");

	if (IsEnviro != 2)
		SpoolQ[FreePos].FileType = IsEnviro != 0;
	else
		SpoolQ[FreePos].FileType = IsEnviro;

	SpoolQ[FreePos].Region = freeregion;
	SpoolQ[FreePos].Occupied = 1;
	SpoolQ[FreePos].KeepTextures = WhichHeap;
	FreePos = (FreePos + 1) % 20;
	return freeregion;
}

/* Spool_GetEnvIndex__Fi (0x8007043c) */
int Spool_GetEnvIndex(int i) {
	if (i == EnvRegions[0])
		return 0;

	if (i == EnvRegions[1])
		return 1;

	return -1;
}

/* Spool_Continue__Fv (0x8007046c) */
/* Demo twin; locals and blocks from its MAIN.SYM (SpoolMode is a function-static
   there, DAT_800d1f08 here). Steps the load queue's head entry through three states: open the file
   and allocate (an environment takes a free environment slot, evicting the oldest, and names its
   region), wait for the read, then process the PSX data and attach the environment. */
void Spool_Continue() {
	SQueue *Q = &SpoolQ[SpoolPos];

	if (!Q->Occupied)
		return;

	switch (DAT_800d1f08) {
		case 0: {
			int FileSize = FileIO_Open(Q->Name);

			switch (Q->FileType) {
				case 1: {
					char *pNew;
					char *pDest;
					int c;

					if (EnvRegions[0] != -1 && EnvRegions[1] != -1) {
						Q->Region = EnvRegions[OtherEnvIndex];
						ClearRegion(Q->Region, 0, 0);

						EnvRegions[OtherEnvIndex] = Q->Region;
					} else if (EnvRegions[0] == -1) {
						EnvRegions[0] = Q->Region;
					} else {
						EnvRegions[1] = Q->Region;
					}

					pNew = Q->Name;
					pDest = PSXRegion[Q->Region].Filename;

					for (c = 0; *pNew != '.' && c < 9; c++)
						*pDest++ = *pNew++;

					*pDest = 0;
					PSXRegion[Q->Region].pPSX = Mem_NewX(FileSize, 1, 1);
					FileIO_Load(PSXRegion[Q->Region].pPSX);
					break;
				}
				case 0:
				case 2:
					PSXRegion[Q->Region].pPSX = Mem_NewX(FileSize, 1, 1);
					FileIO_Load(PSXRegion[Q->Region].pPSX);
					break;
			}

			DAT_800d1f08 = 1;
			PSXRegion[Q->Region].KeepTexturesInRAM = Q->KeepTextures;
			break;
		}
		case 1:
			if (!Loading)
				DAT_800d1f08 = 2;

			break;
		case 2:
			switch (Q->FileType) {
				case 1:
					ProcessNewPSX(Q->Region);
					AttachNewEnviro(Q->Region);

					{
						int e = Spool_GetEnvIndex(Q->Region);

						EnvModelChecksums[e] = Spool_SkipPackets(PSXRegion[Q->Region].pPSX);
					}

					break;
				case 0:
					ProcessNewPSX(Q->Region);
					break;
				case 2:
					break;
			}

			Q->Occupied = 0;
			DAT_800d1f08 = 0;
			SpoolPos = (SpoolPos + 1) % 20;
			break;
	}
}

/* Spool_Sync__Fv (0x8007079c) */
void Spool_Sync(void) {
	while (Loading || SpoolQ[SpoolPos].Occupied) {
		Spool_Continue();
		FileIO_Continue();
	}
}

/* FUN_80070830 (0x80070830) */
int FUN_80070830(void) {
	int r = 0;

	if (Loading || SpoolQ[SpoolPos].Occupied)
		r = 1;

	return r;
}
