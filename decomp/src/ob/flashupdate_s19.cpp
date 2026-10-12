/* FlashUpdate__5CItem (0x80043728) */
#include "game/ob.h"

void CItem::FlashUpdate() {
	if (mFlashTimer) {
		int r, g, b;

		mFlashTimer--;
		r = (mRGB & 255);
		g = ((mRGB >> 8) & 255);
		b = ((mRGB >> 16) & 255);
		r += mFlash_dR;
		g += mFlash_dG;
		b += mFlash_dB;
		mRGB = ((b & 255) << 16) | ((g & 255) << 8) | (r & 255);
		mFlags |= 1024;

		if (mFlashTimer == 0) {
			if (mStoredRGB & 0x02000000)
				mFlags |= 1024;
			else
				mFlags &= ~1024;

			mRGB = mStoredRGB;
			mStoredRGB = 0;
		}
	}
}
