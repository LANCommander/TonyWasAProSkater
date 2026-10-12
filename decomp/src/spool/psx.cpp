/* Spool_PSX__FPciii (0x800701b0) */
/* C++ (SPOOL.CPP). Fuzzy demo twin (0.68); locals from its MAIN.SYM. Returns the region already
   holding Name, or one from the recent-load cache (an entry matches when Name is a prefix ending
   at the '.' of the cached file name). Otherwise claims a free region, records the name and
   queues the load. Retail drops the demo's KeepTextures argument. */
#include "game/spool.h"
#include "game/m3d.h"

extern SPSXRegion PSXRegion[40];
extern SQueue SpoolQ[20];
extern char s_psx[];

int FreePos;

int Utils_CompareStrings(const char *, const char *, unsigned int);
void LoadNameExt(char *, char *);

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

	LoadNameExt(Name, s_psx);

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
