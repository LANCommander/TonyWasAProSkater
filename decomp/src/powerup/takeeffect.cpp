/* TakeEffect__8CPowerUpP6CBruce (0x80020328) */
/* C++ (POWERUP.CPP). Fuzzy demo twin (0.81 by opcode); locals from its MAIN.SYM. A skater picks up a
   power-up: letters and the tape hand out their panel message, the bonus types score a trick and
   pitch the pickup sound, then the item fires its trigger links and, if it did something, dies.
   THPS1 asks the skater whether a letter/tape counts (FUN_80042744) instead of always taking it. */
#include "game/powerup.h"
#include "game/bruce.h"

struct SMessageProg;

extern SMessageProg Messprog_Zoom;
extern char *GameText_25c[];
extern int GNumberOfPlayers;
extern CBruce *MechList;

int SFX_PlayX(unsigned int, short, int);
void SFX_SetVoicePitch(unsigned int, ushort);
int Panel_Letter(int);
void Mess_DeleteAll();
void Mess_Message(const char *, SMessageProg *, bool, short, short);
ushort *Trig_GetLinksPointer(int);
void Trig_SendPulse(ushort *);

extern "C" {
	int FUN_80042744(CBruce *, short);
	void Panel_Tape__Fi(int);
	void FUN_8002f1a0(CBruce *);
}

void CPowerUp::TakeEffect(CBruce *pMech) {
	int HadEffect;
	int sound;

	if ((GNumberOfPlayers != 1 && pMech == MechList) ? (mFlags & 0x40) : (mFlags & 1))
		return;

	HadEffect = 0;
	sound = 0;

	switch (mType) {
		case 4:
		case 5:
		case 6:
		case 10:
		case 14:
		case 15:
		case 16:
			HadEffect = FUN_80042744(pMech, mHealth);

			if (!HadEffect)
				break;

			Mess_DeleteAll();

			{
				char *pMess = 0;

				switch (mType) {
					case 5:
						SFX_PlayX(20, 8192, 0);

						if (!Panel_Letter(0))
							pMess = GameText_25c[3];

						break;
					case 4:
						SFX_PlayX(20, 8192, 0);

						if (!Panel_Letter(1))
							pMess = GameText_25c[4];

						break;
					case 6:
						SFX_PlayX(20, 8192, 0);

						if (!Panel_Letter(2))
							pMess = GameText_25c[5];

						break;
					case 15:
						SFX_PlayX(20, 8192, 0);

						if (!Panel_Letter(3))
							pMess = GameText_25c[0];

						break;
					case 10:
						SFX_PlayX(20, 8192, 0);

						if (!Panel_Letter(4))
							pMess = GameText_25c[2];

						break;
					case 16:
						Panel_Tape__Fi(3);

						pMess = 0;
						break;
				}

				if (pMess)
					Mess_Message(pMess, &Messprog_Zoom, 1, 0, 0);
			}

			break;
		case 21:
		case 22:
		case 23:
			if (pMech->mInAir)
				break;

			HadEffect = 1;

			if (!pMech->mPhysicsState)
				FUN_8002f1a0(pMech);

			{
				int Pong = SFX_PlayX(18, 8192, 0);

				switch (mType) {
					case 21:
						pMech->Trick_Trick(76, "Bonus 100", -100, 0);
						SFX_SetVoicePitch(Pong, 900);
						break;
					case 22:
						pMech->Trick_Trick(76, "Bonus 200", -200, 0);
						SFX_SetVoicePitch(Pong, 1114);
						break;
					case 23:
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
