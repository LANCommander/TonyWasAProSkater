/* __4CCarPsii (0x8006b490) */
/* C++ (CAR.CPP). A car (taxi, police car, van or cable car by `type`): its model and engine loop,
   on the baddy list, cycling animation 0, placed from its object data at node `node` and set on
   the road segment that node links to. */
#include "game/car.h"

extern char s_c_taxi[], s_c_van[], s_c_cable[], s_c_police[];
extern CBody *BaddyList;

int CarVol;

int SFX_PlayX(uint, short, int);
ushort *Trig_GetLinksPointer(int);
char *Trig_GetPosition(CVector &, int);

CCar::CCar(short *p_info, int node, int type) {
	ushort segment;

	mType = type;
	mLoopingSoundUpdate = 1;

	switch (type) {
		case 0xd5:
			InitItem(s_c_taxi);
			mLoopingSound = SFX_PlayX(0x71, 0, 0);
			break;
		case 0xd6:
			mLoopingSoundUpdate = 0;
			InitItem(s_c_police);
			break;
		case 0xd7:
			InitItem(s_c_van);
			mLoopingSound = SFX_PlayX(0x81, 0, 0);
			break;
		case 0xd8:
			InitItem(s_c_cable);
			mLoopingSound = SFX_PlayX(0x86, 0, 0);
			break;
	}

	CarVol = 300;
	AttachTo(&BaddyList);

	mAnimSpeed = 0x10000;
	CycleAnim(0, 1);

	mGroundToOrigin = 52;
	mOriginToEye = 32;
	mHealth = 60;
	SquirtAngles(SquirtPos(p_info));

	mNode = node;
	segment = Trig_GetLinksPointer(node)[1];
	mCurrentRoadSegment = segment;
	Trig_GetPosition(mPos, segment);

	mPos.vy -= 0x6e000;
}
