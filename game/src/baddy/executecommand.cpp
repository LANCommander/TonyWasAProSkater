/* ExecuteCommand__6CBaddyUs (0x8003fcdc) flags=-fstructure-noalias */
/* C++ (BADDY.CPP). Demo twin; locals and blocks from its MAIN.SYM. Layout by THPS1 offsets.
   The generic baddy script commands: power-ups, labels/gotos and conditionals on flags and
   variables, camera shakes, pulses and signals to nodes, animations, sounds, sparks, FMVs and
   screen flashes. Returns 0 to stop the script for this frame. */
#include "game/baddy.h"
#include "game/powerup.h"
#include "game/camera.h"
#include "game/bit.h"
#include "game/career.h"
#include "game/trig.h"
#include "game/gamefmv.h"
#include "game/panel.h"

extern CVector ZeroVector;
extern CBruce *MechList;
extern int CurrentWaterLevel;
extern int CurrentWaterLevel2;
extern int CurrentWaterDamage;
extern CCamera *CameraList;
extern int GGame;
extern int GNumberOfPlayers;
extern int GLevel;
extern SLevel Levels[];

extern CBody *BaddyList;

extern CBody *EnvironmentalObjectList __attribute__((section(".data")));

extern int GVInfo1[];
extern int BitCount;
extern int UseCacheForBits;

CPowerUp *PowerUp_Create(ushort, const CVector &, uint, int, CVector &);
ushort *Trig_GetPosition(CVector &, int);
ushort *Trig_GetLinksPointer(int);
void Trig_SendPulse(ushort *);
void SendSignalToLinks(ushort *);
void SendSignalToNode(CBody *, int);
int Career_Got(int);
void SFX_MIDIFadeIn(int);
void SFX_MIDIFadeOut(int);
void SFX_PlayX(uint, short, int);
void SFX_PlayPos(uint, const CVector &, int);
void GameFMV_StartFMV(int);
void GameFMV_StopFMV();
void Bit_SetSparkTrajectory(const CSVector &);
void Bit_SetSparkTrajectoryCone(const CSVector &);
void Bit_SetSparkSize(uint);
void Bit_SetSparkRGB(uchar, uchar, uchar);
void Bit_SetSparkFadeRGB(uchar, uchar, uchar);
int Mem_GetFree(char);
void Flash_Screen(uchar, uchar, uchar, int, uchar, int);

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
				int ok = GVInfo1[0] != 2;

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

			pNode = Trig_GetPosition(dummy, node);
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
