/* CalculateDynamicLighting__6CBrucePUiP4Item (0x800425c0) */
#include "game/m3d.h"
#include "game/bruce.h"

extern SPSXRegion PSXRegion[] __attribute__((section(".data")));

void CBruce::CalculateDynamicLighting(uint *p_face, Item *p_item) {
	unsigned short f = *(unsigned short *)p_face;

	if (f & 0x800) {
		uint *pColourTable = PSXRegion[p_item->Region].pColourTable;
		int vertices = 4;

		mBrightness = 0;

		if (f & 0x10)
			vertices = 3;

		{
			int i;

			for (i = 0; i < vertices; i++) {
				uint rgb = pColourTable[((unsigned char *)p_face)[8 + i]] & 0xffffff;

				mBrightness = mBrightness + (rgb & 255) + ((rgb >> 8) & 255) + (rgb >> 16);
			}
		}

		mBrightness = mBrightness / (vertices * 3);
	} else {
		uint rgb = p_face[2] & 0xffffff;

		mBrightness = (int)((rgb & 255) + ((rgb >> 8) & 255) + (rgb >> 16)) / 3;
	}

	mBrightness = mBrightness * 3;
	DoLightingSetup();
}
