/* Platform_HandleBounce__Fv (0x8003f65c) */
/* C++ (PLATFORM.CPP). Fuzzy demo twin (0.52; the demo takes the skater as a parameter); locals from
   its MAIN.SYM. Builds each skater's swept box (old to new position, padded by 0x14000) and bounces
   them off every active bouncy platform whose model box it overlaps. THPS1 checks the second skater
   too when two are playing. */
typedef unsigned char uchar;
typedef unsigned short ushort;

struct CVector {
	int vx, vy, vz;
};

struct SModel;

struct SPSXRegion {
	char pad0[0x10];
	SModel **ppModels; /* 0x10 */
	char pad14[64 - 0x14];
};

class CBruce {
  public:
	char pad0[4];
	CVector mPos; /* 0x004 */
	char pad10[0x1c - 0x10];
	CBruce *mpNext; /* 0x01c */
	char pad20[0x6c - 0x20];
	CVector mVel; /* 0x06c */
	char pad78[0xf0 - 0x78];
	CVector mOldPos; /* 0x0f0 */
};

struct Item {
	ushort Flags; /* 0x000 */
	char pad2[2];
	CVector Pos; /* 0x004 */
	char pad10[0x16 - 0x10];
	ushort ModelIndex; /* 0x016 */
	char pad18[0x1b - 0x18];
	uchar Region; /* 0x01b */
	Item *pNext;  /* 0x01c */
};

class CPlatform : public Item {
  public:
	char pad20[0x1ac - 0x20];
	int mBounceFlags; /* 0x1ac */
	void Bounce(CBruce *, const CVector &);
};

extern SPSXRegion PSXRegion[];
extern CBruce *MechList;
extern int NumMechs;
extern Item *EnvironmentalObjectList;

void Platform_HandleBounce() {
	Item *pItem;
	int tmp;
	int LP1, RP1, FP1, BP1, UP1, DP1;
	int LP2, RP2, FP2, BP2, UP2, DP2;
	CBruce *pSkater2;

	LP1 = MechList->mOldPos.vx;
	RP1 = MechList->mPos.vx;
	pSkater2 = MechList->mpNext;

	if (RP1 < LP1) {
		tmp = LP1;
		LP1 = RP1;
		RP1 = tmp;
	}

	LP1 -= 0x14000;
	RP1 += 0x14000;
	UP1 = MechList->mOldPos.vy;
	DP1 = MechList->mPos.vy;

	if (DP1 < UP1) {
		tmp = UP1;
		UP1 = DP1;
		DP1 = tmp;
	}

	UP1 -= 0x14000;
	DP1 += 0x14000;
	FP1 = MechList->mOldPos.vz;
	BP1 = MechList->mPos.vz;

	if (BP1 < FP1) {
		tmp = FP1;
		FP1 = BP1;
		BP1 = tmp;
	}

	FP1 -= 0x14000;
	BP1 += 0x14000;

	if (NumMechs >= 2) {
		LP2 = pSkater2->mOldPos.vx;
		RP2 = pSkater2->mPos.vx;

		if (RP2 < LP2) {
			tmp = LP2;
			LP2 = RP2;
			RP2 = tmp;
		}

		LP2 -= 0x14000;
		RP2 += 0x14000;
		UP2 = pSkater2->mOldPos.vy;
		DP2 = pSkater2->mPos.vy;

		if (DP2 < UP2) {
			tmp = UP2;
			UP2 = DP2;
			DP2 = tmp;
		}

		UP2 -= 0x14000;
		DP2 += 0x14000;
		FP2 = pSkater2->mOldPos.vz;
		BP2 = pSkater2->mPos.vz;

		if (BP2 < FP2) {
			tmp = FP2;
			FP2 = BP2;
			BP2 = tmp;
		}

		FP2 -= 0x14000;
		BP2 += 0x14000;
	}

	for (pItem = EnvironmentalObjectList; pItem; pItem = pItem->pNext) {
		if (pItem->Flags & 0x100) {
			if (((CPlatform *)pItem)->mBounceFlags & 16) {
				pItem->Flags |= 32;

				if (!(((CPlatform *)pItem)->mBounceFlags & 32)) {
					SModel *m = PSXRegion[pItem->Region].ppModels[pItem->ModelIndex];
					short *pBox = (short *)((char *)m + 12);

					if (RP1 >= pItem->Pos.vx + (pBox[1] << 12) &&
					    pItem->Pos.vx + (pBox[0] << 12) >= LP1 &&
					    BP1 >= pItem->Pos.vz + (pBox[5] << 12) &&
					    pItem->Pos.vz + (pBox[4] << 12) >= FP1 &&
					    DP1 >= pItem->Pos.vy + (pBox[3] << 12) &&
					    pItem->Pos.vy + (pBox[2] << 12) >= UP1)
						((CPlatform *)pItem)->Bounce(MechList, MechList->mVel);
					else if (NumMechs >= 2 && RP2 >= pItem->Pos.vx + (pBox[1] << 12) &&
					         pItem->Pos.vx + (pBox[0] << 12) >= LP2 &&
					         BP2 >= pItem->Pos.vz + (pBox[5] << 12) &&
					         pItem->Pos.vz + (pBox[4] << 12) >= FP2 &&
					         DP2 >= pItem->Pos.vy + (pBox[3] << 12) &&
					         pItem->Pos.vy + (pBox[2] << 12) >= UP2)
						((CPlatform *)pItem)->Bounce(pSkater2, pSkater2->mVel);
				}
			}
		}
	}
}
