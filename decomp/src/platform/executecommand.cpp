/* ExecuteCommand__9CPlatformUs (0x8003ec48) */
/* C++ (PLATFORM.CPP). Fuzzy demo twin (0.99); locals from its MAIN.SYM. Platform script commands:
   bounding size, visibility, looping sounds, scale and spin rates per axis, collision radius,
   moves to absolute/relative/node positions, and shattering. Anything else goes to the baddy
   command handler. */
#include "game/platform.h"
#include "game/bruce.h"
#include "game/m3d.h"

extern SPSXRegion PSXRegion[];
extern CBruce *MechList;

void SFX_Stop(uint);
ushort *Trig_GetPosition(CVector &, int);
void Shatter_Item(Item *, int, int);

extern "C" {
	int SFX_PlayX__FUisi(int, short, int);
	int SFX_PlayPos__FUiRC7CVectori(int, const CVector &, int);
}

int CPlatform::ExecuteCommand(ushort command) {
	switch (command) {
		case 17161: {
			SModel *m;
			ushort *pBox;
			ushort v = *mpScript++;

			mType = v;

			if (v)
				mSpecificFlags |= 16;
			else
				mSpecificFlags &= ~16;

			m = PSXRegion[mRegion].ppModels[mModel];
			pBox = (ushort *)&m->Box;
			mWidth = pBox[0] - pBox[1];
			mHeight = pBox[2] - pBox[3];
			mDepth = pBox[4] - pBox[5];
			break;
		}
		case 17157:
			if (*mpScript++)
				mFlags |= 8;
			else
				mFlags &= ~8;

			break;
		case 17671:
			if (mLoopingSound)
				SFX_Stop(mLoopingSound);

			mLoopingSound = SFX_PlayX__FUisi(*mpScript++, 0x2000, 0);
			mLoopingSoundUpdate = -1;
			break;
		case 17672: {
			int id;

			if (mLoopingSound)
				SFX_Stop(mLoopingSound);

			id = *mpScript++;
			mLoopingSoundUpdate = *mpScript++;
			mLoopingSound = SFX_PlayPos__FUiRC7CVectori(id, mPos, 0);
			break;
		}
		case 17673:
			if (mLoopingSound)
				SFX_Stop(mLoopingSound);

			mLoopingSound = 0;
			break;
		case 17156: {
			short p;

			if (!(mFlags & 0x200)) {
				mFlags |= 0x200;
				mScale.vx = mScale.vy = mScale.vz = 4096;
			}

			p = *mpScript++;
			mScaleXTimer = mScaleYTimer = mScaleZTimer = *mpScript++;

			if (mScaleXTimer)
				mScaleYAdjust = mScaleZAdjust = mScaleXAdjust = (p << 12) / (mScaleXTimer * 100);
			else
				mScaleYAdjust = mScaleZAdjust = mScaleXAdjust = (p << 12) / 100;

			mScale.vx += mScaleXAdjust;
			mScale.vy += mScaleYAdjust;
			mScale.vz += mScaleZAdjust;
			break;
		}
		case 17158: {
			short p;

			if (!(mFlags & 0x200)) {
				mFlags |= 0x200;
				mScale.vx = mScale.vy = mScale.vz = 4096;
			}

			p = *mpScript++;
			mScaleXTimer = *mpScript++;

			if (mScaleXTimer)
				mScaleXAdjust = (p << 12) / (mScaleXTimer * 100);
			else
				mScaleXAdjust = (p << 12) / 100;

			mScale.vx += mScaleXAdjust;
			break;
		}
		case 17159: {
			short p;

			if (!(mFlags & 0x200)) {
				mFlags |= 0x200;
				mScale.vx = mScale.vy = mScale.vz = 4096;
			}

			p = *mpScript++;
			mScaleYTimer = *mpScript++;

			if (mScaleYTimer)
				mScaleYAdjust = (p << 12) / (mScaleYTimer * 100);
			else
				mScaleYAdjust = (p << 12) / 100;

			mScale.vy += mScaleYAdjust;
			break;
		}
		case 17160: {
			short p;

			if (!(mFlags & 0x200)) {
				mFlags |= 0x200;
				mScale.vx = mScale.vy = mScale.vz = 4096;
			}

			p = *mpScript++;
			mScaleZTimer = *mpScript++;

			if (mScaleZTimer)
				mScaleZAdjust = (p << 12) / (mScaleZTimer * 100);
			else
				mScaleZAdjust = (p << 12) / 100;

			mScale.vz += mScaleZAdjust;
			break;
		}
		case 17155:
			mBruceHealthAdjust = *mpScript++;
			break;
		case 17154: {
			int x = *mpScript++;
			int y = *mpScript++;
			int z = *mpScript++;

			mSpecificFlags |= 4;
			mSpin[0] = x << 11;
			mSpin[1] = y << 11;
			mSpin[2] = z << 11;
			break;
		}
		case 17152:
			if (mTrodUpon)
				return 1;

			mpScript--;
			return 0;
		case 16928: {
			int *p_pos;
			CVector pos;

			p_pos = (int *)(((int)mpScript + 3) & ~3);
			pos.vx = *p_pos++;
			pos.vy = *p_pos++;
			pos.vz = *p_pos++;
			pos <<= 12;
			mpScript = (ushort *)p_pos;
			MoveTo(pos);
			break;
		}
		case 16929: {
			ushort node = *mpScript++;
			CVector pos;

			if (node & 0x2000)
				node = GetVariable(node);

			Trig_GetPosition(pos, node);
			MoveTo(pos);
			return 1;
		}
		case 16930: {
			int *p_pos;
			CVector pos;

			p_pos = (int *)(((int)mpScript + 3) & ~3);
			pos.vx = *p_pos++;
			pos.vy = *p_pos++;
			pos.vz = *p_pos++;
			pos <<= 12;
			pos += mPos;
			mpScript = (ushort *)p_pos;
			MoveTo(pos);
			break;
		}
		case 16901:
			Die();
			return 0;
		case 17153:
			if (MechList->m868 == this)
				MechList->m868 = 0;

			mFlags |= 1;
			Die();
			Shatter_Item((Item *)this, 0, 0);
			return 0;
		default:
			return CBaddy::ExecuteCommand(command);
	}

	return 1;
}
