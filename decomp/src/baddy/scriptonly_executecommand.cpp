/* ExecuteCommand__16CScriptOnlyBaddyUs (0x8004169c) flags=-fstructure-noalias */
/* C++ (BADDY.CPP). Demo twin; locals and blocks from its MAIN.SYM. Layout by THPS1 offsets.
   Script-only baddy commands: looping sounds, snapping to the skater, a flag toggle, smoke jets
   and the rain effect (a set of falling CGPolyLine droplets around the skater). */
#include "game/baddy.h"
#include "game/bit.h"
#include "game/bruce.h"
#include "game/m3d.h"

extern const int SFX_v;
extern CBruce *MechList;
extern int UseCacheForBits;
extern CBody *CameraList;

void SFX_Stop(uint);
uint SFX_PlayX(uint, short, int);
uint SFX_PlayPos(uint, const CVector &, int);
void *Mem_New(uint, int, int);
int Rnd(int);

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
			mpSmokeJet = new CSmokeJet(mPos, mAngles, mpScript[0], 0xffff, mpScript[1], mpScript[2],
			                           mpScript[3], mpScript[4], ((uchar *)mpScript)[10], 0xcce0187c);
			mpSmokeJet->mProtected = 1;
			mpScript += 6;
			break;
		case 17057:
			mpSmokeJet = new CSmokeJet(mPos, mAngles, mpScript[0], 0xffff, mpScript[1], mpScript[2],
			                           mpScript[3], mpScript[4], ((uchar *)mpScript)[10], 0x68b7b136);
			mpScript += 6;
			break;
		case 17055:
			delete mpSmokeJet;
			mpSmokeJet = 0;
			break;
		case 17667: {
			mWeatherData = GetScriptValue();
			mpWeather = (SRainEntry *)Mem_New(mWeatherData * 16, 0, 1);
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
