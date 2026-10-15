/* BIT2.CPP: more bits: the poly-lines (CPolyLine, CGPolyLine), the smoke generator and the
   gouraud lines with the grind sparks (CGLine, CGrindSpark) (declarations in game/bit2.h). */

#include "game/bit2.h"
#include "game/bit.h"
#include "game/m3d.h"
#include "game/ob.h"
#include "game/mem.h"
#include "game/globals.h"
#include "game/utils.h"

extern "C" char __vt_9CPolyLine[];
extern "C" void *__4CBit(CBit *p); /* CBit::CBit, as retail calls it */

/* FUN_800270e0 (0x800270e0) */
/* CPolyLine's constructor (n segments, white, at the origin; links into PolyLineList), kept as the
   egcs-ABI function retail has: CBit's constructor, then the vtable. */
extern "C" CPolyLine *FUN_800270e0(CPolyLine *p, int n) {
	int i;

	__4CBit(p);

	p->__vptr = __vt_9CPolyLine;
	p->mSegs = (SLineSeg *)Mem_NewX(n << 4, 0, 1);

	for (i = 0; i < n; i++) {
		*(uint *)&p->mSegs[i].r = 0x40ffffff;
		p->mSegs[i].End = ZeroVector;
	}

	p->mNumSegs = n;
	p->AttachTo(&PolyLineList);
	return p;
}

/* _._9CPolyLine (0x800271a0) */
/* Frees the segments and unlinks the line from PolyLineList; CBit's destructor does
   the rest. */
CPolyLine::~CPolyLine() {
	Mem_Delete(mSegs);
	DeleteFrom(&PolyLineList);
}

/* SetSemiTransparent__9CPolyLine (0x80027210) */
/* Sets the semi-transparency bit in the GPU code of every segment. */
void CPolyLine::SetSemiTransparent() {
	for (int i = 0; i < mNumSegs; i++)
		mSegs[i].code |= 2;
}

/* __10CGPolyLinei (0x8002725c) */
/* A gouraud poly-line of n segments, all white and opaque at the origin; links itself
   into GPolyLineList. */
CGPolyLine::CGPolyLine(int n) {
	mSegs = (SLineSeg *)Mem_NewX(n << 4, 0, 1);

	for (int i = 0; i < n; i++) {
		*(uint *)&mSegs[i].r = 0x50ffffff;
		mSegs[i].End = ZeroVector;
	}

	mNumSegs = n;
	AttachTo(&GPolyLineList);

	mStartR = 255;
	mStartG = 255;
	mStartB = 255;
	mCode = 0x50;
}

/* _._10CGPolyLine (0x80027334) */
/* Frees the segments and unlinks the line from GPolyLineList; CBit's destructor does
   the rest. */
CGPolyLine::~CGPolyLine() {
	Mem_Delete(mSegs);
	DeleteFrom(&GPolyLineList);
}

/* SetSemiTransparent__10CGPolyLine (0x800273a4) */
/* Sets the semi-transparency bit in the GPU code of every segment and of the start. */
void CGPolyLine::SetSemiTransparent() {
	for (int i = 0; i < mNumSegs; i++)
		mSegs[i].code |= 2;

	mCode |= 2;
}

/* _._15CSmokeGenerator (0x800273fc) */
/* Nothing of its own to free; CNonRenderedBit's destructor does the rest. */
CSmokeGenerator::~CSmokeGenerator() {}

/* Move__15CSmokeGenerator (0x80027450) */
/* Demo twin; locals from its MAIN.SYM. Spawns mPuffs motion-blur smoke bits rising
   at a random speed, then ages the generator and kills it when its life runs out. The CVector
   inline constructor and the CBit/CFlatBit inline setters (int params) give retail's prologue and
   the hoisted `100` in s4. */
void CSmokeGenerator::Move() {
	CVector upvel(0, 0, 0);
	int i;

	for (i = 0; i < mPuffs; i++) {
		CMotionBlur *foo;

		upvel.vy = -(mVBase + Rnd(mVRandom)) << 12;
		foo = new CMotionBlur(mPos, upvel, "Smoke", mScaleBase + Rnd(mScaleRandom), 0, 10);

		foo->SetFrigDeltaZ(100);
		foo->SetAngle(Rnd(4096));
		foo->SetTint(mR, mG, mB);
	}

	{
		short l;

		mAge++;
		l = mAge;

		if (l == mLifetime && (ushort)l != 0xffff)
			Die();
	}
}

/* __6CGLine (0x80027590) */
/* Demo twin. */
CGLine::CGLine() {
	mCodeBGR0 = 0x50808080;
	mPadBGR1 = 0x55808080;
	AttachTo(&GLineList);
}

/* _._6CGLine (0x800275ec) */
/* Unlinks the line from GLineList; CBit's destructor does the rest. */
CGLine::~CGLine() {
	DeleteFrom(&GLineList);
}

/* __11CGrindSparkP5CBodyR7CVectoriii (0x80027650) */
/* Fuzzy demo twin; members and locals from its MAIN.SYM, written as a real constructor.
   The negative-velocity path works on a temporary: assigning the parameter itself moves its prologue copy. */
CGrindSpark::CGrindSpark(CBody *pParent, CVector &pos, int vel, int gravity, int life) {
	{
		int vel2;

		if (vel < 0) {
			int t = -vel;

			{
				int r = Rnd(t);

				t >>= 1;
				t += r;
			}

			vel2 = -t;
		} else {
			vel2 = (vel >> 1) + Rnd(vel);
		}

		Bit_CalculateSparkVelocity(mVel, vel2);

		mAcc.vy = gravity;
		mFric.Set(3, 3, 3);
	}

	mLifetime = Rnd(life) + 1;
	mpParent = pParent;
	mOldPos = pos;
	mPos = pos;
	mEnd = pos + mpParent->mPos;
	mStart = mEnd;
	mCodeBGR0 = 0x52000080;

	{
		int r = ((Rnd(159) >> 5) + 2) << 12;

		mPadBGR1 = 0x55000000;
		mCodeBGR0 |= r;
	}
}

/* _._11CGrindSpark (0x800277b8) */
/* Nothing of its own; CGLine's destructor does the rest. */
CGrindSpark::~CGrindSpark() {}

/* Move__11CGrindSpark (0x8002780c) */
void CGrindSpark::Move() {
	int px, py, pz, nx, ny, nz, sx, sy, sz;

	px = mPos.vx;
	py = mPos.vy;
	pz = mPos.vz;
	nx = px + mVel.vx;
	ny = py + mVel.vy;
	nz = pz + mVel.vz;
	sx = mpParent->mPos.vx;
	sy = mpParent->mPos.vy;
	sz = mpParent->mPos.vz;
	mOldPos.vx = px;
	mOldPos.vy = py;
	mOldPos.vz = pz;
	mEnd.vx = px + sx;
	mEnd.vy = py + sy;
	mEnd.vz = pz + sz;
	mPos.vx = nx;
	mPos.vy = ny;
	mPos.vz = nz;
	mStart.vx = nx + sx;
	mStart.vy = ny + sy;
	mStart.vz = nz + sz;
	nx = mVel.vx + mAcc.vx;
	ny = mVel.vy + mAcc.vy;
	nz = mVel.vz + mAcc.vz;
	nx -= nx >> 3;
	ny -= ny >> 3;
	nz -= nz >> 3;
	mVel.vx = nx;
	mVel.vy = ny;
	mVel.vz = nz;
	mAge++;

	if (mAge >= mLifetime)
		Die();
}
