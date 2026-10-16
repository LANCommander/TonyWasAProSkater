/* POWERUP.CPP: CPowerUp, the bobbing, glowing pick-ups (letters, stat points, tapes and the
   competition medals), a CBody (declarations in game/powerup.h). */

#include "game/powerup.h"
#include "game/spool.h"
#include "game/utils.h"
#include "game/bit.h"
#include "game/bruce.h"
#include "game/panel.h"
#include "game/classsizes.h"
#include "game/trig.h"
#include "game/globals.h"
#include "game/main.h"
#include "game/mess.h"
#include "game/psyq.h"
#include "game/sfx.h"
#include "pc_video.h"

struct SMessageProg;
extern "C" void FUN_800201c8(CPowerUp *);

/* __8CPowerUpUsRC7CVectorT2Uii (0x8001f934) */
/* Fuzzy demo twin; body from the earlier C draft, as a real constructor so `this`
   is copied where retail copies it. Layout by THPS1 offsets; callees keep their linker names. */
CPowerUp::CPowerUp(ushort type, const CVector &a2_, const CVector &a3_, uint a4, int lifetime) {
	const CVector *pos = &a2_, *a3 = &a3_;

	char *c;

	this->mTimer = 0;
	this->mType = type;
	this->mHoverHeight = 0;
	this->mLifetime = lifetime;
	this->mOrgPos = *pos;
	this->mPos = *pos;

	if (a4 & 8) {
		this->mVel.vy = 0xfffc4000;
	} else {
		this->mVel = *a3;
	}

	c = (char *)&this->mFric;
	this->mFric.vx = 5;
	c[1] = 5;
	c[2] = 5;
	this->mAngVel.vy = 100;
	this->mAcc.vy = 0x8000;
	this->mWobbleSpeed = Rnd(20) + 90;
	this->mMaxWobbleAmplitude = 50;
	this->mWobbleAmplitude = 50;

	if (a4 & 16) {
		this->mDropping = 1;
		this->mDropForever = 1;
	} else if (a4 & 4) {
		this->mDropping = 1;
		this->mGroundY = Utils_GetGroundHeight(this->mOrgPos, 0, 8000);
	}

	this->mRMinor = 300;
	ShadowOn();

	this->mShadowThreshold = 300;
	InitItem("items");

	switch (this->mType) {
		case POWERUP_TYPE_MEDAL_BRONZE:
			InitItem("skmedals");

			this->mIs3d = 1;
			this->mModel = Spool_GetModel(POWERUP_MODEL_MEDAL_BRONZE, CurrentMedalsFileRegion);
			break;
		case POWERUP_TYPE_MEDAL_SILVER:
			InitItem("skmedals");

			this->mIs3d = 1;
			this->mModel = Spool_GetModel(POWERUP_MODEL_MEDAL_SILVER, CurrentMedalsFileRegion);
			break;
		case POWERUP_TYPE_MEDAL_GOLD:
			InitItem("skmedals");

			this->mIs3d = 1;
			this->mModel = Spool_GetModel(POWERUP_MODEL_MEDAL_GOLD, CurrentMedalsFileRegion);
			break;
		case POWERUP_TYPE_SKATE_S:
			this->mIs3d = 1;
			this->mModel = Spool_GetModel(POWERUP_MODEL_SKATE_S, CurrentItemsFileRegion);
			break;
		case POWERUP_TYPE_SKATE_K:
			this->mIs3d = 1;
			this->mModel = Spool_GetModel(POWERUP_MODEL_SKATE_K, CurrentItemsFileRegion);
			break;
		case POWERUP_TYPE_SKATE_A:
			this->mIs3d = 1;
			this->mModel = Spool_GetModel(POWERUP_MODEL_SKATE_A, CurrentItemsFileRegion);
			break;
		case POWERUP_TYPE_SKATE_T:
			this->mIs3d = 1;
			this->mModel = Spool_GetModel(POWERUP_MODEL_SKATE_T, CurrentItemsFileRegion);
			break;
		case POWERUP_TYPE_SKATE_E:
			this->mIs3d = 1;
			this->mModel = Spool_GetModel(POWERUP_MODEL_SKATE_E, CurrentItemsFileRegion);
			break;
		case POWERUP_TYPE_TAPE:
		case POWERUP_TYPE_HIDDEN_TAPE:
			this->mAngVel.vy = 150;
			this->mIs3d = 1;
			this->mModel = Spool_GetModel(POWERUP_MODEL_TAPE, CurrentItemsFileRegion);
			break;
		case POWERUP_TYPE_CREDIT_100:
			this->mAngVel.vy = 200;
			this->mIs3d = 1;
			this->mModel = Spool_GetModel(POWERUP_MODEL_CREDIT_100, CurrentItemsFileRegion);
			break;
		case POWERUP_TYPE_CREDIT_200:
			this->mAngVel.vy = 200;
			this->mIs3d = 1;
			this->mModel = Spool_GetModel(POWERUP_MODEL_CREDIT_200, CurrentItemsFileRegion);
			break;
		case POWERUP_TYPE_CREDIT_500:
			this->mAngVel.vy = 200;
			this->mIs3d = 1;
			this->mModel = Spool_GetModel(POWERUP_MODEL_CREDIT_500, CurrentItemsFileRegion);
			break;
	}

	if (GGame != 1) {
		switch (this->mType) {
			case POWERUP_TYPE_SKATE_K:
			case POWERUP_TYPE_SKATE_S:
			case POWERUP_TYPE_SKATE_A:
			case POWERUP_TYPE_SKATE_E:
			case POWERUP_TYPE_SKATE_T:
			case POWERUP_TYPE_HIDDEN_TAPE:
				this->mLifetime = 1;
				break;
		}
	} else {
		switch (this->mType) {
			case POWERUP_TYPE_SKATE_S:
				GPowS = this;
				break;
			case POWERUP_TYPE_SKATE_K:
				GPowK = this;
				break;
			case POWERUP_TYPE_SKATE_A:
				GPowA = this;
				break;
			case POWERUP_TYPE_SKATE_T:
				GPowT = this;
				break;
			case POWERUP_TYPE_SKATE_E:
				GPowE = this;
				break;
			case POWERUP_TYPE_HIDDEN_TAPE:
				GPowTape = this;
				break;
		}
	}

	AttachTo(&PowerUpList);

	if (this->mIs3d == 0)
		DontDisplay();
}

/* _._8CPowerUp (0x8001fe14) */
/* Leaves the power-up list and frees the extras; CBody's destructor does the
   rest. */
CPowerUp::~CPowerUp() {
	DeleteFrom(&PowerUpList);
	CPowerUp::DeleteStuff();
}

/* DontDisplay__8CPowerUp (0x8001fe7c) */
/* Hides the power-up and frees its extras (virtual DeleteStuff). */
void CPowerUp::DontDisplay() {
	Hide();
	DeleteStuff();
}

/* SetGravity__8CPowerUpii (0x8001feb8) */
/* Sets the power-up's vertical acceleration and vertical friction. */
void CPowerUp::SetGravity(int g, int fric) {
	mAcc.vy = g;
	mFric.vy = fric;
}

/* CreateBit__8CPowerUp (0x8001fec4) */
/* Makes the next bits come from the bit cache. */
void CPowerUp::CreateBit() {
	UseCacheForBits = 1;
}

/* DeleteStuff__8CPowerUp (0x8001fed8) */
/* Frees the power-up's shadow and glow. */
void CPowerUp::DeleteStuff() {
	KillShadow();

	if (mpGlow)
		delete mpGlow;

	mpGlow = 0;
}

/* SetNode__8CPowerUpi (0x8001ff34) */
/* Records the power-up's node index and marks it as having one. */
void CPowerUp::SetNode(int Node) {
	mNodeIndex = Node;
	mHasNode = 1;
}

/* Die__8CPowerUp (0x8001ff44) */
/* Kills the power-up once: marks it dead and hides it. */
void CPowerUp::Die() {
	if (!IsDead()) {
		mCBodyFlags |= CBODY_DEAD;
		DontDisplay();
	}
}

/* DoPhysics__8CPowerUp (0x8001ff84) */
void CPowerUp::DoPhysics() {
	mAngles += mAngVel;
	mAngles.Mask();

	if (mDropping) {
		if (!mDropForever && mGroundY < 0 && mGroundY >= -4) {
			if (TTime & 1) {
				int ground = Utils_GetGroundHeight(mOrgPos, 0, 8000);

				if (ground == -1) {
					mGroundY--;

					if (mGroundY < -4)
						mDropForever = 1;
				} else {
					mGroundY = ground;
				}
			}
		} else {
			mOrgPos += mVel;
			mVel.vy += mAcc.vy;
			mVel.vy -= mVel.vy >> mFric.vy;

			if (!mDropForever) {
				int bottom = mGroundY - (mHoverHeight << 12);

				if (bottom < mOrgPos.vy && mVel.vy > 0) {
					mOrgPos.vy = bottom;
					mVel.Set(0, 0, 0);

					mDropping = 0;
				}
			}

			mPos = mOrgPos;
		}
	} else {
		mPos = mOrgPos;
		int idx = mT & 4095;
		SSinCos *b;

		b = rcossin_tbl;
		mPos.vy += ((mWobbleAmplitude * b[idx].s) / 4096) << 12;
		mT += mWobbleSpeed;
	}

	mShadowPos = mOrgPos;
	mShadowPos.vy = mGroundY;
	mShadowDist = (mGroundY - mPos.vy) >> 12;

	if (mShadowDist)
		mShadowScale = 140 - (mShadowDist * 60) / 128;
	else
		mShadowScale = 0;
}

/* FUN_800201c8 (0x800201c8) */
extern "C" void FUN_800201c8(CPowerUp *p) {
	p->mTimer++;

	if (p->mLifetime != 0xffff) {
		if (p->mLifetime != 0)
			p->mLifetime--;

		if (p->mLifetime < 60) {
			p->mWobbleAmplitude = (p->mMaxWobbleAmplitude * p->mLifetime) / 60;
		}

		if ((ushort)(p->mLifetime - 31) < 29) {
			if (TTime & 2)
				p->mFlags &= 0xfffe;
			else
				p->DontDisplay();
		}

		if (p->mLifetime < 31) {
			if (TTime & 1)
				p->mFlags &= 0xfffe;
			else
				p->DontDisplay();
		}

		if (p->mLifetime == 0)
			p->Die();
	}
}

/* TakeEffect__8CPowerUpP6CBruce (0x80020328) */
/* Fuzzy demo twin (0.81 by opcode); locals from its MAIN.SYM. A skater picks up a
   power-up: letters and the tape hand out their panel message, the bonus types score a trick and
   pitch the pickup sound, then the item fires its trigger links and, if it did something, dies.
   THPS1 asks the skater whether a letter/tape counts (CBruce::CanCollect) instead of always taking it. */
void CPowerUp::TakeEffect(CBruce *pMech) {
	int HadEffect;
	int sound;

	if ((GNumberOfPlayers != 1 && pMech == MechList) ? (mFlags & ITEM_HIDDEN_MECHLIST) : (mFlags & ITEM_HIDDEN))
		return;

	HadEffect = 0;
	sound = 0;

	switch (mType) {
		case POWERUP_TYPE_SKATE_K:
		case POWERUP_TYPE_SKATE_S:
		case POWERUP_TYPE_SKATE_A:
		case POWERUP_TYPE_SKATE_E:
		case 14:
		case POWERUP_TYPE_SKATE_T:
		case POWERUP_TYPE_HIDDEN_TAPE:
			HadEffect = pMech->CanCollect(mHealth);

			if (!HadEffect)
				break;

			Mess_DeleteAll();

			{
				char *pMess = 0;

				switch (mType) {
					case POWERUP_TYPE_SKATE_S:
						SFX_PlayX(20, 8192, 0);

						if (!Panel_Letter(0))
							pMess = GameText.pSPickup;

						break;
					case POWERUP_TYPE_SKATE_K:
						SFX_PlayX(20, 8192, 0);

						if (!Panel_Letter(1))
							pMess = GameText.pKPickup;

						break;
					case POWERUP_TYPE_SKATE_A:
						SFX_PlayX(20, 8192, 0);

						if (!Panel_Letter(2))
							pMess = GameText.pAPickup;

						break;
					case POWERUP_TYPE_SKATE_T:
						SFX_PlayX(20, 8192, 0);

						if (!Panel_Letter(3))
							pMess = GameText.pTPickup;

						break;
					case POWERUP_TYPE_SKATE_E:
						SFX_PlayX(20, 8192, 0);

						if (!Panel_Letter(4))
							pMess = GameText.pEPickup;

						break;
					case 16:
						Panel_Tape(3);

						pMess = 0;
						break;
				}

				if (pMess)
					Mess_Message(pMess, Messprog_Zoom, 1, 0, 0);
			}

			break;
		case POWERUP_TYPE_CREDIT_100:
		case POWERUP_TYPE_CREDIT_200:
		case POWERUP_TYPE_CREDIT_500:
			if (pMech->mInAir)
				break;

			HadEffect = 1;

			if (!pMech->mPhysicsState)
				pMech->Trick_TakeOff();

			{
				int Pong = SFX_PlayX(18, 8192, 0);

				switch (mType) {
					case POWERUP_TYPE_CREDIT_100:
						pMech->Trick_Trick(76, "Bonus 100", -100, 0);
						SFX_SetVoicePitch(Pong, 900);
						break;
					case POWERUP_TYPE_CREDIT_200:
						pMech->Trick_Trick(76, "Bonus 200", -200, 0);
						SFX_SetVoicePitch(Pong, 1114);
						break;
					case POWERUP_TYPE_CREDIT_500:
						pMech->Trick_Trick(76, "Bonus 500", -500, 0);
						SFX_SetVoicePitch(Pong, 1500);
						break;
				}
			}

			if (!pMech->mPhysicsState)
				pMech->Trick_Land();

			break;
	}

	if (mHasNode)
		Trig_SendPulse(Trig_GetLinksPointer(mNodeIndex));

	if (HadEffect) {
		Die();

		if (sound)
			SFX_PlayX(0, 8192, 0);
	}
}

/* FUN_800206ac (0x800206ac) */
/* The power-up's per-frame display: beyond the range of every skater it drops its shadow and
   glow and stops; otherwise physics, glow and the rest. Retail's range is 8000; the port scales
   it with the draw distance like the bodies' suspend distance (Ob_AI), so pick-ups keep glowing
   as far as the level is drawn, and everywhere when it draws the entire level. */
extern "C" void FUN_800206ac(CPowerUp *p) {
	bool withinRangeOfPlayer = true;
	unsigned i;
	CGlow *g;
	CGlow *lg;
	CVector *s3;
	int s0;

	uint withinRangeOfPlayerDistance = PC_DrawSuspendDistance(8001);

	FUN_800201c8(p);

	if (GNumberOfPlayers == 1) {
		if (p->mFlags & ITEM_HIDDEN)
			return;

		withinRangeOfPlayer = p->mPlayerDist < withinRangeOfPlayerDistance;
	} else if ((p->mFlags & ITEM_HIDDEN_ALL) == ITEM_HIDDEN_ALL) {
		return;
	} else if (p->mPlayerDist >= withinRangeOfPlayerDistance) {
		withinRangeOfPlayer = (uint)Utils_CrapDist(MechList->mNextItem->mPos, p->mPos) < withinRangeOfPlayerDistance;
	}

	if (!withinRangeOfPlayer) {
		p->DeleteStuff();
		return;
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
			case POWERUP_TYPE_TAPE:
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

			case POWERUP_TYPE_SKATE_S:
			case POWERUP_TYPE_SKATE_K:
			case POWERUP_TYPE_SKATE_A:
			case POWERUP_TYPE_SKATE_T:
			case POWERUP_TYPE_SKATE_E:
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
			case POWERUP_TYPE_HIDDEN_TAPE:
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
			case POWERUP_TYPE_CREDIT_100:
				p->mpGlow->SetFringeRGB(0, 48, 48, 0);

				p->mGlowRadiusOrg = 60;
				p->mGlowRadiusAmp = 10;
				break;
			case POWERUP_TYPE_CREDIT_200:
				p->mpGlow->SetFringeRGB(0, 64, 32, 8);

				p->mGlowRadiusOrg = 60;
				p->mGlowRadiusAmp = 10;
				break;
			case POWERUP_TYPE_CREDIT_500:
				p->mpGlow->SetFringeRGB(0, 48, 0, 0);

				p->mGlowRadiusOrg = 60;
				p->mGlowRadiusAmp = 10;
				break;
		}
	}

	p->mpGlow->SetPos(*s3);

	s0 = p->mGlowRadiusOrg + (p->mGlowRadiusAmp * ((short *)rcossin_tbl)[((TTime * 600) & 0x3ff8) >> 1]) / 4096;
	p->mpGlow->SetFringeWidth(0, s0);
	p->mpGlow->SetFringeWidth(1, s0);
}

/* PowerUp_Create__FUsRC7CVectorUiiR7CVector (0x80020b18) */
/* the new power-up (0 if there was no memory) */
CPowerUp *PowerUp_Create(ushort type, const CVector &pos, uint a, int b, CVector &v) {
	return new CPowerUp(type, pos, v, a, b);
}
