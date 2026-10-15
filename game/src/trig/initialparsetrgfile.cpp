/* Trig_InitialParseTRGFile__FPc (0x80044ed8) */
/* C++ (TRIG.CPP). Fuzzy demo twin (0.43; THPS1 loads the file here); locals from its MAIN.SYM.
   Loads <name>.TRG, relocates its node table, trims the buffer after the terminating node, then
   collects the restart (type 8) nodes and the 'H' named ones, runs the level's start-up command
   lists (type 15 in a two-player game, else type 4) and parses the rest of the file. Two pointers
   are bumped in place (`pTable += 3`, `pNodeName += 6`): written as `base + n`, CSE keeps the base
   live and folds the offset into the following load, which retail doesn't. */
#include "game/fileio.h"
#include "game/mem.h"
#include "game/trig.h"
#include "game/vector.h"

extern char *CheatRestarts[];
extern char *HorseRestarts[];
extern int GNumberOfPlayers;

char *TrigFile;
short **OffsetList;
int NumNodes;
int NumCheatRestarts;
int NumHorseRestarts;
int RestartNode;

char *Trig_GetPosition(CVector &, int);
void Trig_ParseTRGFile();

void Trig_InitialParseTRGFile(char *pName) {
	char Name[32];
	int Found;

	{
		char *pExt = ".trg";
		char *pDest = Name;

		while (*pName)
			*pDest++ = *pName++;

		while (*pExt)
			*pDest++ = *pExt++;

		*pDest = 0;
	}

	TrigFile = Mem_NewX(FileIO_Open(Name), 0, 1);
	FileIO_Load(TrigFile);
	FileIO_Sync();

	{
		/* the node table: offsets from the file's start. Retail turned them into pointers in
		   place; the port keeps an array of its own (Trig_DeleteTrigFile frees it) */
		uint *pTable = (uint *)TrigFile;
		int i;

		NumNodes = ((ushort *)pTable)[4];
		pTable += 3;
		OffsetList = (short **)Mem_NewX(NumNodes * sizeof(short *), 0, 1);

		for (i = 0; i < NumNodes; i++) {
			OffsetList[i] = (short *)(TrigFile + *pTable);
			pTable++;
		}
	}

	{
		short *pLast = OffsetList[NumNodes - 1];

		if (*pLast == 255)
			Mem_Shrink(TrigFile, ((char *)pLast - TrigFile + 5) & ~3);
	}

	NumCheatRestarts = 0;

	{
		int i;

		for (i = 0; i < NumNodes; i++) {
			short *pNode = OffsetList[i];

			if (*pNode == 8) {
				CVector Dummy;
				char *pNodeName = Trig_GetPosition(Dummy, i) + 6;

				CheatRestarts[NumCheatRestarts++] = pNodeName;
			}
		}
	}

	NumHorseRestarts = 0;

	{
		int i;

		for (i = 0; i < NumNodes; i++) {
			short *pNode = OffsetList[i];

			if (*pNode == 8) {
				CVector Dummy;
				char *pNodeName = Trig_GetPosition(Dummy, i);

				pNodeName += 6;

				if (*pNodeName == 'H')

					HorseRestarts[NumHorseRestarts++] = pNodeName;
			}
		}
	}

	RestartNode = 0xffff;
	Found = 0;

	if (GNumberOfPlayers == 2) {
		int i;

		for (i = 0; i < NumNodes; i++) {
			short *pNode = OffsetList[i];

			if (*pNode == 15) {
				Found = 1;
				ExecuteCommandList((ushort *)(pNode + 1), i, 1);
			}
		}
	}

	if (!Found) {
		int i;

		for (i = 0; i < NumNodes; i++) {
			short *pNode = OffsetList[i];

			if (*pNode == 4)
				ExecuteCommandList((ushort *)(pNode + 1), i, 1);
		}
	}

	Trig_ParseTRGFile();
}
