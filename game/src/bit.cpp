/* BIT.CPP: the particle / effect "bit" family: the bit lists and their move/display passes,
   CBit and its sprite, quad, chunk, glow, spark, motion-blur, ribbon, smoke-trail, glass and
   textured-ribbon kinds (declarations in game/bit.h). */

extern "C" {
	#include "game/scratchpad.h"
}

#include <sys/types.h>
#include <libgte.h>
#include <libgpu.h>
#include "game/bit.h"
#include "game/m3d.h"
#include "game/m3dasm.h"
#include "game/trig.h"
#include "game/camera.h"
#include "game/mem.h"
#include "game/utils.h"
#include "game/bit2.h"
#include "game/front.h"
#include "game/globals.h"
#include "game/physics.h"
#include "game/psyq.h"
#include "game/spool.h"
#include "pc_draw.h"

/* a 3-int vector the chunk-bit normal maths works in */
typedef struct {
	int x, y, z;
} __attribute__((aligned(8))) SChunkVec;
extern CFlatBit *FlatBitList;
extern CLinked2EndedBit *Linked2EndedBitList;
extern CPixel *PixelList;
extern CQuadBit *QuadBitList;
extern CGenPoly *GenPolyList;
extern CChunkBit *ChunkBitList;
extern CGlow *GlowList;
extern CGlassBit *GlassList;
extern CSpecialDisplay *SpecialDisplayList;
DR_TPAGE StoredTPagePrim;
extern char *AnimNames[];
void FUN_80092f30(void *, int, int, int);
/* an egcs vtable entry (game/data/psxdata.h PsxVtEntry) */
struct VtEntry {
	short delta, index;
	void (*fn)(char *, int);
};
extern CSVector SparkTrajectory;
extern CSVector SparkTrajectoryCone;
extern uint SparkSize;
extern uchar SparkRGB[4], SparkFadeRGB[4];
extern "C" char __vt_11CSmokeTrail[];
extern "C" char __vt_9CGlassBit[];
extern "C" char __vt_15CTexturedRibbon[];
extern "C" char __vt_11CWobblyGlow[];
/* egcs-ABI calls, as retail makes them: the base destructors (this, flags) and CBit's
   operator delete */
extern "C" void dt_4CBit(CBit *p, int a);
extern "C" void dt_15CSpecialDisplay(CSpecialDisplay *p, int a);
extern "C" void dt_5CGlow(CGlow *p, int a);
extern "C" void dt_7CRibbon(CRibbon *p, int a);
extern "C" void __dl__4CBitPv(void *p);
extern "C" void FUN_8002aa0c(CBit *p);
/* Widescreen (platform/pc, pc_gpu_gl.h): a full-width view shows pc_wide_pad more columns on
   either side. */
extern "C" int pc_wide_pad;
int M3dAsm_ClipLine(SClipLineBuf *);
int Transform(const CVector &, int &);
void CalcScreenNormal(SCalcBuffer *, int &, int &, int);
int AddFlatMark(P_TAG *, u_long);
/* the port's markers (bit/display.cpp) */
int AddDepthMark(P_TAG *, const int *);

/* Transform__FRC7CVectorRi (0x80027920) */
int Transform(const CVector &in, int &out) {
	CVector v;
	int z;

	v.vx = (in.vx >> 12) - MikeCamera[Camera_CurrentMikeCamera].Position.vx;
	v.vy = (in.vy >> 12) - MikeCamera[Camera_CurrentMikeCamera].Position.vy;
	v.vz = (in.vz >> 12) - MikeCamera[Camera_CurrentMikeCamera].Position.vz;
	{
		GeomProj p;

		p = geom_project_lv(&v);
		out = (int)p.sxy;
		z = p.v.mac[2];
	}
	return z;
}

/* Bit_Init__Fv (0x800279cc) */
void Bit_Init(void) {
	int i;

	BitCount = 0;
	NonRenderedBitList = 0;
	FlatBitList = 0;
	Linked2EndedBitList = 0;
	PixelList = 0;
	PolyLineList = 0;
	GPolyLineList = 0;
	QuadBitList = 0;
	GenPolyList = 0;
	ChunkBitList = 0;
	GlowList = 0;
	GlassList = 0;
	GLineList = 0;
	SpecialDisplayList = 0;
	FUN_80092f30(&StoredTPagePrim, 1, 1, GetTPage(0, 1, 0, 0));

	for (i = 0; i < 6; i++)
		QuickAnimLookup[i] = Spool_FindAnim(AnimNames[i]);

	ButtonsAnim = Spool_FindAnim("Buttons");
	pShadowTexture = Spool_FindTextureEntry(0x2a3b2550);
}

/* DeleteBitList__FP4CBit (0x80027ab0) */
/* C++: walks a CBit list and deletes every unprotected bit through its vtable (egcs-ABI virtual
   destructor, flags 3), as retail does. CBit's fields and vptr move in a 64-bit build. */
void DeleteBitList(CBit *p) {
	CBit *n;

	if (!p)
		return;

	n = p->mNext;

	for (;;) {
		if (p->mProtected == 0) {
			if (p) {
				struct VtEntry *vt = (struct VtEntry *)p->__vptr;

				vt[1].fn((char *)p + vt[1].delta, 3);
			}
		}

		p = n;

		if (n)
			n = n->mNext;
		else
			break;
	}
}

/* Bit_DeleteAll__Fv (0x80027b28) */
void Bit_DeleteAll(void) {
	DeleteBitList(NonRenderedBitList);
	DeleteBitList(FlatBitList);
	DeleteBitList(Linked2EndedBitList);
	DeleteBitList(PixelList);
	DeleteBitList(PolyLineList);
	DeleteBitList(GPolyLineList);
	DeleteBitList(QuadBitList);
	DeleteBitList(GenPolyList);
	DeleteBitList(ChunkBitList);
	DeleteBitList(GlowList);
	DeleteBitList(GlassList);
	DeleteBitList(GLineList);
	DeleteBitList(SpecialDisplayList);
}

/* RemoveDeadBits__FP4CBit (0x80027bdc) */
/* C++: walks a CBit list and deletes every dead bit through its vtable (egcs-ABI virtual
   destructor, flags 3), as retail does. CBit's fields and vptr move in a 64-bit build. */
void RemoveDeadBits(CBit *p) {
	CBit *n;

	if (!p)
		return;

	n = p->mNext;

	for (;;) {
		if (p->mDead != 0) {
			if (p) {
				struct VtEntry *vt = (struct VtEntry *)p->__vptr;

				vt[1].fn((char *)p + vt[1].delta, 3);
			}
		}

		p = n;

		if (n)
			n = n->mNext;
		else
			break;
	}
}

/* Bit_RemoveDeadBits__Fv (0x80027c54) */
void Bit_RemoveDeadBits(void) {
	RemoveDeadBits(NonRenderedBitList);
	RemoveDeadBits(FlatBitList);
	RemoveDeadBits(Linked2EndedBitList);
	RemoveDeadBits(PixelList);
	RemoveDeadBits(PolyLineList);
	RemoveDeadBits(GPolyLineList);
	RemoveDeadBits(QuadBitList);
	RemoveDeadBits(GenPolyList);
	RemoveDeadBits(ChunkBitList);
	RemoveDeadBits(GlowList);
	RemoveDeadBits(GlassList);
	RemoveDeadBits(GLineList);
	RemoveDeadBits(SpecialDisplayList);
}

/* __nw__4CBitUi (0x80027d08) */
/* Allocates a bit from the main heap and zeroes it (both arms of the
   UseCacheForBits test allocate the same way in retail). */
void *CBit::operator new(__SIZE_TYPE__ Size) throw() {
	int *p;

	if (UseCacheForBits)
		p = (int *)Mem_NewX(Size, 0, 1);
	else
		p = (int *)Mem_NewX(Size, 0, 1);

	uint NumWords = (Size + 3) >> 2;
	int *q = p;

	for (uint i = 0; i < NumWords; i++)
		*q++ = 0;

	return p;
}

/* __dl__4CBitPv (0x80027d70) */
/* Frees a bit back to the heap. */
void CBit::operator delete(void *p) {
	Mem_Delete(p);
}

/* AttachTo__4CBitPv (0x80027d90) */
/* Pushes the bit onto the front of the doubly-linked list whose head pointer is at
   pList. */
void CBit::AttachTo(void *pList) {
	CBit **ppHead = (CBit **)pList;

	mPrevious = 0;
	mNext = *ppHead;
	*ppHead = this;

	if (mNext)
		mNext->mPrevious = this;
}

/* DeleteFrom__4CBitPv (0x80027dbc) */
/* Unlinks the bit from the doubly-linked list whose head pointer is at pList. */
void CBit::DeleteFrom(void *pList) {
	CBit **ppHead = (CBit **)pList;

	if (mNext)
		mNext->mPrevious = mPrevious;

	if (mPrevious)
		mPrevious->mNext = mNext;

	if (*ppHead == this)
		*ppHead = mNext;
}

/* Die__4CBit (0x80027e18) */
/* Flags the bit dead; the bit manager deletes it later. */
void CBit::Die() {
	mDead = 1;
}

/* SetPos__4CBitRC7CVector (0x80027e24) */
/* Sets the position of the bit. */
void CBit::SetPos(const CVector &Pos) {
	mPos = Pos;
}

/* __4CBit (0x80027e44) */
/* Base particle: friction shift 1 on each axis; counts live bits. */
CBit::CBit() : mFric(1, 1, 1) {
	BitCount++;
}

/* _._4CBit (0x80027e80) */
/* Counts the bit out of BitCount. */
CBit::~CBit() {
	BitCount--;
}

/* __15CSpecialDisplay (0x80027ec4) */
/* A bit with its own display routine: links itself into SpecialDisplayList. */
CSpecialDisplay::CSpecialDisplay() {
	AttachTo(&SpecialDisplayList);
}

/* _._15CSpecialDisplay (0x80027f04) */
/* Unlinks the bit from SpecialDisplayList; CBit's destructor does the rest. */
CSpecialDisplay::~CSpecialDisplay() {
	DeleteFrom(&SpecialDisplayList);
}

/* __15CNonRenderedBit (0x80027f64) */
/* A bit that is moved but never drawn, linked into NonRenderedBitList. */
CNonRenderedBit::CNonRenderedBit() {
	AttachTo(&NonRenderedBitList);
}

/* _._15CNonRenderedBit (0x80027fa4) */
/* Unlinks the bit from NonRenderedBitList; CBit's destructor does the rest. */
CNonRenderedBit::~CNonRenderedBit() {
	DeleteFrom(&NonRenderedBitList);
}

/* __7CFT4Bit (0x80028004) */
/* A textured sprite bit: grey, opaque, scale 400, animation speed 1.0. */
CFT4Bit::CFT4Bit() {
	mAnimSpeed = 128;
	mScale = 400;
	mCodeBGR = 0x2c808080;
}

/* _._7CFT4Bit (0x80028054) */
/* Nothing of its own; CBit's destructor does the rest. */
CFT4Bit::~CFT4Bit() {}

/* SetScale__7CFT4BitUs (0x800280a8) */
/* Sets the scale of the bit. */
void CFT4Bit::SetScale(ushort Scale) {
	mScale = Scale;
}

/* SetSemiTransparent__7CFT4Bit (0x800280b0) */
/* Sets the semi-transparency bit of the primitive code. */
void CFT4Bit::SetSemiTransparent() {
	mCodeBGR |= 0x2000000;
}

/* SetTint__7CFT4BitUcUcUc (0x800280c4) */
/* Sets the RGB of the bit, keeping the primitive code byte. */
void CFT4Bit::SetTint(uchar r, uchar g, uchar b) {
	mCodeBGR = (mCodeBGR & 0xff000000) | (b << 16) | (g << 8) | r;
}

/* SetTransDecay__7CFT4Biti (0x800280f8) */
/* Sets the per-frame colour fade rate used by Fade. */
void CFT4Bit::SetTransDecay(int TransDecay) {
	mTransDecay = TransDecay;
}

/* SetAnim__7CFT4BitPc (0x80028100) */
/* Looks up a named spooled animation and starts it from frame 0 (the frame count is the byte
   4 bytes before the frame array). */
void CFT4Bit::SetAnim(char *pName) {
	SAnimFrame *pAnim = Spool_FindAnim(pName);

	mpPSXAnim = pAnim;
	mFrame = 0;
	mFrameFrac = 0;
	mpPSXFrame = pAnim;
	mNumFrames = ((char *)pAnim)[-4];
}

/* SetFrame__7CFT4Biti (0x80028140) */
/* Jumps the animation to the given frame. */
void CFT4Bit::SetFrame(int Frame) {
	mFrame = Frame;
	mFrameFrac = 0;
	mpPSXFrame = mpPSXAnim + mFrame;
}

/* IncFrame__7CFT4Bit (0x80028160) */
/* Advances the 8.8 fixed-point animation frame by mAnimSpeed (scaled by TimeScale) and
   points mpPSXFrame at the new frame. No wrapping. */
void CFT4Bit::IncFrame() {
	short Frame = (mFrame << 8) | mFrameFrac;

	Frame += (mAnimSpeed * TimeScale) >> 8;
	mFrame = Frame >> 8;
	mFrameFrac = Frame;
	mpPSXFrame = mpPSXAnim + mFrame;
}

/* IncFrameWithWrap__7CFT4Bit (0x800281c0) */
/* As IncFrame, but the frame wraps back to 0 once it passes the last one. */
void CFT4Bit::IncFrameWithWrap() {
	short Frame = (mFrame << 8) | mFrameFrac;

	Frame += (mAnimSpeed * TimeScale) >> 8;
	mFrame = Frame >> 8;
	mFrameFrac = Frame;

	if (mFrame >= mNumFrames)
		mFrame = 0;

	mpPSXFrame = mpPSXAnim + mFrame;
}

/* Shrink__7CFT4Biti (0x80028234) */
/* Shrinks the bit by mScaleDecay, clamping at 0. Returns 1 (and dies if Kill) once the scale
   is already 0, else 0. */
int CFT4Bit::Shrink(int Kill) {
	if (mScale == 0) {
		if (Kill)
			Die();

		return 1;
	}

	mScale -= mScaleDecay;

	if (mScale < 0)
		mScale = 0;

	return 0;
}

/* Fade__7CFT4Biti (0x80028290) */
/* Fades the RGB of the bit by mTransDecay per channel, clamping at 0. Returns 1 (and dies if Kill)
   once the colour has reached black, else 0. */
int CFT4Bit::Fade(int Kill) {
	uint CodeBGR = mCodeBGR;
	int r, g, b;

	if ((CodeBGR & 0xffffff) == 0) {
		if (Kill)
			Die();

		return 1;
	}

	r = CodeBGR & 255;
	g = (CodeBGR >> 8) & 255;
	b = (CodeBGR >> 16) & 255;

	if (r < mTransDecay)
		r = 0;
	else
		r = (r - (uchar)mTransDecay) & 255;

	if (g < mTransDecay)
		g = 0;
	else
		g = (g - (uchar)mTransDecay) & 255;

	if (b < mTransDecay)
		b = 0;
	else
		b = (b - (uchar)mTransDecay) & 255;

	mCodeBGR = (CodeBGR & 0xff000000) | (b << 16) | (g << 8) | r;
	return 0;
}

/* __8CQuadBit (0x80028394) */
/* A grey opaque textured quad, linked into QuadBitList. */
CQuadBit::CQuadBit() {
	mCodeBGR = 0x2c808080;
	AttachTo(&QuadBitList);
}

/* _._8CQuadBit (0x800283e0) */
/* Unlinks the quad from QuadBitList; CBit's destructor does the rest. */
CQuadBit::~CQuadBit() {
	DeleteFrom(&QuadBitList);
}

/* SetTexture__8CQuadBitPci (0x80028440) */
/* Uses frame Frame of a named spooled animation as the texture. */
void CQuadBit::SetTexture(char *pName, int Frame) {
	mpTexture = Spool_FindAnim(pName)[Frame].pTexture;
}

/* FlagOn__8CQuadBitUc (0x80028484) */
/* Sets the given flag bits. */
void CQuadBit::FlagOn(uchar Flags) {
	mFlags |= Flags;
}

/* SetSemiTransparent__8CQuadBit (0x80028498) */
/* Sets the semi-transparency bit of the primitive code. */
void CQuadBit::SetSemiTransparent() {
	mCodeBGR |= 0x2000000;
}

/* SetTransparency__8CQuadBitUc (0x800284ac) */
/* Sets R, G and B to the same level, keeping the primitive code byte. */
void CQuadBit::SetTransparency(uchar Level) {
	mCodeBGR = (mCodeBGR & 0xff000000) | (Level << 16) | (Level << 8) | Level;
}

/* OrientUsing__8CQuadBitRC7CVectorRC7SVECTORiii (0x800284d8) */
void CQuadBit::OrientUsing(const CVector &Position, const SVECTOR &Normal, int dx, int dy,
                           int Angle) {
	CVector b1;
	CVector b2;
	int l1;
	int l2;

	b1.vx = 0;
	b1.vy = -Normal.vz;
	b1.vz = Normal.vy;
	b2.vx = Normal.vy * Normal.vy + Normal.vz * Normal.vz;
	b2.vy = -Normal.vx * Normal.vy;
	b2.vz = -Normal.vz * Normal.vx;
	l1 = b1.Length();
	l2 = b2.Length();
	b1 = (b1 * dx) / l1;
	b2 = (b2 * dy) / l2;
	mPos = Position - b1 - b2;
	mPosB = Position + b1 - b2;
	mPosC = Position - b1 + b2;
	mPosD = Position + b1 + b2;
}

/* _._8CGenPoly (0x8002870c) */
/* Unlinks the bit from GenPolyList; CBit's destructor does the rest. */
CGenPoly::~CGenPoly() {
	DeleteFrom(&GenPolyList);
}

static inline void VI(SChunkVec *p, int x, int y, int z) {
	p->x = x;
	p->y = y;
	p->z = z;
}

/* __9CChunkBitRC8CSVectorN21 (0x8002876c) */
CChunkBit::CChunkBit(const CSVector &a, const CSVector &b, const CSVector &c) {
	SChunkVec ca, ba, cprod, norm;
	int mul;

	mType = BITTYPE_QUADFRAGMENT;
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

	mul = Rnd(96) + 128;
	mPosD.vx = (norm.x * mul) >> 12;
	mPosD.vy = (norm.y * mul) >> 12;
	mPosD.vz = (norm.z * mul) >> 12;
	AttachTo(&ChunkBitList);
}

/* _._9CChunkBit (0x80028968) */
/* Unlinks the bit from ChunkBitList; CBit's destructor does the rest. */
CChunkBit::~CChunkBit() {
	DeleteFrom(&ChunkBitList);
}

/* SetRGB__9CChunkBitUcUcUc (0x800289c8) */
/* Sets the chunk's colour: corner 0 gets it exactly, the other three a random
   shade of it. */
void CChunkBit::SetRGB(uchar r, uchar g, uchar b) {
	int k;

	mB0G0R0 = (b << 16) | (g << 8) | r;
	k = Rnd(4096);
	mB1G1R1 = (((b * k) >> 12) << 16) | (((g * k) >> 12) << 8) | ((r * k) >> 12);
	k = Rnd(4096);
	mB2G2R2 = (((b * k) >> 12) << 16) | (((g * k) >> 12) << 8) | ((r * k) >> 12);
	k = Rnd(4096);
	mB3G3R3 = (((b * k) >> 12) << 16) | (((g * k) >> 12) << 8) | ((r * k) >> 12);
}

/* SetUVs__9CChunkBitUsUsUcUcUcUcUcUc (0x80028b1c) */
/* Sets the chunk's CLUT, texture page and the UVs of three corners. */
void CChunkBit::SetUVs(ushort clut, ushort tpage, uchar u0, uchar v0, uchar u1, uchar v1, uchar u2,
                       uchar v2) {
	mClutV0U0 = (clut << 16) | (v0 << 8) | u0;
	mTPageV1U1 = (tpage << 16) | (v1 << 8) | u1;
	mV2U2 = (v2 << 8) | u2;
}

/* CalculateWorldCoords__9CChunkBit (0x80028b6c) */
/* Rotates the chunk's four corners by its angles and translates them to its position,
   giving the four world-space corners. */
void CChunkBit::CalculateWorldCoords() {
	MATRIX m;

	M3dMaths_RotMatrixYXZ((SVECTOR *)&mAngles, &m);
	SetRotMatrix(&m);
	M3dAsm_SetTransVector((Vector *)&mPos);
	geom_transform_sv(&mPosA, &mWorldPosA);
	geom_transform_sv(&mPosB, &mWorldPosB);
	geom_transform_sv(&mPosC, &mWorldPosC);
	geom_transform_sv(&mPosD, &mWorldPosD);
}

/* __5CGlowUiUi (0x80028c48) */
/* Fuzzy demo twin; members and locals from its MAIN.SYM. Written as a real constructor so
   `this` is copied where retail copies it. */
CGlow::CGlow(uint NumPoints, uint NumFringes) {
	mNumSections = NumPoints * 2;
	mNumFringes = NumFringes;
	mStepAngle = 4096 / mNumSections;
	mpSections =
	    (SSection *)Mem_New((mNumSections + mNumSections * mNumFringes) * sizeof(SSection), 0, 1);
	mpFringes = (SFringeQuad *)(mpSections + mNumSections);

	for (uint i = 0; i < mNumSections * mNumFringes; i++)
		mpFringes[i].CodeBGR = 0x3a000000;

	mCentreCodeBGR = 0x32000000;
	mMask = 0xffffffff;
	AttachTo(&GlowList);
}

/* __5CGlowRC7CVectoriiUcUcUcUcUcUc (0x80028d54) */
/* Fuzzy demo twin; members and locals from its MAIN.SYM. Written as a real constructor so
   `this` is copied where retail copies it. */
CGlow::CGlow(const CVector &Pos, int InnerRadius, int FringeRadius, uchar r0, uchar g0, uchar b0,
             uchar r1, uchar g1, uchar b1) {
	uint PadBGR;

	mNumSections = 8;
	mStepAngle = 512;
	mNumFringes = 1;
	mpSections = (SSection *)Mem_New(128, 0, 1);
	mpFringes = (SFringeQuad *)(mpSections + mNumSections);
	AttachTo(&GlowList);

	mPos = Pos;

	{
		uint i;

		for (i = 0; i < mNumSections; i++)
			mpSections[i].Radius = InnerRadius;
	}

	PadBGR = ((b1 << 16) | (g1 << 8)) | r1;
	mCentreCodeBGR = 0x32000000 | (b0 << 16) | (g0 << 8) | r0;

	{
		uint i;

		for (i = 0; i < mNumSections; i++)
			mpSections[i].CodeBGR = PadBGR;
	}

	{
		uint i;

		for (i = 0; i < mNumSections; i++) {
			mpFringes[i].Width = FringeRadius;
			mpFringes[i].CodeBGR = 0x3a000000;
		}
	}

	mMask = 0xffffffff;
}

/* _._5CGlow (0x80028f28) */
/* Frees the sections (the fringes share their allocation) and unlinks the glow from
   GlowList; CBit's destructor does the rest. */
CGlow::~CGlow() {
	Mem_Delete(mpSections);
	DeleteFrom(&GlowList);
}

/* SetRadius__5CGlowi (0x80028f94) */
/* Sets the radius of every section of the glow. */
void CGlow::SetRadius(int radius) {
	for (uint i = 0; i < mNumSections; i++)
		mpSections[i].Radius = radius;
}

/* SetFringeWidth__5CGlowUiUi (0x80028fd4) */
/* Sets the width of every quad in one fringe ring. */
void CGlow::SetFringeWidth(uint fringe, uint width) {
	SFringeQuad *pFringe = mpFringes + fringe * mNumSections;

	for (uint i = 0; i < mNumSections; i++)
		pFringe[i].Width = width;
}

/* SetCentreRGB__5CGlowUcUcUc (0x8002901c) */
/* Sets the colour (and the gouraud-triangle GPU code) of the glow's centre. */
void CGlow::SetCentreRGB(uchar r, uchar g, uchar b) {
	mCentreCodeBGR = (0x32 << 24) | (b << 16) | (g << 8) | r;
}

/* SetRGB__5CGlowUcUcUc (0x80029048) */
/* Sets the colour of every outer section of the glow. */
void CGlow::SetRGB(uchar r, uchar g, uchar b) {
	uint CodeBGR = (b << 16) | (g << 8) | r;

	for (uint i = 0; i < mNumSections; i++)
		mpSections[i].CodeBGR = CodeBGR;
}

/* SetFringeRGB__5CGlowUiUcUcUc (0x800290a4) */
/* Sets the colour (and the semi-transparent gouraud-quad GPU code) of every
   quad in one fringe ring. */
void CGlow::SetFringeRGB(uint fringe, uchar r, uchar g, uchar b) {
	uint CodeBGR = (0x3a << 24) | (b << 16) | (g << 8) | r;
	SFringeQuad *pFringe = mpFringes + fringe * mNumSections;

	for (uint i = 0; i < mNumSections; i++)
		pFringe[i].CodeBGR = CodeBGR;
}

/* SetRadialRGB__5CGlowiUcUcUc (0x80029110) */
/* Sets the colour of one radial spoke: its outer section and the matching
   quad of every fringe ring. */
void CGlow::SetRadialRGB(int section, uchar r, uchar g, uchar b) {
	uint CodeBGR = (0x3a << 24) | (b << 16) | (g << 8) | r;
	SFringeQuad *pFringe;

	mpSections[section].CodeBGR = CodeBGR;
	pFringe = mpFringes + section;

	for (uint i = 0; i < mNumFringes; i++) {
		pFringe->CodeBGR = CodeBGR;
		pFringe += mNumSections;
	}
}

/* __11CWobblyGlowRC7CVectoriiiUcUcUcUcUcUc (0x80029188) */
/* A glow whose section radii wobble around InnerRadius by Wobble/256 of it, each
   section starting at a random phase with a random rate. */
CWobblyGlow::CWobblyGlow(const CVector &Pos, int InnerRadius, int FringeRadius, int Wobble,
                         uchar r0, uchar g0, uchar b0, uchar r1, uchar g1, uchar b1)
    : CGlow(Pos, InnerRadius, FringeRadius, r0, g0, b0, r1, g1, b1) {
	mInnerRadius = InnerRadius;
	mAmplitude = (InnerRadius * Wobble) / 256;

	for (uint i = 0; i < mNumSections; i++) {
		mT[i] = Rnd(4096);
		mInc[i] = Rnd(50) + 50;
	}
}

/* Move__11CWobblyGlow (0x80029270) */
/* Each section's radius wobbles sinusoidally around the inner radius, each at its own
   rate. */
void CWobblyGlow::Move() {
	for (uint i = 0; i < mNumSections; i++) {
		mT[i] += mInc[i];
		mpSections[i].Radius = mInnerRadius + mAmplitude * rcossin_tbl[mT[i] & 0xfff].s / 4096;
	}
}

/* __8CFlatBit (0x8002930c) */
/* A screen-facing sprite, linked into FlatBitList: angular friction 1, post-scale
   1.0 x 1.0, semi-transparency rate 32. */
CFlatBit::CFlatBit() {
	AttachTo(&FlatBitList);

	mSemiTransparencyRate = 32;
	mAngFric = 1;
	mPostScale = 0x10001000;
}

/* _._8CFlatBit (0x80029368) */
/* Unlinks the bit from FlatBitList; CFT4Bit's destructor does the rest. */
CFlatBit::~CFlatBit() {
	DeleteFrom(&FlatBitList);
}

/* __16CLinked2EndedBit (0x800293c8) */
/* A sprite stretched between two end points, linked into Linked2EndedBitList. */
CLinked2EndedBit::CLinked2EndedBit() {
	AttachTo(&Linked2EndedBitList);
}

/* _._16CLinked2EndedBit (0x80029408) */
/* Unlinks the bit from Linked2EndedBitList; CFT4Bit's destructor does the rest. */
CLinked2EndedBit::~CLinked2EndedBit() {
	DeleteFrom(&Linked2EndedBitList);
}

/* __6CPixel (0x80029468) */
/* A single-tile particle, linked into PixelList. */
CPixel::CPixel() {
	AttachTo(&PixelList);
}

/* _._6CPixel (0x800294a8) */
/* Unlinks the pixel from PixelList; CBit's destructor does the rest. */
CPixel::~CPixel() {
	DeleteFrom(&PixelList);
}

/* Bit_SetSparkTrajectory__FRC8CSVector (0x80029508) */
void Bit_SetSparkTrajectory(const CSVector &v) {
	SparkTrajectory = v;
}

/* Bit_SetSparkTrajectoryCone__FRC8CSVector (0x8002952c) */
void Bit_SetSparkTrajectoryCone(const CSVector &p) {
	SparkTrajectoryCone = p;
}

/* Bit_CalculateSparkVelocity__FR7CVectori (0x80029550) */
void Bit_CalculateSparkVelocity(CVector &out, int mag) {
	CSVector d = SparkTrajectory;

	if (SparkTrajectoryCone.vx)
		d.vx += Rnd(SparkTrajectoryCone.vx) - (SparkTrajectoryCone.vx >> 1);

	if (SparkTrajectoryCone.vy)
		d.vy += Rnd(SparkTrajectoryCone.vy) - (SparkTrajectoryCone.vy >> 1);

	if (SparkTrajectoryCone.vz)
		d.vz += Rnd(SparkTrajectoryCone.vz) - (SparkTrajectoryCone.vz >> 1);

	Utils_GetVecFromMagDir(out, mag, d);
}

/* Bit_SetSparkSize__FUi (0x80029648) */
void Bit_SetSparkSize(uint a) {
	SparkSize = (a << 16) | a;
}

/* Bit_SetSparkRGB__FUcUcUc (0x8002965c) */
void Bit_SetSparkRGB(uchar a, uchar b, uchar c) {
	SparkRGB[0] = a;
	SparkRGB[1] = b;
	SparkRGB[2] = c;
}

/* Bit_SetSparkFadeRGB__FUcUcUc (0x80029670) */
void Bit_SetSparkFadeRGB(uchar a, uchar b, uchar c) {
	SparkFadeRGB[0] = a;
	SparkFadeRGB[1] = b;
	SparkFadeRGB[2] = c;
}

/* __6CSparkR7CVectoriii (0x80029684) */
/* Fuzzy demo twin; members and the inlined CFriction block from its MAIN.SYM, written
   as a real constructor. A spark is a TILE (or a 1x1 TILE_1 when the global spark size is 1) coloured
   from the current spark colour, given a random-ish velocity, gravity and lifetime. Needs
   -fstructure-noalias: retail loads the globals above the vptr and mLifetime stores. */
CSpark::CSpark(CVector &pos, int vel, int gravity, int life) {
	if ((SparkSize & 15) == 1) {
		code = 0x68;
		tag = 0x02000000;
		mWidthHeight = 1;
	} else {
		code = 0x60;
		mWidthHeight = SparkSize;
		tag = 0x03000000;
	}

	r0 = SparkRGB[0];
	g0 = SparkRGB[1];
	b0 = SparkRGB[2];
	mFadeR = SparkFadeRGB[0];
	mFadeG = SparkFadeRGB[1];
	mFadeB = SparkFadeRGB[2];
	mPos = pos;
	Bit_CalculateSparkVelocity(mVel, vel);

	mAcc.vy = gravity;
	mFric.Set(3, 3, 3);

	mLifetime = Rnd(life);

	if (SparkSemiTrans)
		code |= 2;
}

/* _._6CSpark (0x800297bc) */
/* Nothing of its own; CPixel's destructor does the rest. */
CSpark::~CSpark() {}

/* Move__6CSpark (0x80029810) */
/* A spark falls under gravity with 1/8 friction until it reaches its floor (if it has
   one), fading its colour each frame, and dies when its lifetime runs out. */
void CSpark::Move() {
	int x = mPos.vx + mVel.vx;
	int y = mPos.vy + mVel.vy;
	int z = mPos.vz + mVel.vz;

	if (mFloorY && mFloorY < y) {
		mPos.vy = mFloorY;
	} else {
		mPos.vx = x;
		mPos.vy = y;
		mPos.vz = z;

		x = mVel.vx + mAcc.vx;
		y = mVel.vy + mAcc.vy;
		z = mVel.vz + mAcc.vz;
		mVel.vx = x - (x >> 3);
		mVel.vy = y - (y >> 3);
		mVel.vz = z - (z >> 3);
	}

	mAge++;

	if (mAge >= mLifetime)
		Die();

	r0 -= mFadeR;
	g0 -= mFadeG;
	b0 -= mFadeB;
}

/* _._11CSimpleAnim (0x80029920) */
/* Nothing of its own; CFlatBit's destructor does the rest. */
CSimpleAnim::~CSimpleAnim() {}

/* Move__11CSimpleAnim (0x80029974) */
/* Steps the animation. A one-shot anim dies on reaching mDieFrame; a looping one
   wraps to frame 0, or holds the last frame if mDieFrame is -2. */
void CSimpleAnim::Move() {
	IncFrame();

	if (mDie) {
		if (mFrame >= mDieFrame)
			Die();
	} else if (mFrame >= mNumFrames) {
		if (mDieFrame == -2)
			SetFrame(mNumFrames - 1);
		else
			SetFrame(0);
	}
}

/* __11CMotionBlurRC7CVectorR7CVectorPciii (0x80029a0c) */
/* A semi-transparent animated flat bit moving with a fixed velocity, shrinking and
   fading as it goes. */
CMotionBlur::CMotionBlur(const CVector &pos, CVector &vel, char *pAnimName, int scale,
                         int scaleDecay, int transDecay) {
	mPos = pos;
	mVel = vel;
	SetAnim(pAnimName);
	SetSemiTransparent();

	mScale = scale;
	mScaleDecay = scaleDecay;
	mTransDecay = transDecay;
}

/* _._11CMotionBlur (0x80029ad8) */
/* Nothing of its own to free; CFlatBit's destructor does the rest. */
CMotionBlur::~CMotionBlur() {}

/* Move__11CMotionBlur (0x80029b2c) */
/* The blur moves with its velocity, shrinks and fades (dying when either runs out)
   and otherwise steps its animation. */
void CMotionBlur::Move() {
	mPos += mVel;

	if (!Shrink(1) && !Fade(1))
		IncFrameWithWrap();
}

/* _._19CMotionBlurImmortal (0x80029b88) */
/* Nothing of its own to free; CMotionBlur's destructor does the rest. */
CMotionBlurImmortal::~CMotionBlurImmortal() {}

/* Move__19CMotionBlurImmortal (0x80029bdc) */
/* Like CMotionBlur::Move, but shrinking and fading never kill the bit. */
void CMotionBlurImmortal::Move() {
	mPos += mVel;

	if (!Shrink(0) && !Fade(0))
		IncFrameWithWrap();
}

/* _._5CFrag (0x80029c38) */
/* Nothing of its own; CFlatBit's destructor does the rest. */
CFrag::~CFrag() {}

/* Move__5CFrag (0x80029c8c) */
/* A fragment flies with acceleration and per-axis shift friction, steps its frame
   up to (and holding at) the last one, and dies when its lifetime runs out. */
void CFrag::Move() {
	mPos.vx += mVel.vx;
	mPos.vy += mVel.vy;
	mPos.vz += mVel.vz;
	mVel.vx += mAcc.vx;
	mVel.vy += mAcc.vy;
	mVel.vz += mAcc.vz;
	mVel.vx -= mVel.vx >> mFric.vx;
	mVel.vy -= mVel.vy >> mFric.vy;
	mVel.vz -= mVel.vz >> mFric.vz;
	mFrame++;

	if (mFrame >= mNumFrames)
		mFrame = mNumFrames - 1;

	mAge++;

	if (mAge >= mLifetime)
		Die();
}

/* __10CRibbonBit (0x80029d80) */
/* One segment of a CRibbon: a CLinked2EndedBit with its own vtable. */
CRibbonBit::CRibbonBit() {}

/* _._10CRibbonBit (0x80029db4) */
/* Nothing of its own; CLinked2EndedBit's destructor does the rest. */
CRibbonBit::~CRibbonBit() {}

/* Move__10CRibbonBit (0x80029e08) */
/* A ribbon segment just loops its animation. */
void CRibbonBit::Move() {
	IncFrameWithWrap();
}

/* __7CRibbonRC7CVectoriiPcT4ii (0x80029e28) */
/* Fuzzy demo twin; members and locals from its MAIN.SYM. A real constructor, with the
   parameters left unmodified so the prologue copies them in retail's order. */
CRibbon::CRibbon(const CVector &pos, int numbits, int pointsperbit, char *pMiddleAnimName,
                 char *pEndAnimName, int scale, int semitrans) {
	mNumBits = numbits;
	mPointsPerBit = pointsperbit - 1;
	mNumPoints = numbits * mPointsPerBit + 1;
	mPoints = (CVector *)Mem_New(mNumPoints * sizeof(CVector), 0, 1);

	{
		int i;

		for (i = 0; i < mNumPoints; i++)
			mPoints[i] = pos;
	}

	{
		int i;

		mBits = (CRibbonBit **)Mem_New(numbits * sizeof(CRibbonBit *), 0, 1);

		for (i = mNumBits - 1; i >= 0; i--) {
			mBits[i] = new CRibbonBit;
			mBits[i]->mProtected = 1;
			mBits[i]->SetAnim(pMiddleAnimName);
			mBits[i]->SetScale(scale);

			if (semitrans)
				mBits[i]->SetSemiTransparent();
		}
	}

	mBits[0]->mBitFlags |= 16;
	mBits[0]->SetAnim(pEndAnimName);
}

/* _._7CRibbon (0x8002a018) */
/* Deletes the ribbon's segments, then frees the segment and point arrays;
   CNonRenderedBit's destructor does the rest. */
CRibbon::~CRibbon() {
	for (int i = 0; i < mNumBits; i++)
		delete mBits[i];

	Mem_Delete(mBits);
	Mem_Delete(mPoints);
}

/* SetTint__7CRibbonUcUcUc (0x8002a0e8) */
/* Tints every segment of the ribbon. */
void CRibbon::SetTint(uchar r, uchar g, uchar b) {
	for (int i = 0; i < mNumBits; i++)
		mBits[i]->SetTint(r, g, b);
}

/* SetScale__7CRibboni (0x8002a178) */
/* Sets the scale of every segment of the ribbon. */
void CRibbon::SetScale(int scale) {
	for (int i = 0; i < mNumBits; i++)
		mBits[i]->SetScale(scale);
}

/* SetPos__7CRibbonR7CVector (0x8002a1e8) */
/* Pushes a new head position into the ribbon's ring of points and re-spans every
   segment between points mPointsPerBit apart. */
void CRibbon::SetPos(CVector &pos) {
	int j;

	mCurrent++;

	if (mCurrent == mNumPoints)
		mCurrent = 0;

	mPoints[mCurrent] = pos;
	j = mCurrent;

	for (int i = mNumBits - 1; i >= 0; i--) {
		mBits[i]->mEndPosB = mPoints[j];
		j -= mPointsPerBit;

		if (j < 0)
			j += mNumPoints;

		mBits[i]->mEndPosA = mPoints[j];
	}
}

/* __11CSmokeTrailR7CVectoriiii (0x8002a2ec) */
/* A two-point-per-segment ribbon whose segments are tinted from dark at the tail up
   to (r, g, b) at the head, each fading at 8 per frame. */
CSmokeTrail::CSmokeTrail(CVector &pos, int numbits, int r, int g, int b)
    : CRibbon(pos, numbits, 2, "Ribbon", "Ribbon", 400, 1) {
	int dr = r / mNumBits;
	int dg = g / mNumBits;
	int db = b / mNumBits;

	for (int i = 0; i < mNumBits; i++) {
		int steps = mNumBits - (i + 1);

		mBits[i]->SetTint(r - steps * dr, g - steps * dg, b - steps * db);
		mBits[i]->SetTransDecay(8);
	}
}

/* FUN_8002a448 (0x8002a448) */
/* CSmokeTrail's destructor (vtable slot 0), kept as the egcs-ABI function (this, flags) retail
   has: CRibbon's destructor does the work. */
extern "C" void FUN_8002a448(CSmokeTrail *p, int f) {
	p->__vptr = __vt_11CSmokeTrail;
	dt_7CRibbon(p, 0);

	if (f & 1)
		__dl__4CBitPv(p);
}

/* Move__11CSmokeTrail (0x8002a49c) */
/* Once the trail is fading away, fades every segment and dies when all have gone. */
void CSmokeTrail::Move() {
	if (mFadeAway) {
		int dead = 1;

		for (int i = 0; i < mNumBits; i++)
			if (!mBits[i]->Fade(0))
				dead = 0;

		if (dead)
			Die();
	}
}

/* __9CGlassBitRC7CVectorT1iUcUcUciii (0x8002a534) */
/* A shard of glass, linked into GlassList: a triangle with each corner placed
   randomly within +-(RangeX, RangeY, RangeZ) units of Pos, a quarter-intensity colour and a random
   30-59 frame hold before it starts to fade. */
CGlassBit::CGlassBit(const CVector &Pos, const CVector &Vel, int GroundY, uchar r, uchar g, uchar b,
                     int RangeX, int RangeY, int RangeZ) {
	AttachTo(&GlassList);

	int SpanX = RangeX * 2 + 1;
	int SpanY = RangeY * 2 + 1;
	int SpanZ = RangeZ * 2 + 1;

	mPos = Pos;
	mPosA.vx = Pos.vx + ((Rnd(SpanX) - RangeX) << 12);
	mPosA.vy = Pos.vy + ((Rnd(SpanY) - RangeY) << 12);
	mPosA.vz = Pos.vz + ((Rnd(SpanZ) - RangeZ) << 12);
	mPosB.vx = Pos.vx + ((Rnd(SpanX) - RangeX) << 12);
	mPosB.vy = Pos.vy + ((Rnd(SpanY) - RangeY) << 12);
	mPosB.vz = Pos.vz + ((Rnd(SpanZ) - RangeZ) << 12);
	mPosC.vx = Pos.vx + ((Rnd(SpanX) - RangeX) << 12);
	mPosC.vy = Pos.vy + ((Rnd(SpanY) - RangeY) << 12);
	mPosC.vz = Pos.vz + ((Rnd(SpanZ) - RangeZ) << 12);
	mVel = Vel;
	mGroundY = GroundY;
	mDefaultR = r >> 2;
	mDefaultG = g >> 2;
	mDefaultB = b >> 2;
	mR = r >> 2;
	mG = g >> 2;
	mB = b >> 2;
	mLifetime = Rnd(30) + 30;
	mFadeRate = 4;
}

/* FUN_8002a77c (0x8002a77c) */
/* C++: CGlassBit's destructor (vtable slot 0), kept as the egcs-ABI function (this, flags) retail
   has: leaves GlassList, then CBit's destructor. */
/* egcs-ABI call, as retail makes it */
extern "C" void FUN_8002a77c(CGlassBit *p, int f) {
	p->__vptr = __vt_9CGlassBit;
	p->DeleteFrom(&GlassList);
	dt_4CBit(p, 0);

	if (f & 1)
		__dl__4CBitPv(p);
}

/* Move__9CGlassBit (0x8002a7dc) */
/* A glass shard falls under gravity and bounces off the ground twice (re-scattering
   its corners), sparkling at random until then; after its hold time it fades out and dies. */
void CGlassBit::Move() {
	int vx = mVel.vx;
	int vy = mVel.vy;
	int vz = mVel.vz;

	mPosA.vx += vx;
	mPosA.vy += vy;
	mPosA.vz += vz;
	mPosB.vx += vx;
	mPosB.vy += vy;
	mPosB.vz += vz;
	mPosC.vx += vx;
	mPosC.vy += vy;
	mPosC.vz += vz;
	vy += 10000;

	if (mGroundY < mPosA.vy) {
		if (mAge < 2) {
			mPosA.vy = mGroundY - (Rnd(50) << 12);
			mPosB.vy = mGroundY - (Rnd(50) << 12);
			vy = -vy >> 2;
			mPosC.vy = mGroundY - (Rnd(50) << 12);
			mAge++;
		} else {
			vy = 0;
		}

		vx >>= 1;
		vz >>= 1;
	}

	mVel.vx = vx;
	mVel.vy = vy;
	mVel.vz = vz;

	if (mAge < 2 && Rnd(5) == 0) {
		mR = 48;
		mG = 48;
		mB = 64;
	} else {
		mR = mDefaultR;
		mG = mDefaultG;
		mB = mDefaultB;
	}

	if (mLifetime == 0 || --mLifetime == 0) {
		mDefaultR = mDefaultR > mFadeRate ? mDefaultR - mFadeRate : 0;
		mDefaultG = mDefaultG > mFadeRate ? mDefaultG - mFadeRate : 0;
		mDefaultB = mDefaultB > mFadeRate ? mDefaultB - mFadeRate : 0;
	}

	if ((mDefaultR | mDefaultG | mDefaultB) == 0)
		Die();
}

/* FUN_8002aa0c (0x8002aa0c) */
/* C++: moves every live bit of a list (CBit::Move, vtable slot 1). */
extern "C" void FUN_8002aa0c(CBit *p) {
	for (; p; p = p->mNext) {
		if (p->mDead == 0)
			p->Move();
	}
}

/* Bit_Move__Fv (0x8002aa6c) */
void Bit_Move(void) {
	FUN_8002aa0c(PixelList);
	FUN_8002aa0c(FlatBitList);
	FUN_8002aa0c(GlassList);
	FUN_8002aa0c(GenPolyList);
	FUN_8002aa0c(ChunkBitList);
	FUN_8002aa0c(QuadBitList);
	FUN_8002aa0c(GlowList);
	FUN_8002aa0c(NonRenderedBitList);
	FUN_8002aa0c(Linked2EndedBitList);
	FUN_8002aa0c(PolyLineList);
	FUN_8002aa0c(GPolyLineList);
	FUN_8002aa0c(GLineList);
	FUN_8002aa0c(SpecialDisplayList);
}

/* Links a packet of n command words (the port's markers, pc_draw.h); 0 when the buffer is full. */
static int AddMark(P_TAG *OTPos, const u_long *words, int n) {
	P_TAG *p = (P_TAG *)pPoly;
	uchar *end = pPoly + sizeof(P_TAG) + n * sizeof(u_long);

	if (end > PolyBufferEnd)
		return 0;

	pPoly = end;
	setlen(p, n);

	for (int i = 0; i < n; i++)
		P_WORDS(p)[i] = words[i];

	addPrim(OTPos, p);
	return 1;
}

/* Also for CTexturedRibbon::Display. */
int AddFlatMark(P_TAG *OTPos, u_long code) {
	return AddMark(OTPos, &code, 1);
}

/* Packets made in screen space around projected points, linked at OTPos before this (and after a
   PC_DRAW_FLAT_END there): the view depths of their polygons' vertices 0-3 for the port
   (PC_DRAW_DEPTH), so that geometry in front hides them. Also for CTexturedRibbon::Display. */
int AddDepthMark(P_TAG *OTPos, const int z[4]) {
	u_long w[PC_DRAW_DEPTH_WORDS];

	w[0] = PC_DRAW_DEPTH;

	for (int i = 0; i < 4; i++)
		w[1 + i] = z[i] > 1 ? z[i] : 1;

	return AddMark(OTPos, w, PC_DRAW_DEPTH_WORDS);
}

/* A particle around one projection (zed, its view depth), ext pixels across at most: its depth
   brought forward by its size as if a ball, so that geometry it touches doesn't cut it. */
static int AddParticleDepth(P_TAG *OTPos, int zed, int ext) {
	int z = zed - ext * zed / pCurrentViewport->Zoom;
	int zs[4] = {z, z, z, z};

	return AddDepthMark(OTPos, zs);
}

static u_long FadeBGR(u_long bgr, int fade) {
	if (!fade)
		return bgr & 0xffffff;

	return (((bgr & 0xff) * fade >> 8) | (((bgr >> 8) & 0xff) * fade >> 8) << 8 |
	        (((bgr >> 16) & 0xff) * fade >> 8) << 16);
}

/* A plain glow (a light's: one fringe, all sections drawn) for the port to draw smoothly
   (PC_DRAW_GLOW, pc_draw.h); others are drawn as the PS1 had them. */
static int AddGlowMark(P_TAG *OTPos, CGlow *pGlow, int CentreX, int CentreY, int vz, int fade) {
	u_long w[PC_DRAW_GLOW_WORDS];
	uint i;

	if (pGlow->mNumSections != 8 || pGlow->mNumFringes != 1 || pGlow->mSkipTriangles ||
	    (pGlow->mMask & 0xff) != 0xff)
		return AddFlatMark(OTPos, PC_DRAW_FLAT_BEGIN);

	w[0] = PC_DRAW_GLOW;
	w[1] = ((u_long)(ushort)CentreY << 16) | (ushort)CentreX;
	w[2] = FadeBGR(pGlow->mCentreCodeBGR, fade);
	w[3] = FadeBGR(pGlow->mpSections[0].CodeBGR, fade);
	w[4] = FadeBGR(pGlow->mpFringes[0].CodeBGR, fade);
	w[5] = (pGlow->mAngle & 4095) | (pGlow->mStepAngle & 0xffff) << 16;

	/* screen radii (vertical, in 16ths of a pixel) as Bit_Display has them: Radius * 400 / z */
	{
		uint r = pGlow->mpFringes[0].Width * 400 * 16 / vz;

		w[6] = r > 0xffff ? 0xffff : r;
	}

	for (i = 0; i < 8; i += 2) {
		uint r0 = pGlow->mpSections[i].Radius * 400 * 16 / vz;
		uint r1 = pGlow->mpSections[i + 1].Radius * 400 * 16 / vz;

		w[7 + i / 2] = (r0 > 0xffff ? 0xffff : r0) | (r1 > 0xffff ? 0xffff : r1) << 16;
	}

	return AddMark(OTPos, w, PC_DRAW_GLOW_WORDS);
}

/* Bit_Display__Fv (0x8002ab20) */
/* Demo twin (0.991); locals and blocks from its MAIN.SYM. Draws every effect list
   (special, glow, gouraud/flat polylines, flat bits, glass, gouraud lines, quads, chunks, pixels,
   linked two-ended bits) into the primitive buffer.
   Near-miss (2979/3030 aligned): the glow's first OT/RadA block schedules the OT chain later than
   retail (insns 207-235), and in the flat-bit block Angle/Width/OffX get different registers.
   That one is local-alloc order in the !Angle block: retail gives the Height/OffY/Width product
   temps v0/v1/a0 and the OffX (short) shift temp a1 (so global puts OffX in a1, Width in t5,
   Angle in s3); ours allocates the OffX chain first (v0). Statement order follows the demo lines. */
void Bit_Display() {
	CVector Temp;
	CVector camera_pos;
	int GTEZedLimit;
	CSpecialDisplay *pSpecial;
	CGlow *pGlow;

	GeomProj proj; /* the last projection's results */

	geom_ld_mat(pc_geom.rot, &MikeCamera[Camera_CurrentMikeCamera].View);
	geom_load_vector(pc_geom.tr, 0, 0, 0);

	camera_pos.vx = MikeCamera[Camera_CurrentMikeCamera].Position.vx;
	camera_pos.vy = MikeCamera[Camera_CurrentMikeCamera].Position.vy;
	camera_pos.vz = MikeCamera[Camera_CurrentMikeCamera].Position.vz;
	GTEZedLimit = pCurrentViewport->Zoom >> 1;

	/* the glows clamp their corners to 100 columns beyond the screen; in widescreen beyond its
	   margins, as far as the GPU's polygon width limit (1023) allows */
	int XMin = -100;
	int XMax = 612;

	if (pCurrentViewport->xL == 0 && pCurrentViewport->xR == Xres) {
		int pad = pc_wide_pad < 155 ? pc_wide_pad : 155;

		XMin -= pad;
		XMax += pad;
	}
	pSpecial = SpecialDisplayList;

	while (pSpecial) {
		pSpecial->Display();

		pSpecial = (CSpecialDisplay *)pSpecial->mNext;
	}

	{
		for (pGlow = GlowList; pGlow; pGlow = (CGlow *)pGlow->mNext) {
			uint y0x0;
			VECTOR RotCoords;
			CVector Centre = (pGlow->mPos >> 12) - camera_pos;

			proj = geom_project_lv(&Centre);
			DR_TPAGE *ptpage = (DR_TPAGE *)pPoly;
			uchar *newppoly = pPoly + sizeof(DR_TPAGE);

			if (newppoly > PolyBufferEnd)
				return;

			pPoly = newppoly;
			uint a = getlen(&StoredTPagePrim);
			uint b = StoredTPagePrim.code[0];

			setlen(ptpage, a);
			ptpage->code[0] = b;
			int n = pGlow->mNumSections;
			int Step = pGlow->mStepAngle;
			int Ang = pGlow->mAngle;
			SSection *pSection = pGlow->mpSections;

			geom_st_lv(&RotCoords, proj.v.mac);

			if ((uint)RotCoords.vz > 20000)
				continue;

			short CentreX;
			short CentreY;
			int RGBFade = 0;

			if (RotCoords.vz < 700)
				RGBFade = RotCoords.vz * 255 / 700 + 1;

			if (RotCoords.vz < GTEZedLimit) {
				CentreX =
				    pCurrentViewport->Zoom * RotCoords.vx / RotCoords.vz + pCurrentViewport->xC;

				if (CentreX > 1023)
					CentreX = 1023;
				else if (CentreX < -1024)
					CentreX = -1024;

				CentreY =
				    pCurrentViewport->Zoom * RotCoords.vy / RotCoords.vz + pCurrentViewport->yC;

				if (CentreY > 1023)
					CentreY = 1023;
				else if (CentreY < -1024)
					CentreY = -1024;
			} else {
				geom_st_word(&y0x0, proj.sxy);

				CentreX = (short)y0x0;
				CentreY = (short)(y0x0 >> 16);
			}

			if (CentreX == -1024 || CentreX == 1023 || CentreY == -1024 || CentreY == 1023)
				continue;

			P_TAG *OTPos = pDoubleBuffer->pOT + ((RotCoords.vz & 0x3ffc) >> 2);

			/* the glow is made in screen space around its centre's projection: the port draws
			   it as given (linked first, so the end marker follows the glow's packets) */
			if (!AddFlatMark(OTPos, PC_DRAW_FLAT_END))
				return;

			DVECTOR *pBufA = (DVECTOR *)SCRATCHPAD;
			DVECTOR *pBufB = (DVECTOR *)(SCRATCHPAD + 0x4) + pGlow->mNumFringes;
			SSinCos *pTab = rcossin_tbl;
			SSinCos *p = &pTab[Ang & 4095];
			int RadA = pSection[n - 1].Radius * 400 / RotCoords.vz;
			int MSin = p->s * 400 / 256;
			int MCos = p->c;
			int x = CentreX + RadA * MSin / 4096;
			int y = CentreY - RadA * MCos / 4096;

			if (x > XMax)
				x = XMax;

			if (x < XMin)
				x = XMin;

			if (y > 340)
				y = 340;

			if (y < -100)
				y = -100;

			int Rad;
			SFringeQuad *pFringe;

			{
				int f;

				pBufA->vx = x;
				pBufA->vy = y;
				Rad = pSection[n - 1].Radius;
				pFringe = pGlow->mpFringes + n - 1;

				for (f = 0; f < pGlow->mNumFringes; f++) {
					Rad += pFringe->Width;
					x = CentreX + (Rad * 400 / RotCoords.vz) * MSin / 4096;
					y = CentreY - (Rad * 400 / RotCoords.vz) * MCos / 4096;

					if (x > XMax)
						x = XMax;

					if (x < XMin)
						x = XMin;

					if (y > 340)
						y = 340;

					if (y < -100)
						y = -100;

					pBufA[f + 1].vx = x;
					pBufA[f + 1].vy = y;
					pFringe += n;
				}
			}

			{
				int i;
				int f;

				Ang += Step;

				for (i = 0; i < n; i++) {
					int DoDisplay;

					DoDisplay = (pGlow->mMask >> i) & 1;

					if (i == n - 1)
						Ang = pGlow->mAngle;

					int RadB = pSection->Radius * 400 / RotCoords.vz;
					SSinCos *q = (SSinCos *)((Ang & 4095) * 4 + (uintptr_t)pTab);

					MSin = q->s * 400 / 256;
					MCos = q->c;
					x = CentreX + RadB * MSin / 4096;
					y = CentreY - RadB * MCos / 4096;

					if (x > XMax)
						x = XMax;

					if (x < XMin)
						x = XMin;

					if (y > 340)
						y = 340;

					if (y < -100)
						y = -100;

					pBufB->vx = x;
					pBufB->vy = y;

					if (!pGlow->mSkipTriangles) {
						if (DoDisplay) {
							POLY_G3 *aGou;
							uchar *newppoly;

							aGou = (POLY_G3 *)pPoly;
							newppoly = pPoly + sizeof(POLY_G3);

							if (newppoly > PolyBufferEnd)
								return;

							pPoly = newppoly;
							setlen(aGou, 6);
							*(u_long *)&aGou->r0 = pGlow->mCentreCodeBGR;
							*(u_long *)&aGou->r1 = pSection->CodeBGR;
							*(u_long *)&aGou->r2 = pSection->CodeBGR;

							if (RGBFade) {
								aGou->r0 = aGou->r0 * RGBFade >> 8;
								aGou->g0 = aGou->g0 * RGBFade >> 8;
								aGou->b0 = aGou->b0 * RGBFade >> 8;
								aGou->r1 = aGou->r1 * RGBFade >> 8;
								aGou->g1 = aGou->g1 * RGBFade >> 8;
								aGou->b1 = aGou->b1 * RGBFade >> 8;
								aGou->r2 = aGou->r2 * RGBFade >> 8;
								aGou->g2 = aGou->g2 * RGBFade >> 8;
								aGou->b2 = aGou->b2 * RGBFade >> 8;
							}

							aGou->x0 = CentreX;
							aGou->y0 = CentreY;
							aGou->x1 = pBufA->vx;
							aGou->y1 = pBufA->vy;
							aGou->x2 = pBufB->vx;
							aGou->y2 = pBufB->vy;
							addPrim(OTPos, aGou);
						}
					}

					{
						int f;

						Rad = pSection->Radius;
						pFringe = pGlow->mpFringes + i;

						for (f = 0; f < pGlow->mNumFringes; f++) {
							Rad += pFringe->Width;
							x = CentreX + (Rad * 400 / RotCoords.vz) * MSin / 4096;
							y = CentreY - (Rad * 400 / RotCoords.vz) * MCos / 4096;

							if (x > XMax)
								x = XMax;

							if (x < XMin)
								x = XMin;

							if (y > 340)
								y = 340;

							if (y < -100)
								y = -100;

							pBufB[f + 1].vx = x;
							pBufB[f + 1].vy = y;

							if (DoDisplay) {
								POLY_G4 *aGou;
								uchar *newppoly;
								uint CodeBGR;

								aGou = (POLY_G4 *)pPoly;
								newppoly = pPoly + sizeof(POLY_G4);

								if (newppoly > PolyBufferEnd)
									return;

								pPoly = newppoly;
								setlen(aGou, 8);
								CodeBGR = pFringe->CodeBGR;
								*(u_long *)&aGou->r0 = CodeBGR;
								*(u_long *)&aGou->r1 = CodeBGR;

								if (f) {
									uint PadBGR;

									PadBGR = (pFringe - n)->CodeBGR;
									*(u_long *)&aGou->r2 = PadBGR;
									*(u_long *)&aGou->r3 = PadBGR;
								} else {
									*(u_long *)&aGou->r2 = pSection->CodeBGR;
									*(u_long *)&aGou->r3 = pSection->CodeBGR;
								}

								if (RGBFade) {
									aGou->r0 = aGou->r0 * RGBFade >> 8;
									aGou->g0 = aGou->g0 * RGBFade >> 8;
									aGou->b0 = aGou->b0 * RGBFade >> 8;
									aGou->r1 = aGou->r1 * RGBFade >> 8;
									aGou->g1 = aGou->g1 * RGBFade >> 8;
									aGou->b1 = aGou->b1 * RGBFade >> 8;
									aGou->r2 = aGou->r2 * RGBFade >> 8;
									aGou->g2 = aGou->g2 * RGBFade >> 8;
									aGou->b2 = aGou->b2 * RGBFade >> 8;
									aGou->r3 = aGou->r3 * RGBFade >> 8;
									aGou->g3 = aGou->g3 * RGBFade >> 8;
									aGou->b3 = aGou->b3 * RGBFade >> 8;
								}

								aGou->x2 = pBufA[f].vx;
								aGou->y2 = pBufA[f].vy;
								aGou->x3 = pBufB[f].vx;
								aGou->y3 = pBufB[f].vy;
								aGou->x0 = pBufA[f + 1].vx;
								aGou->y0 = pBufA[f + 1].vy;
								aGou->x1 = pBufB[f + 1].vx;
								aGou->y1 = pBufB[f + 1].vy;
								addPrim(OTPos, aGou);
							}

							pFringe += n;
						}
					}

					{
						DVECTOR *pTemp;

						pTemp = pBufA;
						pBufA = pBufB;
						pBufB = pTemp;
					}

					Ang += Step;
					pSection++;
				}
			}

			addPrim(OTPos, ptpage);
			AddGlowMark(OTPos, pGlow, CentreX, CentreY, RotCoords.vz, RGBFade);
		}
	}

	CGPolyLine *pGPolyLine;

	{
		uint LastBGR;
		SLineSeg *pSeg;
		SClipLineBuf *pClip = (SClipLineBuf *)SCRATCHPAD;

		pGPolyLine = GPolyLineList;

		while (pGPolyLine) {
			int i;

			LastBGR = *(uint *)&pGPolyLine->mStartR;
			pClip->Start.vx = pGPolyLine->mStart.vx;
			pClip->Start.vy = pGPolyLine->mStart.vy;
			pClip->Start.vz = pGPolyLine->mStart.vz;
			pSeg = pGPolyLine->mSegs;

			for (i = 0; i < pGPolyLine->mNumSegs; i++) {
				uint NewBGR;
				int sort;

				NewBGR = *(uint *)&pSeg->r;
				pClip->End.vx = pSeg->End.vx;
				pClip->End.vy = pSeg->End.vy;
				pClip->End.vz = pSeg->End.vz;
				sort = M3dAsm_ClipLine(pClip);

				if (sort >= 0) {
					DR_TPAGE *ptpage;
					LINE_G2 *aline;
					uchar *newppoly;
					P_TAG *otpos;

					ptpage = (DR_TPAGE *)pPoly;
					aline = (LINE_G2 *)(ptpage + 1);
					newppoly = pPoly + sizeof(DR_TPAGE) + sizeof(LINE_G2);

					if (newppoly > PolyBufferEnd)
						return;

					pPoly = newppoly;
					setlen(aline, 4);
					*(u_long *)&aline->r0 = LastBGR;
					*(u_long *)&aline->r1 = NewBGR;

					if (pGPolyLine->mForcedPriority)
						otpos = pDoubleBuffer->pOT + pGPolyLine->mForcedPriority;
					else
						otpos = pDoubleBuffer->pOT + sort;

					*(u_long *)&aline->x0 = pClip->ScrStart;
					*(u_long *)&aline->x1 = pClip->ScrEnd;
					addPrim(otpos, aline);

					if (LastBGR & 0x02000000) {
						uint a;
						uint b;

						a = getlen(&StoredTPagePrim);
						b = StoredTPagePrim.code[0];
						setlen(ptpage, a);
						ptpage->code[0] = b;
						addPrim(otpos, ptpage);
					}
				}

				LastBGR = NewBGR;
				pClip->Start.vx = pClip->End.vx;
				pClip->Start.vy = pClip->End.vy;
				pClip->Start.vz = pClip->End.vz;
				pSeg++;
			}

			pGPolyLine = (CGPolyLine *)pGPolyLine->mNext;
		}
	}

	CPolyLine *pPolyLine;

	{
		SLineSeg *pSeg;
		SClipLineBuf *pClip = (SClipLineBuf *)SCRATCHPAD;

		pPolyLine = PolyLineList;

		while (pPolyLine) {
			int i;

			pClip->Start.vx = pPolyLine->mStart.vx;
			pClip->Start.vy = pPolyLine->mStart.vy;
			pClip->Start.vz = pPolyLine->mStart.vz;
			pSeg = pPolyLine->mSegs;

			for (i = 0; i < pPolyLine->mNumSegs; i++) {
				uint CodeBGR;
				int sort;

				CodeBGR = *(uint *)&pSeg->r;
				pClip->End.vx = pSeg->End.vx;
				pClip->End.vy = pSeg->End.vy;
				pClip->End.vz = pSeg->End.vz;
				sort = M3dAsm_ClipLine(pClip);

				if (sort >= 0) {
					DR_TPAGE *ptpage;
					LINE_F2 *aline;
					uchar *newppoly;
					P_TAG *otpos;

					ptpage = (DR_TPAGE *)pPoly;
					aline = (LINE_F2 *)(ptpage + 1);
					newppoly = pPoly + sizeof(DR_TPAGE) + sizeof(LINE_F2);

					if (newppoly > PolyBufferEnd)
						return;

					pPoly = newppoly;
					setlen(aline, 3);
					*(u_long *)&aline->r0 = CodeBGR;

					if (pPolyLine->mForcedPriority)
						otpos = pDoubleBuffer->pOT + pPolyLine->mForcedPriority;
					else
						otpos = pDoubleBuffer->pOT + sort;

					*(u_long *)&aline->x0 = pClip->ScrStart;
					*(u_long *)&aline->x1 = pClip->ScrEnd;
					addPrim(otpos, aline);

					if (CodeBGR & 0x02000000) {
						uint a;
						uint b;

						a = getlen(&StoredTPagePrim);
						b = StoredTPagePrim.code[0];
						setlen(ptpage, a);
						ptpage->code[0] = b;
						addPrim(otpos, ptpage);
					}
				}

				pClip->Start.vx = pClip->End.vx;
				pClip->Start.vy = pClip->End.vy;
				pClip->Start.vz = pClip->End.vz;
				pSeg++;
			}

			pPolyLine = (CPolyLine *)pPolyLine->mNext;
		}
	}

	CFlatBit *pFlatBit;

	for (pFlatBit = FlatBitList; pFlatBit; pFlatBit = (CFlatBit *)pFlatBit->mNext) {
		int zed;
		uint y0x0;

		Temp.vx = (pFlatBit->mPos.vx >> 12) - camera_pos.vx;
		Temp.vy = (pFlatBit->mPos.vy >> 12) - camera_pos.vy;
		Temp.vz = (pFlatBit->mPos.vz >> 12) - camera_pos.vz;
		proj = geom_project_lv(&Temp);
		POLY_FT4 *apoly = (POLY_FT4 *)pPoly;
		uchar *newppoly = pPoly + sizeof(POLY_FT4);

		if (newppoly > PolyBufferEnd)
			return;

		pPoly = newppoly;
		setlen(apoly, 9);
		*(u_long *)&apoly->r0 = pFlatBit->mCodeBGR;
		zed = proj.v.mac[2];

		if (zed < GTEZedLimit || zed > pCurrentViewport->Yon) {
			pPoly = (uchar *)apoly;
			continue;
		}

		uint Scale = pFlatBit->mScale;

		Scale = (Scale << 7) / zed;

		if (Scale > 512 && !(pFlatBit->mBitFlags & 8)) {
			pPoly = (uchar *)apoly;
			continue;
		}

		geom_st_word(&y0x0, proj.sxy);
		Texture *stored = pFlatBit->mpPSXFrame->pTexture;
		uint foo1 = *(uint *)&stored->u0;
		uint foo2 = *(uint *)&stored->u1;
		uint foo3 = *(uint *)&stored->u2;
		ushort foo4 = *(ushort *)&stored->u3;

		if (pFlatBit->mCLUT)
			*(u_long *)&apoly->u0 = (foo1 & 0xffff) | (pFlatBit->mCLUT << 16);
		else
			*(u_long *)&apoly->u0 = foo1;

		*(u_long *)&apoly->u1 = (foo2 & 0xff9fffff) | (pFlatBit->mSemiTransparencyRate << 16);
		*(u_long *)&apoly->u2 = foo3;
		*(ushort *)&apoly->u3 = foo4;
		short Angle = pFlatBit->mAngle;

		if (Scale >= 256 || Angle) {
			apoly->u1--;
			apoly->v2--;
			apoly->u3--;
			apoly->v3--;
		}

		uint *ptr;
		uchar Flipped;
		int OffX;
		int OffY;
		uint Width;
		uint Height;
		uint x_post_scale;
		uint y_post_scale;

		{
			ptr = (uint *)&apoly->x0;
			Flipped = pFlatBit->mBitFlags & 1;
			OffX = pFlatBit->mpPSXFrame->OffX;
			OffY = pFlatBit->mpPSXFrame->OffY;
			Width = pFlatBit->mpPSXFrame->Width;
			Height = pFlatBit->mpPSXFrame->Height;
			x_post_scale = pFlatBit->mPostScale & 0xffff;
			y_post_scale = pFlatBit->mPostScale >> 16;
			uint xs = Scale * x_post_scale;
			uint ys = Scale * y_post_scale;

			if (!Angle) {
				int Y1;

				OffX = (short)(Scale * OffX >> 7);
				Y1 = (Scale * OffY >> 7) << 16;
				y0x0 += Y1 + OffX;
				Width *= Scale;
				Width >>= 7;
				Height = Scale * Height >> 7;
				Height <<= 16;

				if (Width < 5)
					Width = 2;

				if (Height < 0x50000)
					Height = 0x10000;

				if (!Flipped) {
					*(uint *)&apoly->x0 = y0x0;
					ptr[2] = y0x0 + Width;
					ptr[4] = y0x0 + Height;
					ptr[6] = y0x0 + Width + Height;
				} else {
					ptr[2] = y0x0;
					*(uint *)&apoly->x0 = y0x0 + Width;
					ptr[6] = y0x0 + Height;
					ptr[4] = y0x0 + Width + Height;
				}
			} else {
				int sin;
				int cos;
				int xsin;
				int ysin;
				int xcos;
				int ycos;
				int xwsin;
				int yhsin;
				int xwcos;
				int yhcos;
				uint x_scale;
				uint y_scale;

				{
					SSinCos *p = rcossin_tbl;

					p += Angle & 4095;
					sin = p->s;
					cos = p->c;
				}

				xsin = OffX * sin;
				ysin = OffY * sin;
				xcos = OffX * cos;
				ycos = OffY * cos;
				xwsin = xsin + Width * sin;
				yhsin = ysin + Height * sin;
				xwcos = xcos + Width * cos;
				yhcos = ycos + Height * cos;
				x_scale = xs >> 12;
				y_scale = ys >> 12;

				if (!Flipped) {
					*(uint *)&apoly->x0 = y0x0 + (x_scale * (xcos - ysin) >> 19) +
					                      ((y_scale * (xsin + ycos) >> 19) << 16);
					ptr[2] = y0x0 + (x_scale * (xwcos - ysin) >> 19) +
					         ((y_scale * (xwsin + ycos) >> 19) << 16);
					ptr[4] = y0x0 + (x_scale * (xcos - yhsin) >> 19) +
					         ((y_scale * (xsin + yhcos) >> 19) << 16);
					ptr[6] = y0x0 + (x_scale * (xwcos - yhsin) >> 19) +
					         ((y_scale * (xwsin + yhcos) >> 19) << 16);
				} else {
					ptr[2] = y0x0 + (x_scale * (xcos - ysin) >> 19) +
					         ((y_scale * (xsin + ycos) >> 19) << 16);
					*(uint *)&apoly->x0 = y0x0 + (x_scale * (xwcos - ysin) >> 19) +
					                      ((y_scale * (xwsin + ycos) >> 19) << 16);
					ptr[6] = y0x0 + (x_scale * (xcos - yhsin) >> 19) +
					         ((y_scale * (xsin + yhcos) >> 19) << 16);
					ptr[4] = y0x0 + (x_scale * (xwcos - yhsin) >> 19) +
					         ((y_scale * (xwsin + yhcos) >> 19) << 16);
				}
			}
		}

		{
			P_TAG *OTPos;

			if (pFlatBit->mForcedPriority)
				OTPos = pDoubleBuffer->pOT + pFlatBit->mForcedPriority;
			else
				OTPos = pDoubleBuffer->pOT + (((zed - pFlatBit->mFrigDeltaZ) & 0x3ffc) >> 2);

			/* the port: the sprite at its own depth (its corners are only near projections) */
			if (!AddFlatMark(OTPos, PC_DRAW_FLAT_END))
				return;

			addPrim(OTPos, apoly);

			if (pFlatBit->mForcedPriority) {
				if (!AddFlatMark(OTPos, PC_DRAW_FLAT_BEGIN))
					return;
			} else {
				short xs[4] = {apoly->x0, apoly->x1, apoly->x2, apoly->x3};
				short ys[4] = {apoly->y0, apoly->y1, apoly->y2, apoly->y3};
				int x0 = xs[0], x1 = xs[0], y0 = ys[0], y1 = ys[0];

				for (int i = 1; i < 4; i++) {
					x0 = xs[i] < x0 ? xs[i] : x0;
					x1 = xs[i] > x1 ? xs[i] : x1;
					y0 = ys[i] < y0 ? ys[i] : y0;
					y1 = ys[i] > y1 ? ys[i] : y1;
				}

				if (!AddParticleDepth(OTPos, zed - pFlatBit->mFrigDeltaZ,
				                  x1 - x0 > y1 - y0 ? x1 - x0 : y1 - y0))
					return;
			}
		}
	}

	CGlassBit *pGlass;

	pGlass = GlassList;

	while (pGlass) {
		SVECTOR STemp;
		int zed;
		int16_t tri[3][3];
		GeomProj triProj[3];
		POLY_F3 *aGou;
		DR_TPAGE *ptpage;
		uchar *newppoly;
		uint a;
		uint b;
		P_TAG *OTPos;

		STemp.vx = (pGlass->mPosA.vx >> 12) - camera_pos.vx;
		STemp.vy = (pGlass->mPosA.vy >> 12) - camera_pos.vy;
		STemp.vz = (pGlass->mPosA.vz >> 12) - camera_pos.vz;
		geom_ld_sv(tri[0], &STemp);

		STemp.vx = (pGlass->mPosB.vx >> 12) - camera_pos.vx;
		STemp.vy = (pGlass->mPosB.vy >> 12) - camera_pos.vy;
		STemp.vz = (pGlass->mPosB.vz >> 12) - camera_pos.vz;
		geom_ld_sv(tri[1], &STemp);

		STemp.vx = (pGlass->mPosC.vx >> 12) - camera_pos.vx;
		STemp.vy = (pGlass->mPosC.vy >> 12) - camera_pos.vy;
		STemp.vz = (pGlass->mPosC.vz >> 12) - camera_pos.vz;
		geom_ld_sv(tri[2], &STemp);
		geom_rtpt(&pc_geom, tri, 12, 0, triProj, 0, 0);

		aGou = (POLY_F3 *)pPoly;
		ptpage = (DR_TPAGE *)(aGou + 1);
		newppoly = pPoly + sizeof(POLY_F3) + sizeof(DR_TPAGE);

		if (newppoly > PolyBufferEnd)
			return;

		pPoly = newppoly;
		setlen(aGou, 4);
		aGou->code = 0x22;
		aGou->r0 = pGlass->mR;
		aGou->g0 = pGlass->mG;
		aGou->b0 = pGlass->mB;
		a = getlen(&StoredTPagePrim);
		b = StoredTPagePrim.code[0];
		setlen(ptpage, a);
		ptpage->code[0] = b;
		geom_st_word(&aGou->x0, triProj[0].sxy);
		geom_st_word(&aGou->x1, triProj[1].sxy);
		geom_st_word(&aGou->x2, triProj[2].sxy);
		zed = triProj[2].v.mac[2];

		OTPos = pDoubleBuffer->pOT + ((zed & 0x3ffc) >> 2);
		addPrim(OTPos, aGou);
		addPrim(OTPos, ptpage);

		pGlass = (CGlassBit *)pGlass->mNext;
	}

	CGLine *pGLine;

	pGLine = GLineList;

	while (pGLine) {
		uint y0x01;
		uint y0x02;
		int zed;
		int zed2;

		Temp = (pGLine->mStart >> 12) - camera_pos;
		proj = geom_project_lv(&Temp);
		zed = proj.v.mac[2];

		if (zed >= pCurrentViewport->Hither && zed <= pCurrentViewport->Yon) {
			geom_st_word(&y0x01, proj.sxy);

			Temp = (pGLine->mEnd >> 12) - camera_pos;
			proj = geom_project_lv(&Temp);
			zed2 = proj.v.mac[2];

			if (zed2 >= pCurrentViewport->Hither && zed2 <= pCurrentViewport->Yon) {
				LINE_G2 *aline;
				uchar *newppoly;

				geom_st_word(&y0x02, proj.sxy);

				aline = (LINE_G2 *)pPoly;
				newppoly = pPoly + sizeof(LINE_G2);

				if (newppoly > PolyBufferEnd)
					return;

				pPoly = newppoly;
				setlen(aline, 4);
				*(u_long *)&aline->r0 = pGLine->mCodeBGR0;
				*(u_long *)&aline->r1 = pGLine->mPadBGR1;
				*(u_long *)&aline->x0 = y0x01;
				*(u_long *)&aline->x1 = y0x02;

				if (zed2 < zed)
					zed = zed2;

				addPrim(pDoubleBuffer->pOT + ((zed & 0x3ffc) >> 2), aline);
			}
		}

		pGLine = (CGLine *)pGLine->mNext;
	}

	CQuadBit *pQuad;

	pQuad = QuadBitList;

	while (pQuad) {
		if (pQuad->mpTexture) {
			int ZedB;
			int ZedC;
			int ZedD;
			int MaxZed;
			Texture *stored;
			POLY_FT4 *apoly;
			uchar *newppoly;
			uint foo1;
			uint foo2;
			uint foo3;

			Temp = (pQuad->mPos >> 12) - camera_pos;
			proj = geom_project_lv(&Temp);

			stored = pQuad->mpTexture;
			apoly = (POLY_FT4 *)pPoly;
			newppoly = pPoly + sizeof(POLY_FT4);

			if (newppoly > PolyBufferEnd)
				return;

			pPoly = newppoly;
			MaxZed = proj.v.mac[2];

			if (MaxZed < pCurrentViewport->Hither || MaxZed > pCurrentViewport->Yon) {
				pPoly = (uchar *)apoly;
				goto NEXTQUAD;
			}

			geom_st_word(&apoly->x0, proj.sxy);

			Temp = (pQuad->mPosB >> 12) - camera_pos;
			proj = geom_project_lv(&Temp);
			ZedB = proj.v.mac[2];

			if (ZedB < pCurrentViewport->Hither || ZedB > pCurrentViewport->Yon) {
				pPoly = (uchar *)apoly;
				goto NEXTQUAD;
			}

			geom_st_word(&apoly->x1, proj.sxy);

			Temp = (pQuad->mPosC >> 12) - camera_pos;
			proj = geom_project_lv(&Temp);

			setlen(apoly, 9);
			*(u_long *)&apoly->r0 = pQuad->mCodeBGR;

			if (ZedB > MaxZed)
				MaxZed = ZedB;

			ZedC = proj.v.mac[2];

			if (ZedC < pCurrentViewport->Hither || ZedC > pCurrentViewport->Yon) {
				pPoly = (uchar *)apoly;
				goto NEXTQUAD;
			}

			geom_st_word(&apoly->x2, proj.sxy);

			Temp = (pQuad->mPosD >> 12) - camera_pos;
			proj = geom_project_lv(&Temp);
			ushort foo4;

			foo1 = *(uint *)&stored->u0;
			foo2 = *(uint *)&stored->u1;
			foo3 = *(uint *)&stored->u2;
			foo4 = *(ushort *)&stored->u3;
			*(u_long *)&apoly->u0 = foo1;
			*(u_long *)&apoly->u1 = foo2;
			*(u_long *)&apoly->u2 = foo3;
			*(ushort *)&apoly->u3 = foo4;

			if (pQuad->mFlags & 4)
				apoly->tpage = (apoly->tpage & 0xff9f) | 0x40;

			if (ZedC > MaxZed)
				MaxZed = ZedC;

			ZedD = proj.v.mac[2];

			if (ZedD < pCurrentViewport->Hither || ZedD > pCurrentViewport->Yon) {
				pPoly = (uchar *)apoly;
				goto NEXTQUAD;
			}

			geom_st_word(&apoly->x3, proj.sxy);

			if (MaxZed < ZedD)
				MaxZed = ZedD;

			addPrim(pDoubleBuffer->pOT + (((MaxZed - pQuad->mFrigDeltaZ) & 0x3ffc) >> 2),
			        apoly);
		}
	NEXTQUAD:
		pQuad = (CQuadBit *)pQuad->mNext;
	}

	CChunkBit *p_chunk;

	p_chunk = ChunkBitList;

	while (p_chunk) {
		DR_TWIN *p_overflow2;
		int opz;
		int ZedA;
		int ZedB;
		int ZedC;
		int ZedD;
		int MaxZed;
		uint y0x0A;
		uint y0x0B;
		uint y0x0C;
		uint y0x0D;
		POLY_FT3 *aft3poly;
		DR_TWIN *pWinOff;
		DR_TWIN *pWinOn;
		uchar *newppoly;
		P_TAG *pOTPos;

		if (pPoly + 152 > PolyBufferEnd)
			return;

		MaxZed = -1;
		Temp = p_chunk->mWorldPosA - camera_pos;
		proj = geom_project_lv(&Temp);
		ZedA = proj.v.mac[2];
		geom_st_word(&y0x0A, proj.sxy);

		Temp = p_chunk->mWorldPosB - camera_pos;
		proj = geom_project_lv(&Temp);

		if (ZedA < pCurrentViewport->Hither || ZedA > pCurrentViewport->Yon)
			goto NEXTCHUNK;

		if (ZedA > MaxZed)
			MaxZed = ZedA;

		ZedB = proj.v.mac[2];
		geom_st_word(&y0x0B, proj.sxy);

		Temp = p_chunk->mWorldPosC - camera_pos;
		proj = geom_project_lv(&Temp);

		if (ZedB < pCurrentViewport->Hither || ZedB > pCurrentViewport->Yon)
			goto NEXTCHUNK;

		if (ZedB > MaxZed)
			MaxZed = ZedB;

		ZedC = proj.v.mac[2];
		geom_st_word(&y0x0C, proj.sxy);

		Temp = p_chunk->mWorldPosD - camera_pos;
		proj = geom_project_lv(&Temp);

		if (ZedC < pCurrentViewport->Hither || ZedC > pCurrentViewport->Yon)
			goto NEXTCHUNK;

		if (ZedC > MaxZed)
			MaxZed = ZedC;

		ZedD = proj.v.mac[2];

		if (ZedD < pCurrentViewport->Hither || ZedD > pCurrentViewport->Yon)
			goto NEXTCHUNK;

		if (ZedD > MaxZed)
			MaxZed = ZedD;

		geom_st_word(&y0x0D, proj.sxy);

		pOTPos = pDoubleBuffer->pOT + ((MaxZed & 0x3ffc) >> 2);

		if (p_chunk->mTexWin) {
			pWinOff = (DR_TWIN *)pPoly;
			pWinOn = pWinOff + 1;
			newppoly = (uchar *)(pWinOn + 1);
			pPoly = newppoly;

			{
				RECT win;

				win.x = win.y = win.w = win.h = 0;
				setTexWindow(pWinOff, &win);
				setTexWindow(pWinOn, &win);

				pWinOn->code[0] = p_chunk->mTexWin;
			}

			if (p_chunk->mTexWin)
				addPrim(pOTPos, pWinOff);
		}

		aft3poly = (POLY_FT3 *)pPoly;
		opz = geom_nclip(y0x0A, y0x0B, y0x0C, 0);

		if (opz <= 0) {
			setlen(aft3poly, 7);
			*(u_long *)&aft3poly->r0 = p_chunk->mB0G0R0 | 0x24000000;
			*(u_long *)&aft3poly->u0 = p_chunk->mClutV0U0;
			*(u_long *)&aft3poly->u1 = p_chunk->mTPageV1U1;
			*(ushort *)&aft3poly->u2 = p_chunk->mV2U2;
			*(u_long *)&aft3poly->x0 = y0x0A;
			*(u_long *)&aft3poly->x1 = y0x0B;
			*(u_long *)&aft3poly->x2 = y0x0C;
			addPrim(pOTPos, aft3poly);

			aft3poly++;
		}

		opz = geom_nclip(y0x0A, y0x0D, y0x0B, 0);

		if (opz <= 0) {
			setlen(aft3poly, 7);
			*(u_long *)&aft3poly->r0 = p_chunk->mB1G1R1 | 0x24000000;
			*(u_long *)&aft3poly->u0 = p_chunk->mClutV0U0;
			*(u_long *)&aft3poly->u1 = p_chunk->mTPageV1U1;
			*(ushort *)&aft3poly->u2 = p_chunk->mV2U2;
			*(u_long *)&aft3poly->x0 = y0x0A;
			*(u_long *)&aft3poly->x1 = y0x0D;
			*(u_long *)&aft3poly->x2 = y0x0B;
			addPrim(pOTPos, aft3poly);

			aft3poly++;
		}

		opz = geom_nclip(y0x0B, y0x0D, y0x0C, 0);

		if (opz <= 0) {
			setlen(aft3poly, 7);
			*(u_long *)&aft3poly->r0 = p_chunk->mB2G2R2 | 0x24000000;
			*(u_long *)&aft3poly->u0 = p_chunk->mClutV0U0;
			*(u_long *)&aft3poly->u1 = p_chunk->mTPageV1U1;
			*(ushort *)&aft3poly->u2 = p_chunk->mV2U2;
			*(u_long *)&aft3poly->x0 = y0x0B;
			*(u_long *)&aft3poly->x1 = y0x0D;
			*(u_long *)&aft3poly->x2 = y0x0C;
			addPrim(pOTPos, aft3poly);

			aft3poly++;
		}

		opz = geom_nclip(y0x0C, y0x0D, y0x0A, 0);

		if (opz <= 0) {
			setlen(aft3poly, 7);
			*(u_long *)&aft3poly->r0 = p_chunk->mB3G3R3 | 0x24000000;
			*(u_long *)&aft3poly->u0 = p_chunk->mClutV0U0;
			*(u_long *)&aft3poly->u1 = p_chunk->mTPageV1U1;
			*(ushort *)&aft3poly->u2 = p_chunk->mV2U2;
			*(u_long *)&aft3poly->x0 = y0x0C;
			*(u_long *)&aft3poly->x1 = y0x0D;
			*(u_long *)&aft3poly->x2 = y0x0A;
			addPrim(pOTPos, aft3poly);

			aft3poly++;
		}

		pPoly = (uchar *)aft3poly;

		if (p_chunk->mTexWin)
			addPrim(pOTPos, pWinOn);
	NEXTCHUNK:
		p_chunk = (CChunkBit *)p_chunk->mNext;
	}

	CPixel *pPixel;

	pPixel = PixelList;

	while (pPixel) {
		int zed;
		P_TAG *OTPos; /* the port's depth marks */

		Temp = (pPixel->mPos >> 12) - camera_pos;
		proj = geom_project_lv(&Temp);

		if ((pPixel->mWidthHeight & 0xf) == 1) {
			TILE_1 *atile;
			uchar *newppoly;

			atile = (TILE_1 *)pPoly;
			newppoly = pPoly + sizeof(TILE_1);

			if (newppoly > PolyBufferEnd)
				return;

			pPoly = newppoly;
			setlen(atile, pPixel->tag >> 24);
			*(u_long *)&atile->r0 = *(u_long *)&pPixel->r0;
			zed = proj.v.mac[2];

			if (zed < pCurrentViewport->Hither) {
				pPoly = (uchar *)atile;
				goto SKIPPIXEL;
			}

			if (zed > pCurrentViewport->Yon) {
				pPoly = (uchar *)atile;
				goto SKIPPIXEL;
			}

			geom_st_word(&atile->x0, proj.sxy);
			OTPos = pDoubleBuffer->pOT + (((zed - pPixel->mFrigDeltaZ) & 0x3ffc) >> 2);

			if (!AddFlatMark(OTPos, PC_DRAW_FLAT_END))
				return;

			addPrim(OTPos, atile);

			if (!AddParticleDepth(OTPos, zed - pPixel->mFrigDeltaZ, 1))
				return;
		} else {
			TILE *atile;
			uchar *newppoly;

			atile = (TILE *)pPoly;
			newppoly = pPoly + sizeof(TILE);

			if (newppoly > PolyBufferEnd)
				return;

			pPoly = newppoly;
			setlen(atile, pPixel->tag >> 24);
			*(u_long *)&atile->r0 = *(u_long *)&pPixel->r0;
			*(u_long *)&atile->w = pPixel->mWidthHeight;
			zed = proj.v.mac[2];

			if (zed < pCurrentViewport->Hither) {
				pPoly = (uchar *)atile;
				goto SKIPPIXEL;
			}

			if (zed > pCurrentViewport->Yon) {
				pPoly = (uchar *)atile;
				goto SKIPPIXEL;
			}

			geom_st_word(&atile->x0, proj.sxy);
			OTPos = pDoubleBuffer->pOT + (((zed - pPixel->mFrigDeltaZ) & 0x3ffc) >> 2);

			if (!AddFlatMark(OTPos, PC_DRAW_FLAT_END))
				return;

			addPrim(OTPos, atile);

			if (!AddParticleDepth(OTPos, zed - pPixel->mFrigDeltaZ,
			                  atile->w > atile->h ? atile->w : atile->h))
				return;
		}
	SKIPPIXEL:
		pPixel = (CPixel *)pPixel->mNext;
	}

	int Loop1x;
	int Loop1y;
	int Loop2x;
	int Loop2y;
	int StoredA1x;
	int StoredA1y;
	int StoredA2x;
	int StoredA2y;
	int StoredAx;
	int StoredAy;
	int Broken;
	CLinked2EndedBit *pLinked2EndedBit;

	Loop1x = 0;
	Loop1y = 0;
	Loop2x = 0;
	Loop2y = 0;
	StoredA1x = 0;
	StoredA1y = 0;
	StoredA2x = 0;
	StoredA2y = 0;
	StoredAx = 0;
	StoredAy = 0;
	Broken = 0;
	pLinked2EndedBit = Linked2EndedBitList;

	while (pLinked2EndedBit) {
		Texture *stored;
		int zed;
		uint Width;
		int dA;
		int Ax;
		int Ay;
		int NeedToTransformA;
		POLY_FT4 *apoly;
		uchar *newppoly;
		uint foo1;
		uint foo2;
		uint foo3;
		uint By0x0;
		int Bx;
		int By;
		int dB;
		int A1x;
		int A1y;
		int A2x;
		int A2y;
		int B1x;
		int B1y;
		int B2x;
		int B2y;
		int dy;
		int dx;
		int d;

		NeedToTransformA = (pLinked2EndedBit->mBitFlags & 16) || Broken;

		if (NeedToTransformA) {
			uint Ay0x0;

			Temp = (pLinked2EndedBit->mEndPosA >> 12) - camera_pos;
			proj = geom_project_lv(&Temp);

			stored = pLinked2EndedBit->mpPSXFrame->pTexture;
			Width = pLinked2EndedBit->mpPSXFrame->Width;
			zed = proj.v.mac[2];

			if (zed < pCurrentViewport->Hither || zed > pCurrentViewport->Yon) {
				Broken = 1;
				goto NEXTL2E;
			}

			geom_st_word(&Ay0x0, proj.sxy);

			Ax = (short)Ay0x0;
			Ay = (short)(Ay0x0 >> 16);
			dA = (pLinked2EndedBit->mScale * Width) / zed >> 1;

			if (dA < 2)
				dA = 2;
		} else {
			stored = pLinked2EndedBit->mpPSXFrame->pTexture;
			Width = pLinked2EndedBit->mpPSXFrame->Width;
			Ax = StoredAx;
			Ay = StoredAy;
		}

		Temp = (pLinked2EndedBit->mEndPosB >> 12) - camera_pos;
		proj = geom_project_lv(&Temp);

		apoly = (POLY_FT4 *)pPoly;
		newppoly = pPoly + sizeof(POLY_FT4);

		if (newppoly > PolyBufferEnd)
			return;

		pPoly = newppoly;
		setlen(apoly, 9);
		*(u_long *)&apoly->r0 = pLinked2EndedBit->mCodeBGR;
		ushort foo4;

		foo1 = *(uint *)&stored->u0;
		foo2 = *(uint *)&stored->u1;
		foo3 = *(uint *)&stored->u2;
		foo4 = *(ushort *)&stored->u3;
		*(u_long *)&apoly->u0 = foo1;
		*(u_long *)&apoly->u1 = foo2;
		*(u_long *)&apoly->u2 = foo3;
		*(ushort *)&apoly->u3 = foo4;
		apoly->u1--;
		apoly->v2--;
		apoly->u3--;
		apoly->v3--;
		zed = proj.v.mac[2];

		if (zed < pCurrentViewport->Hither || zed > pCurrentViewport->Yon) {
			pPoly = (uchar *)apoly;
			Broken = 1;
			goto NEXTL2E;
		}

		geom_st_word(&By0x0, proj.sxy);

		Bx = (short)By0x0;
		By = (short)(By0x0 >> 16);
		StoredAx = Bx;
		StoredAy = By;
		dB = (pLinked2EndedBit->mScale * Width) / zed >> 1;

		if (dB < 2)
			dB = 2;

		dy = By - Ay;
		dx = Bx - Ax;
		d = M3dMaths_SquareRoot0(dx * dx + dy * dy);

		if (!d) {
			pPoly = (uchar *)apoly;
			Broken = 1;
			goto NEXTL2E;
		}

		if (NeedToTransformA) {
			int dydA;
			int dxdA;

			dydA = dy * dA / d;
			dxdA = dx * dA / d;
			A1x = Ax + dydA;
			A1y = Ay - dxdA;
			A2x = Ax - dydA;
			A2y = Ay + dxdA;
		} else {
			A1x = StoredA1x;
			A1y = StoredA1y;
			A2x = StoredA2x;
			A2y = StoredA2y;
		}

		if (pLinked2EndedBit->mBitFlags & 16) {
			Loop1x = A1x;
			Loop1y = A1y;
			Loop2x = A2x;
			Loop2y = A2y;
		}

		if (pLinked2EndedBit->mBitFlags & 32) {
			B1x = Loop1x;
			B1y = Loop1y;
			B2x = Loop2x;
			B2y = Loop2y;
		} else {
			int dydB;
			int dxdB;

			dydB = dy * dB / d;
			dxdB = dx * dB / d;
			B1x = Bx + dydB;
			B1y = By - dxdB;
			B2x = Bx - dydB;
			B2y = By + dxdB;
		}

		StoredA1x = B1x;
		StoredA1y = B1y;
		StoredA2x = B2x;
		StoredA2y = B2y;

		if (pLinked2EndedBit->mBitFlags & 2) {
			if (!(pLinked2EndedBit->mBitFlags & 1)) {
				apoly->x0 = B2x;
				apoly->x1 = B1x;
				apoly->y0 = B2y;
				apoly->y1 = B1y;
				apoly->x2 = A2x;
				apoly->x3 = A1x;
				apoly->y2 = A2y;
				apoly->y3 = A1y;
			} else {
				apoly->x0 = B1x;
				apoly->x1 = B2x;
				apoly->y0 = B1y;
				apoly->y1 = B2y;
				apoly->x2 = A1x;
				apoly->x3 = A2x;
				apoly->y2 = A1y;
				apoly->y3 = A2y;
			}
		} else {
			if (pLinked2EndedBit->mBitFlags & 1) {
				apoly->x0 = A2x;
				apoly->x1 = B2x;
				apoly->y0 = A2y;
				apoly->y1 = B2y;
				apoly->x2 = A1x;
				apoly->x3 = B1x;
				apoly->y2 = A1y;
				apoly->y3 = B1y;
			} else {
				apoly->x0 = A1x;
				apoly->x1 = B1x;
				apoly->y0 = A1y;
				apoly->y1 = B1y;
				apoly->x2 = A2x;
				apoly->x3 = B2x;
				apoly->y2 = A2y;
				apoly->y3 = B2y;
			}
		}

		addPrim(
		    pDoubleBuffer->pOT + (((zed - pLinked2EndedBit->mFrigDeltaZ) & 0x3ffc) >> 2),
		    apoly);

		Broken = 0;
	NEXTL2E:
		pLinked2EndedBit = (CLinked2EndedBit *)pLinked2EndedBit->mNext;
	}
}

/* _._14CGouraudRibbon (0x8002da78) */
/* Really CGouraudRibbon::~CGouraudRibbon (functions.tsv's TextScreenElement name is a
   mis-port; see build/hdr_notes/bit.md): frees the ribbon's points; CSpecialDisplay's destructor does
   the rest. */
CGouraudRibbon::~CGouraudRibbon() {
	Mem_Delete(mpPoints);
}

/* CalcScreenNormal__FP11SCalcBufferRiT1i (0x8002dad8) */
void CalcScreenNormal(SCalcBuffer *pBuf, int &UnitX, int &UnitY, int ClosenessLimit) {
	if (pBuf[0].Bad == 0 && pBuf[1].Bad == 0) {
		int xa, ya, xb, yb, dx, dy, l;

		xa = ((short *)&pBuf[0].Pos)[0];
		ya = ((short *)&pBuf[0].Pos)[1];
		xb = ((short *)&pBuf[1].Pos)[0];
		yb = ((short *)&pBuf[1].Pos)[1];
		dx = xb - xa;
		dy = yb - ya;
		dx = (dx * 320) / 512;
		UnitX = dy;
		UnitY = -dx;

		if (dx < 0)
			dx = -dx;

		if (dy < 0)
			dy = -dy;

		if (dy < dx)
			l = dx + dy / 2;
		else
			l = dy + dx / 2;

		if (l >= ClosenessLimit) {
			UnitX = (UnitX << 6) / l;
			UnitY = (UnitY << 6) / l;
			UnitX = (UnitX << 9) / 320;
			return;
		}
	}

	UnitX = UnitY = 0;
}

/* Display__14CGouraudRibbon (0x8002dbf0) */
/* Fuzzy demo twin (0.97); locals from its MAIN.SYM. Draws a Gouraud-shaded ribbon:
   transforms each point into a scratchpad buffer (screen position, depth, width), computes
   averaged screen normals, then emits two semi-transparent POLY_G4 quads per segment (top and
   bottom half) plus a DR_TPAGE. After the first frames it adds a third quad between the last two
   frames' positions (the trail). */
void CGouraudRibbon::Display() {
	SRibbonPoint *pPoint;
	SCalcBuffer *pBuf;
	int LastNormalX;
	int LastNormalY;
	SCalcBuffer *pLastBuf;
	SRibbonPoint *pLastPoint;
	int x;
	int y;
	int dx;
	int dy;
	int LastTop;
	int LastBottom;

	pBuf = (SCalcBuffer *)SCRATCHPAD;
	pPoint = mpPoints;

	{
		int i;

		for (i = 0; i < mNumPoints; i++) {
			pBuf->Zed = Transform(pPoint->Pos, pBuf->Pos);

			if (pBuf->Zed < pCurrentViewport->Hither || pCurrentViewport->Yon < pBuf->Zed) {
				pBuf->Bad = 1;
			} else {
				pBuf->Bad = 0;
				pBuf->ScrWidth = pPoint->Width * 400 / pBuf->Zed;
			}

			pBuf++;
			pPoint++;
		}
	}

	pBuf = (SCalcBuffer *)SCRATCHPAD;
	CalcScreenNormal(pBuf, LastNormalX, LastNormalY, 2);

	pBuf->NormalX = LastNormalX;
	pBuf->NormalY = LastNormalY;
	pBuf++;

	{
		int i;

		for (i = 1; i < mNumPoints - 1; i++) {
			int TempX;
			int TempY;

			CalcScreenNormal(pBuf, TempX, TempY, 2);

			pBuf->NormalX = (TempX + LastNormalX) / 2;
			pBuf->NormalY = (TempY + LastNormalY) / 2;
			pBuf++;
			LastNormalX = TempX;
			LastNormalY = TempY;
		}
	}

	pBuf->NormalX = LastNormalX;
	pBuf->NormalY = LastNormalY;
	pBuf = (SCalcBuffer *)SCRATCHPAD;
	dx = pBuf->NormalX * pBuf->ScrWidth >> 6;
	dy = pBuf->NormalY * pBuf->ScrWidth >> 6;
	x = (short)pBuf->Pos;
	y = pBuf->Pos >> 16;
	LastTop = ((y + dy) << 16) | ((x + dx) & 0xffff);
	LastBottom = ((y - dy) << 16) | ((x - dx) & 0xffff);
	pLastBuf = (SCalcBuffer *)SCRATCHPAD;
	pBuf = pLastBuf + 1;
	pLastPoint = mpPoints;
	pPoint = pLastPoint + 1;

	{
		int i;

		for (i = 1; i < mNumPoints; pLastBuf++, pBuf++, pLastPoint++, i++, pPoint++) {
			P_TAG *pOTPos;

			if (!(pBuf->NormalX | pBuf->NormalY))
				continue;

			if (!(pLastBuf->NormalX | pLastBuf->NormalY))
				continue;

			{
				int NewTop;
				int NewBottom;
				POLY_G4 *pTopPoly;
				POLY_G4 *pBottomPoly;
				DR_TPAGE *ptpage;
				uchar *newppoly;
				uint a;
				uint b;

				pOTPos =
				    pDoubleBuffer->pOT + (((pBuf->Zed - mFrigDeltaZ) & 0x3ffc) >> 2);
				dx = pBuf->NormalX * pBuf->ScrWidth >> 6;
				dy = pBuf->NormalY * pBuf->ScrWidth >> 6;
				x = (short)pBuf->Pos;
				y = pBuf->Pos >> 16;
				NewTop = ((y + dy) << 16) | ((x + dx) & 0xffff);
				NewBottom = ((y - dy) << 16) | ((x - dx) & 0xffff);
				pTopPoly = (POLY_G4 *)pPoly;
				pBottomPoly = pTopPoly + 1;
				ptpage = (DR_TPAGE *)(pBottomPoly + 1);
				newppoly = (uchar *)(ptpage + 1);

				if (newppoly > PolyBufferEnd)
					goto SEEYA;

				a = getlen(&StoredTPagePrim);
				b = StoredTPagePrim.code[0];
				setlen(ptpage, a);
				ptpage->code[0] = b;
				pPoly = newppoly;

				if (mTrail >= 3) {
					{
						uchar *newppoly = pPoly + sizeof(POLY_G4);

						if (newppoly > PolyBufferEnd)
							goto SEEYA;

						pPoly = newppoly;
					}

					{
						POLY_G4 *pTrail = (POLY_G4 *)newppoly;

						setlen(pTrail, 8);
						*(u_long *)&pTrail->r0 = 0x3a000000;
						*(u_long *)&pTrail->r1 = 0;
						*(u_long *)&pTrail->r3 = *(uint *)&pLastPoint->r;
						*(u_long *)&pTrail->r2 = *(uint *)&pPoint->r;
						*(u_long *)&pTrail->x1 = pLastPoint->Last2Scr;
						*(u_long *)&pTrail->x0 = pPoint->Last2Scr;
						*(u_long *)&pTrail->x3 = pLastBuf->Pos;
						*(u_long *)&pTrail->x2 = pBuf->Pos;
						addPrim(pOTPos, pTrail);
					}
				}

				setlen(pTopPoly, 8), setlen(pBottomPoly, 8);
				*(u_long *)&pTopPoly->r0 = *(u_long *)&pBottomPoly->r0 = 0x3a000000;
				*(u_long *)&pTopPoly->r1 = 0;
				*(u_long *)&pTopPoly->r3 = *(uint *)&pLastPoint->r;
				*(u_long *)&pTopPoly->r2 = *(uint *)&pPoint->r;
				*(u_long *)&pTopPoly->x0 = NewTop;
				*(u_long *)&pTopPoly->x1 = LastTop;
				*(u_long *)&pTopPoly->x2 = pBuf->Pos;
				*(u_long *)&pTopPoly->x3 = pLastBuf->Pos;
				addPrim(pOTPos, pTopPoly);

				*(u_long *)&pBottomPoly->r1 = 0;
				*(u_long *)&pBottomPoly->r3 = *(uint *)&pLastPoint->r;
				*(u_long *)&pBottomPoly->r2 = *(uint *)&pPoint->r;
				*(u_long *)&pBottomPoly->x0 = NewBottom;
				*(u_long *)&pBottomPoly->x1 = LastBottom;
				*(u_long *)&pBottomPoly->x2 = pBuf->Pos;
				LastBottom = NewBottom;
				*(u_long *)&pBottomPoly->x3 = pLastBuf->Pos;
				addPrim(pOTPos, pBottomPoly);

				LastTop = NewTop;
				addPrim(pOTPos, ptpage);
			}
		}
	}
SEEYA:
	if (mTrail) {
		int i;

		pBuf = (SCalcBuffer *)SCRATCHPAD;
		pPoint = mpPoints;

		for (i = 0; i < mNumPoints; i++) {
			if (mTrail == 1) {
				pPoint->Last2Scr = pPoint->Last1Scr = pBuf->Pos;
			} else {
				pPoint->Last2Scr = pPoint->Last1Scr;
				pPoint->Last1Scr = pBuf->Pos;
			}

			pPoint++;
			pBuf++;
		}

		mTrail++;
	}
}

/* __15CTexturedRibbonii (0x8002e1e0) */
/* A textured ribbon of NumPoints points, all zero width. */
CTexturedRibbon::CTexturedRibbon(int NumPoints, int Trail) {
	mNumPoints = NumPoints;
	mpPoints = (SRibbonPoint *)Mem_NewX(NumPoints * 36, 0, 1);

	for (int i = 0; i < mNumPoints; i++) {
		mpPoints[i].WidthB = 0;
		mpPoints[i].Width = 0;
	}

	mTrail = Trail;
	mDv = 8;
}

/* FUN_8002e290 (0x8002e290) */
/* CTexturedRibbon's destructor (vtable slot 0), kept as the egcs-ABI function (this, flags). */
extern "C" void FUN_8002e290(CTexturedRibbon *p, int f) {
	p->__vptr = __vt_15CTexturedRibbon;
	Mem_Delete(p->mpPoints);
	dt_15CSpecialDisplay(p, 0);

	if (f & 1)
		__dl__4CBitPv(p);
}

/* SetTexture__15CTexturedRibbonUi (0x8002e2f0) */
/* Looks up the ribbon's texture by checksum. */
void CTexturedRibbon::SetTexture(unsigned int checksum) {
	mpTex = Spool_FindTextureEntry(checksum);
}

/* Display__15CTexturedRibbon (0x8002e31c) */
/* Fuzzy demo twin (0.99); locals and blocks from its MAIN.SYM. The textured sibling
   of CGouraudRibbon::Display: transforms the points into the scratchpad (screen position, depth,
   inner and outer widths), smooths the screen normals, then per segment emits four POLY_GT4s
   (inner and outer strips, top and bottom), bracketed by texture-window on/off prims and a
   DR_TPAGE. The upper half of a wibbling ribbon gets sine offsets on its vertices.
   Near-miss (685/695): in the wibble block the sine-table address is built through v1
   (`lui v1; addiu t0,v1`), and gcc's delay-slot filler hoists that lui; retail builds it in t0. */
void CTexturedRibbon::Display() {
	SRibbonPoint *pPoint;
	SCalcBuffer *pBuf;
	int LastNormalX;
	int LastNormalY;
	SCalcBuffer *pLastBuf;
	SRibbonPoint *pLastPoint;
	int x;
	int y;
	int dx;
	int dy;
	int dxB;
	int dyB;
	int LastTop;
	int LastBottom;
	int LastTopB;
	int LastBottomB;
	int voff;

	if (mDontDisplay)
		return;

	pBuf = (SCalcBuffer *)SCRATCHPAD;
	pPoint = mpPoints;

	{
		int i;

		for (i = 0; i < mNumPoints; i++) {
			pBuf->Zed = Transform(pPoint->Pos, pBuf->Pos);

			if (pBuf->Zed < pCurrentViewport->Hither || pCurrentViewport->Yon < pBuf->Zed) {
				pBuf->Bad = 1;
			} else {
				pBuf->Bad = 0;
				pBuf->ScrWidth = pPoint->Width * 400 / pBuf->Zed;
				pBuf->ScrWidthB = pPoint->WidthB * 400 / pBuf->Zed;
			}

			pBuf++;
			pPoint++;
		}
	}

	LastNormalX = 0;
	pBuf = (SCalcBuffer *)SCRATCHPAD;
	LastNormalY = 0;

	{
		int i;

		for (i = 0; i < mNumPoints - 1; i++) {
			CalcScreenNormal(pBuf, pBuf->NormalX, pBuf->NormalY, 4);

			if (!pBuf->NormalX && !pBuf->NormalY) {
				pBuf->NormalX = LastNormalX;
				pBuf->NormalY = LastNormalY;
			}

			LastNormalX = pBuf->NormalX;
			LastNormalY = pBuf->NormalY;
			pBuf++;
		}
	}

	pBuf->NormalX = LastNormalX;
	pBuf->NormalY = LastNormalY;
	pBuf = (SCalcBuffer *)SCRATCHPAD;
	dx = pBuf->NormalX * pBuf->ScrWidth >> 6;
	dy = pBuf->NormalY * pBuf->ScrWidth >> 6;
	dxB = pBuf->NormalX * pBuf->ScrWidthB >> 6;
	dyB = pBuf->NormalY * pBuf->ScrWidthB >> 6;
	x = (short)pBuf->Pos;
	y = pBuf->Pos >> 16;
	pLastBuf = (SCalcBuffer *)SCRATCHPAD;
	voff = mV;
	LastTop = ((y + dy) << 16) | ((x + dx) & 0xffff);
	LastBottom = ((y - dy) << 16) | ((x - dx) & 0xffff);
	LastTopB = ((y + dyB) << 16) | ((x + dxB) & 0xffff);
	LastBottomB = ((y - dyB) << 16) | ((x - dxB) & 0xffff);
	pBuf = pLastBuf + 1;
	pLastPoint = mpPoints;
	pPoint = pLastPoint + 1;

	{
		int i;

		for (i = 1; i < mNumPoints; pLastBuf++, pBuf++, pLastPoint++, pPoint++, i++) {
			P_TAG *pOTPos;

			if (!(pBuf->NormalX | pBuf->NormalY))
				continue;

			if (!(pLastBuf->NormalX | pLastBuf->NormalY))
				continue;

			{
				int MinZ;
				int NewTop;
				int NewBottom;
				int NewTopB;
				int NewBottomB;
				POLY_GT4 *pTopPoly;
				POLY_GT4 *pBottomPoly;
				POLY_GT4 *pTopPolyB;
				POLY_GT4 *pBottomPolyB;
				DR_TWIN *pWinOn;
				DR_TWIN *pWinOff;
				DR_TPAGE *ptpage;
				uchar *newppoly;
				RECT win;
				int u;
				int v;
				uint a;
				uint b;

				MinZ = pLastBuf->Zed;

				if (pBuf->Zed < MinZ)
					MinZ = pBuf->Zed;

				dx = pBuf->NormalX * pBuf->ScrWidth >> 6;
				dy = pBuf->NormalY * pBuf->ScrWidth >> 6;
				dxB = pBuf->NormalX * pBuf->ScrWidthB >> 6;
				dyB = pBuf->NormalY * pBuf->ScrWidthB >> 6;
				x = (short)pBuf->Pos;
				y = pBuf->Pos >> 16;
				NewTop = ((y + dy) << 16) | ((x + dx) & 0xffff);
				NewBottom = ((y - dy) << 16) | ((x - dx) & 0xffff);
				pOTPos = pDoubleBuffer->pOT + ((MinZ & 0x3ffc) >> 2);
				NewTopB = ((y + dyB) << 16) | ((x + dxB) & 0xffff);
				NewBottomB = ((y - dyB) << 16) | ((x - dxB) & 0xffff);

				if (mWibble && i >= mNumPoints / 2) {
					NewTop = ((y + dy + rcossin_tbl[(TTime * 200 + 130) & 4095].s / 2048) << 16) |
					         ((x + dx) & 0xffff);
					y += rcossin_tbl[(TTime * 250) & 4094].s * 3 / 4096;
					pBuf->Pos = (pBuf->Pos & 0xffff) | (y << 16);
					NewBottom =
					    ((y - dy + rcossin_tbl[(TTime * 150 + 100) & 4095].s / 2048) << 16) |
					    ((x - dx) & 0xffff);
				}

				pTopPoly = (POLY_GT4 *)pPoly;
				pBottomPoly = pTopPoly + 1;
				pTopPolyB = pBottomPoly + 1;
				pBottomPolyB = pTopPolyB + 1;
				pWinOn = (DR_TWIN *)(pBottomPolyB + 1);
				pWinOff = pWinOn + 1;
				ptpage = (DR_TPAGE *)(pWinOff + 1);
				newppoly = (uchar *)(ptpage + 1);

				if (newppoly > PolyBufferEnd)
					return;

				pPoly = newppoly;
				win.x = win.y = win.w = win.h = 0;
				setTexWindow(pWinOff, &win);

				win.x = mpTex->u0;
				win.y = mpTex->v0;
				win.w = 64;
				win.h = 64;
				setTexWindow(pWinOn, &win);

				/* the port: the strips' edges are made in screen space, so the segment has its ends'
				   depths, brought forward by their widths (pc_draw.h PC_DRAW_DEPTH) */
				if (!AddFlatMark(pOTPos, PC_DRAW_FLAT_END))
					return;

				addPrim(pOTPos, pWinOff);

				setlen(pTopPoly, 12), setlen(pBottomPoly, 12), setlen(pTopPolyB, 12), setlen(pBottomPolyB, 12);
				pTopPoly->clut = pBottomPoly->clut = pTopPolyB->clut = pBottomPolyB->clut =
				    mpTex->clut;
				pTopPoly->tpage = pBottomPoly->tpage = pTopPolyB->tpage = pBottomPolyB->tpage =
				    mpTex->tpage;
				u = mpTex->u0 + (mU & 63);
				v = mpTex->v0 + (voff & 63);
				pTopPolyB->u0 = u;
				pTopPolyB->v0 = v;
				pTopPolyB->u2 = u + 16;
				pTopPolyB->v2 = v;
				pTopPolyB->u1 = u;
				pTopPolyB->v1 = v + mDv;
				pTopPolyB->u3 = u + 16;
				pTopPolyB->v3 = v + mDv;
				pTopPoly->u0 = u + 16;
				pTopPoly->v0 = v;
				pTopPoly->u2 = u + 32;
				pTopPoly->v2 = v;
				pTopPoly->u1 = u + 16;
				pTopPoly->v1 = v + mDv;
				pTopPoly->u3 = u + 32;
				pTopPoly->v3 = v + mDv;
				pBottomPoly->u0 = u + 48;
				pBottomPoly->v0 = v;
				pBottomPoly->u2 = u + 32;
				pBottomPoly->v2 = v;
				pBottomPoly->u1 = u + 48;
				pBottomPoly->v1 = v + mDv;
				pBottomPoly->u3 = u + 32;
				pBottomPoly->v3 = v + mDv;
				pBottomPolyB->u0 = u + 64;
				pBottomPolyB->v0 = v;
				pBottomPolyB->u2 = u + 48;
				pBottomPolyB->v2 = v;
				pBottomPolyB->u1 = u + 64;
				pBottomPolyB->v1 = v + mDv;
				pBottomPolyB->u3 = u + 48;
				pBottomPolyB->v3 = v + mDv;
				voff -= mDv;
				*(u_long *)&pTopPoly->r0 = *(uint *)&pPoint->rB;
				*(u_long *)&pTopPoly->r1 = *(uint *)&pLastPoint->rB;
				*(u_long *)&pTopPoly->r2 = *(uint *)&pPoint->r;
				*(u_long *)&pTopPoly->r3 = *(uint *)&pLastPoint->r;
				*(u_long *)&pTopPoly->x0 = NewTop;
				*(u_long *)&pTopPoly->x1 = LastTop;
				*(u_long *)&pTopPoly->x2 = pBuf->Pos;
				*(u_long *)&pTopPoly->x3 = pLastBuf->Pos;
				*(u_long *)&pTopPolyB->r0 = 0;
				*(u_long *)&pTopPolyB->r1 = 0;
				*(u_long *)&pTopPolyB->r2 = *(uint *)&pPoint->rB;
				*(u_long *)&pTopPolyB->r3 = *(uint *)&pLastPoint->rB;
				*(u_long *)&pTopPolyB->x0 = NewTopB;
				*(u_long *)&pTopPolyB->x1 = LastTopB;
				*(u_long *)&pTopPolyB->x2 = NewTop;
				*(u_long *)&pTopPolyB->x3 = LastTop;
				*(u_long *)&pBottomPoly->r0 = *(uint *)&pPoint->rB;
				*(u_long *)&pBottomPoly->r1 = *(uint *)&pLastPoint->rB;
				*(u_long *)&pBottomPoly->r2 = *(uint *)&pPoint->r;
				*(u_long *)&pBottomPoly->r3 = *(uint *)&pLastPoint->r;
				*(u_long *)&pBottomPoly->x0 = NewBottom;
				*(u_long *)&pBottomPoly->x1 = LastBottom;
				*(u_long *)&pBottomPoly->x2 = pBuf->Pos;
				*(u_long *)&pBottomPoly->x3 = pLastBuf->Pos;
				*(u_long *)&pBottomPolyB->r0 = 0;
				*(u_long *)&pBottomPolyB->r1 = 0;
				*(u_long *)&pBottomPolyB->r2 = *(uint *)&pPoint->rB;
				*(u_long *)&pBottomPolyB->r3 = *(uint *)&pLastPoint->rB;
				*(u_long *)&pBottomPolyB->x0 = NewBottomB;
				*(u_long *)&pBottomPolyB->x1 = LastBottomB;
				*(u_long *)&pBottomPolyB->x2 = NewBottom;
				*(u_long *)&pBottomPolyB->x3 = LastBottom;
				pTopPoly->code = pBottomPoly->code = pTopPolyB->code = pBottomPolyB->code = 0x3e;
				addPrim(pOTPos, pTopPoly);
				addPrim(pOTPos, pTopPolyB);
				addPrim(pOTPos, pBottomPoly);
				addPrim(pOTPos, pBottomPolyB);

				LastTop = NewTop;
				addPrim(pOTPos, pWinOn);

				LastTopB = NewTopB;
				a = getlen(&StoredTPagePrim);
				b = StoredTPagePrim.code[0];
				setlen(ptpage, a);
				ptpage->code[0] = b;
				addPrim(pOTPos, ptpage);

				{
					int z = pBuf->Zed - pPoint->WidthB;
					int zLast = pLastBuf->Zed - pLastPoint->WidthB;
					int zs[4] = {z, zLast, z, zLast};

					if (!AddDepthMark(pOTPos, zs))
						return;
				}

				LastBottom = NewBottom;
				LastBottomB = NewBottomB;
			}
		}
	}
}

/* FUN_8002edf8 (0x8002edf8) */
void FUN_8002edf8(void) {}

/* FUN_8002ee00 (0x8002ee00) */
/* CWobblyGlow's destructor (vtable slot 0), kept as the egcs-ABI function (this, flags). */
extern "C" void FUN_8002ee00(CWobblyGlow *p, int f) {
	p->__vptr = __vt_11CWobblyGlow;
	dt_5CGlow(p, 0);

	if (f & 1)
		__dl__4CBitPv(p);
}

/* FUN_8002ee54 (0x8002ee54) */
void FUN_8002ee54(void) {}

/* FUN_8002ee5c (0x8002ee5c) */
void FUN_8002ee5c(void) {}

/* FUN_8002ee64 (0x8002ee64) */
void FUN_8002ee64(void) {}
