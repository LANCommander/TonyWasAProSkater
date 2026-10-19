/* SHATTER.CPP: breakable glass: shatters an item's faces into flying chunks (CShatterBit) or
   glass bits, and the shatter sound (declarations in game/shatter.h). */

#include "game/shatter.h"
#include "game/bit.h"
#include "game/utils.h"
#include "game/vector.h"
#include <sys/types.h>
#include <libgte.h>
#include "game/m3d.h"
#include "game/m3dasm.h"
#include "pc_dctex.h"
#include "pc_assets.h"
#include "game/colij.h"
#include "game/camera.h"
#include "game/globals.h"
#include "game/m3dcolij.h"
#include "game/mem.h"
#include "game/sfx.h"
#include "game/m3dzone.h"

extern "C" char __vt_11CShatterBit[];
/* egcs-ABI calls, as retail makes them: ~CChunkBit (this, flags) and CBit's operator delete */
extern "C" void FUN_80028968(CChunkBit *p, int a);
extern "C" void __dl__4CBitPv(void *p);
/* the face being shattered's normal */
extern CSVector *pNormal;
/* the face's centre, the shards fly out from it */
extern CVector DAT_800d2574;
/* the face texture's CLUT and texture page (ints; Split reads their low halves) */
extern int VRAMClut;
extern int VRAMTPage;
extern uchar AverageR, AverageG, AverageB;
void TransformVertex(CVector &, SVECTOR *, uchar *, int);
void CalcRGB(int, uint, int, uint *);
void Shatter_Glass(int, const CVector &, const CVector &, const CVector &, const CVector &, uchar,
                   uchar, uchar);
void Split(const CVector &, const CVector &, const CVector &, int, int, int, int, int, int, uint,
           int);
extern int ShatterSound;
extern CVector ShatterSoundPos;

/* __11CShatterBitRC8CSVectorN21RC7CVectori (0x80020b84) */
/* Demo twin; locals from its MAIN.SYM (two inlined CSVector setters, `this` in
   v1). A shattered-glass chunk: random spin about X or Y, a random lifetime, placed at pos, and
   half the time a smoke trail. THPS1 drops the demo's lifetime * 5 / 6. */
CShatterBit::CShatterBit(const CSVector &a, const CSVector &b, const CSVector &c,
                         const CVector &pos, int Smoke)
    : CChunkBit(a, b, c) {
	if (Rnd(2))
		mAngVel.Set(Rnd(160) + 80, 0, 0);
	else
		mAngVel.Set(0, Rnd(160) + 80, 0);

	mLifetime = Rnd(30) + 45;
	SetPos(pos);
	CalculateWorldCoords();

	if (Rnd(2)) {
		mpSmokeTrail = new CSmokeTrail(mPos, 4, 128, 128, 128);
		mpSmokeTrail->mProtected = 1;
		mpSmokeTrail->SetScale(1200);
		mpSmokeTrail->SetTint(32, 32, 32);
	}
}

/* FUN_80020c90 (0x80020c90) */
/* CShatterBit's destructor (vtable slot 0), kept as the egcs-ABI function (this, flags): the smoke
   trail is deleted through its vtable, then ~CChunkBit. */
extern "C" void FUN_80020c90(CShatterBit *p, int f) {
	CSmokeTrail *o = p->mpSmokeTrail;

	p->__vptr = __vt_11CShatterBit;

	if (o)
		delete o;

	FUN_80028968(p, 0);

	if (f & 1)
		__dl__4CBitPv(p);
}

/* SetPos__11CShatterBitRC7CVector (0x80020d0c) */
/* Places a glass chunk and throws it out from the shattered face's centre, plus a
   random push along the face normal and upwards (sometimes doubled); never downwards. */
void CShatterBit::SetPos(const CVector &Pos) {
	CBit::SetPos(Pos);

	CVector Dir = Pos - DAT_800d2574;
	int Len = Dir.Length();

	if (Len) {
		int Speed = Rnd(48);

		mVel = (Speed * Dir) / Len;
	} else {
		Die();
	}

	int Push = Rnd(45);

	mVel.vx += Push * pNormal->vx;
	mVel.vy += Push * pNormal->vy;
	mVel.vz += Push * pNormal->vz;
	mVel.vy -= Rnd(175) << 12;

	if (Rnd(20) == 0)
		mVel <<= 1;

	if (mVel.vy > 0)
		mVel.vy = 0;

	if (mpSmokeTrail)
		mpSmokeTrail->SetPos(mPos);
}

/* Move__11CShatterBit (0x80020e8c) */
/* A glass chunk flies and spins under gravity, drags its smoke trail along and
   dies when its lifetime runs out. */
void CShatterBit::Move() {
	mPos += mVel;
	mVel.vy += 0x5014;
	mAngles += mAngVel;

	if (mpSmokeTrail)
		mpSmokeTrail->SetPos(mPos);

	CalculateWorldCoords();

	if (mLifetime)
		mLifetime--;
	else
		Die();
}

/* Split__FRC7CVectorN20iiiiiiUii (0x80020f1c) */
void Split(const CVector &a0, const CVector &a1, const CVector &a2, int a3, int a4, int a5, int a6,
           int a7, int a8, uint a9, int a10) {
	CVector t30;
	int s0, s1, l2, x7, x8, r;

	if (LowMemory == 0) {
		if (a10 != 0) {
			t30 = a1 - a2;
			s1 = t30.Length();
			s0 = 0;

			t30 = a0 - a2;
			l2 = t30.Length();

			if (s1 < l2) {
				s1 = l2;
				s0 = 1;
			}

			t30 = a0 - a1;

			if (s1 < t30.Length())
				s0 = 2;

			r = Rnd(9);

			switch (s0) {
				case 0: {
					int n;

					t30 = a1 + (((a2 - a1) * r) >> 3);
					n = a10 - 1;
					x7 = a5 + (((a7 - a5) * r) >> 3);
					x8 = a6 + (((a8 - a6) * r) >> 3);
					Split(a0, a1, t30, a3, a4, a5, a6, x7, x8, a9, n);
					Split(a0, a2, t30, a3, a4, a7, a8, x7, x8, a9, n);
					return;
				}
				case 1: {
					int n;

					t30 = a0 + (((a2 - a0) * r) >> 3);
					n = a10 - 1;
					x7 = a3 + (((a7 - a3) * r) >> 3);
					x8 = a4 + (((a8 - a4) * r) >> 3);
					Split(a1, a0, t30, a5, a6, a3, a4, x7, x8, a9, n);
					Split(a1, a2, t30, a5, a6, a7, a8, x7, x8, a9, n);
					return;
				}
				case 2: {
					int n;

					t30 = a0 + (((a1 - a0) * r) >> 3);
					n = a10 - 1;
					x7 = a3 + (((a5 - a3) * r) >> 3);
					x8 = a4 + (((a6 - a4) * r) >> 3);
					Split(a2, a0, t30, a7, a8, a3, a4, x7, x8, a9, n);
					Split(a2, a1, t30, a7, a8, a5, a6, x7, x8, a9, n);
					return;
				}
			}
		} else {
			CVector t40, t50, t60;
			CSVector p0, p1, p2;
			CShatterBit *o;

			t30 = a0 >> 12;
			t40 = a1 >> 12;
			t50 = a2 >> 12;

			t60.vx = (t30.vx + t40.vx + t50.vx) / 3;
			t60.vy = (t30.vy + t40.vy + t50.vy) / 3;
			t60.vz = (t30.vz + t40.vz + t50.vz) / 3;
			p0.vx = (unsigned short)t30.vx - (unsigned short)t60.vx;
			p0.vy = (unsigned short)t30.vy - (unsigned short)t60.vy;
			p0.vz = (unsigned short)t30.vz - (unsigned short)t60.vz;
			p1.vx = (unsigned short)t40.vx - (unsigned short)t60.vx;
			p1.vy = (unsigned short)t40.vy - (unsigned short)t60.vy;
			p1.vz = (unsigned short)t40.vz - (unsigned short)t60.vz;
			p2.vx = (unsigned short)t50.vx - (unsigned short)t60.vx;
			p2.vy = (unsigned short)t50.vy - (unsigned short)t60.vy;
			p2.vz = (unsigned short)t50.vz - (unsigned short)t60.vz;
			r = Rnd(100);
			p0 += (r * *pNormal) >> 12;
			r = Rnd(100);
			p1 += (r * *pNormal) >> 12;
			r = Rnd(100);
			p2 += (r * *pNormal) >> 12;
			t60 <<= 12;

			UseCacheForBits = 0;
			o = new CShatterBit(p0, p1, p2, t60, 0);
			UseCacheForBits = 1;
			o->SetRGB(AverageR, AverageG, AverageB);
			o->SetUVs(*(ushort *)&VRAMClut, *(ushort *)&VRAMTPage, a3 & 255, a4 & 255, a5 & 255, a6 & 255, a7 & 255,
			          a8 & 255);
			o->mTexWin = a9;
		}
	}
}

/* TransformVertex__FR7CVectorP7SVECTORPUci (0x800216d0) */
void TransformVertex(CVector &out, SVECTOR *verts, uchar *idx, int i) {
	int s;

	/* geom_transform_sv written out: as C++, its GeomAcc copy keeps the dead IR clamps that the
	   C original's compiler dropped */
	{
		int16_t v[3];
		GeomAcc a;
		uint32_t f = 0;

		geom_ld_sv(v, &verts[idx[i]]);
		geom__mat_vec(&a, pc_geom.rot, pc_geom.tr, v[0], v[1], v[2], 12, 0, &f);
		__builtin_memcpy(&out, a.mac, 8);
		out.vz = a.mac[2];
	}

	s = 12;
	out <<= s;
}

/* CalcRGB__FiUiiPUi (0x8002172c) */
void CalcRGB(int f, unsigned c, int mode, unsigned *t) {
	unsigned a, b, d;
	int r, g, bl;

	if (f) {
		a = t[c & 255];
		b = t[(c >> 8) & 255];
		d = t[(c >> 16) & 255];
		r = (a & 255) + (b & 255) + (d & 255);
		g = ((a >> 8) & 255) + ((b >> 8) & 255) + ((d >> 8) & 255);
		bl = ((a >> 16) & 255) + ((b >> 16) & 255) + ((d >> 16) & 255);

		if (mode == 4) {
			unsigned e = t[c >> 24];

			r += e & 255;
			g += (e >> 8) & 255;
			bl += (e >> 16) & 255;
			AverageR = r / 4;
			AverageG = g / 4;
			AverageB = bl / 4;
		} else {
			AverageR = r / 3;
			AverageG = g / 3;
			AverageB = bl / 3;
		}
	} else {
		AverageR = c;
		AverageG = c >> 8;
		AverageB = c >> 16;
	}
}

static inline void SetV(CVector *v, int _x, int _y, int _z) {
	v->vx = _x;
	v->vy = _y;
	v->vz = _z;
}

/* Shatter_Face__FP4ItemPUiiiiii (0x800218a4) */
/* Fuzzy demo twin (0.98); locals from its MAIN.SYM. Shatters one face of an
   item's model. Optionally it first checks/updates the face's shatterable flags. Then it
   transforms the face's three or four vertices into world space and sets the colour and centroid.
   Untextured (or flagged) faces become a flash; textured ones are split into shards (a quad as
   two triangles). Returns 0 if not shatterable, 1 if nothing was tried, 2 for a flash. */
int Shatter_Face(Item *pItem, uint *pFace, int NumSplits, int MakeFlash, int UseFlags,
                 int ModifyFlags, int TryToShatter) {
	SModel *pModel;
	SVECTOR *pVertices;
	SVECTOR *pNormals;
	uchar *pVertexNums;
	Texture *pTexture;
	MATRIX Transform;
	CVector a;
	CVector b;
	CVector c;
	int retval;
	SFace *f = (SFace *)pFace;

	pModel = PSXRegion[pItem->Region].ppModels[pItem->Model];
	pVertices = pModel->Vertices;
	pNormals = pVertices + pModel->NumVertices;
	pVertexNums = (uchar *)(pFace + 1);

	if (UseFlags) {
		if (!(f->Flags2 & 8))
			return 0;

		if (ModifyFlags) {
			f->Flags2 = (f->Flags2 | 1) & ~8;
			f->Flags &= ~0x80;
			f->Flags &= ~0x100;

			if (!TryToShatter && ((f->Flags & 0x40) || !(f->Flags & 1)))
				f->Flags &= ~0x140;
		}
	}

	if (!TryToShatter)
		return 1;

	if (f->Flags & 1) {
		/* a Dreamcast face names its Texture entry (pc_dctex.h); the shards use the whole
		   texture */
		if ((f->Flags & 2) && (f->TexWord & PC_DC_FACE_TEX))
			pTexture = (Texture *)PC_DcFaceTex(f->TexWord);
		else if (f->Flags & 2)
			pTexture = (Texture *)&f->TexWord;
		else
			pTexture = PSXRegion[pItem->Region].ppTextures[f->TexWord];

		{
			int clut = pTexture->clut;
			int tpage = pTexture->tpage;

			VRAMClut = clut;
			VRAMTPage = tpage;
		}
	}

	pNormal = (CSVector *)((char *)pNormals + (f->NormalOffset & ~7));
	M3dMaths_RotMatrixYXZ((SVECTOR *)&pItem->Angles, &Transform);
	SetRotMatrix(&Transform);
	M3dAsm_SetTransVector((Vector *)&pItem->Position);
	TransformVertex(a, pVertices, pVertexNums, 0);
	TransformVertex(b, pVertices, pVertexNums, 1);
	TransformVertex(c, pVertices, pVertexNums, 2);

	retval = 1;

	if (f->Flags & 16) {
		CalcRGB(f->Flags & 0x800, f->Colour, 3, PSXRegion[pItem->Region].pColourTable);

		DAT_800d2574.vx = ((a.vx >> 12) + (b.vx >> 12) + (c.vx >> 12)) / 3 << 12;
		DAT_800d2574.vy = ((a.vy >> 12) + (b.vy >> 12) + (c.vy >> 12)) / 3 << 12;
		DAT_800d2574.vz = ((a.vz >> 12) + (b.vz >> 12) + (c.vz >> 12)) / 3 << 12;

		if ((f->Flags & 0x40) || !(f->Flags & 1)) {
			if (!LowMemory) {
				CVector norm;

				SetV(&norm, pNormal->vx, pNormal->vy, pNormal->vz);
				Shatter_Glass(15, a, b, c, norm, AverageR, AverageG, AverageB);

				retval = 2;
			}

			if (ModifyFlags)
				f->Flags &= ~0x40;
		} else {
			uint TextureWindow;

			if (f->Flags & 32)
				TextureWindow = pTexture->TexWin;
			else
				TextureWindow = 0;

			if (!LowMemory)
				Split(a, b, c, pTexture->u0, pTexture->v0, pTexture->u1, pTexture->v1, pTexture->u2,
				      pTexture->v2, TextureWindow, NumSplits);
		}
	} else {
		CVector d;

		TransformVertex(d, pVertices, pVertexNums, 3);
		CalcRGB(f->Flags & 0x800, f->Colour, 4, PSXRegion[pItem->Region].pColourTable);

		DAT_800d2574.vx = ((a.vx >> 12) + (b.vx >> 12) + (c.vx >> 12) + (d.vx >> 12)) >> 2 << 12;
		DAT_800d2574.vy = ((a.vy >> 12) + (b.vy >> 12) + (c.vy >> 12) + (d.vy >> 12)) >> 2 << 12;
		DAT_800d2574.vz = ((a.vz >> 12) + (b.vz >> 12) + (c.vz >> 12) + (d.vz >> 12)) >> 2 << 12;

		if ((f->Flags & 0x40) || !(f->Flags & 1)) {
			if (!LowMemory) {
				CVector norm;

				SetV(&norm, pNormal->vx, pNormal->vy, pNormal->vz);
				Shatter_Glass(30, a, b, c, norm, AverageR, AverageB, AverageB);

				retval = 2;
			}

			if (ModifyFlags)
				f->Flags &= ~0x40;
		} else {
			uint TextureWindow;

			if (f->Flags & 32)
				TextureWindow = pTexture->TexWin;
			else
				TextureWindow = 0;

			if (!LowMemory) {
				Split(a, b, c, pTexture->u0, pTexture->v0, pTexture->u1, pTexture->v1, pTexture->u2,
				      pTexture->v2, TextureWindow, NumSplits);
				Split(b, c, d, pTexture->u1, pTexture->v1, pTexture->u2, pTexture->v2, pTexture->u3,
				      pTexture->v3, TextureWindow, NumSplits);
			}
		}
	}

	return retval;
}

/* Shatter_Item__FP4Itemii (0x80021ecc) */
int Shatter_Item(Item *it, int a, int b) {
	int cnt = 0, i = 0, res = 0;
	SModel *m;
	char *s;
	unsigned short n0, n1, n;
	int r;

	m = PSXRegion[it->Region].ppModels[it->Model];
	n0 = m->NumVertices;
	n1 = m->NumNormals;
	n = m->NumFaces;
	/* the faces (model file data) follow the vertices and normals */
	s = (char *)m + (n0 * 8 + 0x1c) + n1 * 8;

	for (; i < n; i++) {
		r = Shatter_Face(it, (uint *)s, Rnd(5) == 0, 0, a, b, cnt < 6);

		if (r) {
			res = 1;

			if (r == 1)
				cnt++;
		}

		s += *(unsigned short *)(s + 2) & 0xfffc;
	}

	if (b && res) {
		it->Flags |= 1;
		it->BoxHidden = 0; /* broken: gone even when drawing the entire level */
	}

	return res;
}

/* Shatter_Glass__FiRC7CVectorN31UcUcUc (0x80022004) */
/* Demo twin; locals from its MAIN.SYM. Breaks a pane (triangle p0/p1/p2) into n
   glass bits: finds the floor under the middle, scatters bits at random points of the triangle
   flying outward from the middle plus along the normal, and remembers the shatter point nearest
   the camera (for the sound). THPS1 drops the demo's bit-budget check. */
void Shatter_Glass(int n, const CVector &p0, const CVector &p1, const CVector &p2,
                   const CVector &Normal, uchar r, uchar g, uchar b) {
	CVector middle = (p1 + p2) >> 1;
	SLineInfo ColInfo;
	int GroundY;

	ColInfo.StartCoords.vx = middle.vx + Normal.vx * 500;
	ColInfo.StartCoords.vy = middle.vy + Normal.vy * 500;
	ColInfo.StartCoords.vz = middle.vz + Normal.vz * 500;
	ColInfo.EndCoords.vx = ColInfo.StartCoords.vx;
	ColInfo.EndCoords.vy = ColInfo.StartCoords.vy + 0xbb8000;
	ColInfo.EndCoords.vz = ColInfo.StartCoords.vz;
	M3dColij_InitLineInfo(&ColInfo);
	M3dZone_LineToItemNotSCached(&ColInfo, 1);

	GroundY = 0x7fffffff;

	if (ColInfo.pItem)
		GroundY = ColInfo.Position.vy;

	CVector u = (p2 - p0) >> 12;
	CVector v = (p1 - p0) >> 12;

	{
		int i;

		for (i = 0; i < n; i++) {
			int alpha = Rnd(4096);
			int beta = Rnd(4096);
			CVector p = (alpha * u + beta * v) >> 12;

			p = (p << 12) + p0;
			CVector a = p - middle;

			Utils_CalcUnit(a, a);

			a = a * Rnd(40) + Normal * Rnd(40);
			new CGlassBit(p, a, GroundY, r, g, b, 100, 100, 100);
		}
	}

	if (!ShatterSound) {
		ShatterSoundPos = middle;
		ShatterSound = 1;
	} else {
		CVector CamPos(MikeCamera[Camera_CurrentMikeCamera].Position.vx,
		               MikeCamera[Camera_CurrentMikeCamera].Position.vy,
		               MikeCamera[Camera_CurrentMikeCamera].Position.vz);

		CamPos <<= 12;

		if ((uint)Utils_CrapDist(CamPos, middle) < (uint)Utils_CrapDist(CamPos, ShatterSoundPos))
			ShatterSoundPos = middle;
	}
}

/* Shatter_MaybeMakeGlassShatterSound__Fv (0x80022440) */
void Shatter_MaybeMakeGlassShatterSound(void) {
	ESfx s;

	if (ShatterSound) {
		/* PC: as the Dreamcast version does (1ST_READ.BIN 0x8c0f5140): SND_GLASS1 at the
		   shatter, then the level's glass below; but never SND_GLASS2 in Downtown (its playSFX
		   0x8c098060 refuses it), where downtown.kat's sound for it is a loop, not glass. */
		if (PC_DreamcastData()) {
			SFX_PlayPos(SND_GLASS1, ShatterSoundPos, 0);

			if (GLevel == 4) {
				ShatterSound = 0;
				return;
			}
		}

		if (GLevel == 1)
			s = SND_GLASS2;
		else {
			s = SND_GLASS2;

			if (GLevel != 4)
				s = SND_GLASS1;
		}

		SFX_PlayX(s, 0x2000, 0);
	}

	ShatterSound = 0;
}

/* FUN_80022490 (0x80022490) */
void FUN_80022490(void) {}
