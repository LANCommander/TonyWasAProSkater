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
		for (i = 0; i < NumVectors; i++) {
			/* p0 + (p1 - p0) * Alpha (16-bit differences and weight), the sums' low halves */
			int32_t base[3];
			int16_t d[3];
			GeomAcc r;

			geom_load_vector(base, pAnim0->vx, pAnim0->vy, pAnim0->vz);
			d[0] = (int16_t)(pAnim1->vx - pAnim0->vx);
			d[1] = (int16_t)(pAnim1->vy - pAnim0->vy);
			d[2] = (int16_t)(pAnim1->vz - pAnim0->vz);
			r = geom_gpl((int16_t)Alpha, d, base, 12, 0, 0, 0, 0);

			pAnim0++;
			pAnim1++;
			pTween->vx = (short)r.mac[0];
			pTween->vy = (short)r.mac[1];
			pTween->vz = (short)r.mac[2];
			pTween++;
		}
	}
}
