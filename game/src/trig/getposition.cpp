/* Trig_GetPosition__FR7CVectori (0x80047600) */
/* C++ (TRIG.CPP). Demo twin. Finds a trigger node's word-aligned position (after its variable-length header), stores it <<12 and returns the data past it. Each case repeated the stores; cross-jumping merges them. */
#include "game/vector.h"

ushort **OffsetList;

int *Trig_GetPosition(CVector &pos, int node) {
	ushort *pNode = OffsetList[node];
	ushort *pPos;
	int *pLong;

	switch (*pNode) {
		{
			uchar *pFlags;
			case 1:
			case 7:
				pFlags = (uchar *)(pNode + 4 + pNode[3]);

				while (*pFlags != 0xff)
					pFlags++;

				pLong = (int *)((intptr_t)(pFlags + 4) & ~3);
				pos.vx = pLong[0] << 12;
				pos.vy = pLong[1] << 12;
				pos.vz = pLong[2] << 12;
				break;
			case 3:
			case 8:
			case 10:
			case 11:
			case 13:
			case 1000:
			case 1001:
				pPos = pNode + 2 + pNode[1];
				pLong = (int *)((intptr_t)((uchar *)pPos + 3) & ~3);
				pos.vx = pLong[0] << 12;
				pos.vy = pLong[1] << 12;
				pos.vz = pLong[2] << 12;
				break;
			case 5:
				pPos = pNode + 3 + pNode[2];
				pLong = (int *)((intptr_t)((uchar *)pPos + 3) & ~3);
				pos.vx = pLong[0] << 12;
				pos.vy = pLong[1] << 12;
				pos.vz = pLong[2] << 12;
				break;
			case 500:
			case 501:
				pPos = pNode + 1;
				pLong = (int *)((intptr_t)((uchar *)pPos + 3) & ~3);
				pos.vx = pLong[0] << 12;
				pos.vy = pLong[1] << 12;
				pos.vz = pLong[2] << 12;
				break;
		}
	}

	return pLong + 3;
}
