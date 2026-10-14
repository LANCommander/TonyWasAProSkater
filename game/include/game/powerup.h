/* CPowerUp (POWERUP.H / POWERUP.CPP): bobbing, glowing pick-ups (letters, stat points, and the
   competition medals: Medal_Create makes CPowerUps, not CMedals). A CBody; retail layout = the
   demo's shifted +0x2c. Vtable 0x8009f0f8. See build/hdr_notes/powerup.md. */
#ifndef GAME_POWERUP_H
#define GAME_POWERUP_H

#pragma interface /* see class.h */

#include "game/types.h"
#include "game/vector.h"
#include "game/ob.h"

class CBruce;
class CGlow;

class CPowerUp : public CBody {
  public:
	CGlow *mpGlow;         /* 0x134 */
	ushort mGlowRadiusOrg; /* 0x138 */
	ushort mGlowRadiusAmp; /* 0x13a */
	uchar mHasNode;        /* 0x13c */
	uchar mIs3d;           /* 0x13d */
	uchar mDropping;       /* 0x13e */
	uchar mDropForever;    /* 0x13f */
	uchar mHoverHeight;    /* 0x140 */
	char pad141[0x142 - 0x141];
	ushort mNodeIndex;          /* 0x142 */
	int mGroundY;               /* 0x144 */
	CVector mOrgPos;            /* 0x148 */
	ushort mT;                  /* 0x154 */
	ushort mMaxWobbleAmplitude; /* 0x156 */
	ushort mWobbleAmplitude;    /* 0x158 */
	ushort mWobbleSpeed;        /* 0x15a */
	ushort mLifetime;           /* 0x15c */
	char pad15e[0x160 - 0x15e];
	uint mTimer; /* 0x160 */
	/* sizeof(CPowerUp) == 0x164 */

	CPowerUp(ushort, const CVector &, const CVector &, uint, int); /* 0x8001f934 */
	virtual ~CPowerUp();                                           /* slot 0: 0x8001fe14 */
	void DontDisplay();                                            /* 0x8001fe7c */
	void SetGravity(int, int);                                     /* 0x8001feb8 */
	void CreateBit();                                              /* 0x8001fec4 */
	virtual void DeleteStuff();                                    /* slot 6: 0x8001fed8 */
	void SetNode(int Node);                                        /* 0x8001ff34 */
	virtual void Die();                                            /* slot 1: 0x8001ff44 */
	void DoPhysics();                                              /* 0x8001ff84 */
	void TakeEffect(CBruce *);                                     /* 0x80020328 */
	virtual void AI();                                             /* slot 2: 0x800206ac */
};

#endif
