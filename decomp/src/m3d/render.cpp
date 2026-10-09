/* M3d_Render__FPv (0x80083838) flags=-fstructure-noalias */
/* C++ (M3D.CPP), from the demo twin + its MAIN.SYM locals (pItem, Position, PosnRelCam, Transform,
   WorldTransform, TempMatrix, pModel, GlobalFaceFlags, yon; block tmp/r/g/b).
   Renders an item list: per item, picks the face flags, renders super items (twice, tinted, when
   flagged), else walks the model's LOD chain by distance, builds the transform and lights, and
   draws the model fast / non-rotated / fully transformed.
   Levers: -fstructure-noalias (Flags stays in a register across the scratchpad stores); the zero
   translation is gte_ldtr on a `register int asm("$0")` (retail's ctc2 $0; gte_ldtr(0,0,0) makes a
   zero pseudo that cse shares with gte_ldbkdir's and costs s7); the light block is one struct
   (back colour at +6/+0xe, matrices at +0x50/+0x70); the 0x800 semi-trans merge needs the mask and
   the new bits in separate temps; the yon test is written `vz - r > (ushort)(Yon / 10)`; the
   0x2186 store is an if/else of two full statements (crossjumped). */
extern "C" {
	#include <sys/types.h>
	#include <libgte.h>
	#include <inline_c.h>
}

#ifdef PC_PORT
#include <stdio.h>
#include <stdlib.h>
#endif

typedef unsigned char uchar;
typedef unsigned short ushort;
typedef unsigned int uint;

struct SVector {
	short vx, vy, vz;
};

struct SModel {
	ushort Flags;
	ushort NumVertices;
	ushort NumNormals;
	char pad6[8 - 6];
	int Radius;
	char padc[0x18 - 0xc];
	short LODDist;
	ushort NextLOD;
};

struct SCamera {
	int pad;
	VECTOR Pos; /* 0x04 */
	char pad14[0x74 - 0x14];
	MATRIX View; /* 0x74 */
};

struct SViewport {
	char pad0[0xa];
	ushort Yon; /* 0x0a */
	char padc[0xe - 0xc];
	ushort Fov; /* 0x0e */
};

struct SPSXRegion {
	char pad0[0xa];
	uchar Active; /* 0x0a */
	char padb[0x10 - 0xb];
	SModel **ppModels; /* 0x10 */
	char pad14[0x24 - 0x14];
	void *pAnimTex; /* 0x24 */
	char pad28[64 - 0x28];
};

struct Item {
	ushort Flags; /* 0x000 */
	ushort Inquiry;
	struct {
		int vx, vy, vz;
	} Position;     /* 0x004 */
	SVector Angles; /* 0x010 */
	ushort Model;   /* 0x016 */
	uchar Frame;    /* 0x018 */
	uchar TintIndex;
	uchar Anim;     /* 0x01a */
	uchar Region;   /* 0x01b */
	Item *NextItem; /* 0x01c */
	uint RGB;       /* 0x020 */
	char pad24[0x2c - 0x24];
	int SemiTrans; /* 0x02c */
	char pad30[0x3c - 0x30];
	char *pLight; /* 0x03c */
	char pad40[0x15a - 0x40];
	uchar NumShadows; /* 0x15a */
};

#define RGB2GPU(c)    ((((c) & 0xff) << 2) | (((c) & 0xff00) << 5) | (((c) & 0xff0000) << 8))
#define ANGLES_SET(p) (*(int *)&(p)->Angles.vx || (p)->Angles.vz)
#define SCRATCH_S(o)  (*(ushort *)(0x1f800000 + (o)))
#define SCRATCH_I(o)  (*(int *)(0x1f800000 + (o)))

struct SLights {
	short pad0[3];
	short BackR; /* 0x06 */
	short pad8[3];
	short BackG; /* 0x0e */
	char pad10[0x50 - 0x10];
	MATRIX LightMatrix;  /* 0x50 */
	MATRIX ColourMatrix; /* 0x70 */
};

extern "C" {
	extern SLights FrustumNormals;
	extern SPSXRegion PSXRegion[];
	extern Item *BaddyList;
	extern int Camera_CurrentMikeCamera;
	extern Item *EnviroList;
	extern int EnvRegions;
	extern int EnvRegions_4;
	extern Item *MechList;
	extern int CurrentWaterLevel;
	extern int CurrentWaterLevel2;
	extern int GLevel;

	void FUN_8008b238(MATRIX *);
	void FUN_8008b254(MATRIX *, MATRIX *);
	void FUN_8008b3d4(SVector *, MATRIX *);
	void FUN_8008b4e0(Item *, MATRIX *);
	MATRIX *FUN_8009738c(MATRIX *, MATRIX *, MATRIX *);
	void FUN_800866cc(MATRIX *, long *, SModel *);
	void FUN_80086f20(char *);
	void FUN_80086a4c(char *, int);
	void FUN_80084d18(SModel *);
	void FUN_80084dc0(SModel *, VECTOR *);
	void FUN_80085018(SModel *, VECTOR *);
	void FUN_800858ac(Item *, int);
}

ushort DpqMin, DpqShift, M3d_OTPushback, M3d_OTPushback2;
int RefMapClut, RefMapTpage, M3d_FadeColour;
int TestTheWater, Fast, WaterClipValue, M3d_WireframeMode, DpqMax;
short WaterNormal_6;
SCamera *pCurrentCamera;
SViewport *pCurrentViewport;
void *pColourTable;

void M3d_PreprocessWibblyTextures(int);
void RenderSuperItem(Item *);

void M3d_Render(void *p) {
	Item *pItem = (Item *)p;
	VECTOR Position;
	VECTOR PosnRelCam;
	MATRIX Transform;
	MATRIX WorldTransform;
	MATRIX TempMatrix;
	SModel *pModel;
	uint GlobalFaceFlags;
	ushort yon;

	if (!pItem)
		return;

	gte_SetLightMatrix(&FrustumNormals.LightMatrix);
	gte_SetColorMatrix(&FrustumNormals.ColourMatrix);
	gte_ldbkdir(FrustumNormals.BackR >> 1, FrustumNormals.BackG >> 1, 0);

	if (pItem != BaddyList)
		FUN_800858ac(pItem, Camera_CurrentMikeCamera ? 64 : 1);
	else
		FUN_800858ac(pItem, 1);

	if (pItem == EnviroList) {
		M3d_PreprocessWibblyTextures(EnvRegions);
		M3d_PreprocessWibblyTextures(EnvRegions_4);
	}

	SCRATCH_S(0x284) = DpqMin;
	SCRATCH_S(0x294) = DpqShift;
	SCRATCH_S(0x2b4) = M3d_OTPushback;
	SCRATCH_S(0x2c4) = M3d_OTPushback2;
	SCRATCH_S(0x2d4) = 0;
	SCRATCH_I(0x364) = RefMapClut;
	SCRATCH_I(0x374) = RefMapTpage;
	SCRATCH_I(0x354) = M3d_FadeColour;

	while (pItem) {
		if (pItem->Flags & 0x8000)
			goto next;

#ifdef PC_PORT
		if (getenv("THPS_TRACE_CULL")) {
			static uchar prev_active[64];
			static int prev_valid;
			uchar reg = pItem->Region;
			uchar now = PSXRegion[reg].Active != 0;
			if (reg < 64) {
				if (prev_valid && prev_active[reg] != now) {
					extern volatile unsigned pc_heartbeat;
					fprintf(stderr, "region f%u region=%u Active %u->%u item=%p\n", pc_heartbeat,
					        reg, prev_active[reg], now, (void *)pItem);
				}
				prev_active[reg] = now;
			}
			prev_valid = 1;
		}
#endif

		if (!PSXRegion[pItem->Region].Active)
			goto next;

		GlobalFaceFlags = 0xffff0000;

		if (pItem->Flags & 0x80) {
			if (pItem->Flags & 0x400)
				GlobalFaceFlags = 0xffff000c;
			else
				GlobalFaceFlags = 0xfff70004;
		} else if (pItem->Flags & 0x400) {
			GlobalFaceFlags = 0xfffb0008;
			SCRATCH_I(0x3e4) = RGB2GPU(pItem->RGB);
		}

		if (pItem->Flags & 0x4000) {
			GlobalFaceFlags |= 2;
			GlobalFaceFlags &= ~0x10000;
			GlobalFaceFlags &= ~0x200000;
		} else {
			GlobalFaceFlags &= ~2;
			GlobalFaceFlags |= 0x10000;
			GlobalFaceFlags |= 0x200000;
		}

		if (pItem->Flags & 0x800) {
			uint m = GlobalFaceFlags & ~0x180;
			uint t = (pItem->SemiTrans & 0x180) | 0x40;

			GlobalFaceFlags = m | t;
		}

		SCRATCH_I(0x3f4) = GlobalFaceFlags;
		TestTheWater = (pItem->Flags >> 3) & 1;

		if (pItem->Flags & 2) {
			if (pItem == MechList)
				WaterNormal_6 = (CurrentWaterLevel >> 12) - pCurrentCamera->Pos.vy;
			else
				WaterNormal_6 = (CurrentWaterLevel2 >> 12) - pCurrentCamera->Pos.vy;

			RenderSuperItem(pItem);

			if (TestTheWater) {
				int tmp;
				int r;
				int g;
				int b;
				uchar NumShadows;

				tmp = pItem->Flags;
				pItem->Flags = tmp | 0x400;

				if (!GLevel) {
					b = 10;
					g = 13;
					r = 8;
				} else {
					b = 34;
					g = 32;
					r = 32;
				}

				pItem->RGB = (b << 16) | (g << 8) | r;
				WaterClipValue = 0;
				GlobalFaceFlags &= ~0x40000;
				GlobalFaceFlags |= 8;
				SCRATCH_I(0x3e4) = RGB2GPU(pItem->RGB);
				SCRATCH_I(0x3f4) = GlobalFaceFlags;
				NumShadows = pItem->NumShadows;
				pItem->NumShadows = 0;
				RenderSuperItem(pItem);

				pItem->Flags = tmp;
				pItem->NumShadows = NumShadows;
				WaterClipValue = 0xffff;
			}

			goto next;
		}

		pModel = PSXRegion[pItem->Region].ppModels[pItem->Model];

		if (pModel->Flags & 0x20)
			goto next;

		if (pModel->Flags & 0x200)
			M3d_WireframeMode = 1;

		gte_SetRotMatrix(&pCurrentCamera->View);

		{
			register int zero asm("$0");
			gte_ldtr(zero, zero, zero);
		}

		PosnRelCam.vx = (pItem->Position.vx >> 12) - pCurrentCamera->Pos.vx;
		PosnRelCam.vy = (pItem->Position.vy >> 12) - pCurrentCamera->Pos.vy;
		PosnRelCam.vz = (pItem->Position.vz >> 12) - pCurrentCamera->Pos.vz;
		gte_ldlv0(&PosnRelCam);
		gte_rtv0();
		gte_stlvnl(&Position);

		if (pModel->LODDist < 0) {
			while (-((short)(pModel->LODDist * pCurrentViewport->Fov / 191) * DpqMax >> 10) <
			           Position.vz &&
			       pModel->NextLOD != 0xffff)
				pModel = PSXRegion[pItem->Region].ppModels[pModel->NextLOD];

			if (pModel->LODDist < 0) {
				if (-((short)(pModel->LODDist * pCurrentViewport->Fov / 191) * DpqMax >> 10) <
				        Position.vz &&
				    pModel->NextLOD == 0xffff) {
#ifdef PC_PORT
					if (getenv("THPS_TRACE_CULL")) {
						extern volatile unsigned pc_heartbeat;
						fprintf(stderr,
						        "lodskip(neg) f%u item=%p model=%p LODDist=%d vz=%d thresh=%d\n",
						        pc_heartbeat, (void *)pItem, (void *)pModel, pModel->LODDist,
						        Position.vz,
						        -((short)(pModel->LODDist * pCurrentViewport->Fov / 191) * DpqMax >>
						          10));
					}
#endif
					goto next;
				}
			} else {
				if (pModel->LODDist * pCurrentViewport->Fov / 191 < Position.vz &&
				    pModel->NextLOD == 0xffff) {
#ifdef PC_PORT
					if (getenv("THPS_TRACE_CULL")) {
						extern volatile unsigned pc_heartbeat;
						fprintf(stderr, "lodskip(pos) f%u item=%p model=%p LODDist=%d vz=%d thresh=%d\n",
						        pc_heartbeat, (void *)pItem, (void *)pModel, pModel->LODDist,
						        Position.vz, pModel->LODDist * pCurrentViewport->Fov / 191);
					}
#endif
					goto next;
				}
			}
		} else {
			while (pModel->LODDist * pCurrentViewport->Fov / 191 < Position.vz &&
			       pModel->NextLOD != 0xffff)
				pModel = PSXRegion[pItem->Region].ppModels[pModel->NextLOD];

			if (pModel->LODDist * pCurrentViewport->Fov / 191 < Position.vz &&
			    pModel->NextLOD == 0xffff)
				goto next;
		}

		if (pModel->NumVertices < 101 && !(pModel->Flags & 0x100) && !(pItem->Flags & 0x480) &&
		    Position.vz - (pModel->Radius >> 12) > (ushort)(pCurrentViewport->Yon / 10) &&
		    !(pItem->Angles.vx | pItem->Angles.vy | pItem->Angles.vz))
			Fast = 1;
		else
			Fast = 0;

#ifdef PC_PORT
		if (getenv("THPS_TRACE_CULL")) {
			extern volatile unsigned pc_heartbeat;
			fprintf(stderr,
			        "render f%u item=%p model=%p verts=%d vz=%d radius=%d yon10=%d Fast=%d\n",
			        pc_heartbeat, (void *)pItem, (void *)pModel, pModel->NumVertices, Position.vz,
			        pModel->Radius >> 12, (ushort)(pCurrentViewport->Yon / 10), Fast);
		}
#endif

		if (ANGLES_SET(pItem)) {
			FUN_8008b3d4(&pItem->Angles, &WorldTransform);

			if (pItem->Flags & 0x200)
				FUN_8008b4e0(pItem, &WorldTransform);

			FUN_8008b254(&WorldTransform, &Transform);
			gte_SetRotMatrix(&Transform);
		} else {
			FUN_8008b238(&WorldTransform);

			if (pItem->Flags & 0x200) {
				FUN_8008b4e0(pItem, &WorldTransform);
				FUN_8008b254(&WorldTransform, &Transform);
				gte_SetRotMatrix(&Transform);
			}
		}

		pColourTable = PSXRegion[pItem->Region].pAnimTex;

		if ((pModel->Flags & 1) || (pItem->Flags & 0x4000)) {
			FUN_800866cc(&WorldTransform, (long *)&Position, pModel);
		} else if ((pModel->Flags & 4) || (pItem->Flags & 0x80)) {
			if (ANGLES_SET(pItem)) {
				FUN_8009738c((MATRIX *)pItem->pLight, &WorldTransform, &TempMatrix);
				gte_SetLightMatrix(&TempMatrix);
			} else {
				gte_SetLightMatrix(pItem->pLight);
			}

			gte_SetColorMatrix(pItem->pLight + 20);
			FUN_80086f20(pItem->pLight + 40);
			FUN_80086a4c((char *)pModel + (pModel->NumVertices * 8 + 28), pModel->NumNormals);
		}

		yon = pCurrentViewport->Yon;

		if (pItem->Flags & 0x2000) {
			SCRATCH_S(0x284) = 0x7fff;
			SCRATCH_S(0x2d4) = 0;
			pCurrentViewport->Yon = 0x7fff;
		}

		if (Fast) {
			if (*(uint *)&pItem->Frame & 0xff00ff) {
				if (pItem->Frame)
					SCRATCH_S(0x2b4) = (signed char)pItem->Frame << 5;

				if (pItem->Anim)
					SCRATCH_S(0x2c4) = (signed char)pItem->Anim << 5;

				FUN_80084dc0(pModel, &PosnRelCam);

				SCRATCH_S(0x2b4) = M3d_OTPushback;
				SCRATCH_S(0x2c4) = M3d_OTPushback2;
			} else {
				FUN_80084dc0(pModel, &PosnRelCam);
			}
		} else if (!ANGLES_SET(pItem) && !(pItem->Flags & 0x200)) {
			if (*(uint *)&pItem->Frame & 0xff00ff) {
				if (pItem->Frame)
					SCRATCH_S(0x2b4) = (signed char)pItem->Frame << 5;

				if (pItem->Anim)
					SCRATCH_S(0x2c4) = (signed char)pItem->Anim << 5;

				FUN_80085018(pModel, &PosnRelCam);

				SCRATCH_S(0x2b4) = M3d_OTPushback;
				SCRATCH_S(0x2c4) = M3d_OTPushback2;
			} else {
				FUN_80085018(pModel, &PosnRelCam);
			}
		} else {
			gte_SetTransVector(&Position);

			if (*(uint *)&pItem->Frame & 0xff00ff) {
				if (pItem->Frame)
					SCRATCH_S(0x2b4) = (signed char)pItem->Frame << 5;

				if (pItem->Anim)
					SCRATCH_S(0x2c4) = (signed char)pItem->Anim << 5;

				FUN_80084d18(pModel);

				SCRATCH_S(0x2b4) = M3d_OTPushback;
				SCRATCH_S(0x2c4) = M3d_OTPushback2;
			} else {
				FUN_80084d18(pModel);
			}
		}

		if (pItem->Flags & 0x2000) {
			SCRATCH_S(0x2d4) = 0;
			SCRATCH_S(0x284) = DpqMin;
			pCurrentViewport->Yon = yon;
		}
	next:
		pItem = pItem->NextItem;
	}
}
