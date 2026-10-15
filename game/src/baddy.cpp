/* BADDY.CPP: the script-driven world objects: CBaddy (a CSuper with a script interpreter,
   base of CPlatform and CCar) and the invisible CScriptOnlyBaddy (weather, smoke jets, looping
   sounds). Declarations in game/baddy.h. */
#include "game/baddy.h"
#include "game/powerup.h"
#include "game/camera.h"
#include "game/bit.h"
#include "game/career.h"
#include "game/trig.h"
#include "game/gamefmv.h"
#include "game/panel.h"
#include "game/bruce.h"
#include "game/m3d.h"
#include "game/sfx.h"
#include "game/bit2.h"
#include "game/smoke.h"
#include "game/flash.h"
#include "game/globals.h"
#include "game/mech.h"
#include "game/physics.h"
#include "game/spool.h"
#include "game/utils.h"
#include "game/video.h"

extern int NumBaddies;
extern "C" char __vt_16CScriptOnlyBaddy[];
extern "C" void dt_6CBaddy(CBaddy *p, int a);
extern "C" void __dl__5CItemPv(void *p);

/* __6CBaddy (0x8003fa34) */
/* Demo twin (statement order from its line table). Counts baddies and sets the
   script/collision defaults. */
CBaddy::CBaddy() {
	NumBaddies++;
	mNode = 0xffff;
	mRMinor = 128;
	mWaitTimer = 0;
	mVerticalCompliance = 32;
	mPushVal = 64;
	mpWeaponAttributes = 0;
	mLoopingSound = 0;
	mLoopingSoundUpdate = -1;
}

/* _._6CBaddy (0x8003faa8) */
/* Stops the baddy's looping sound; CSuper's destructor does the rest. */
CBaddy::~CBaddy() {
	NumBaddies--;

	if (mLoopingSound)
		SFX_Stop(mLoopingSound);
}

/* Die__6CBaddy (0x8003fb1c) */
/* Kills the baddy once: sends its death pulse, marks it dead and hides it. */
void CBaddy::Die() {
	if (!IsDead()) {
		SendDeathPulse();

		mCBodyFlags |= 0x40;
		mFlags |= 0x41;
	}
}

/* SendDeathPulse__6CBaddy (0x8003fb68) */
/* Once only, and only for a baddy placed at a node: pulses the node's links. */
void CBaddy::SendDeathPulse() {
	if (!mDeathPulseSent && mNode != 0xffff) {
		mDeathPulseSent = 1;
		Trig_SendPulse((ushort *)(OffsetList[mNode] + 3));
	}
}

/* SkipToEndif__6CBaddy (0x8003fbc4) */
/* Skips script words up to the endif matching the current if (nested ifs
   0x4112-0x4116 count), or to the end of the script. */
void CBaddy::SkipToEndif() {
	int depth = 0;
	ushort op;

	do {
		op = *mpScript++;

		if (op == 0x4120) {
			if (depth == 0)
				return;

			depth--;
		} else if (op >= 0x4112 && op <= 0x4116)
			depth++;
	} while (op != 0x4100);
}

/* ParseScript__6CBaddyPUs (0x8003fc1c) */
/* Demo twin; locals from its MAIN.SYM. Runs script commands until the end marker:
   0x4000 commands go to ExecuteCommand (stopping the parse if it returns 0), 0x2000 ones to another
   virtual; both take the command word (still in a1, which is why retail keeps it there). */
void CBaddy::ParseScript(ushort *p_script) {
	mpScript = p_script;

	while (*mpScript != 0x4100) {
		ushort command = *mpScript++;

		if (command & 0x4000) {
			if (!ExecuteCommand(command))
				return;
		} else if (command & 0x2000) {
			SetVariable(command);
		}
	}

	mDoScript = 0;
}

/* ExecuteCommand__6CBaddyUs (0x8003fcdc) */
/* Demo twin; locals and blocks from its MAIN.SYM. Layout by THPS1 offsets.
   The generic baddy script commands: power-ups, labels/gotos and conditionals on flags and
   variables, camera shakes, pulses and signals to nodes, animations, sounds, sparks, FMVs and
   screen flashes. Returns 0 to stop the script for this frame. */
int CBaddy::ExecuteCommand(ushort command) {
	switch (command) {
		case 17056: {
			ushort type = *mpScript++;
			int gravity;
			CPowerUp *p_powerup;

			if (type & 0x2000)
				type = GetVariable(type);

			uint a = *mpScript++;
			int b = *mpScript++;

			gravity = *mpScript++;
			p_powerup = PowerUp_Create(type, mPos, a, b, ZeroVector);

			if (p_powerup)
				p_powerup->SetGravity(gravity << 12, 5);

			break;
		}
		case 17054:
			if (mpCreatedByPlayer) {
				if (mpCreatedByPlayer == MechList) {
					CurrentWaterLevel = mPos.vy;
					break;
				}

				CurrentWaterLevel2 = mPos.vy;
				break;
			}

			CurrentWaterLevel = mPos.vy;
			CurrentWaterLevel2 = mPos.vy;
			break;
		case 17058:
			CurrentWaterDamage = *mpScript++;
			break;
		case 17050:
		case 17062: {
			CVector pos;

			if (command == 17050) {
				ushort node = *mpScript++;

				if (node & 0x2000)
					node = GetVariable(node);

				Trig_GetPosition(pos, node);
			} else {
				pos = mPos;
			}

			switch (*mpScript++) {
				case 0:
					break;
				case 1:
					break;
				case 2:
					GetScriptValue();

					mpScript += 2;
					break;
			}

			break;
		}
		case 16935: {
			ushort v = *mpScript++;

			if (v) {
				mComplexSurfaceInterval = v;
				mComplexSurfaceTimer = 0;
				mSpecificFlags |= 2;
			} else {
				mSpecificFlags &= ~2;
			}

			break;
		}
		case 17048: {
			ushort shake = *mpScript++;

			if (shake & 0x2000)
				shake = GetVariable(shake);

			if (CameraList) {
				if (shake == 0)
					CameraList->Shake(mPos, CAMERASHAKE_SMALL);
				else if (shake == 1)
					CameraList->Shake(mPos, CAMERASHAKE_MEDIUM);
				else
					CameraList->Shake(mPos, CAMERASHAKE_BIG);
			}

			break;
		}
		case 17046:
			SFX_MIDIFadeIn(*mpScript++);
			break;
		case 17047:
			SFX_MIDIFadeOut(*mpScript++);
			break;
		case 17049:
			GameFMV_SetStartTrack(*mpScript++);
			break;
		case 17043: {
			int *p = (int *)(((intptr_t)mpScript + 3) & ~3);

			GameFMV_SetScreenTexture(*p++);

			mpScript = (ushort *)p;
			break;
		}
		case 17044: {
			CVector pos;

			GameFMV_AddSoundSource(&mPos);
			ushort *p_links = Trig_GetLinksPointer(mNode);

			for (int i = 0; i < *p_links; i++) {
				Trig_GetPosition(pos, p_links[i + 1]);
				GameFMV_AddSoundSource(&mPos);
			}

			if (GGame == 7)
				break;

			GameFMV_StartFMV(GNumberOfPlayers == 1 && Levels[GLevel].FMV);
			break;
		}
		case 17053:
			GameFMV_AddSoundSource(&mPos);
			GameFMV_StartFrameBufferCopy();
			break;
		case 17045:
			GameFMV_StopFMV();
			break;
		case 17073:
		case 17074: {
			ushort node = *mpScript++;
			ushort *p_links;

			if (node & 0x2000)
				node = GetVariable(node);

			p_links = Trig_GetLinksPointer(node);

			if (command == 17073) {
				SendSignalToLinks(p_links);
				break;
			}

			Trig_SendPulse(p_links);
			break;
		}
		case 17075:
		case 17076: {
			ushort node = *mpScript++;

			if (node & 0x2000)
				node = GetVariable(node);

			if (command == 17075) {
				SendSignalToNode(BaddyList, node);
				SendSignalToNode(EnvironmentalObjectList, node);
				break;
			}

			Trig_SendPulseToNode(node);
			break;
		}
		case 17088: {
			ushort goals = *mpScript++;

			if (GGame == 1 && !Career_Got(1)) {
				int ok = GVInfo1[0].VideoMode != 2;

				if (ok)
					Panel_GoalCounter(goals);
			}

			break;
		}
		case 17072: {
			char *p_string = (char *)mpScript;

			while (*p_string)
				p_string++;

			if ((intptr_t)p_string & 1)
				mpScript = (ushort *)(p_string + 1);
			else
				mpScript = (ushort *)(p_string + 2);

			break;
		}
		case 16934:
			mVel.Zero();
			break;
		case 16896: {
			InitItem((char *)mpScript);
			char *p_string = (char *)mpScript;

			while (*p_string)
				p_string++;

			if ((intptr_t)p_string & 1)
				mpScript = (ushort *)(p_string + 1);
			else
				mpScript = (ushort *)(p_string + 2);

			break;
		}
		case 16646: {
			ushort node = *mpScript++;
			ushort *pNode;
			CVector dummy;

			if (node & 0x2000)
				node = GetVariable(node);

			pNode = (ushort *)Trig_GetPosition(dummy, node);
			mpScript = pNode + 3;
			break;
		}
		case 16644: {
			ushort label = *mpScript++;

			mpScriptLabel[label] = mpScript;
			break;
		}
		case 16645: {
			ushort *p_return = mpScript;

			while (*mpScript != 16640) {
				if (*mpScript++ == 16644) {
					ushort label = *mpScript++;

					mpScriptLabel[label] = mpScript;
				}
			}

			mpScript = p_return;
			break;
		}
		case 16641: {
			ushort label = *mpScript++;

			mpScript = mpScriptLabel[label];
			break;
		}
		case 16642: {
			ushort label = *mpScript++;

			mpScript = mpScriptLabel[label];
			return 0;
		}
		case 16662:
			if (!(mSpecificFlags & *mpScript++))
				break;

			SkipToEndif();
			break;
		case 16661: {
			ushort f = *mpScript++;

			if ((mSpecificFlags & f) == f)
				break;

			SkipToEndif();
			break;
		}
		case 16656:
		case 16657: {
			ushort dummy_script[8];
			int dummy_script_index = 0;
			int var_id = *mpScript++;
			ushort *p_start = mpScript;
			int v0 = GetVariable(var_id);
			int v1;

			while (p_start != mpScript)
				dummy_script[dummy_script_index++] = *p_start++;

			v1 = *mpScript++;

			if (v1 & 0x2000)
				v1 = GetVariable(v1);

			dummy_script[dummy_script_index] = (command == 16656) ? v0 + v1 : v0 - v1;
			p_start = mpScript;
			mpScript = dummy_script;
			SetVariable(var_id);

			mpScript = p_start;
			break;
		}
		case 16658: {
			int v0 = *mpScript++;
			int v1;

			if (v0 & 0x2000)
				v0 = GetVariable(v0);

			v1 = *mpScript++;

			if (v1 & 0x2000)
				v1 = GetVariable(v1);

			if (v1 < v0)
				break;

			SkipToEndif();
			break;
		}
		case 16659: {
			int v0 = *mpScript++;
			int v1;

			if (v0 & 0x2000)
				v0 = GetVariable(v0);

			v1 = *mpScript++;

			if (v1 & 0x2000)
				v1 = GetVariable(v1);

			if (v0 < v1)
				break;

			SkipToEndif();
			break;
		}
		case 16660: {
			int v0 = *mpScript++;
			int v1;

			if (v0 & 0x2000)
				v0 = GetVariable(v0);

			v1 = *mpScript++;

			if (v1 & 0x2000)
				v1 = GetVariable(v1);

			if (v0 == v1)
				break;

			SkipToEndif();
			break;
		}
		case 17024:
			mWaitTimer = *mpScript++;

			if (mWaitTimer & 0x2000)
				mWaitTimer = GetVariable(mWaitTimer);

			return 0;
		case 17025:
			if (mInputFlagsCopy & 1)
				break;

			mpScript--;
			return 0;
		case 16960:
			StopScript();
		case 16647:
			return 0;
		case 16897: {
			int anim = *mpScript++;
			int dir = *mpScript++;

			CycleAnim(anim, dir);
			break;
		}
		case 16898:
			RunAnim(*mpScript++, 0, -1, -1);
			break;
		case 16899:
			Hide();
			break;
		case 16900:
			Show();
			break;
		case 16901:
			Die();
			break;
		case 17040:
			SFX_PlayX((short)*mpScript++, 0x2000, 0);
			break;
		case 17041:
			SFX_PlayPos((short)*mpScript++, mPos, 0);
			break;
		case 17042: {
			ushort sparks = *mpScript++;

			if (BitCount < 200) {
				Bit_SetSparkTrajectory(mAngles);
				CSVector cone(512, 4096, 0);

				Bit_SetSparkTrajectoryCone(cone);
				Bit_SetSparkSize(1);
				Bit_SetSparkRGB(128, 128, 128);
				Bit_SetSparkFadeRGB(4, 4, 4);

				UseCacheForBits = 0;

				for (int i = 0; i < sparks; i++) {
					if (Mem_GetFree(1) > 10000)
						new CSpark(mPos, 32, 0x2000, 32);
				}

				UseCacheForBits = 1;
			}

			break;
		}
		case 17052:
			Flash_Screen(mpScript[0], mpScript[1], mpScript[2], mpScript[3], 0, mpScript[4]);

			mpScript += 5;
			break;
		/* 16768 and 17152 are invisible (same target as default); casetree.py fit. 17152 widens the
	   range past 10x the node count, so the switch stays a compare tree rather than a jump table. */
		case 16768:
		case 17059:
		case 17152:
			break;
	}

	return 1;
}

/* GetScriptValue__6CBaddy (0x80040c68) */
/* Demo twin; locals from its MAIN.SYM. Reads the next script word: a literal value,
   or (0x2000 set, 0x8000 clear) a variable number resolved through the virtual GetVariable. */
int CBaddy::GetScriptValue() {
	ushort value = *mpScript++;

	if (!(value & 0x8000)) {
		if (value & 0x2000)
			value = GetVariable(value);
	}

	return value;
}

/* SetVariable__6CBaddyUs (0x80040ccc) */
/* Script "set" commands: each reads its operands from the script stream and
   stores them into the baddy (model, position, flags, attributes, registers, ...). */
void CBaddy::SetVariable(ushort variable) {
	uint *p_word;
	ushort which;

	switch (variable) {
		case 8501:
			p_word = (uint *)(((intptr_t)mpScript + 3) & -4);
			mDeadChecksum = *p_word;
			mpScript = (ushort *)(p_word + 1);
			break;
		case 8495:
			p_word = (uint *)(((intptr_t)mpScript + 3) & -4);
			mModel = Spool_GetModel(*p_word, mRegion);
			mpScript = (ushort *)(p_word + 1);
			break;
		case 8512:
			mPos.vx = (short)GetScriptValue() << 12;
			break;
		case 8513:
			mPos.vy = (short)GetScriptValue() << 12;
			break;
		case 8514:
			mPos.vz = (short)GetScriptValue() << 12;
			break;
		case 8497:
			if (*mpScript++)
				mFlags |= 8;
			else
				mFlags &= ~8;

			break;
		case 8494:
			mVerticalCompliance = *mpScript++;
			break;
		case 8492:
			mInputSignal = *(uchar *)mpScript;
			mpScript++;
			break;
		case 8493:
			mHit = *(uchar *)mpScript;
			mpScript++;
			break;
		case 8448:
			mHealth = *mpScript++;
			break;
		case 8449:
			mRMinor = *mpScript++;
			break;
		case 8485:
			which = *mpScript++;
			mBaddyAttributes[which] = *mpScript++;
			break;
		case 8468:
			mPattern = *mpScript++;
			break;
		case 8469:
			mSubPattern = *mpScript++;
			break;
		case 8480: {
			uint reg = *mpScript++;
			int value = GetScriptValue();

			mRegister[reg & 255] = value;
			break;
		}
		case 8481:
			mAnimSpeed = *(short *)mpScript++;
			break;
		case 8482:
			mGroundToOrigin = *mpScript++;
			break;
	}
}

/* GetVariable__6CBaddyUs (0x80040f50) */
/* Fuzzy demo twin (0.74); locals from its MAIN.SYM. Reads a script variable:
   distance to / line of sight to the skater, positions (>> 12), counters, and script operands. */
short CBaddy::GetVariable(ushort variable) {
	switch (variable) {
		case 0x2136:
			return GameFMV_CurrentFrame;
		case 0x2132:
			if (MechList) {
				uint dist = Utils_XZDist(mPos, MechList->mPos);

				if (dist >= 0x2000)
					dist = 0x1fff;

				return dist;
			}

			return 0x1fff;
		case 0x2133:
			if (MechList) {
				CVector eye_pos = mPos;

				eye_pos.vy -= mOriginToEye << 12;
				return Utils_LineOfSight(eye_pos, MechList->mPos);
			}

			break;
		case 0x212e:
			return mVerticalCompliance;
		case 0x212c:
			return mInputSignal;
		case 0x212d:
			return mHit;
		case 0x2140:
			return mPos.vx >> 12;
		case 0x2141:
			return mPos.vy >> 12;
		case 0x2142:
			return mPos.vz >> 12;
		case 0x2150:
			if (MechList)
				return MechList->mPos.vx >> 12;

			break;
		case 0x2151:
			if (MechList)
				return MechList->mPos.vy >> 12;

			break;
		case 0x2152:
			if (MechList)
				return MechList->mPos.vz >> 12;

			break;
		case 0x2100:
			return mHealth;
		case 0x2129:
			return Rnd(*mpScript++);
		case 0x2120: {
			uchar i = *(uchar *)mpScript;

			mpScript++;
			return mRegister[i];
		}
		case 0x212a:
			return mNode;
		case 0x212b: {
			ushort node = *mpScript++;
			ushort *p_links;

			if (node & 0x2000)
				node = GetVariable(node);

			p_links = Trig_GetLinksPointer(node);

			if (*p_links)
				return p_links[1];

			break;
		}
	}

	return 0;
}

/* __16CScriptOnlyBaddyPsi (0x800411ac) */
/* A script-only baddy: placed from its object data at node `node`, owned by the
   current player, runs its script and joins the baddy list. */
CScriptOnlyBaddy::CScriptOnlyBaddy(short *p_info, int node) {
	mpCreatedByPlayer = pCurrentPlayer;
	p_info = SquirtPos(p_info);
	mpScript = (ushort *)SquirtAngles(p_info);
	mLoopingSoundUpdate = -1;
	mDoScript = 1;
	mNode = node;
	mFlags |= 0x41;
	mCBodyFlags &= ~0x10;
	AttachTo(&BaddyList);

	mRMinor = 0;
	mType = 203;
}

/* FUN_80041264 (0x80041264) */
/* C++ (BADDY.CPP): CScriptOnlyBaddy's destructor (vtable slot 0), kept as the egcs-ABI function
   (this, flags) retail has; the rain droplets and smoke jet are deleted through their vtables. */
/* egcs-ABI calls, as retail makes them */
extern "C" void FUN_80041264(CScriptOnlyBaddy *p, int f) {
	int i;

	p->__vptr = __vt_16CScriptOnlyBaddy;
	p->DeleteFrom(&BaddyList);

	if (p->mLoopingSound)
		SFX_Stop(p->mLoopingSound);

	if (p->mpSmokeJet)
		delete p->mpSmokeJet;

	if (p->mpWeather) {
		for (i = 0; i < p->mWeatherData; i++) {
			CGPolyLine *o = p->mpWeather[i].p_line;

			if (o)
				delete o;
		}

		Mem_Delete(p->mpWeather);
	}

	dt_6CBaddy(p, 0);

	if (f & 1)
		__dl__5CItemPv(p);
}

/* AI__16CScriptOnlyBaddy (0x80041388) */
/* Fuzzy demo twin; locals and blocks from its MAIN.SYM. Layout by THPS1 offsets. */
void CScriptOnlyBaddy::AI() {
	mFlags |= 0x41;
	mInputFlagsCopy = mInputFlags;

	if (mInputFlags & 1) {
		mInputFlags &= 0xfffe;
		mInputSignal++;
	}

	if (mDoScript) {
		if (mWaitTimer)
			mWaitTimer--;
		else
			ParseScript(mpScript);
	} else {
		Die();
	}

	if (mLoopingSoundUpdate != -1 && TTime % mLoopingSoundUpdate == 0) {
		int t = SFXLEVEL;
		unsigned int snd = mLoopingSound;

		SFXLEVEL = t * mLoopingSoundVol / 100;
		SFX_ModifyPos(snd, mPos, 0);

		SFXLEVEL = t;
	}

	if (mpWeather) {
		int ground = MechList->mShadowPos.vy;

		for (int i = 0; i < mWeatherData; i++) {
			SRainEntry *p_entry = &mpWeather[i];
			CVector pos = p_entry->p_line->mStart;

			pos += p_entry->vel;

			if (ground < pos.vy) {
				CVector offset((Rnd(2400) - 1200) << 12, (Rnd(400) - 1200) << 12,
				               (Rnd(2400) - 1200) << 12);

				pos = MechList->mPos + offset;
			}

			p_entry->p_line->mStart = pos;
			p_entry->p_line->mSegs->End = (pos - p_entry->vel) + (CameraList->mVel >> 1);
		}
	}
}

/* SetVariable__16CScriptOnlyBaddyUs (0x8004167c) */
/* A script-only baddy has no variables of its own. */
void CScriptOnlyBaddy::SetVariable(ushort variable) {
	CBaddy::SetVariable(variable);
}

/* ExecuteCommand__16CScriptOnlyBaddyUs (0x8004169c) */
/* Demo twin; locals and blocks from its MAIN.SYM. Layout by THPS1 offsets.
   Script-only baddy commands: looping sounds, snapping to the skater, a flag toggle, smoke jets
   and the rain effect (a set of falling CGPolyLine droplets around the skater). */
int CScriptOnlyBaddy::ExecuteCommand(ushort command) {
	switch (command) {
		case 17671:
			if (mLoopingSound)
				SFX_Stop(mLoopingSound);

			mLoopingSound = SFX_PlayX(*mpScript++, 0x2000, 0);
			mLoopingSoundUpdate = -1;
			mLoopingSoundVol = SFX_v;
			break;
		case 17672: {
			if (mLoopingSound)
				SFX_Stop(mLoopingSound);

			ushort sound = *mpScript++;

			mLoopingSoundUpdate = *mpScript++;
			mLoopingSound = SFX_PlayPos(sound, mPos, 0);
			mLoopingSoundVol = SFX_v;
			break;
		}
		case 17673:
			if (mLoopingSound)
				SFX_Stop(mLoopingSound);

			mLoopingSound = 0;
			break;
		case 17670:
			if (MechList) {
				mPos = MechList->mPos;
				return 1;
			}

			break;
		case 18176:
			if (*mpScript++)
				mSpecificFlags |= 0x100;
			else
				mSpecificFlags &= ~0x100;

			break;
		case 17668:
			mpScript += 4;
		case 17051:
			mpSmokeJet =
			    new CSmokeJet(mPos, mAngles, mpScript[0], 0xffff, mpScript[1], mpScript[2],
				              mpScript[3], mpScript[4], ((uchar *)mpScript)[10], 0xcce0187c);
			mpSmokeJet->mProtected = 1;
			mpScript += 6;
			break;
		case 17057:
			mpSmokeJet =
			    new CSmokeJet(mPos, mAngles, mpScript[0], 0xffff, mpScript[1], mpScript[2],
				              mpScript[3], mpScript[4], ((uchar *)mpScript)[10], 0x68b7b136);
			mpScript += 6;
			break;
		case 17055:
			delete mpSmokeJet;
			mpSmokeJet = 0;
			break;
		case 17667: {
			mWeatherData = GetScriptValue();
			mpWeather = (SRainEntry *)Mem_New(mWeatherData * sizeof(SRainEntry), 0, 1);
			CVector origin = MechList->mPos;

			UseCacheForBits = 0;

			for (int i = 0; i < mWeatherData; i++) {
				SRainEntry *p_entry = &mpWeather[i];
				CVector offset((Rnd(2400) - 1200) << 12, (Rnd(400) - 1200) << 12,
				               (Rnd(2400) - 1200) << 12);

				offset += origin;
				p_entry->vel.Set((Rnd(16) - 8) << 12, (Rnd(48) + 48) << 12, (Rnd(16) - 8) << 12);
				CGPolyLine *p_droplet = new CGPolyLine(1);

				p_entry->p_line = p_droplet;
				p_droplet->mProtected = 1;
				p_droplet->mStart = offset;
				p_droplet->mSegs->End = (offset - p_entry->vel) + (CameraList->mVel >> 1);
				p_droplet->mStartR = 64;
				p_droplet->mStartG = 64;
				p_droplet->mStartB = 80;
				p_droplet->mSegs->r = 16;
				p_droplet->mSegs->g = 16;
				p_droplet->mSegs->b = 32;
				p_droplet->SetSemiTransparent();
			}

			UseCacheForBits = 1;
			break;
		}
		default:
			return CBaddy::ExecuteCommand(command);
	}

	return 1;
}
