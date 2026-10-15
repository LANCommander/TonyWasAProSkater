/* FUN_800206ac (0x800206ac) */
#include "game/powerup.h"
#include "game/bit.h"

extern int GNumberOfPlayers;
extern CItem *MechList[];
extern int TTime;
extern short rcossin_tbl[];

extern "C" void FUN_800201c8(CPowerUp *);
ulong Utils_CrapDist(const CVector &, const CVector &);

extern "C" void FUN_800206ac(CPowerUp *p) {
	unsigned v;
	unsigned i;
	CGlow *g;
	CGlow *lg;
	CVector *s3;
	int s0;

	FUN_800201c8(p);

	if (GNumberOfPlayers == 1) {
		if (p->mFlags & 1)
			return;

		v = p->mPlayerDist < 8001;
		goto test;
	} else if ((p->mFlags & 0x41) == 0x41) {
		return;
	} else if (p->mPlayerDist >= 8001) {
		v = Utils_CrapDist(MechList[0]->mNextItem->mPos, p->mPos) < 8001;
	test:
		if (!v) {
			p->DeleteStuff();
			return;
		}
	}

	p->DoPhysics();

	s3 = &p->mPos;

	if (p->mIs3d == 0)
		p->CreateBit();

	if (p->mpGlow == 0) {
		g = new CGlow(4, 2);

		p->mpGlow = g;
		g->mSkipTriangles = 1;
		p->mpGlow->SetCentreRGB(0, 0, 0);
		p->mpGlow->SetRGB(0, 0, 0);
		p->mpGlow->SetFringeRGB(1, 0, 0, 0);
		p->mpGlow->SetRadius(0);

		p->mpGlow->mProtected = 1;

		switch (p->mType) {
			case 18:
				i = 0;
				lg = p->mpGlow;

				if (lg->mNumSections != 0) {
					do {
						if (i & 1)
							lg->SetRadialRGB(i, 96, 96, 0);
						else
							p->mpGlow->SetRadialRGB(i, 64, 64, 64);

						i++;
						lg = p->mpGlow;
					} while (i < lg->mNumSections);
				}

				p->mpGlow->SetRGB(32, 0, 0);
				p->mpGlow->SetFringeRGB(1, 0, 0, 0);

				p->mGlowRadiusOrg = 10;
				p->mGlowRadiusAmp = 3;
				break;
			case 4:
			case 5:
			case 6:
			case 10:
			case 15:
				i = 0;
				lg = p->mpGlow;

				if (lg->mNumSections != 0) {
					do {
						if (i & 1)
							lg->SetRadialRGB(i, 0, 0, 96);
						else
							p->mpGlow->SetRadialRGB(i, 0, 0, 0);

						i++;
						lg = p->mpGlow;
					} while (i < lg->mNumSections);
				}

				p->mpGlow->SetRGB(0, 0, 0);
				p->mpGlow->SetFringeRGB(1, 0, 0, 0);

				p->mGlowRadiusOrg = 60;
				p->mGlowRadiusAmp = 10;
				break;
			case 16:
				i = 0;
				lg = p->mpGlow;

				if (lg->mNumSections != 0) {
					do {
						if (i & 1)
							lg->SetRadialRGB(i, 96, 96, 0);
						else
							p->mpGlow->SetRadialRGB(i, 64, 64, 64);

						i++;
						lg = p->mpGlow;
					} while (i < lg->mNumSections);
				}

				p->mpGlow->SetRGB(0, 0, 0);
				p->mpGlow->SetFringeRGB(1, 0, 0, 0);

				p->mGlowRadiusOrg = 60;
				p->mGlowRadiusAmp = 10;
				break;
			case 21:
				p->mpGlow->SetFringeRGB(0, 48, 48, 0);

				p->mGlowRadiusOrg = 60;
				p->mGlowRadiusAmp = 10;
				break;
			case 22:
				p->mpGlow->SetFringeRGB(0, 64, 32, 8);

				p->mGlowRadiusOrg = 60;
				p->mGlowRadiusAmp = 10;
				break;
			case 23:
				p->mpGlow->SetFringeRGB(0, 48, 0, 0);

				p->mGlowRadiusOrg = 60;
				p->mGlowRadiusAmp = 10;
				break;
		}
	}

	p->mpGlow->SetPos(*s3);

	s0 = p->mGlowRadiusOrg + (p->mGlowRadiusAmp * rcossin_tbl[((TTime * 600) & 0x3ff8) >> 1]) / 4096;
	p->mpGlow->SetFringeWidth(0, s0);
	p->mpGlow->SetFringeWidth(1, s0);
}
