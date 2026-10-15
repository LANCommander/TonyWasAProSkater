/* TAPES.CPP: CTape, the collectable video tapes (a CBody): creating them, their glow,
   physics, and the display when one is picked up. Declarations in game/tapes.h. */
#include "game/tapes.h"
#include "game/bit.h"
#include "game/camera.h"
#include "game/career.h"
#include "game/front.h"
#include "game/globals.h"
#include "game/m3d.h"
#include "game/main.h"
#include "game/mess.h"
#include "game/powerup.h"
#include "game/psyq.h"
#include "game/spool.h"
#include "game/utils.h"

extern STapeMessage TapeMessage[];

/* DisplayTitle__5CTape (0x80072ec8) */
/* Whether tape titles may be shown. */
int CTape::DisplayTitle() {
	return mOkayToDisplayTitle != 0;
}

/* DisplayContinueButton__5CTape (0x80072ed4) */
/* Whether the tape message's continue button may be shown. */
int CTape::DisplayContinueButton() {
	return mOkayToDisplayContinueButton != 0;
}

/* __5CTapeUsRC7CVectorT2iiT2Uii (0x80072ee0) */
/* Locals and statement order from the demo's MAIN.SYM. CItem is the demo's
   layout; THPS1's CBody is 0x2c bytes bigger than the demo's, so its fields are placed by offset. */
CTape::CTape(ushort PowerupType, const CVector &Pos, const CVector &Pos2, int Size1, int Size2,
             const CVector &Vel, uint Flags, int Lifetime) {
	mSize = Size1;
	mSize1 = Size1;
	mSize2 = Size2;
	mVec1 = Pos;
	mVec2 = Pos2;
	mScale.Set(Size1, Size1, Size1);

	mTimer = 0;
	mType = PowerupType;
	mLifetime = Lifetime;
	mTapeNum = 0;
	mSessionTapeNum = 0;
	mOrgPos = Pos;
	mPos = Pos;
	mVel = Vel;
	mFric.Set(5, 5, 5);

	mAngVel.vy = 100;
	mAcc.vy = 0x8000;
	mWobbleSpeed = Rnd(20) + 90;
	mMaxWobbleAmplitude = 50;
	mWobbleAmplitude = 50;
	mRMinor = 300;
	InitItem("items");

	switch (mType) {
		case 0x664:
			InitItem("skmedals");

			mIs3d = 1;
			mModel = Spool_GetModel(0x54636518, CurrentMedalsFileRegion);
			break;
		case 0x665:
			InitItem("skmedals");

			mIs3d = 1;
			mModel = Spool_GetModel(0xba6d0434, CurrentMedalsFileRegion);
			break;
		case 0x666:
			InitItem("skmedals");

			mIs3d = 1;
			mModel = Spool_GetModel(0x2364558e, CurrentMedalsFileRegion);
			break;
		case 0x10:
		case 0x12:
			mMessageTable = TapeMessage;
			mAngVel.vy = 150;
			mIs3d = 1;
			mModel = Spool_GetModel(0xd5108e8c, CurrentItemsFileRegion);
			break;
	}

	if (GGame != 1) {
		switch (mType) {
			case 4:
			case 5:
			case 6:
			case 10:
			case 15:
			case 16:
				mLifetime = 1;
				break;
		}
	} else {
		switch (mType) {
			case 4:
			case 5:
			case 6:
			case 10:
			case 15:
				if (Career_Got(2))
					mLifetime = 1;

				break;
			case 16:
				if (Career_Got(3))
					mLifetime = 1;

				break;
		}
	}

	AttachTo(&PowerUpList);

	if (!mIs3d)
		DontDisplay();
}

/* _._5CTape (0x80073228) */
/* Unlinks the tape from the power-up list and deletes its glow; CBody's
   destructor does the rest. */
CTape::~CTape() {
	DeleteFrom(&PowerUpList);
	CTape::DeleteStuff(); /* (a virtual call in a destructor: this class's own) */
}

/* DontDisplay__5CTape (0x80073294) */
/* Hides the tape (flags 0x41) and deletes its glow (virtual DeleteStuff). */
void CTape::DontDisplay() {
	mFlags |= 0x41;
	DeleteStuff();
}

/* CreateBit__5CTape (0x800732d0) */
/* Makes new bits come from the bit cache. */
void CTape::CreateBit() {
	UseCacheForBits = 1;
}

/* DeleteStuff__5CTape (0x800732e4) */
/* Deletes the tape's glow. */
void CTape::DeleteStuff() {
	delete mpGlow;
	mpGlow = 0;
}

/* DoPhysics__5CTape (0x8007333c) */
/* A tape spins at its angular velocity and bobs up and down about its original
   position. */
void CTape::DoPhysics() {
	mAngles += mAngVel;
	mAngles.Mask();

	mPos = mOrgPos;
	mPos.vy += (mWobbleAmplitude * rcossin_tbl[mT & 0xfff].s / 4096) << 12;
	mT += mWobbleSpeed;
}

/* AI__5CTapei (0x800733ec) */
/* Locals, their order and types come from the demo's MAIN.SYM; field offsets are THPS1's
   (its CBody is 0x2c bytes larger than the THPS2 demo's). */
void CTape::AI(int tape_no) {
	mTimer++;

	if (mTimer < 60)
		mOkayToDisplayTitle = 1;
	else
		mOkayToDisplayTitle = 0;

	if (mSessionTapeNum * 20 + 80 < mTimer)
		mOkayToDisplayContinueButton = 1;
	else
		mOkayToDisplayContinueButton = 0;

	if (mTimer > 60) {
		int temptime = mTimer - 60;
		CVector pos_offset = (mVec2 - mVec1) * temptime / 20;
		int size_offset = (mSize2 - mSize1) * temptime / 20;

		if (temptime < 20) {
			mOrgPos = pos_offset + mVec1;
			mSize = size_offset + mSize1;
			mWobbleAmplitude = mMaxWobbleAmplitude - mMaxWobbleAmplitude * temptime / 20;
		} else {
			mOrgPos = mVec2;
			mWobbleAmplitude = 0;
			mSize = mSize2;
		}
	}

	int message_count = 0;
	STapeMessage *messages[5];
	int message_no;

	messages[0] = 0;
	messages[1] = 0;
	messages[2] = 0;
	messages[3] = 0;
	messages[4] = 0;
	mScale.Set(mSize, mSize, mSize);

	for (int i = 0; i < 5; i++) {
		if (GTape[i]) {
			if (i == 4) {
				messages[4] = &mMessageTable[GTape[4]->mTapeNum];
			} else {
				STapeMessage *m = &mMessageTable[GTape[i]->mTapeNum];

				if (m->essential == 1)
					messages[i] = m;
			}
		} else {
			messages[i - 1] = &mMessageTable[GTape[i - 1]->mTapeNum];
			break;
		}
	}

	for (int i = 0; i < 5; i++)
		if (messages[i])
			message_count++;

	message_no = 0;

	for (int i = 0; i < mSessionTapeNum; i++)
		if (messages[i])
			message_no++;

	if (message_no * 20 + 80 < mTimer && messages[mSessionTapeNum]) {
		switch (message_count) {
			case 1:
				mTextX = 130;
				mTextY = 120;
				break;
			case 2:
				switch (message_no) {
					case 0:
						mTextX = 130;
						mTextY = 85;
						break;
					case 1:
						mTextX = 130;
						mTextY = 155;
						break;
				}

				break;
			case 3:
				switch (message_no) {
					case 0:
						mTextX = 130;
						mTextY = 64;
						break;
					case 1:
						mTextX = 130;
						mTextY = 120;
						break;
					case 2:
						mTextX = 130;
						mTextY = 176;
						break;
				}

				break;
			case 4:
				switch (message_no) {
					case 0:
						mTextX = 130;
						mTextY = 50;
						break;
					case 1:
						mTextX = 150;
						mTextY = 92;
						break;
					case 2:
						mTextX = 130;
						mTextY = 134;
						break;
					case 3:
						mTextX = 150;
						mTextY = 176;
						break;
				}

				break;
			case 5:
				switch (message_no) {
					case 0:
						mTextX = 130;
						mTextY = 50;
						break;
					case 1:
						mTextX = 150;
						mTextY = 85;
						break;
					case 2:
						mTextX = 130;
						mTextY = 120;
						break;
					case 3:
						mTextX = 150;
						mTextY = 155;
						break;
					case 4:
						mTextX = 130;
						mTextY = 190;
						break;
				}

				break;
		}

		Mess_SetTextJustify(1);
		Mess_SetScale(224);
		Mess_SetRGB(122, 122, 122, 0);

		if (!messages[mSessionTapeNum]->Line2) {
			Mess_DrawText(mTextX, mTextY, messages[mSessionTapeNum]->Line1, 0, 0, 256, 0);
		} else if (!messages[mSessionTapeNum]->Line3) {
			Mess_DrawText(mTextX, mTextY - 8, messages[mSessionTapeNum]->Line1, 0, 0, 256, 0);
			Mess_DrawText(mTextX, mTextY + 8, messages[mSessionTapeNum]->Line2, 0, 0, 256, 0);
		} else {
			Mess_DrawText(mTextX, mTextY - 16, messages[mSessionTapeNum]->Line1, 0, 0, 256, 0);
			Mess_DrawText(mTextX, mTextY, messages[mSessionTapeNum]->Line2, 0, 0, 256, 0);
			Mess_DrawText(mTextX, mTextY + 16, messages[mSessionTapeNum]->Line3, 0, 0, 256, 0);
		}
	}

	DoPhysics();

	if (!mIs3d)
		CreateBit();
}

/* Tape_Create__Fiiiiiiii (0x80073a64) */
/* C++. A video tape pickup (powerup 18) flying from (x, y, z) to (e, f, g), both in the current
   camera's frame. */
CTape *Tape_Create(int x, int y, int z, int p, int e, int f, int g, int h) {
	SVECTOR v;
	VECTOR o;
	CVector pos1, pos2;
	CTape *t;

	{
		v.vx = x;
		v.vy = y;
		v.vz = z;
		{
			/* v through the camera's transform, unshifted */
			GeomAcc r;

			geom_ld_mat(pc_geom.rot, &pCurrentCamera->Transform);
			r = geom_mvmva(pc_geom.rot, 0, v.vx, v.vy, v.vz, 0, 0, 0);
			geom_st_lv(&o, r.mac);
		}

		pos1.vx = o.vx + (pCurrentCamera->Position.vx << 12);
		pos1.vy = o.vy + (pCurrentCamera->Position.vy << 12);
		pos1.vz = o.vz + (pCurrentCamera->Position.vz << 12);
	}

	{
		v.vx = e;
		v.vy = f;
		v.vz = g;
		o.vx = 0;
		o.vy = 0;
		o.vz = 0;
		{
			/* v through the camera's transform, unshifted */
			GeomAcc r;

			geom_ld_mat(pc_geom.rot, &pCurrentCamera->Transform);
			r = geom_mvmva(pc_geom.rot, 0, v.vx, v.vy, v.vz, 0, 0, 0);
			geom_st_lv(&o, r.mac);
		}

		pos2.vx = o.vx + (pCurrentCamera->Position.vx << 12);
		pos2.vy = o.vy + (pCurrentCamera->Position.vy << 12);
		pos2.vz = o.vz + (pCurrentCamera->Position.vz << 12);
	}

	t = new CTape(18, pos1, pos2, p, h, ZeroVector, 0, 0xffff);

	t->mWobbleAmplitude = 10;
	t->mMaxWobbleAmplitude = 10;
	t->mWobbleSpeed = 80;
	t->mFlags |= 0x200;
	return t;
}
