/* M3dColij_InitLineInfo__FP9SLineInfo (0x8007eb08) */
/* C++ (M3DCOLIJ.CPP). Locals, types and scopes from the demo's MAIN.SYM, except the two short
   cosines, which the SYM doesn't list. Cos2 has to be a fresh variable: reassigning r1Norm gives it
   two deaths, so local-alloc skips it and it loses s1 to Shift1. */
extern "C" {
	#include <sys/types.h>
	#include <libgte.h>
	#include <inline_c.h>
}

#include "game/colij.h"

extern MATRIX Tfm;

ushort Inquiry;

int M3dMaths_SquareRoot0(int);
void NextInquiry();

void M3dColij_InitLineInfo(SLineInfo *pInfo) {
	short *pShort;
	SVECTOR L;
	SVECTOR AbsL;
	Vector *p1;
	int lzc;
	int Shift1;
	int Shift2;
	int s1;
	int s2;
	int r1Norm;
	int r2Norm;
	short Sin;

	pInfo->pItem = 0;
	pInfo->pFace = 0;
	pInfo->Distance = 0x7fffffff;
	pInfo->tNear = 0x7fffffff;
	pInfo->Model = -1;
	pInfo->RecordTriggerZoneHits = 0;
	pInfo->DropDown = 0;

	L.vx = (pInfo->EndCoords.vx - pInfo->StartCoords.vx) >> 12;
	L.vy = (pInfo->EndCoords.vy - pInfo->StartCoords.vy) >> 12;
	L.vz = (pInfo->EndCoords.vz - pInfo->StartCoords.vz) >> 12;

	s1 = L.vx * L.vx + L.vz * L.vz;

	if (s1) {
		gte_ldlzc(s1);
		gte_nop();
		gte_nop();
		gte_stlzc(&lzc);

		Shift1 = (lzc - 1) >> 1;
		r1Norm = M3dMaths_SquareRoot0(s1 << ((lzc - 1) & 0x1e));
		Sin = (L.vx << (Shift1 + 12)) / r1Norm;
		short Cos = (L.vz << (Shift1 + 12)) / r1Norm;

		pShort = (short *)&Tfm;
		*pShort++ = Cos;
		*pShort++ = 0;
		*pShort++ = -Sin;
		*pShort++ = 0;
		*pShort++ = 4096;
		*pShort++ = 0;
		*pShort++ = Sin;
		*pShort++ = 0;
		*pShort = Cos;

		s2 = s1 + L.vy * L.vy;
		gte_ldlzc(s2);
		gte_nop();
		gte_nop();
		gte_stlzc(&lzc);

		Shift2 = (lzc - 1) >> 1;
		r2Norm = M3dMaths_SquareRoot0(s2 << ((lzc - 1) & 0x1e));
		pInfo->Length = r2Norm >> Shift2;
		Sin = (L.vy << (Shift2 + 12)) / r2Norm;
		short Cos2 = (r1Norm << 12) / (r2Norm << (Shift1 - Shift2));

		pInfo->WorldCst.m[0][0] = 4096;
		pInfo->WorldCst.m[0][1] = 0;
		pInfo->WorldCst.m[0][2] = 0;
		pInfo->WorldCst.m[1][0] = 0;
		pInfo->WorldCst.m[1][1] = Cos2;
		pInfo->WorldCst.m[1][2] = -Sin;
		pInfo->WorldCst.m[2][0] = 0;
		pInfo->WorldCst.m[2][1] = Sin;
		pInfo->WorldCst.m[2][2] = Cos2;
		MulMatrix(&pInfo->WorldCst, &Tfm);
	} else {
		Sin = 4096;

		if (L.vy < 0) {
			Sin = -4096;
			pInfo->Length = -L.vy;
		} else {
			pInfo->Length = L.vy;
			pInfo->DropDown = 1;
		}

		pInfo->WorldCst.m[0][0] = 4096;
		pInfo->WorldCst.m[0][1] = 0;
		pInfo->WorldCst.m[0][2] = 0;
		pInfo->WorldCst.m[1][0] = 0;
		pInfo->WorldCst.m[1][1] = 0;
		pInfo->WorldCst.m[1][2] = -Sin;
		pInfo->WorldCst.m[2][0] = 0;
		pInfo->WorldCst.m[2][1] = Sin;
		pInfo->WorldCst.m[2][2] = 0;
	}

	gte_SetRotMatrix(&pInfo->WorldCst);

	p1 = &pInfo->EndCoords;

	if (pInfo->StartCoords.vx < p1->vx) {
		pInfo->MinCoords.vx = pInfo->StartCoords.vx;
		pInfo->MaxCoords.vx = p1->vx;
	} else {
		pInfo->MinCoords.vx = p1->vx;
		pInfo->MaxCoords.vx = pInfo->StartCoords.vx;
	}

	if (pInfo->StartCoords.vy < p1->vy) {
		pInfo->MinCoords.vy = pInfo->StartCoords.vy;
		pInfo->MaxCoords.vy = p1->vy;
	} else {
		pInfo->MinCoords.vy = p1->vy;
		pInfo->MaxCoords.vy = pInfo->StartCoords.vy;
	}

	if (pInfo->StartCoords.vz < p1->vz) {
		pInfo->MinCoords.vz = pInfo->StartCoords.vz;
		pInfo->MaxCoords.vz = p1->vz;
	} else {
		pInfo->MinCoords.vz = p1->vz;
		pInfo->MaxCoords.vz = pInfo->StartCoords.vz;
	}

	NextInquiry();

	pInfo->Inquiry = Inquiry;
}
