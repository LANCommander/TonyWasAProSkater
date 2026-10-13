/* Trig_ExecuteRestart__Fv (0x80044c84) */
/* C++ (TRIG.CPP). Fuzzy demo twin (0.82); locals from its MAIN.SYM. Puts every skater on the
   current restart node, runs its command list, and in mode 2 (unless state 7) nudges the skater
   and moves it to the restart node named "RE_2P". */
#include "game/bruce.h"

extern CBruce *MechList;
extern int GNumberOfPlayers;
extern int GGame;

int RestartNode;
short **OffsetList;
int NumNodes;

CSVector *Trig_GetPosition(CVector &, int);
void FudgeAngles(CBruce *);
int Utils_CompareStrings(const char *, const char *, unsigned int);

extern "C" {
	void FUN_8008c190(void *, int, int, int);
	void ExecuteCommandList__FPUsii(ushort *, int, int);
}

void Trig_ExecuteRestart() {
	CVector StartPos;
	CSVector *pAngles;
	CBruce *MechListCopy;
	char *pName;
	CBruce *pMan;

	pAngles = Trig_GetPosition(StartPos, RestartNode);

	for (MechListCopy = MechList; MechListCopy; MechListCopy = (CBruce *)MechListCopy->mNextItem) {
		MechListCopy->mPos = StartPos;
		MechListCopy->mOldPos = StartPos;
		MechListCopy->mAngles = *pAngles;
		FUN_8008c190("%d,%d,%d\n", MechListCopy->mAngles.vx, MechListCopy->mAngles.vy,
		             MechListCopy->mAngles.vz);
		FudgeAngles(MechListCopy);
	}

	pName = (char *)(pAngles + 1);

	while (*pName)
		pName++;

	pName++;
	pName += (int)pName & 1;
	ExecuteCommandList__FPUsii((ushort *)pName, RestartNode, 1);

	if (GNumberOfPlayers == 2 && GGame != 7) {
		int i;

		pMan = MechList;
		pMan->mPos += pMan->mFront * 100;

		for (i = 0; i < NumNodes; i++) {
			short *pNode = OffsetList[i];
			CVector Dummy;

			if (*pNode == 8 &&
			    Utils_CompareStrings((char *)(Trig_GetPosition(Dummy, i) + 1), "RE_2P", -1)) {
				CVector StartPos;

				pAngles = Trig_GetPosition(StartPos, i);
				pMan->mPos = StartPos;
				pMan->mOldPos = StartPos;
				pMan->mAngles = *pAngles;
				FudgeAngles(pMan);
			}
		}
	}
}
