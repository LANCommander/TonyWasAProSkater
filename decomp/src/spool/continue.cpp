/* Spool_Continue__Fv (0x8007046c) */
/* C++ (SPOOL.CPP). Demo twin; locals and blocks from its MAIN.SYM (SpoolMode is a function-static
   there, DAT_800d1f08 here). Steps the load queue's head entry through three states: open the file
   and allocate (an environment takes a free environment slot, evicting the oldest, and names its
   region), wait for the read, then process the PSX data and attach the environment. */
#include "game/spool.h"
#include "game/m3d.h"

extern SQueue SpoolQ[];
extern SPSXRegion PSXRegion[];

int SpoolPos;
int DAT_800d1f08;
int EnvRegions[2];
int OtherEnvIndex;
uint *EnvModelChecksums[2];

extern int Loading;

void ClearRegion(int, int, int);
void AttachNewEnviro(int);
int Spool_GetEnvIndex(int);
uint *Spool_SkipPackets(uint *);

extern "C" {
	int FileIO_Open__FPCc(char *);
	uint *Mem_NewX__FUiii(int, int, int);
	void FileIO_Load__FPv(uint *);
	void ProcessNewPSX__Fi(int);
}

void Spool_Continue() {
	SQueue *Q = &SpoolQ[SpoolPos];

	if (!Q->Occupied)
		return;

	switch (DAT_800d1f08) {
		case 0: {
			int FileSize = FileIO_Open__FPCc(Q->Name);

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
					PSXRegion[Q->Region].pPSX = Mem_NewX__FUiii(FileSize, 1, 1);
					FileIO_Load__FPv(PSXRegion[Q->Region].pPSX);
					break;
				}
				case 0:
				case 2:
					PSXRegion[Q->Region].pPSX = Mem_NewX__FUiii(FileSize, 1, 1);
					FileIO_Load__FPv(PSXRegion[Q->Region].pPSX);
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
					ProcessNewPSX__Fi(Q->Region);
					AttachNewEnviro(Q->Region);

					{
						int e = Spool_GetEnvIndex(Q->Region);

						EnvModelChecksums[e] = Spool_SkipPackets(PSXRegion[Q->Region].pPSX);
					}

					break;
				case 0:
					ProcessNewPSX__Fi(Q->Region);
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
