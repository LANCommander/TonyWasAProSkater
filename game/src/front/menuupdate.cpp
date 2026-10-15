/* Update__5CMenu (0x80016dc0) */
/* C++ (FRONT.CPP). Types from the demo's MAIN.SYM. */
#include "game/front.h"
#include "game/pad.h"

extern SControl Pad[];

extern int UpDownAutoRepeatTimer;

int SFX_PlayX(unsigned int, short, int);

void CMenu::Update() {
	while (!mEntry[mLine].Visible) {
		mEntry[mLine].TargetScale = mEntry[mLine].LowScale;
		mEntry[mLine].CurrentScale = mEntry[mLine].LowScale;
		mLine++;

		if (mLine >= mNumLines)
			mLine = 0;

		mEntry[mLine].TargetScale = mEntry[mLine].HiScale;
		mEntry[mLine].CurrentScale = mEntry[mLine].LowScale;
	}

	if (Pad[0].MenuUp.Pressed || Pad[0].MenuDown.Pressed || Pad[1].MenuUp.Pressed ||
	    Pad[1].MenuDown.Pressed) {
		if (UpDownAutoRepeatTimer == 0 ||
		    (UpDownAutoRepeatTimer >= 16 && (UpDownAutoRepeatTimer & 3) == 0)) {
			if (Pad[0].MenuUp.Pressed || Pad[1].MenuUp.Pressed) {
				mEntry[mLine].TargetScale = mEntry[mLine].LowScale;

				do {
					if (mLine == 0)
						mLine = mNumLines - 1;
					else
						mLine--;
				} while (!mEntry[mLine].Visible || !mEntry[mLine].Enabled);

				mEntry[mLine].TargetScale = mEntry[mLine].HiScale;
				SFX_PlayX(6, 8192, 0);
			}

			if (Pad[0].MenuDown.Pressed || Pad[1].MenuDown.Pressed) {
				mEntry[mLine].TargetScale = mEntry[mLine].LowScale;

				do {
					mLine++;

					if (mLine >= mNumLines)
						mLine = 0;
				} while (!mEntry[mLine].Visible || !mEntry[mLine].Enabled);

				mEntry[mLine].TargetScale = mEntry[mLine].HiScale;
				SFX_PlayX(6, 8192, 0);
			}
		}

		UpDownAutoRepeatTimer++;
	} else {
		UpDownAutoRepeatTimer = 0;
	}

	for (int i = 0; i < mNumLines; i++) {
		if (mEntry[i].CurrentScale < mEntry[i].TargetScale) {
			mEntry[i].CurrentScale += 40;

			if (mEntry[i].CurrentScale > mEntry[i].TargetScale)
				mEntry[i].CurrentScale = mEntry[i].TargetScale;
		}

		if (mEntry[i].CurrentScale > mEntry[i].TargetScale) {
			mEntry[i].CurrentScale -= 40;

			if (mEntry[i].CurrentScale < mEntry[i].TargetScale)
				mEntry[i].CurrentScale = mEntry[i].TargetScale;
		}
	}
}
