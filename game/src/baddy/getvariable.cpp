/* GetVariable__6CBaddyUs (0x80040f50) */
/* C++ (BADDY.CPP). Fuzzy demo twin (0.74); locals from its MAIN.SYM. Reads a script variable:
   distance to / line of sight to the skater, positions (>> 12), counters, and script operands. */
#include "game/baddy.h"
#include "game/bruce.h"

extern short GameFMV_CurrentFrame;
extern CBruce *MechList;

uint Utils_XZDist(const CVector &, const CVector &);
int Utils_LineOfSight(const CVector &, const CVector &);
int Redbook_Rnd(int);
ushort *Trig_GetLinksPointer(int);

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
			return Redbook_Rnd(*mpScript++);
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
