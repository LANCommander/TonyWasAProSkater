/* __9CChunkBitRC8CSVectorN21 (0x8002876c) */
#include "game/bit.h"

typedef struct {
	int x, y, z;
} __attribute__((aligned(8))) V;

int Redbook_Rnd(int);
void Utils_CalcUnit(CVector &, const CVector &);

char ChunkBitList[8];

static inline void VI(V *p, int x, int y, int z) {
	p->x = x;
	p->y = y;
	p->z = z;
}

CChunkBit::CChunkBit(const CSVector &a, const CSVector &b, const CSVector &c) {
	V ca, ba, cprod, norm;
	int mul;

	mType = 6;
	mPosA.vx = a.vx;
	mPosA.vy = a.vy;
	mPosA.vz = a.vz;
	mPosB.vx = b.vx;
	mPosB.vy = b.vy;
	mPosB.vz = b.vz;
	mPosC.vx = c.vx;
	mPosC.vy = c.vy;
	mPosC.vz = c.vz;
	VI(&ca, c.vx - a.vx, c.vy - a.vy, c.vz - a.vz);
	VI(&ba, b.vx - a.vx, b.vy - a.vy, b.vz - a.vz);

	cprod.x = ca.y * ba.z - ca.z * ba.y;
	cprod.y = ca.z * ba.x - ca.x * ba.z;
	cprod.z = ca.x * ba.y - ca.y * ba.x;
	Utils_CalcUnit(*(CVector *)&norm, *(CVector *)&cprod);

	mul = Redbook_Rnd(96) + 128;
	mPosD.vx = (norm.x * mul) >> 12;
	mPosD.vy = (norm.y * mul) >> 12;
	mPosD.vz = (norm.z * mul) >> 12;
	AttachTo(ChunkBitList);
}
