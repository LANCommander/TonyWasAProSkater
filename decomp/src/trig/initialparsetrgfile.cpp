/* Trig_InitialParseTRGFile__FPc (0x80044ed8) */
/* C++ (TRIG.CPP). Fuzzy demo twin (0.43; THPS1 loads the file here); locals from its MAIN.SYM.
   Loads <name>.TRG, relocates its node table, trims the buffer after the terminating node, then
   collects the restart (type 8) nodes and the 'H' named ones, runs the level's start-up command
   lists (type 15 in a two-player game, else type 4) and parses the rest of the file. Two pointers
   are bumped in place (`pTable += 3`, `pNodeName += 6`): written as `base + n`, CSE keeps the base
   live and folds the offset into the following load, which retail doesn't. */
typedef unsigned char uchar;
typedef unsigned short ushort;

struct CVector {
	int vx, vy, vz;
};

extern char s_trg[];
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

extern "C" {
	int FileIO_Open__FPCc(char *);
	char *Mem_NewX__FUiii(int, int, int);
	void FileIO_Load__FPv(char *);
	void FileIO_Sync__Fv(void);
	void FUN_8004d6a0(char *, int);
	void ExecuteCommandList__FPUsii(ushort *, int, int);
}

void Trig_InitialParseTRGFile(char *pName) {
	char Name[32];
	int Found;

	{
		char *pExt = s_trg;
		char *pDest = Name;

		while (*pName)
			*pDest++ = *pName++;

		while (*pExt)
			*pDest++ = *pExt++;

		*pDest = 0;
	}

	TrigFile = Mem_NewX__FUiii(FileIO_Open__FPCc(Name), 0, 1);
	FileIO_Load__FPv(TrigFile);
	FileIO_Sync__Fv();

	{
		short **pTable = (short **)TrigFile;
		int i;

		NumNodes = ((ushort *)pTable)[4];
		pTable += 3;
		OffsetList = pTable;

		for (i = 0; i < NumNodes; i++) {
			*pTable = (short *)((int)*pTable + (int)TrigFile);
			pTable++;
		}
	}

	{
		short *pLast = OffsetList[NumNodes - 1];

		if (*pLast == 255)
			FUN_8004d6a0(TrigFile, ((char *)pLast - TrigFile + 5) & ~3);
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
				ExecuteCommandList__FPUsii((ushort *)(pNode + 1), i, 1);
			}
		}
	}

	if (!Found) {
		int i;

		for (i = 0; i < NumNodes; i++) {
			short *pNode = OffsetList[i];

			if (*pNode == 4)
				ExecuteCommandList__FPUsii((ushort *)(pNode + 1), i, 1);
		}
	}

	Trig_ParseTRGFile();
}
