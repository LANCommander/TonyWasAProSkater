/* HandleWater__6CBruce (0x80043258) */
#include "game/trig.h"
#include "game/bruce.h"

extern "C" {
	void *__nw__4CBitUi(unsigned int);
	void __11CFlatRippleRC7CVectoriUcUcUcii(void *, const CVector &, int, uchar, uchar, uchar, int,
	                                        int);
}

extern int GNumberOfPlayers;
extern int CurrentWaterLevel;
extern int CurrentWaterLevel2;
extern const int TTime;
extern SSinCos rcossin_tbl[];

void SFX_PlayX(unsigned int, short, int);
int Rnd(int);

CBruce *MechList;

void CBruce::HandleWater() {
	int WaterLevel;
	int bottom;

	if (GNumberOfPlayers != 2) {
		if (this == MechList)
			WaterLevel = CurrentWaterLevel;
		else
			WaterLevel = CurrentWaterLevel2;

		bottom = mPos.vy + 0x19000;

		if (WaterLevel < bottom) {
			if (!(mFlags & 8))
				SFX_PlayX(43, 8192, 0);

			mFlags |= 8;

			if (!(TTime & 3)) {
				CVector p;
				int offset;

				p = mPos;
				p.vy = WaterLevel;
				offset = Rnd(24) + 16;

				{
					SSinCos *b;
					int idx = Rnd(4096) & 4095;

					b = rcossin_tbl;
					SSinCos *e = b + idx;

					p.vx += offset * e->s;
					p.vz += offset * e->c;
				}

				void *r = __nw__4CBitUi(116);

				if (r)
					__11CFlatRippleRC7CVectoriUcUcUcii(r, p, Rnd(24) + 40, 221, 238, 255, 12,
					                                   Rnd(3) + 4);
			}
		} else {
			mFlags &= ~8;
		}
	}
}
