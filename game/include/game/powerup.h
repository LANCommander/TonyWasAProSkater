/* CPowerUp (POWERUP.H / POWERUP.CPP): bobbing, glowing pick-ups (letters, stat points, and the
   competition medals: Medal_Create makes CPowerUps, not CMedals). A CBody; retail layout = the
   demo's shifted +0x2c. Vtable 0x8009f0f8. See build/hdr_notes/powerup.md. */
#ifndef GAME_POWERUP_H
#define GAME_POWERUP_H

#ifndef GAME_DATA_TU /* (the data files: full debug types) */
#pragma interface /* see class.h */
#endif

#include "game/classsizes.h"
#include "game/types.h"
#include "game/vector.h"
#include "game/ob.h"
#include "game/mess.h"

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

	CPowerUp(ushort a1, const CVector &a2_, const CVector &a3_, uint a4, int a5);
	virtual ~CPowerUp();
	void DontDisplay();
	void SetGravity(int g, int fric);
	void CreateBit();
	virtual void DeleteStuff();
	void SetNode(int Node);
	virtual void Die();
	void DoPhysics();
	void TakeEffect(CBruce *pMech);
	virtual void AI();
};
CHECK_CLASS_SIZE(CPowerUp, SIZEOF_CPOWERUP);

/* functions */
CPowerUp *PowerUp_Create(ushort type, const CVector &pos, uint a, int b, CVector &v);

/* globals */
extern uchar CurrentItemsFileRegion;
extern uchar CurrentMedalsFileRegion;
extern SMessageProg Messprog_Zoom[];
#endif
