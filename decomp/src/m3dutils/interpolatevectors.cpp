/* M3dUtils_InterpolateVectors__FiiPUiP4Itemii (0x800814b0) */
#include "game/m3d.h"
#include "game/ob.h"

extern SMatrix TweenBuffer[];

void M3dUtils_InterpolateVectors(int NumVectors, int Interval, uint *pAnimFile, Item *pItem,
                                 int Part, int NumParts) {
	CSVector dAnim;
	SMatrix *pAnims;
	int Alpha;
	int Frame0;
	int Frame1;
	int Frame;
	int NumFrames;
	int i;
	int Anim;
	CSVector *pTween;
	CSVector *pAnim0;

	Frame = ((CSuper *)pItem)->mFrame;
	Frame0 = Frame - (Frame % Interval);
	Frame1 = Frame0 + Interval;
	Anim = ((CSuper *)pItem)->mAnim;
	pAnims = (SMatrix *)((char *)pAnimFile + pAnimFile[Anim * 2 + 1]);
	NumFrames = ((ushort *)pAnimFile)[Anim * 4 + 4];

	if (Frame1 >= NumFrames) {
		if (((CSuper *)pItem)->mAnimMode == 1) {
			Frame1 = 0;
			Alpha = ((Frame - Frame0) << 12) / (NumFrames - Frame0);
		} else {
			Frame0 -= Interval;

			if (Frame0 < 0)
				Frame0 = 0;

			Frame1 -= Interval;

			if (Frame1 == 0)
				Alpha = 0;
			else
				Alpha = ((Frame - Frame0) << 12) / (Frame1 - Frame0);
		}
	} else {
		Alpha = ((Frame - Frame0) << 12) / (Frame1 - Frame0);
	}

	pTween = (CSVector *)((char *)TweenBuffer + Part * 24);
	pAnim0 = (CSVector *)((char *)pAnims + (NumParts * Frame0 / Interval) * 24 + Part * 24);

	if (Alpha == 0) {
		for (i = 0; i < NumVectors; i++)
			*pTween++ = *pAnim0++;
	} else {
		CSVector *pAnim1 =
		    (CSVector *)((char *)pAnims + (NumParts * Frame1 / Interval) * 24 + Part * 24);
		__asm__ volatile("mtc2 %0, $8" : : "r"(Alpha));

		for (i = 0; i < NumVectors; i++) {
			int p0x = pAnim0->vx;
			int p0y = pAnim0->vy;
			int p0z = pAnim0->vz;
			int dx = pAnim1->vx - p0x;
			int dy = pAnim1->vy - p0y;
			int dz = pAnim1->vz - p0z;
			__asm__ volatile("mtc2 %0, $25" : : "r"(p0x));
			__asm__ volatile("mtc2 %0, $26" : : "r"(p0y));
			__asm__ volatile("mtc2 %0, $27" : : "r"(p0z));
			__asm__ volatile("mtc2 %0, $9" : : "r"(dx));
			__asm__ volatile("mtc2 %0, $10" : : "r"(dy));
			__asm__ volatile("mtc2 %0, $11" : : "r"(dz));
			__asm__ volatile("nop; nop; cop2 0x1a8003e"); /* gte_gpl12 */

			pAnim0++;
			pAnim1++;
			/* one source line (demo line 407): a store-MAC1..3-as-SVECTOR macro on fixed $8..$10 */
			__asm__ volatile("mfc2 $8, $25; mfc2 $9, $26; mfc2 $10, $27; sh $8, 0(%0); sh $9, "
			                 "2(%0); sh $10, 4(%0)"
			                 :
			                 : "r"(pTween)
			                 : "$8", "$9", "$10", "memory");

			pTween++;
		}
	}
}
