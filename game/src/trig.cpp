/* TRIG.CPP: the level trigger file (.TRG): its node table, the command points and the command
   lists they run (restarts, spooling, objects, pulses, visibility, kills, fog, camera settings)
   (declarations in game/trig.h). */

#include "game/trig.h"
#include "game/bruce.h"
#include "game/psyq.h"
#include "game/fileio.h"
#include "game/mem.h"
#include "game/vector.h"
#include "game/ob.h"
#include "game/baddy.h"
#include "game/platform.h"
#include "game/car.h"
#include "game/powerup.h"
#include "game/classsizes.h"
#include "game/m3d.h"
#include "game/bit.h"
#include "game/types.h"
#include "game/backgrnd.h"
#include "game/camera.h"
#include "game/trickob.h"
#include "game/light.h"
#include "game/campt.h"
#include "game/db.h"
#include "game/exp.h"
#include "game/front.h"
#include "game/gamefmv.h"
#include "game/globals.h"
#include "game/m3dinit.h"
#include "game/main.h"
#include "game/mess.h"
#include "game/rail.h"
#include "game/sfx.h"
#include "game/spool.h"
#include "game/utils.h"
#include "game/video.h"
#include "game/script.h"

/* a light node's CLight parameters, after its position (Trig_GetPosition) */
struct STrigLightParams {
	ushort w0, w2, w4, w6;
	uchar b8, p9, b10, p11, b12, p13, b14, p15, b16, p17, b18;
};

extern char *MenuFilenamePointers[40];
extern int NumTrigMenuEntries;
extern SCommandPoint *HashTable[];
extern SCommandPoint *CommandPoints;
extern int NumNodes;
extern char *TrigFile;
extern char DAT_800d1a10[8];
extern void *pPSXDualBuffer;
extern SGapTrick GapTricks[];

void CreateCommandPoint(uint, ushort, ushort *);
SCommandPoint *GetCommandPoint(int);
void FudgeAngles(CBruce *);
int GetFlag(uchar, uchar *);
uchar *SkipFlags(uchar *);
CBody *Trig_CreateObject(int);
void SendSuspend(CBody **, int);
void SendUnSuspend(int);
char *SkipString(char *);
ushort *SkipCommand(ushort *);
void Trig_AllocateDualBuffer(uint);
void Front_Restart();
void SendSuspendOrActivate(ushort *, int);
void SendVisibleFromNode(int, int);
void SendKillFromNode(int, int);

/* Trig_ParseTRGFile allocates and constructs its lights and trick objects through the retail
   symbols, not new-expressions: CTrickOb's constructor gets a stray third argument (retail passes
   it), and g++ gives operator new results no-alias semantics, which changes the code */
extern "C" {
void *__nw__4CBitUi(uint);
void *__nw__6CClassUi(uint);
void __6CLightRC7CVectoriiiiiUcUcUcUcUcUc(void *, const CVector &, int, int, int, int, int, uchar,
                                          uchar, uchar, uchar, uchar, uchar);
void __8CTrickObUs(void *, ushort, int);
}

/* Trig_ClearTrigMenu__Fv (0x800447f4) */
void Trig_ClearTrigMenu() {
	int i;

	for (i = 0; i < 40; i++)
		MenuFilenamePointers[i] = 0;

	NumTrigMenuEntries = 0;
}

/* CreateCommandPoint__FUiUsPUs (0x80044820) */
/* Allocates a trigger node (see Trig_TriggerCommandPoint): linked into the id's hash chain (on the id's low
   byte) and onto the list of all triggers. */
void CreateCommandPoint(uint id, ushort line, ushort *script) {
	SCommandPoint *t = (SCommandPoint *)Mem_NewX(sizeof(SCommandPoint), 0, 1);

	t->pNext = CommandPoints;
	t->pNextSimilar = HashTable[id & 0xff];
	HashTable[id & 0xff] = t;
	t->NodeIndex = line;
	t->pCommands = script;
	t->Checksum = id;
	CommandPoints = t;
	t->Collision = 0;
	t->Executed = 0;
	t->PulsesReceived = 0;
	t->NumPulsesSet = 0;
	t->NumPulses = 0;
}

/* FUN_800448b8 (0x800448b8) */
/* SCommandPoint (game/trig.h) */
void FUN_800448b8(void) {
	int i;
	SCommandPoint *p;
	SCommandPoint *n;

	for (i = 0; i < 256; i++)
		HashTable[i] = 0;

	for (p = CommandPoints; p; p = n) {
		n = p->pNext;
		Mem_Delete(p);
	}

	CommandPoints = 0;
}

/* GetCommandPoint__Fi (0x80044920) */
SCommandPoint *GetCommandPoint(int n) {
	SCommandPoint *p;

	if (n != 0xffff && *OffsetList[n] == NODE_COMMANDPOINT) {
		for (p = CommandPoints; p; p = p->pNext)
			if (p->NodeIndex == n)
				return p;
	}

	return 0;
}

/* Trig_ResetCPCollisionFlags__Fi (0x80044994) */
void Trig_ResetCPCollisionFlags(int m) {
	SCommandPoint *p;

	for (p = CommandPoints; p; p = p->pNext)
		p->Collision &= ~m;
}

/* Trig_ResetCPExecutedFlags__Fi (0x800449cc) */
void Trig_ResetCPExecutedFlags(int m) {
	SCommandPoint *p;

	for (p = CommandPoints; p; p = p->pNext)
		if ((p->Executed & m) && !(p->Collision & m))
			p->Executed &= ~m;
}

/* Trig_TriggerCommandPoint__FUib (0x80044a20) */
/* Fires the trigger with the given id: marks it hit for this side (1 or 2) and, the first time
   for that side, runs its script. Nodes are hashed on the id's low byte (see FUN_80044820). */
SCommandPoint *Trig_TriggerCommandPoint(uint id, bool) {
	SCommandPoint *t;

	for (t = HashTable[id & 0xff]; t; t = t->pNextSimilar) {
		if (t->Checksum == id) {
			int bit = 2;

			if (pCurrentPlayer == GSkater[0])
				bit = 1;

			t->Collision |= bit;

			if (!(t->Executed & bit)) {
				ExecuteCommandList(t->pCommands, t->NodeIndex, 0);

				t->Executed |= bit;
				return t;
			}
		}
	}

	return 0;
}

/* Trig_SetRestart__FPc (0x80044ae4) */
void Trig_SetRestart(char *name) {
	int i;

	RestartNode = 0xffff;

	for (i = 0; i < NumNodes; i++) {
		if (*OffsetList[i] == NODE_RESTART) {
			CVector v;

			if (Utils_CompareStrings((char *)Trig_GetPosition(v, i) + 6, name, -1)) {
				RestartNode = i;
				return;
			}
		}
	}
}

/* FudgeAngles__FP6CBruce (0x80044b90) */
void FudgeAngles(CBruce *t) {
	CVector a, r;
	int ang;

	t->mAngles.vz = 0;
	t->mAngles.vx = 0;
	a.vx = -4096;
	a.vy = 0;
	a.vz = 0;
	ang = ((ushort)t->mAngles.vy + 0x800) & 0xfff;
	t->mAngles.vy = ang;
	Utils_RotateY(r, a, ang);

	a.vx = 0;
	a.vy = 0;
	a.vz = -4096;
	t->mRotMatrix.m[0][0] = r.vx;
	t->mRotMatrix.m[1][0] = r.vy;
	t->mRotMatrix.m[2][0] = r.vz;
	Utils_RotateY(r, a, t->mAngles.vy);

	t->mRotMatrix.m[1][1] = -4096;
	t->mUp.vy = -4096;
	t->mRotMatrix.m[0][1] = 0;
	t->mUp.vx = 0;
	t->mRotMatrix.m[2][1] = 0;
	t->mUp.vz = 0;
	t->mRotMatrix.m[0][2] = r.vx;
	t->mFront.vx = (short)r.vx;
	t->mRotMatrix.m[1][2] = r.vy;
	t->mFront.vy = (short)r.vy;
	t->mRotMatrix.m[2][2] = r.vz;
	t->mFront.vz = (short)r.vz;
}

/* Trig_ExecuteRestart__Fv (0x80044c84) */
/* Fuzzy demo twin (0.82); locals from its MAIN.SYM. Puts every skater on the
   current restart node, runs its command list, and in mode 2 (unless state 7) nudges the skater
   and moves it to the restart node named "RE_2P". */
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
		printf("%d,%d,%d\n", MechListCopy->mAngles.vx, MechListCopy->mAngles.vy,
		       MechListCopy->mAngles.vz);
		FudgeAngles(MechListCopy);
	}

	pName = (char *)(pAngles + 1);

	while (*pName)
		pName++;

	pName++;
	pName += (intptr_t)pName & 1;
	ExecuteCommandList((ushort *)pName, RestartNode, 1);

	if (GNumberOfPlayers == 2 && GGame != 7) {
		int i;

		pMan = MechList;
		pMan->mPos += pMan->mFront * 100;

		for (i = 0; i < NumNodes; i++) {
			short *pNode = OffsetList[i];
			CVector Dummy;

			if (*pNode == NODE_RESTART &&
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

/* Trig_DeleteTrigFile__Fv (0x80044ea8) */
void Trig_DeleteTrigFile(void) {
	if (TrigFile) {
		Mem_Delete(TrigFile);
		Mem_Delete(OffsetList); /* (the port's node array, Trig_InitialParseTRGFile) */

		TrigFile = 0;
		OffsetList = 0;
	}

	NumCheatRestarts = 0;
	NumHorseRestarts = 0;
}

/* Trig_InitialParseTRGFile__FPc (0x80044ed8) */
/* Fuzzy demo twin (0.43; THPS1 loads the file here); locals from its MAIN.SYM.
   Loads <name>.TRG, relocates its node table, trims the buffer after the terminating node, then
   collects the restart (type 8) nodes and the 'H' named ones, runs the level's start-up command
   lists (type 15 in a two-player game, else type 4) and parses the rest of the file. Two pointers
   are bumped in place (`pTable += 3`, `pNodeName += 6`): written as `base + n`, CSE keeps the base
   live and folds the offset into the following load, which retail doesn't. */
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

	TrigFile = (char *)Mem_NewX(FileIO_Open(Name), 0, 1);
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

		if (*pLast == NODE_TERMINATOR)
			Mem_Shrink(TrigFile, ((char *)pLast - TrigFile + 5) & ~3);
	}

	NumCheatRestarts = 0;

	{
		int i;

		for (i = 0; i < NumNodes; i++) {
			short *pNode = OffsetList[i];

			if (*pNode == NODE_RESTART) {
				CVector Dummy;
				char *pNodeName = (char *)Trig_GetPosition(Dummy, i) + 6;

				CheatRestarts[NumCheatRestarts++] = pNodeName;
			}
		}
	}

	NumHorseRestarts = 0;

	{
		int i;

		for (i = 0; i < NumNodes; i++) {
			short *pNode = OffsetList[i];

			if (*pNode == NODE_RESTART) {
				CVector Dummy;
				char *pNodeName = (char *)Trig_GetPosition(Dummy, i);

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

			if (*pNode == NODE_AUTOEXEC2) {
				Found = 1;
				ExecuteCommandList((ushort *)(pNode + 1), i, 1);
			}
		}
	}

	if (!Found) {
		int i;

		for (i = 0; i < NumNodes; i++) {
			short *pNode = OffsetList[i];

			if (*pNode == NODE_AUTOEXEC)
				ExecuteCommandList((ushort *)(pNode + 1), i, 1);
		}
	}

	Trig_ParseTRGFile();
}

/* GetFlag__FUcPUc (0x80045200) */
int GetFlag(unsigned char f, unsigned char *p) {
	while (*p != 0xff) {
		if (*p++ == f)
			return 1;
	}

	return 0;
}

/* SkipFlags__FPUc (0x80045238) */
unsigned char *SkipFlags(unsigned char *p) {
	while (*p != 0xff)
		p++;

	return p + 1;
}

/* Trig_CreateObject__Fi (0x80045268) */
/* Fuzzy demo twin (0.71); locals from its MAIN.SYM. Creates the object for a
   trigger node: baddies (203), cars (213-216) and platforms (402) from type-1/7 nodes, powerups
   from type-5 nodes. Objects not active at start are suspended out of their list. */
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
		case NODE_SEEDABLEBADDY:
			Inactive = 1;
		case NODE_BADDY:
			Type = *pInfo++;
			Seedable = (ushort)*pInfo++;
			pInfo += *pInfo + 1;
			NotSuspendedAtStart = GetFlag(ACTIVEWHENCREATED, (uchar *)pInfo);
			ManualSuspendControl = GetFlag(MANUALSUSPENDCONTROL, (uchar *)pInfo);
			pInfo = (short *)(((intptr_t)SkipFlags((uchar *)pInfo) + 3) & ~3);

			switch (Type) {
				case OBJ_SCRIPTONLYBADDY:
					NewObject = new CScriptOnlyBaddy(pInfo, NodeIndex);
					break;
				case OBJ_PLATFORM:
					NewObject = new CPlatform(pInfo, NodeIndex);
					break;
				case OBJ_CAR_TAXI:
				case OBJ_CAR_POLICE:
				case OBJ_CAR_BUS:
				case OBJ_CAR_CABLE:
					NewObject = new CCar(pInfo, NodeIndex, Type);
					break;
			}

			break;
		case NODE_POWRUP: {
			CVector Pos;
			ushort *pStuff;

			Type = *pInfo;
			pStuff = (ushort *)Trig_GetPosition(Pos, NodeIndex);
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

/* Trig_SendPulseToNode__Fi (0x80045538) */
void Trig_SendPulseToNode(int idx) {
	struct {
		CVector Pos;
		int Pad; /* (the original's local was 16 bytes) */
	} v;
	STrigLightParams *p;
	SCommandPoint *cp;
	void *o2;

	/* (retail passes OffsetList as a stray second argument to the calls made through casts) */
	switch (*OffsetList[idx]) {
		case NODE_POWRUP:
		case NODE_SEEDABLEBADDY:
		case NODE_BADDY:
			((CBody * (*)(int, short **)) Trig_CreateObject)(idx, OffsetList);
			break;
		case NODE_COMMANDPOINT:
			cp = ((SCommandPoint * (*)(int, short **)) GetCommandPoint)(idx, OffsetList);
			cp->PulsesReceived = cp->PulsesReceived + 1;
			ExecuteCommandList(cp->pCommands, idx, 0);
			break;
		case NODE_OFFLIGHT:
			p = (STrigLightParams *)Trig_GetPosition(v.Pos, idx);
			UseCacheForBits = 0;
			new CLight(v.Pos, idx, p->w0, p->w2, p->w4, p->w6, p->b8, p->b10, p->b12, p->b14, p->b16,
			           p->b18);
			UseCacheForBits = 1;
			break;
		case NODE_RAILPOINT:
		case NODE_RAILDEF:
			o2 = ((void *(*)(uint, short **))CClass::operator new)(SIZEOF_CRAIL, OffsetList);

			if (o2)
				FUN_8004f684((CRail *)o2, idx);

			break;
		case NODE_GOALOB:
		case NODE_TRICKOB:
			((void (*)(int, short **))TrickOb_DarkenNode)(idx, OffsetList);
			break;
	}
}

/* Trig_SendPulse__FPUs (0x80045718) */
void Trig_SendPulse(unsigned short *p) {
	int i, n = p[0];
	unsigned short *q = p + 1;

	i = 0;

	while (i < n) {
		i++;
		Trig_SendPulseToNode(*q++);
	}
}

/* KillInList__FiP5CBodyi (0x80045774) */
/* Sends a trigger event to every body in the list whose node id matches: mode 0 kills it
   (Die, virtual slot 1), mode 1 hits it (Hit, slot 5) with its mHealth. */
void KillInList(int id, CBody *pBody, int mode) {
	for (; pBody; pBody = (CBody *)pBody->mNextItem) {
		if (pBody->mNode != id)
			continue;

		switch (mode) {
			case 0:
				pBody->Die();
				break;
			case 1:
				pBody->Hit(pBody->mHealth, ZeroVector, 0);
				break;
		}
	}
}

/* SendVisibleFromNode__Fii (0x80045838) */
void SendVisibleFromNode(int node, int visible) {
	int SetFlags;
	int ClearFlags;
	ushort *pNodes;
	ushort NumLinks;

	if (!pCurrentPlayer || GNumberOfPlayers == 1)
		SetFlags = 65;
	else if (pCurrentPlayer == MechList)
		SetFlags = 64;
	else
		SetFlags = 1;

	if (visible) {
		ClearFlags = ~SetFlags;
		SetFlags = 0;
	} else {
		ClearFlags = -1;
	}

	pNodes = Trig_GetLinksPointer(node);
	NumLinks = *pNodes++;

	{
		int i;

		for (i = 0; i < NumLinks; i++) {
			ushort VisNode = *pNodes++;
			short *pVisNode = OffsetList[VisNode];

			switch (*pVisNode) {
				case 5: {
					CItem *pUp;

					for (pUp = PowerUpList; pUp; pUp = pUp->mNextItem) {
						if (((CBody *)pUp)->mNode == VisNode) {
							pUp->mFlags = (pUp->mFlags & ClearFlags) | SetFlags;
							break;
						}
					}

					break;
				}
				case 501: {
					CBit *pBit;

					for (pBit = NonRenderedBitList; pBit; pBit = pBit->mNext) {
						if (pBit->mType == 2 && pBit->mLifetime == VisNode) {
							if (visible)
								((CLight *)pBit)->mOn = 1;
							else if (GNumberOfPlayers == 1)
								((CLight *)pBit)->mOn = 0;

							break;
						}
					}

					break;
				}
				case 9:
				case 14:
				case 2:
				case 12: {
					Item *pItem;

					pVisNode++;
					pVisNode += *pVisNode + 1;

					if ((intptr_t)pVisNode & 2)
						pVisNode++;

					pItem = Spool_FindEnviroItem(*(uint *)pVisNode);

					if (pItem)
						pItem->Flags = (pItem->Flags & ClearFlags) | SetFlags;

					break;
				}
			}
		}
	}
}

/* SendKillFromNode__Fii (0x80045a90) */
/* Fuzzy demo twin (0.73); locals from its MAIN.SYM. Sends a kill to every node
   linked from Node: platforms bounce the skater and pulse their own links, environment items are
   hit (How 1) or flagged, bits are killed. Anything else goes to the body lists by node id. */
void SendKillFromNode(int Node, int How) {
	ushort *pNodes;
	ushort NumLinks;
	int Bounced;
	int i;

	pNodes = Trig_GetLinksPointer(Node);
	NumLinks = *pNodes++;

	for (i = 0; i < NumLinks; i++) {
		ushort KillNode = *pNodes++;
		short *pKillNode = OffsetList[KillNode];
		CBaddy *pList;

		Bounced = 0;

		switch (*pKillNode) {
			case 1:
				for (pList = (CBaddy *)EnvironmentalObjectList; pList; pList = (CBaddy *)pList->mNextItem) {
					if (pList->mNode == KillNode && (pList->mFlags & 0x100) &&
					    (pList->mSpecificFlags & 0x10)) {
						pList->mFlags |= 0x20;

						if (!(pList->mSpecificFlags & 0x20)) {
							Trig_SendPulse(Trig_GetLinksPointer(pList->mNode));

							pList->mDeathPulseSent = 1;
							((CPlatform *)pList)->Bounce(pCurrentPlayer, pCurrentPlayer->mVel);
							Bounced = 1;
						}
					}
				}

				if (!Bounced) {
					KillInList(KillNode, BaddyList, How);
					KillInList(KillNode, EnvironmentalObjectList, How);
				}

				break;
			case 2:
			case 9: {
				Item *pItem;

				pKillNode++;
				pKillNode += *pKillNode + 1;

				if ((intptr_t)pKillNode & 2)
					pKillNode++;

				pItem = Spool_FindEnviroItem(*(uint *)pKillNode);

				if (!pItem)
					break;

				if (How == 1)
					Exp_HitEnvItem(pItem, 0, 0xffff);
				else
					pItem->Flags |= 0x41;

				break;
			}
			case 10:
			case 11:
			case 12:
				break;
			/* 13 and 14 are separate compare-tree nodes in retail, so their bodies weren't plain
		   breaks; whatever they did folded away */
			case 13:
				Bounced = 0;
				break;
			case 14:
				Bounced = 0;
				break;
			case 500:
			case 501: {
				CBit *pBit;

				for (pBit = NonRenderedBitList; pBit; pBit = pBit->mNext) {
					if (pBit->mType == 2 && pBit->mLifetime == KillNode) {
						pBit->Die();
						break;
					}
				}

				break;
			}
		}
	}
}

/* SendSignalToNode__FP5CBodyi (0x80045d30) */
/* C++. Sets input flag bit 0 on every body in the list attached to the given node. */
void SendSignalToNode(CBody *b, int node) {
	while (b) {
		if (b->mNode == node)
			b->mInputFlags |= 1;

		b = (CBody *)b->mNextItem;
	}
}

/* SendSignalToLinks__FPUs (0x80045d70) */
void SendSignalToLinks(unsigned short *p) {
	int i, n = p[0];
	unsigned short *q = p + 1;

	for (i = 0; i < n; i++) {
		int node = *q;
		short t = *OffsetList[node];

		if (t == 1 || t == 7) {
			SendSignalToNode(BaddyList, node);
			SendSignalToNode(EnvironmentalObjectList, node);
		}

		q++;
	}
}

/* SendSuspend__FPP5CBodyi (0x80045e30) */
void SendSuspend(CBody **pp, int node) {
	CBody *b;

	for (b = *pp; b; b = (CBody *)b->mNextItem)
		if (b->mNode == node)
			b->Suspend(pp);
}

/* SendUnSuspend__Fi (0x80045e98) */
void SendUnSuspend(int node) {
	CBody *b;

	for (b = SuspendedList; b; b = (CBody *)b->mNextItem)
		if (b->mNode == node)
			b->UnSuspend();
}

/* SendSuspendOrActivate__FPUsi (0x80045ef4) */
void SendSuspendOrActivate(unsigned short *p, int mode) {
	int i, n = p[0];
	unsigned short *q = p + 1;

	for (i = 0; i < n; i++, q++) {
		int node = *q;
		short t = *OffsetList[node];

		if (t == 1 || t == 7) {
			if (mode == 5) {
				SendSuspend(&BaddyList, node);
				SendSuspend(&EnvironmentalObjectList, node);
			} else {
				SendUnSuspend(node);
			}
		}
	}
}

/* SkipString__FPc (0x80045fe4) */
char *SkipString(char *p) {
	while (*p)
		p++;

	p++;
	return p + ((intptr_t)p & 1);
}

/* SkipCommand__FPUs (0x8004601c) */
/* The case groups were fitted to retail's compare tree with tools/casetree.py; the
   empty groups (10-12, 0x88-0x89, 0x95 sharing the default label) only show up in its shape. */
ushort *SkipCommand(ushort *pCommand) {
	switch (*pCommand++) {
		case TRG_ENDCOMMANDLIST:
			return 0;

		case TRG_SETCAMANGLE:
		case TRG_SETCAMDISTXZ:
		case TRG_PLAYXA:
		case TRG_SETCAMDISTY:
		case TRG_SETCAMOFFSETX:
		case TRG_SETCAMOFFSETY:
		case TRG_SETCAMOFFSETZ:
		case TRG_SETCAMZOOM:
		case TRG_SETDROPDAMAGEON:
		case TRG_SETFADECOLOR:
		case TRG_SETSKYCOLOR:
			pCommand += 2;
			break;

		case TRG_SETFOGGINGPARAMS:
			pCommand += 3;
			break;

		case 0x73:
		case TRG_DEBUGTEXT:
		case TRG_SPOOLIN:
		case TRG_SPOOLOUT:
		case TRG_SPOOLENV:
		case TRG_SETRESTART:
		case TRG_SETOBJFILE:
		case TRG_SETRESTARTNOMESSAGE:
		case TRG_SETRESTART2:
			pCommand = (ushort *)SkipString((char *)pCommand);
			break;

		case TRG_SETCHEATRESTARTS:
			while (*(char *)pCommand)
				pCommand = (ushort *)SkipString((char *)pCommand);

			pCommand++;
			break;

		case TRG_BACKGROUNDCREATE: {
			uint *p_checksum = (uint *)(((intptr_t)pCommand + 3) & ~3);

			pCommand = (ushort *)p_checksum + 5;
			break;
		}

		case TRG_GAPPOLYHIT: {
			uint *p_checksum = (uint *)(((intptr_t)pCommand + 3) & ~3);

			pCommand = (ushort *)p_checksum + 3;
			break;
		}

		case TRG_SETVISIBILITYINBOX:
			pCommand += 2;
		case TRG_KILLEVERYTHINGINBOX: {
			while (*pCommand != 0xff) {
				int *p_pos = (int *)(((intptr_t)pCommand + 3) & ~3);

				p_pos += 6;
				pCommand = (ushort *)p_pos;
			}

			pCommand++;
			break;
		}

		case TRG_SENDVISIBLE:
		case TRG_PLAYSOUND:
		case TRG_STOPSOUND:
		case TRG_BACKGROUNDON:
		case TRG_BACKGROUNDOFF:
		case TRG_SETINITIALPULSES:
		case TRG_SEEKXA:
		case TRG_SETGAMELEVEL:
		case TRG_IFPULSECOUNT:
		case TRG_SETCAMCOLIJSIDE:
		case TRG_SETCAMCOLIJBACK:
		case TRG_MIDIFADEIN:
		case TRG_MIDIFADEOUT:
		case TRG_SETCAMMODE:
		case TRG_IGNOREBRUCEINPUT:
		case TRG_SETCAMCOLIJANGLESIDE:
		case TRG_SETCAMCOLIJANGLEBACK:
		case TRG_SETOTPUSHBACK:
		case TRG_SETCAMPITCHDAMP:
		case TRG_SETOTPUSHBACK2:
		case TRG_SETSUSPENDDISTANCE:
		case TRG_SETCAMYDAMP:
		case TRG_IGNOREBRUCEINPUTFREEZE:
			pCommand++;
			break;

		case TRG_SENDPULSE:
		case TRG_SENDACTIVATE:
		case TRG_SENDSUSPEND:
		case TRG_WATEREFFECTON:
		case TRG_WATEREFFECTOFF:
		case 0x79:
		case 0x7a:
		case TRG_SPOOLLOCK:
		case TRG_SETCAMFOCUSEQUALSTRIPOD:
		case TRG_SETDROPDAMAGEOFF:
			break;

		case TRG_SENDSIGNAL:
		case TRG_SENDKILL:
		case TRG_SENDKILLLOUDLY:
			return pCommand;
		case TRG_ALLOWXA:
		case TRG_DISALLOWXA:
			return pCommand;
		case TRG_ENDIF:
		default:
			return pCommand;
	}

	return pCommand;
}

/* Trig_AllocateDualBuffer__FUi (0x80046344) */
void Trig_AllocateDualBuffer(unsigned int size) {}

/* Trig_RemoveDualBuffer__Fv (0x8004634c) */
void Trig_RemoveDualBuffer(void) {
	if (pPSXDualBuffer)
		Mem_Delete(pPSXDualBuffer);

	pPSXDualBuffer = 0;
}

/* ExecuteCommandList__FPUsii (0x80046374) */
/* Fuzzy demo twin (0.52); locals from its MAIN.SYM. Runs a trigger node's command
   script: restart names and points, spooling, kill/visibility boxes, backgrounds, fog and sky
   colours, gaps, sounds, pulses and signals to linked nodes, and the camera settings. */
void ExecuteCommandList(ushort *pCommands, int Node, int WaitForSpooling) {
	int NewFoggingParameters = 0;
	int ZHither;
	int ZDpqMin;
	uint Range;
	SCommandPoint *pCommandPoint = GetCommandPoint(Node);

	while (*pCommands != TRG_ENDCOMMANDLIST) {
		ushort Command = *pCommands++;

		switch (Command) {
			case TRG_SETCHEATRESTARTS:
				NumCheatRestarts = 0;

				while (*(char *)pCommands) {
					CheatRestarts[NumCheatRestarts++] = (char *)pCommands;
					pCommands = (ushort *)SkipString((char *)pCommands);
				}

				pCommands++;
				break;

			case TRG_SETDUALBUFFERSIZE:
				Trig_RemoveDualBuffer();
				Trig_AllocateDualBuffer(*pCommands++ * 1000);
				break;

			case TRG_SETRESTART:
			case TRG_SETRESTARTNOMESSAGE: {
				int OldRestart = RestartNode;

				if (GGame == 7)
					Trig_SetRestart(HorseRestarts[GHorseRestart]);
				else
					Trig_SetRestart((char *)pCommands);

				pCommands = (ushort *)SkipString((char *)pCommands);

				if (OldRestart != 0xffff && RestartNode != OldRestart) {
					if (Command != TRG_SETRESTARTNOMESSAGE)
						Mess_DeleteAll();

					Front_SaveGameState();
				}

				break;
			}

			case TRG_SETRESTART2:
				if (GNumberOfPlayers == 2) {
					if (GGame == 7)
						Trig_SetRestart(HorseRestarts[GHorseRestart]);
					else
						Trig_SetRestart((char *)pCommands);
				}

				pCommands = (ushort *)SkipString((char *)pCommands);
				break;

			case TRG_SPOOLIN:
				Spool_PSX((char *)pCommands, 0, 0);

				if (WaitForSpooling)
					Spool_Sync();

				pCommands = (ushort *)SkipString((char *)pCommands);
				break;

			case TRG_SPOOLOUT:
				Spool_ClearPSX((char *)pCommands);

				pCommands = (ushort *)SkipString((char *)pCommands);
				break;

			case TRG_SPOOLENV:
				Spool_PSX((char *)pCommands, 1, 0);

				if (WaitForSpooling)
					Spool_Sync();

				pCommands = (ushort *)SkipString((char *)pCommands);
				break;

			case TRG_SPOOLLOCK:
				Spool_Sync();
				break;

			case TRG_SPOOLMIDI:
				SFX_SpoolInMIDI((char *)pCommands);

				pCommands = (ushort *)SkipString((char *)pCommands);
				break;

			case TRG_SETOBJFILE:
				pCurrentObjFile = (char *)pCommands;
				pCommands = (ushort *)SkipString((char *)pCommands);
				CurrentObjFileRegion = Spool_FindRegion(pCurrentObjFile);
				break;

			case TRG_KILLEVERYTHINGINBOX: {
				CVector min, max;

				while (*pCommands != 0xff) {
					int *p_pos = (int *)(((intptr_t)pCommands + 3) & ~3);

					min.vx = *p_pos++ << 12;
					min.vy = *p_pos++ << 12;
					min.vz = *p_pos++ << 12;
					max.vx = *p_pos++ << 12;
					max.vy = *p_pos++ << 12;
					max.vz = *p_pos++ << 12;
					Utils_KillEverythingInBox(min, max);

					pCommands = (ushort *)p_pos;
				}

				pCommands++;
				break;
			}

			case TRG_SEEKXA:
				pCommands++;
				break;

			case TRG_SETVISIBILITYINBOX: {
				ushort on = *pCommands++;
				ushort in = *pCommands++;
				CVector min, max;

				while (*pCommands != 0xff) {
					int *p_pos = (int *)(((intptr_t)pCommands + 3) & ~3);

					min.vx = *p_pos++ << 12;
					min.vy = *p_pos++ << 12;
					min.vz = *p_pos++ << 12;
					max.vx = *p_pos++ << 12;
					max.vy = *p_pos++ << 12;
					max.vz = *p_pos++ << 12;
					Utils_SetVisibilityInBox(min, max, on, in);

					pCommands = (ushort *)p_pos;
				}

				pCommands++;
				break;
			}

			case TRG_WATEREFFECTON:
				DAT_800d1f14 = 1;
				break;

			case TRG_WATEREFFECTOFF:
				DAT_800d1f14 = 0;
				break;

			case TRG_BACKGROUNDCREATE: {
				uint *p_checksum = (uint *)(((intptr_t)pCommands + 3) & ~3);
				uint checksum = *p_checksum++;
				CSVector ang_vel;

				pCommands = (ushort *)p_checksum;
				ang_vel.vx = *pCommands++;
				ang_vel.vy = *pCommands++;
				ang_vel.vz = *pCommands++;
				new CBackground(checksum, ang_vel);
				break;
			}

			case TRG_BACKGROUNDON:
				Backgrnd_On(*pCommands++);
				break;

			case TRG_BACKGROUNDOFF:
				Backgrnd_Off(*pCommands++);
				break;

			case TRG_SETFOGGINGPARAMS:
				ZHither = *pCommands++;
				ZDpqMin = *pCommands++;
				Range = *pCommands++;
				NewFoggingParameters = 1;
				break;

			case TRG_SETFADECOLOR: {
				uint fadecolor = *pCommands++ << 16;

				fadecolor += *pCommands++;
				M3d_FadeColour = fadecolor;
				break;
			}

			case TRG_SETSKYCOLOR: {
				uint skycolor = *pCommands++ << 16;

				skycolor += *pCommands++;
				Db_SkyColor = skycolor;
				Db_UpdateSky();
				break;
			}

			case TRG_GAPPOLYHIT: {
				uint *pChecksum = (uint *)(((intptr_t)pCommands + 3) & ~3);
				uint Checksum = *pChecksum++;
				int GapType;

				pCommands = (ushort *)pChecksum;
				GapType = *pCommands++;

				if (GVInfo1[0].VideoMode != 2) {
					int i;

					for (i = 0; i < pCurrentPlayer->mGaps; i++) {
						if (pCurrentPlayer->mGapChecksum[i] &&
						    pCurrentPlayer->mGapChecksum[i] == pCommandPoint->Checksum) {
							pCurrentPlayer->mGapChecksum[i] = 0;

							if (!pCurrentPlayer->mInAir) {
								SGapTrick *pGapTrick = GapTricks;

								while (pGapTrick->Type != -1) {
									if (pGapTrick->Type == pCurrentPlayer->mGapType[i])
										break;

									pGapTrick++;
								}

								if (pGapTrick->Type != -1) {
									if (pGapTrick->Type < 20000) {
										pCurrentPlayer->Trick_Trick(77, pGapTrick->Name,
										                            -pGapTrick->Score, 0);
										SFX_PlayX(161, 0x2000, 0);
									} else {
										pCurrentPlayer->mpGrindGapTrick = pGapTrick;
									}
								}
							}
						}
					}

					if (Checksum && pCurrentPlayer->mGaps < 10) {
						pCurrentPlayer->mGapChecksum[pCurrentPlayer->mGaps] = Checksum;
						pCurrentPlayer->mGapType[pCurrentPlayer->mGaps] = GapType;
						pCurrentPlayer->mGaps++;
					}
				}

				break;
			}

			case TRG_PLAYSOUND:
				SFX_PlayX((short)*pCommands++, 0x2000, 0);
				break;

			case TRG_STOPSOUND:
				SFX_Stop((short)*pCommands++);
				break;

			case TRG_MIDICHANNELMUTE:
				SFX_MIDIChannelMute(*pCommands++);
				break;

			case TRG_MIDIFADEIN:
				SFX_MIDIFadeIn(*pCommands++);
				break;

			case TRG_MIDIFADEOUT:
				SFX_MIDIFadeOut(*pCommands++);
				break;

			case TRG_SETREVERBTYPE:
				SFX_SetReverbType(*pCommands++);
				break;

			case TRG_ENDLEVEL:
				Game_Over();
				break;

			case TRG_IGNOREBRUCEINPUT:
				if (MechList)
					MechList->mIgnoreBruceInputTimer = *pCommands++;

				break;

			case TRG_IGNOREBRUCEINPUTFREEZE:
				if (MechList)
					MechList->mIgnoreBruceInputFreezeTimer = *pCommands++;

				break;

			case TRG_KILLBRUCE: {
				CVector StartPos;
				CVector BestStartPos;
				CSVector *pBestAngles;
				CBruce *pBruce;
				int BestDist = 10000000;
				ushort *pLinks = Trig_GetLinksPointer(Node);

				if (*pLinks++) {
					pBestAngles = Trig_GetPosition(BestStartPos, *pLinks);
					BestDist = 1;
				} else {
					int n;

					for (n = 0; n < NumHorseRestarts; n++) {
						int i;

						for (i = 0; i < NumNodes; i++) {
							CVector Dummy;

							if (*OffsetList[i] == NODE_RESTART &&
							    Utils_CompareStrings((char *)(Trig_GetPosition(Dummy, i) + 1),
							                         HorseRestarts[n], -1)) {
								CSVector *pAngles = Trig_GetPosition(StartPos, i);
								int Dist = (StartPos - pCurrentPlayer->m1DC).Length();

								if (Dist < BestDist) {
									BestDist = Dist;
									BestStartPos = StartPos;
									pBestAngles = pAngles;
								}
							}
						}
					}
				}

				if (BestDist != 10000000) {
					pBruce = pCurrentPlayer;
					pBruce->mPos = BestStartPos;
					pBruce->mVel.vx = 0;
					pBruce->mVel.vy = -16384;
					pBruce->mVel.vz = 0;
					pBruce->mAcc.vx = 0;
					pBruce->mAcc.vy = 0;
					pBruce->mAcc.vz = 0;
					pBruce->mPos.vy += -102400;
					pBruce->Trick_Bail();

					pBruce->mDoingATrick = 0;
					pBruce->mJumpPending = 0;
					pBruce->mJumpStrength = 0;
					pBruce->ResetTrickBuffers();

					pBruce->mStickToRail = 0;
					pBruce->mAngles = *pBestAngles;
					FudgeAngles(pBruce);

					pBruce->mOldPos = pBruce->mPos;
					pBruce->mSafePos = pBruce->mPos;
					pBruce->mInAir = 0;
					pBruce->mpCamera->ResetOrientation();
					pBruce->RunAnim(0, 0, -1, -1);
					pBruce->mPanel.OutOfBounds();
				} else {
					Front_Restart();
				}

				break;
			}

			case TRG_IFPULSECOUNT:
				if (pCommandPoint->PulsesReceived != *pCommands++) {
					while (*pCommands != TRG_ENDIF)
						pCommands = SkipCommand(pCommands);

					pCommands++;
				}

				break;

			case TRG_SETINITIALPULSES:
				if (!pCommandPoint->NumPulsesSet) {
					pCommandPoint->NumPulses = *pCommands;
					pCommandPoint->NumPulsesSet = 1;
				}

				pCommands++;
				break;

			case TRG_SENDPULSE:
				if (pCommandPoint) {
					if (pCommandPoint->NumPulses) {
						Trig_SendPulse(Trig_GetLinksPointer(Node));

						if (pCommandPoint->NumPulses != 0xffff)
							pCommandPoint->NumPulses--;
					}
				} else {
					Trig_SendPulse(Trig_GetLinksPointer(Node));
				}

				break;

			case TRG_SENDACTIVATE:
			case TRG_SENDSUSPEND:
				SendSuspendOrActivate(Trig_GetLinksPointer(Node), Command);
				break;

			case TRG_SENDSIGNAL:
				SendSignalToLinks(Trig_GetLinksPointer(Node));
				break;

			case TRG_SENDVISIBLE:
				SendVisibleFromNode(Node, *pCommands++);
				break;

			case TRG_SENDKILL:
				SendKillFromNode(Node, 0);
				break;

			case TRG_SENDKILLLOUDLY:
				SendKillFromNode(Node, 1);
				break;

			case TRG_DEBUGTEXT:
			case TRG_LOADAI:
				pCommands = (ushort *)SkipString((char *)pCommands);
				break;

			case TRG_SETSUSPENDDISTANCE:
				SuspendDistance = *pCommands++;
				break;

			case TRG_SETOTPUSHBACK:
				M3d_OTPushback = *pCommands++;
				break;

			case TRG_SETOTPUSHBACK2:
				M3d_OTPushback2 = *pCommands++;
				break;

			case TRG_SETCAMFOCUSEQUALSTRIPOD:
				if (CameraList)
					CameraList->mFocus = CameraList->mTripod;

				break;

			case TRG_SETCAMZOOM:
				if (CameraList) {
					ushort a = pCommands[0];
					ushort b = pCommands[1];

					pCommands += 2;
					CameraList->SetZoom(a, b);
				} else
					pCommands += 2;

				break;

			case TRG_SETCAMPITCHDAMP:
				if (CameraList)
					CameraList->SetPitchDamper(*pCommands);

				pCommands++;
				break;

			case TRG_SETCAMYDAMP:
				if (CameraList)
					CameraList->SetTripodPosYDamper(*pCommands);

				pCommands++;
				break;

			case TRG_SETCAMANGLE:
				if (CameraList) {
					short a = pCommands[0];
					ushort b = pCommands[1];

					pCommands += 2;
					CameraList->SetCamAngle(a, b);
				} else
					pCommands += 2;

				break;

			case TRG_SETCAMDISTXZ:
				if (CameraList) {
					ushort a = pCommands[0];
					ushort b = pCommands[1];

					pCommands += 2;
					CameraList->SetCamXZDistance(a, b);
				} else
					pCommands += 2;

				break;

			case TRG_SETCAMDISTY:
				if (CameraList) {
					short a = pCommands[0];
					ushort b = pCommands[1];

					pCommands += 2;
					CameraList->SetCamYDistance(a, b);
				} else
					pCommands += 2;

				break;

			case TRG_SETCAMOFFSETX:
				if (CameraList) {
					short a = pCommands[0];
					ushort b = pCommands[1];

					pCommands += 2;
					CameraList->SetCamXOffset(a, b);
				} else
					pCommands += 2;

				break;

			case TRG_SETCAMOFFSETY:
				if (CameraList) {
					short a = pCommands[0];
					ushort b = pCommands[1];

					pCommands += 2;
					CameraList->SetCamYOffset(a, b);
				} else
					pCommands += 2;

				break;

			case TRG_SETCAMOFFSETZ:
				if (CameraList) {
					short a = pCommands[0];
					ushort b = pCommands[1];

					pCommands += 2;
					CameraList->SetCamZOffset(a, b);
				} else
					pCommands += 2;

				break;

			case TRG_SETCAMCOLIJSIDE:
				if (CameraList)
					CameraList->SetCollisionRayLR((short)*pCommands++);
				else
					pCommands++;

				break;

			case TRG_SETCAMCOLIJBACK:
				if (CameraList)
					CameraList->SetCollisionRayBack((short)*pCommands++);
				else
					pCommands++;

				break;

			case TRG_SETCAMCOLIJANGLESIDE:
				if (CameraList)
					CameraList->SetCollisionAngLR(*pCommands++);
				else
					pCommands++;

				break;

			case TRG_SETCAMCOLIJANGLEBACK:
				if (CameraList)
					CameraList->SetCollisionAngBack(*pCommands++);
				else
					pCommands++;

				break;

			case TRG_SETCAMMODE:
				if (CameraList)
					CameraList->mCameraMode = (ECameraMode)*pCommands;

				pCommands++;
				break;

			case TRG_SETCAMCOMPLEXMODE:
				if (CameraList)
					CameraList->SetComplexMode(*pCommands++);
				else
					pCommands++;

				break;

			case TRG_SETGAMELEVEL:
				CurrentGameLevel = *pCommands++;
				break;

			case TRG_ALLOWXA:
				Redbook_XAAllow(1);
				break;

			case TRG_DISALLOWXA:
				Redbook_XAAllow(0);
				break;

			case TRG_SETDROPDAMAGEON:
				if (MechList)
					MechList->SetM8A4(pCommands[0], pCommands[1]);
			case TRG_PLAYXA:
				pCommands += 2;
				break;

			case TRG_SETDROPDAMAGEOFF:
				if (MechList)
					MechList->m8A4 = 0;

				break;
		}
	}

	if (NewFoggingParameters)
		M3dInit_SetFoggingParams(ZHither, ZDpqMin, Range);
}

/* Trig_ParseTRGFile__Fv (0x80047214) */
void Trig_ParseTRGFile(void) {
	int i, j, k;
	unsigned short **e;
	unsigned short ui;
	unsigned short *b;
	short *s;
	CVector v;
	int *r;
	STrigLightParams *q;
	void *it;

	for (i = 0; i < NumNodes; i++) {
		b = (ushort *)OffsetList[i];

		switch (*(short *)b) {
			case NODE_SEEDABLEBADDY:
				GetFlag(CREATEDATSTART, (unsigned char *)b + (((short *)b)[3] * 2 + 8));
				break;
			case NODE_BADDY:
				if (GetFlag(CREATEDATSTART, (unsigned char *)b + (((short *)b)[3] * 2 + 8)))
					Trig_CreateObject(i);

				break;
			case NODE_POWRUP:
				r = (int *)Trig_GetPosition(v, i);

				if (*(unsigned short *)r == 1)
					Trig_CreateObject(i);

				break;
			case NODE_COMMANDPOINT:
				ui = i;
				b += 1;
				b = (unsigned short *)((char *)b + (*(short *)b * 2 + 2));

				if ((intptr_t)b & 2)
					b += 1;

				CreateCommandPoint(*(int *)b, ui, (ushort *)((char *)b + 4));
				break;
			case NODE_CRATE:
			case NODE_BARREL:
				b += 1;
				b = (unsigned short *)((char *)b + (*(short *)b * 2 + 2));

				if ((intptr_t)b & 2)
					b += 1;

				CreateCommandPoint(*(int *)b, i, (ushort *)DAT_800d1a10);
				break;
			case NODE_LIGHT:
				q = (STrigLightParams *)Trig_GetPosition(v, i);
				UseCacheForBits = 0;
				it = __nw__4CBitUi(SIZEOF_CLIGHT);

				if (it)
					__6CLightRC7CVectoriiiiiUcUcUcUcUcUc(it, v, i, q->w0, q->w2, q->w4, q->w6, q->b8,
					                                     q->b10, q->b12, q->b14, q->b16, q->b18);

				UseCacheForBits = 1;
				break;
			case NODE_TRICKOB:
			case NODE_GOALOB:
				it = __nw__6CClassUi(SIZEOF_CTRICKOB);

				if (it)
					__8CTrickObUs(it, i, 0);

				break;
			case NODE_CAMPT:
				CamPt_New(i);
				break;
		}
	}

	Trig_ClearTrigMenu();

	for (j = 0; j < NumNodes; j++) {
		short *pNode = (short *)OffsetList[j];

		if (*pNode == NODE_RESTART) {
			char *pNodeName = (char *)Trig_GetPosition(v, j) + 6;

			MenuFilenamePointers[NumTrigMenuEntries++] = pNodeName;
		}
	}
}

/* Trig_GetLinksPointer__Fi (0x8004754c) */
unsigned short *Trig_GetLinksPointer(int n) {
	unsigned short *p = (ushort *)OffsetList[n];

	switch (*p) {
		case NODE_BADDY:
			return p + 3;
		case NODE_POWRUP:
			return p + 2;
		case NODE_CRATE:
		case NODE_POINT:
		case NODE_COMMANDPOINT:
		case NODE_RESTART:
		case NODE_BARREL:
		case NODE_RAILPOINT:
		case NODE_RAILDEF:
		case NODE_TRICKOB:
		case NODE_CAMPT:
		case NODE_GOALOB:
		case NODE_SCRIPTPOINT:
		case NODE_CAMERAPATH:
			return p + 1;
	}

	return p; /* (retail falls off the end here, returning p by chance) */
}

/* Trig_GetPosition__FR7CVectori (0x80047600) */
/* Demo twin. Finds a trigger node's word-aligned position (after its variable-length header), stores it <<12 and returns the data past it. Each case repeated the stores; cross-jumping merges them. */
CSVector *Trig_GetPosition(CVector &pos, int node) {
	ushort *pNode = (ushort *)OffsetList[node];
	ushort *pPos;
	int *pLong;

	switch (*pNode) {
		{
			uchar *pFlags;
			case NODE_BADDY:
			case NODE_SEEDABLEBADDY:
				pFlags = (uchar *)(pNode + 4 + pNode[3]);

				while (*pFlags != 0xff)
					pFlags++;

				pLong = (int *)((intptr_t)(pFlags + 4) & ~3);
				pos.vx = pLong[0] << 12;
				pos.vy = pLong[1] << 12;
				pos.vz = pLong[2] << 12;
				break;
			case NODE_POINT:
			case NODE_RESTART:
			case NODE_RAILPOINT:
			case NODE_RAILDEF:
			case NODE_CAMPT:
			case NODE_SCRIPTPOINT:
			case NODE_CAMERAPATH:
				pPos = pNode + 2 + pNode[1];
				pLong = (int *)((intptr_t)((uchar *)pPos + 3) & ~3);
				pos.vx = pLong[0] << 12;
				pos.vy = pLong[1] << 12;
				pos.vz = pLong[2] << 12;
				break;
			case NODE_POWRUP:
				pPos = pNode + 3 + pNode[2];
				pLong = (int *)((intptr_t)((uchar *)pPos + 3) & ~3);
				pos.vx = pLong[0] << 12;
				pos.vy = pLong[1] << 12;
				pos.vz = pLong[2] << 12;
				break;
			case NODE_LIGHT:
			case NODE_OFFLIGHT:
				pPos = pNode + 1;
				pLong = (int *)((intptr_t)((uchar *)pPos + 3) & ~3);
				pos.vx = pLong[0] << 12;
				pos.vy = pLong[1] << 12;
				pos.vz = pLong[2] << 12;
				break;
		}
	}

	return (CSVector *)(pLong + 3);
}
