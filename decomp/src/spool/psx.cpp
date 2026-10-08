/* Spool_PSX__FPciii (0x800701b0) */
/* C++ (SPOOL.CPP). Fuzzy demo twin (0.68); locals from its MAIN.SYM. Returns the region already
   holding Name, or one from the recent-load cache (an entry matches when Name is a prefix ending
   at the '.' of the cached file name). Otherwise claims a free region, records the name and
   queues the load. Retail drops the demo's KeepTextures argument. */
typedef unsigned char uchar;

struct SRegion {
	char Name[64];
};

struct SPSXCache {
	char Name[13];
	uchar Used;   /* 13 */
	uchar Type;   /* 14 */
	uchar Region; /* 15 */
	uchar Heap;   /* 16 */
};

extern SRegion PSXRegion[40];
extern SPSXCache SpoolQ[20];
extern char s_psx[];

int FreePos;

int Utils_CompareStrings(const char *, const char *, unsigned int);
void LoadNameExt(char *, char *);

int Spool_PSX(char *Name, int IsEnviro, int WhichHeap) {
	int freeregion;

	{
		int i;

		for (i = 0; i < 40; i++)
			if (PSXRegion[i].Name[0] && Utils_CompareStrings(Name, PSXRegion[i].Name, -1))
				return i;
	}

	{
		int i;

		for (i = 0; i < 20; i++) {
			if (SpoolQ[i].Used) {
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
			if (!PSXRegion[i].Name[0]) {
				freeregion = i;
				break;
			}
		}
	}

	if (IsEnviro == 0 || IsEnviro == 2) {
		char *pNew = Name;
		char *pDest = PSXRegion[freeregion].Name;
		int c;

		for (c = 0; *pNew && c < 9; c++)
			*pDest++ = *pNew++;

		*pDest = 0;
	}

	LoadNameExt(Name, s_psx);

	if (IsEnviro != 2)
		SpoolQ[FreePos].Type = IsEnviro != 0;
	else
		SpoolQ[FreePos].Type = IsEnviro;

	SpoolQ[FreePos].Region = freeregion;
	SpoolQ[FreePos].Used = 1;
	SpoolQ[FreePos].Heap = WhichHeap;
	FreePos = (FreePos + 1) % 20;
	return freeregion;
}
