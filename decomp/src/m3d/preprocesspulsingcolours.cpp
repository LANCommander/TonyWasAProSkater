/* M3d_PreprocessPulsingColours__Fi (0x800856d4) */
/* C++ (M3D.CPP). Byte-identical demo twin; locals from its MAIN.SYM. Each pulse record is a header
   word {colour index, list length, list position, time} followed by a list of {r, g, b, interval}
   keys. It advances by the frame delta and interpolates the current colour with the GTE (GPL)
   into the region's colour table. */
extern "C" {
	#include <sys/types.h>
	#include <libgte.h>
	#include <inline_c.h>
}
#include "game/m3d.h"

#define gte_ldRGBC_r(r0) __asm__ volatile("mtc2 %0, $6" : : "r"(r0))
#define gte_ldlvnl(r0)                                                                             \
	__asm__ volatile("lwc2 $25, 0( %0 ); lwc2 $26, 4( %0 ); lwc2 $27, 8( %0 )" : : "r"(r0))

extern SPSXRegion PSXRegion[];

uint XblanksNow;
uint XblanksThen;
u_long *pColourTable;

void M3d_PreprocessPulsingColours(int Region) {
	uint *pData;
	uint *pEnd;
	uint dt;
	uint ListLen;
	uint ListPos;
	int t;
	int rgb0[3];
	int drgb[3];
	SRGBI *RGBs;

	if (Region == -1)
		return;

	pData = PSXRegion[Region].pColourPulseData;

	if (!pData)
		return;

	pEnd = (uint *)((char *)pData + pData[-1]);
	dt = XblanksNow - XblanksThen;
	pColourTable = (u_long *)PSXRegion[Region].pColourTable;

	if (XblanksNow < XblanksThen)
		dt = 1;

	while (pData < pEnd) {
		RGBs = (SRGBI *)(pData + 1);
		ListPos = ((uchar *)pData)[2];
		t = ((uchar *)pData)[3];
		t += dt;
		ListLen = ((uchar *)pData)[1];

		while (t >= RGBs[ListPos].Interval) {
			t -= RGBs[ListPos].Interval;

			if (++ListPos == ListLen)
				ListPos = 0;
		}

		((uchar *)pData)[2] = ListPos;
		((uchar *)pData)[3] = t;
		rgb0[0] = RGBs[ListPos].r << 4;
		rgb0[1] = RGBs[ListPos].g << 4;
		rgb0[2] = RGBs[ListPos].b << 4;

		{
			uint Old = ListPos; /* not in the demo SYM; it gives the copy of the key index */

			if (++ListPos == ListLen)
				ListPos = 0;

			drgb[0] = (RGBs[ListPos].r << 4) - rgb0[0];
			drgb[1] = (RGBs[ListPos].g << 4) - rgb0[1];
			drgb[2] = (RGBs[ListPos].b << 4) - rgb0[2];
			gte_ldRGBC_r(0);
			gte_lddp((t << 12) / RGBs[Old].Interval);
		}

		gte_ldlvnl(&rgb0[0]);
		gte_ldlvl(&drgb[0]);
		gte_gpl12();
		gte_strgb(&pColourTable[((uchar *)pData)[0]]);

		pData = (uint *)(ListLen * 4 + (int)pData) + 1;
	}
}
