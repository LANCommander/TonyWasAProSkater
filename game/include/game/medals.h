/* CMedal (MEDALS.H / MEDALS.CPP): a spinning, bobbing medal model, a CBody. Retail layout = the
   demo's shifted +0x2c. Vtable 0x800a314c. Retail never constructs one: Medal_Create
   (0x80073dc8) makes CPowerUps (powerup.h). See build/hdr_notes/medals.md. */
#ifndef GAME_MEDALS_H
#define GAME_MEDALS_H

#ifndef GAME_DATA_TU /* (the data files: full debug types) */
#pragma interface /* see class.h */
#endif

#include "game/types.h"
#include "game/vector.h"
#include "game/ob.h"

class CMedal : public CBody {
  public:
	uchar mHasNode;     /* 0x134 */
	uchar mIs3d;        /* 0x135 */
	uchar mDropping;    /* 0x136 */
	uchar mDropForever; /* 0x137 */
	uchar mHoverHeight; /* 0x138 */
	char pad139[0x13a - 0x139];
	ushort mNodeIndex;          /* 0x13a */
	int mGroundY;               /* 0x13c */
	CVector mOrgPos;            /* 0x140 */
	ushort mT;                  /* 0x14c */
	ushort mMaxWobbleAmplitude; /* 0x14e */
	ushort mWobbleAmplitude;    /* 0x150 */
	ushort mWobbleSpeed;        /* 0x152 */
	ushort mLifetime;           /* 0x154 */
	char pad156[0x158 - 0x156];
	/* sizeof(CMedal) == 0x158 */

	virtual ~CMedal();          /* slot 0: 0x80073c94 */
	virtual void DeleteStuff(); /* slot 6: 0x80073cf0 (empty) */
	void DoPhysics();           /* 0x80073cf8 */
	virtual void AI();          /* slot 2: 0x80073da8 (calls DoPhysics) */
};

#endif
