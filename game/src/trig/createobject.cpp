/* Trig_CreateObject__Fi (0x80045268) */
/* C++ (TRIG.CPP). Fuzzy demo twin (0.71); locals from its MAIN.SYM. Creates the object for a
   trigger node: baddies (203), cars (213-216) and platforms (402) from type-1/7 nodes, powerups
   from type-5 nodes. Objects not active at start are suspended out of their list. */
#include "game/ob.h"
#include "game/baddy.h"
#include "game/platform.h"
#include "game/car.h"
#include "game/powerup.h"

short **OffsetList;

extern CBody *BaddyList;
extern CBody *EnvironmentalObjectList;
extern CBody *PowerUpList;
extern CVector ZeroVector;

int GetFlag(uchar, uchar *);
uchar *SkipFlags(uchar *);
ushort *Trig_GetPosition(CVector &, int);
ushort *Trig_GetLinksPointer(int);
CBody *PowerUp_Create(ushort, const CVector &, uint, int, CVector &);

CBody *Trig_CreateObject(int NodeIndex) {
	int Type;
	int NotSuspendedAtStart;
	int Seedable = 0xffff;
	int ManualSuspendControl = 0;
	int Inactive = 0;
	CBody *NewObject = 0;
	short *pInfo;
	int NodeType;

	pInfo = OffsetList[NodeIndex];
	NodeType = *pInfo++;

	switch (NodeType) {
		case 7:
			Inactive = 1;
		case 1:
			Type = *pInfo++;
			Seedable = (ushort)*pInfo++;
			pInfo += *pInfo + 1;
			NotSuspendedAtStart = GetFlag(2, (uchar *)pInfo);
			ManualSuspendControl = GetFlag(4, (uchar *)pInfo);
			pInfo = (short *)(((int)SkipFlags((uchar *)pInfo) + 3) & ~3);

			switch (Type) {
				case 203:
					NewObject = new CScriptOnlyBaddy(pInfo, NodeIndex);
					break;
				case 402:
					NewObject = new CPlatform(pInfo, NodeIndex);
					break;
				case 213:
				case 214:
				case 215:
				case 216:
					NewObject = new CCar(pInfo, NodeIndex, Type);
					break;
			}

			break;
		case 5: {
			CVector Pos;
			ushort *pStuff;

			Type = *pInfo;
			pStuff = Trig_GetPosition(Pos, NodeIndex);
			pStuff++;

			{
				int Flag = (*pStuff++ == 0) << 2;

				NewObject = PowerUp_Create(Type, Pos, Flag, *pStuff, ZeroVector);
			}

			if (*Trig_GetLinksPointer(NodeIndex))
				((CPowerUp *)NewObject)->SetNode(NodeIndex);

			NotSuspendedAtStart = 1;
			break;
		}
	}

	if (NewObject) {
		NewObject->mNode = NodeIndex;
		NewObject->mSeed = Seedable;

		if (ManualSuspendControl)
			NewObject->mCBodyFlags &= ~2;

		if (Inactive)
			NewObject->mCBodyFlags |= 4;

		if (!NotSuspendedAtStart) {
			CBody **ppList = 0;

			if (NewObject == BaddyList)
				ppList = &BaddyList;

			if (NewObject == EnvironmentalObjectList)
				ppList = &EnvironmentalObjectList;

			if (NewObject == PowerUpList)
				ppList = &PowerUpList;

			NewObject->Suspend(ppList);
		}

		return NewObject;
	}

	return 0;
}
