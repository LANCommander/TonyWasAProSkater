/* Flash__5CItemiUcUcUc (0x800435e8) */
#include "game/ob.h"

void CItem::Flash(int frames, uchar r, uchar g, uchar b) {
	if (frames == 0) {
		mFlashTimer = 0;
		mRGB = (b << 16) | (g << 8) | r;
		mFlags |= 1024;
	} else {
		if (frames < 0) {
			mFlashTimer = -frames;
			mFlash_dR = (r - 128) / mFlashTimer;
			mFlash_dG = (g - 128) / mFlashTimer;
			mFlash_dB = (b - 128) / mFlashTimer;
			mRGB = 0x808080;
			mFlags |= 1024;
			return;
		}

		mFlash_dR = (128 - r) / frames;
		mFlash_dG = (128 - g) / frames;
		mFlash_dB = (128 - b) / frames;
		mFlashTimer = frames + 1;

		if (!(mStoredRGB & 0x01000000)) {
			mStoredRGB = (mRGB & 0x00ffffff) | 0x01000000;

			if (mFlags & 1024)
				mStoredRGB |= 0x02000000;
		}

		mRGB = (b << 16) | (g << 8) | r;
		mFlags |= 1024;
	}
}
