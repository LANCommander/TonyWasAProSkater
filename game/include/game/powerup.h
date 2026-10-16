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

/* CPowerUp::mType (the hide/dead flags are CItem's and CBody's: game/ob.h) */
enum EPowerUpType {
	POWERUP_TYPE_SKATE_K = 4,
	POWERUP_TYPE_SKATE_S = 5,
	POWERUP_TYPE_SKATE_A = 6,
	POWERUP_TYPE_SKATE_E = 10,
	POWERUP_TYPE_SKATE_T = 15,
	POWERUP_TYPE_HIDDEN_TAPE = 16, /* the career's secret tape (GPowTape) */
	POWERUP_TYPE_TAPE = 18,
	POWERUP_TYPE_CREDIT_100 = 21,
	POWERUP_TYPE_CREDIT_200 = 22,
	POWERUP_TYPE_CREDIT_500 = 23,
	POWERUP_TYPE_MEDAL_BRONZE = 0x664,
	POWERUP_TYPE_MEDAL_SILVER = 0x665,
	POWERUP_TYPE_MEDAL_GOLD = 0x666,
};

/* the models' name hashes for Spool_GetModel (items file; medals: the skmedals file) */
enum EPowerUpModel {
	POWERUP_MODEL_SKATE_S = 0x311d55d4,
	POWERUP_MODEL_SKATE_K = 0x2328a71c,
	POWERUP_MODEL_SKATE_A = 0x2ebf22ca,
	POWERUP_MODEL_SKATE_T = 0x34524351,
	POWERUP_MODEL_SKATE_E = 0x29b68a16,
	POWERUP_MODEL_TAPE = 0xd5108e8c, /* both tape types */
	POWERUP_MODEL_MEDAL_BRONZE = 0x54636518,
	POWERUP_MODEL_MEDAL_SILVER = 0xba6d0434,
	POWERUP_MODEL_MEDAL_GOLD = 0x2364558e,
	POWERUP_MODEL_CREDIT_100 = 0x6f0f6b9f,
	POWERUP_MODEL_CREDIT_200 = 0xf6063a25,
	POWERUP_MODEL_CREDIT_500 = 0x81010ab3,
};

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

	CPowerUp(ushort type, const CVector &pos, const CVector &a3_, uint a4, int lifetime);
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
